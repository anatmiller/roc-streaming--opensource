/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_table.h"
#include "roc_audio/channel_layout.h"

namespace roc {
namespace audio {

// These tables define downmixing coefficients for mapping between different
// surround channel sets. They are used for both downmixing and upmixing.
//
// Mappings should be ordered from smaller to larger masks, becase channel mapper
// will use the very first pair that covers both output and input masks.
//
// Only downmixing mappings are defined. Upmixing mappings are derived
// automatically from them.
//
// Technically, some of the mappings are actually partially downmixing, and
// partially upmixing, for example mapping from 6.x to 5.1.x downmixes some
// channels and upmixes others. However, for convenience, we still call it
// "downmixing" because we consider 6.x to be a "larger" channel set than 5.x.
//
// For groups of similar layouts, when possible, mappings are defined only for
// the most complete layout, and are automatically reused for the rest. For example,
// mappings for 5.1.2 may be automatically used for 5.1 and 5.0.
//
// These tables are based on the following documents:
//  - ITU-R BS.775-1, ANNEX 4
//  - A/52, Digital Audio Compression (AC-3) (E-AC-3) Standard, sections 6.1.12 and 7.8
//
// Useful links:
//  https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.775-1-199407-S!!PDF-E.pdf
//  https://prdatsc.wpenginepowered.com/wp-content/uploads/2021/04/A52-2018.pdf
//  https://www.audiokinetic.com/en/library/edge/?source=Help&id=downmix_tables
//  https://trac.ffmpeg.org/wiki/AudioChannelManipulation
//  https://superuser.com/questions/852400

const ChannelMap chan_maps[chan_map_count] = {
    // 2.1->...
    {
        "2.1->1.0",
        ChanMask_Surround_2_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 1.000f },
        },
    },
    // 3.1->...
    {
        "3.1->1.0",
        ChanMask_Surround_3_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
        },
    },
    {
        "3.1->2.1",
        ChanMask_Surround_3_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 4.1->...
    {
        "4.1->1.0",
        ChanMask_Surround_4_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
        },
    },
    {
        "4.1->2.1",
        ChanMask_Surround_4_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "4.1->3.1",
        ChanMask_Surround_4_1,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 5.1.2->...
    {
        "5.1.2->1.0",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidRight, 0.500f },
        },
    },
    {
        "5.1.2->2.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.2->3.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.2->4.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopMidLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.2->5.1",
        ChanMask_Surround_5_1_2,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopMidLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 5.1.4->...
    {
        "5.1.4->1.0",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopBackLeft, 0.354f },
            { ChanPos_FrontCenter, ChanPos_TopBackRight, 0.354f },
        },
    },
    {
        "5.1.4->2.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->3.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->4.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopBackLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->5.1",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_5_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopBackLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "5.1.4->5.1.2",
        ChanMask_Surround_5_1_4,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopBackLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopBackRight, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 1.000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 6.1->...
    {
        "6.1->1.0",
        ChanMask_Surround_6_1,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundCenter, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
        },
    },
    {
        "6.1->2.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundCenter, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundCenter, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->3.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundCenter, 0.500f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundCenter, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->4.1",
        ChanMask_Surround_6_1,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_SurroundCenter, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_SurroundCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->5.1.2",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_SurroundCenter, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_SurroundCenter, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_SurroundCenter, 0.707f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_SurroundCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "6.1->5.1.4",
        ChanMask_Surround_6_1,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_SurroundCenter, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_SurroundCenter, 0.707f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_FrontLeft, 1.000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_FrontRight, 1.000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_TopBackLeft, ChanPos_SurroundCenter, 0.707f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_TopBackRight, ChanPos_SurroundCenter, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 7.1.2->...
    {
        "7.1.2->1.0",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopMidRight, 0.500f },
        },
    },
    {
        "7.1.2->2.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->3.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->4.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopMidLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->5.1.2",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopMidLeft, 1.000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopMidRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->5.1.4",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopMidLeft, 1.000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopMidRight, 1.000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopMidLeft, 1.000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopMidRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->6.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopMidLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopMidRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopMidLeft, 0.707f },
            // SC
            { ChanPos_SurroundCenter, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_TopMidLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopMidRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.2->7.1",
        ChanMask_Surround_7_1_2,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopMidLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopMidRight, 0.707f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    // 7.1.4->...
    {
        "7.1.4->1.0",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_Mono,
        {
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontLeft, 0.707f },
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            { ChanPos_FrontCenter, ChanPos_FrontRight, 0.707f },
            { ChanPos_FrontCenter, ChanPos_SurroundLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_SurroundRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_BackRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontLeft, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopFrontRight, 0.500f },
            { ChanPos_FrontCenter, ChanPos_TopBackLeft, 0.354f },
            { ChanPos_FrontCenter, ChanPos_TopBackRight, 0.354f },
        },
    },
    {
        "7.1.4->2.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_2_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->3.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_3_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_SurroundLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_BackLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopBackLeft, 0.500f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_SurroundRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_BackRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopBackRight, 0.500f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->4.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_4_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_FrontCenter, 0.707f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopBackLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->5.1.2",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopBackLeft, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopBackRight, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 1.000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->5.1.4",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_5_1_4,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            // TFL
            { ChanPos_TopFrontLeft, ChanPos_TopFrontLeft, 1.000f },
            // TFR
            { ChanPos_TopFrontRight, ChanPos_TopFrontRight, 1.000f },
            // TBL
            { ChanPos_TopBackLeft, ChanPos_TopBackLeft, 1.000f },
            // TBR
            { ChanPos_TopBackRight, ChanPos_TopBackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->6.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_6_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundLeft, ChanPos_TopBackLeft, 0.707f },
            // SC
            { ChanPos_SurroundCenter, ChanPos_SurroundLeft, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_BackLeft, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundCenter, ChanPos_TopBackLeft, 0.707f },
            { ChanPos_SurroundCenter, ChanPos_TopBackRight, 0.707f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_BackRight, 1.000f },
            { ChanPos_SurroundRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->7.1",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_7_1,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
    {
        "7.1.4->7.1.2",
        ChanMask_Surround_7_1_4,
        ChanMask_Surround_7_1_2,
        {
            // FL
            { ChanPos_FrontLeft, ChanPos_FrontLeft, 1.000f },
            { ChanPos_FrontLeft, ChanPos_TopFrontLeft, 0.707f },
            // FC
            { ChanPos_FrontCenter, ChanPos_FrontCenter, 1.000f },
            // FR
            { ChanPos_FrontRight, ChanPos_FrontRight, 1.000f },
            { ChanPos_FrontRight, ChanPos_TopFrontRight, 0.707f },
            // SL
            { ChanPos_SurroundLeft, ChanPos_SurroundLeft, 1.000f },
            // SR
            { ChanPos_SurroundRight, ChanPos_SurroundRight, 1.000f },
            // BL
            { ChanPos_BackLeft, ChanPos_BackLeft, 1.000f },
            { ChanPos_BackLeft, ChanPos_TopBackLeft, 0.707f },
            // BR
            { ChanPos_BackRight, ChanPos_BackRight, 1.000f },
            { ChanPos_BackRight, ChanPos_TopBackRight, 0.707f },
            // TML
            { ChanPos_TopMidLeft, ChanPos_TopFrontLeft, 1.000f },
            { ChanPos_TopMidLeft, ChanPos_TopBackLeft, 1.000f },
            // TMR
            { ChanPos_TopMidRight, ChanPos_TopFrontRight, 1.000f },
            { ChanPos_TopMidRight, ChanPos_TopBackRight, 1.000f },
            // LFE
            { ChanPos_LowFrequency, ChanPos_LowFrequency, 1.000f },
        },
    },
};

} // namespace audio
} // namespace roc
