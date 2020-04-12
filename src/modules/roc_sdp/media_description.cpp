/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sdp/media_description.h"

namespace roc {
namespace sdp {

MediaDescription::MediaDescription(core::IAllocator& allocator)
    : payload_ids_(allocator)
    , connection_data_(allocator)
    , allocator_(allocator) {
    clear();
}

void MediaDescription::clear() {
    payload_ids_.resize(0);
    connection_data_.resize(0);
    type_ = MediaType_None;
    port_ = 0;
    nb_ports_ = 0;
    proto_ = MediaTransport_None;
}

MediaType MediaDescription::type() const {
    return type_;
}

int MediaDescription::port() const {
    return port_;
}

int MediaDescription::nb_ports() const {
    return nb_ports_;
}

MediaTransport MediaDescription::proto() const {
    return proto_;
}

unsigned MediaDescription::default_payload_id() const {
    if (payload_ids_.size() == 0) {
        roc_panic(
            "media description: MediaDescription should have at least one payload id.");
    }

    return payload_ids_.front();
}

bool MediaDescription::set_type(MediaType type) {
    type_ = type;
    return true;
}

bool MediaDescription::set_proto(MediaTransport proto) {
    proto_ = proto;
    return true;
}

bool MediaDescription::set_port(long port) {
    if (port < 0 || port > 65535) {
        return false;
    }

    port_ = (int)port;
    return true;
}

bool MediaDescription::set_nb_ports(long nb_ports) {
    if (nb_ports < 0 || nb_ports > 65535) {
        return false;
    }

    nb_ports_ = (int)nb_ports;
    return true;
}

bool MediaDescription::add_payload_id(unsigned payload_id) {
    if (!payload_ids_.grow_exp(payload_ids_.size() + 1)) {
        return false;
    }

    payload_ids_.push_back(payload_id);
    return true;
}

bool MediaDescription::add_connection_data(address::AddrFamily addrtype,
                                           const char* str,
                                           size_t str_len) {
    ConnectionData c;

    if (!c.set_connection_address(addrtype, str, str_len)) {
        return false;
    }

    if (!connection_data_.grow_exp(connection_data_.size() + 1)) {
        return false;
    }

    connection_data_.push_back(c);
    return true;
}

void MediaDescription::destroy() {
    allocator_.destroy(*this);
}

} // namespace roc
} // namespace sdp