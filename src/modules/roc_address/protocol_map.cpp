/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/protocol_map.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

ProtocolMap::ProtocolMap() {
    memset(protos_, 0, sizeof(protos_));
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTP;
        attrs.iface = Iface_AudioSource;
        attrs.fec_scheme = packet::FEC_None;
        attrs.default_port = -1;
        attrs.path_supported = false;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTP_RS8M_Source;
        attrs.iface = Iface_AudioSource;
        attrs.fec_scheme = packet::FEC_ReedSolomon_M8;
        attrs.default_port = -1;
        attrs.path_supported = false;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RS8M_Repair;
        attrs.iface = Iface_AudioRepair;
        attrs.fec_scheme = packet::FEC_ReedSolomon_M8;
        attrs.default_port = -1;
        attrs.path_supported = false;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTP_LDPC_Source;
        attrs.iface = Iface_AudioSource;
        attrs.fec_scheme = packet::FEC_LDPC_Staircase;
        attrs.default_port = -1;
        attrs.path_supported = false;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_LDPC_Repair;
        attrs.iface = Iface_AudioRepair;
        attrs.fec_scheme = packet::FEC_LDPC_Staircase;
        attrs.default_port = -1;
        attrs.path_supported = false;
        add_proto_(attrs);
    }
    {
        ProtocolAttrs attrs;
        attrs.protocol = Proto_RTSP;
        attrs.iface = Iface_Aggregate;
        attrs.fec_scheme = packet::FEC_None;
        attrs.default_port = 554;
        attrs.path_supported = true;
        add_proto_(attrs);
    }
}

const ProtocolAttrs* ProtocolMap::find_proto(Protocol proto) const {
    if ((int)proto < 0 || (int)proto >= MaxProtos) {
        return NULL;
    }

    if (protos_[proto].protocol == Proto_None) {
        return NULL;
    }

    if (protos_[proto].protocol != proto) {
        return NULL;
    }

    return &protos_[proto];
}

void ProtocolMap::add_proto_(const ProtocolAttrs& proto) {
    roc_panic_if((int)proto.protocol < 0);
    roc_panic_if((int)proto.protocol >= MaxProtos);
    roc_panic_if(protos_[proto.protocol].protocol != 0);

    protos_[proto.protocol] = proto;
}

} // namespace address
} // namespace roc
