/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_reader.h"
#include "test_helpers/packet_writer.h"

#include "roc_core/atomic.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

namespace {

const rtp::PayloadType PayloadType_Ch1 = rtp::PayloadType_L16_Mono;
const rtp::PayloadType PayloadType_Ch2 = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 500,

    SampleRate = 44100,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    Latency = SamplesPerPacket * 8,
    Timeout = Latency * 13,

    ManyPackets = Latency / SamplesPerPacket * 10,

    MaxSnJump = ManyPackets * 5,
    MaxTsJump = ManyPackets * 7 * SamplesPerPacket
};

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);
core::BufferFactory<uint8_t> byte_buffer_factory(allocator, MaxBufSize, true);
packet::PacketFactory packet_factory(allocator, true);

rtp::FormatMap format_map(allocator, true);
rtp::Composer rtp_composer(NULL);

ReceiverSlot* create_slot(ReceiverSource& source) {
    ReceiverSlot* slot = source.create_slot();
    CHECK(slot);
    return slot;
}

packet::IWriter*
create_endpoint(ReceiverSlot* slot, address::Interface iface, address::Protocol proto) {
    CHECK(slot);
    ReceiverEndpoint* endpoint = slot->create_endpoint(iface, proto);
    CHECK(endpoint);
    return &endpoint->writer();
}

} // namespace

TEST_GROUP(receiver_source) {
    audio::SampleSpec packet_sample_spec;
    audio::SampleSpec output_sample_spec;

    address::SocketAddr src1;
    address::SocketAddr src2;

    address::SocketAddr dst1;
    address::SocketAddr dst2;

    address::Protocol proto1;
    address::Protocol proto2;

    ReceiverConfig make_config() {
        ReceiverConfig config;

        config.common.output_sample_spec = output_sample_spec;

        config.common.internal_frame_length = MaxBufSize * core::Second
            / core::nanoseconds_t(SampleRate
                                  * std::max(output_sample_spec.num_channels(),
                                             packet_sample_spec.num_channels()));

        config.common.enable_timing = false;
        config.common.enable_poisoning = true;
        config.common.enable_profiling = true;

        config.default_session.latency_monitor.fe_enable = false;
        config.default_session.target_latency = Latency * core::Second / SampleRate;

        config.default_session.latency_monitor.min_latency =
            -Timeout * 10 * core::Second / SampleRate;
        config.default_session.latency_monitor.max_latency =
            +Timeout * 10 * core::Second / SampleRate;

        config.default_session.watchdog.no_playback_timeout =
            Timeout * core::Second / SampleRate;

        config.default_session.rtp_validator.max_sn_jump = MaxSnJump;
        config.default_session.rtp_validator.max_ts_jump =
            MaxTsJump * core::Second / SampleRate;

        return config;
    }

    void init(size_t output_channels, size_t packet_channels) {
        output_sample_spec.set_sample_rate(SampleRate);
        output_sample_spec.channel_set().set_layout(output_channels == 1
                                                        ? audio::ChannelLayout_Mono
                                                        : audio::ChannelLayout_Surround);
        output_sample_spec.channel_set().set_channel_range(0, output_channels - 1, true);

        packet_sample_spec.set_sample_rate(SampleRate);
        packet_sample_spec.channel_set().set_layout(packet_channels == 1
                                                        ? audio::ChannelLayout_Mono
                                                        : audio::ChannelLayout_Surround);
        packet_sample_spec.channel_set().set_channel_range(0, packet_channels - 1, true);

        src1 = test::new_address(1);
        src2 = test::new_address(2);

        dst1 = test::new_address(3);
        dst2 = test::new_address(4);

        proto1 = address::Proto_RTP;
        proto2 = address::Proto_RTP;
    }
};

TEST(receiver_source, no_sessions) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver_source, one_session) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, one_session_long_run) {
    enum { NumCh = 2, NumIterations = 10 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (size_t np = 0; np < ManyPackets; np++) {
            for (size_t nf = 0; nf < FramesPerPacket; nf++) {
                frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

                UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
            }

            packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        }
    }
}

TEST(receiver_source, initial_latency) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver_source, initial_latency_timeout) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Timeout / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

TEST(receiver_source, timeout) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);
    }
}

TEST(receiver_source, initial_trim) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency * 3 / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    frame_reader.set_offset(Latency * 2);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, two_sessions_synchronous) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src2, dst1);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_overlapping) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst1);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    test::PacketWriter packet_writer2(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src2, dst1);

    packet_writer2.set_offset(packet_writer1.offset() - Latency);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_two_endpoints) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot1 = create_slot(receiver);
    CHECK(slot1);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot1, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    ReceiverSlot* slot2 = create_slot(receiver);
    CHECK(slot2);

    packet::IWriter* endpoint2_writer =
        create_endpoint(slot2, address::Iface_AudioSource, proto2);
    CHECK(endpoint2_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint2_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src2, dst2);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_same_address_same_stream) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(11);

    packet_writer2.set_offset(77);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_same_address_different_streams) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_factory, byte_buffer_factory,
                                      PayloadType_Ch2, src1, dst2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(22);

    packet_writer2.set_offset(77);
    packet_writer2.set_seqnum(5);
    packet_writer2.set_timestamp(5 * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, seqnum_overflow) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.set_seqnum(packet::seqnum_t(-1) - ManyPackets / 2);
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, seqnum_small_jump) {
    enum { NumCh = 2, SmallJump = 5 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + SmallJump);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, seqnum_large_jump) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + MaxSnJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);
    }
}

TEST(receiver_source, seqnum_reorder) {
    enum { NumCh = 2, ReorderWindow = Latency / SamplesPerPacket };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    size_t pos = 0;

    for (size_t ni = 0; ni < ManyPackets / ReorderWindow; ni++) {
        if (pos >= Latency / SamplesPerPacket) {
            for (size_t nf = 0; nf < ReorderWindow * FramesPerPacket; nf++) {
                frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
            }
        }

        for (ssize_t np = ReorderWindow - 1; np >= 0; np--) {
            packet_writer.shift_to(pos + size_t(np), SamplesPerPacket);
            packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        }

        pos += ReorderWindow;
    }
}

TEST(receiver_source, seqnum_late) {
    enum { NumCh = 2, DelayedPackets = 5 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    packet_writer.shift_to(Latency / SamplesPerPacket + DelayedPackets, SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.shift_to(Latency / SamplesPerPacket, SamplesPerPacket);
    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
    }

    frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
}

TEST(receiver_source, timestamp_overflow) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.set_timestamp(packet::timestamp_t(-1)
                                - ManyPackets * SamplesPerPacket / 2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_small_jump) {
    enum { NumCh = 2, ShiftedPackets = 5 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + ShiftedPackets * SamplesPerPacket);
    packet_writer.set_offset(Latency + ShiftedPackets * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ShiftedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_large_jump) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + MaxTsJump);
    packet_writer.set_offset(Latency + MaxTsJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);
    }
}

TEST(receiver_source, timestamp_overlap) {
    enum { NumCh = 2, OverlappedSamples = SamplesPerPacket / 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency - OverlappedSamples);
    packet_writer.set_offset(Latency - OverlappedSamples);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_reorder) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (ssize_t np = Latency / SamplesPerPacket - 1; np >= 0; np--) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        packet_writer.set_offset(Latency + size_t(np) * SamplesPerPacket);

        packet_writer.set_timestamp(
            packet::timestamp_t(Latency + size_t(np) * SamplesPerPacket));

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_offset(Latency * 2);
    packet_writer.set_timestamp(Latency * 2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_late) {
    enum { NumCh = 2, DelayedPackets = 5 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + DelayedPackets * SamplesPerPacket);
    packet_writer.set_offset(Latency + DelayedPackets * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_timestamp(Latency);
    packet_writer.set_offset(Latency);

    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
    }

    frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
}

TEST(receiver_source, packet_size_small) {
    enum {
        NumCh = 2,
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,
        ManySmallPackets = Latency / SamplesPerSmallPacket * 10
    };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerSmallPacket, SamplesPerSmallPacket,
                                packet_sample_spec);

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        }
    }
}

TEST(receiver_source, packet_size_large) {
    enum {
        NumCh = 2,
        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,
        ManyLargePackets = Latency / SamplesPerLargePacket * 10
    };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerLargePacket, SamplesPerLargePacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyLargePackets; np++) {
        for (size_t nf = 0; nf < FramesPerLargePacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
    }
}

TEST(receiver_source, packet_size_variable) {
    enum {
        NumCh = 2,

        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,

        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,

        SamplesPerTwoPackets = (SamplesPerSmallPacket + SamplesPerLargePacket),

        NumIterations = Latency / SamplesPerTwoPackets * 10
    };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    size_t available = 0;

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (; available >= Latency; available -= SamplesPerFrame) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);

        available += SamplesPerTwoPackets;
    }
}

TEST(receiver_source, corrupted_packets_new_session) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.set_corrupt(true);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, corrupted_packets_existing_session) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    packet_writer.set_corrupt(true);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_corrupt(false);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, channels_stereo_to_mono) {
    enum { OutputCh = 1, PacketCh = 2 };

    init(OutputCh, PacketCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, channels_mono_to_stereo) {
    enum { OutputCh = 2, PacketCh = 1 };

    init(OutputCh, PacketCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch1, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, state) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ReceiverSource receiver(make_config(), format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_factory, byte_buffer_factory,
                                     PayloadType_Ch2, src1, dst1);

    core::Slice<audio::sample_t> samples = sample_buffer_factory.new_buffer();
    CHECK(samples);
    samples.reslice(0, FramesPerPacket * NumCh);

    CHECK(receiver.state() == sndio::DeviceState_Idle);

    {
        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);
    }

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    CHECK(receiver.state() == sndio::DeviceState_Active);

    {
        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);
    }

    for (;;) {
        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);

        if (receiver.state() == sndio::DeviceState_Idle) {
            break;
        }
    }
}

} // namespace pipeline
} // namespace roc
