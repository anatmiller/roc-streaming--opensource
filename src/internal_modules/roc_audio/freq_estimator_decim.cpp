/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/freq_estimator_decim.h"

const double roc::audio::fe_decim_h_gain = 1.041106363770361;

const double roc::audio::fe_decim_h[fe_decim_len] = {
    0.00002171816595,  0.001551611349,   0.0005492189666,  0.0006585246301,
    0.0007556059863,   0.0008557052352,  0.0009566077497,  0.00105746265,
    0.001156042097,    0.001251217443,   0.001340582385,   0.001422821078,
    0.001495413715,    0.001557081006,   0.001605384634,   0.001639141934,
    0.001656142762,    0.001655626693,   0.001635767752,   0.001596169546,
    0.00153552962,     0.001454023994,   0.001350906445,   0.001226948923,
    0.001082253992,    0.0009182369104,  0.0007358249859,  0.0005372641608,
    0.0003243022948,   0.00009991019033, -0.0001335729467, -0.0003724064736,
    -0.0006139134057,  -0.000853834732,  -0.001088955207,  -0.001314683235,
    -0.001527603134,   -0.001722955029,  -0.001897459268,  -0.002046865411,
    -0.002168231178,   -0.002257786226,  -0.002313192002,  -0.002331318334,
    -0.002310532611,   -0.002248884644,  -0.002146301325,  -0.002002278809,
    -0.001817821641,   -0.001593292691,  -0.001330877072,  -0.001032887027,
    -0.00070385047,    -0.0003474208061, 0.00003207824193, 0.0004301121517,
    0.0008386967238,   0.001251969952,   0.001665653544,   0.002068829956,
    0.002457238268,    0.002822061069,   0.003156311577,   0.00345301209,
    0.003704900853,    0.003905905643,   0.004049625248,   0.00413125474,
    0.004145852756,    0.004090175498,   0.003961127717,   0.003757563187,
    0.003478640225,    0.003125529503,   0.002699935576,   0.002205569064,
    0.001646643854,    0.001029419829,   0.0003607070539,  -0.0003507778456,
    -0.001095934655,   -0.001863716752,  -0.002643183107,  -0.003421482397,
    -0.00418635644,    -0.004923689179,  -0.005620298441,  -0.006261858623,
    -0.0068346099,     -0.007324781734,  -0.007719407789,  -0.008005576208,
    -0.008171834052,   -0.008207269013,  -0.008102368563,  -0.007848716341,
    -0.007440029643,   -0.006871134974,  -0.006138926838,  -0.005241762381,
    -0.004180506337,   -0.002957628109,  -0.00157801155,   -0.00004807108053,
    0.001623484422,    0.003426218173,   0.005347401369,   0.007373005617,
    0.00948741287,     0.01167352311,    0.01391246822,    0.01618501171,
    0.01847111061,     0.02074959502,    0.02299895883,    0.02519862913,
    0.02732633427,     0.02936225012,    0.0312856175,     0.03307762742,
    0.03471996263,     0.03619609028,    0.03749086335,    0.03859099001,
    0.03948510438,     0.04016397893,    0.04062053561,    0.04085000232,
    0.04085000232,     0.04062053561,    0.04016397893,    0.03948510438,
    0.03859099001,     0.03749086335,    0.03619609028,    0.03471996263,
    0.03307762742,     0.0312856175,     0.02936225012,    0.02732633427,
    0.02519862913,     0.02299895883,    0.02074959502,    0.01847111061,
    0.01618501171,     0.01391246822,    0.01167352311,    0.00948741287,
    0.007373005617,    0.005347401369,   0.003426218173,   0.001623484422,
    -0.00004807108053, -0.00157801155,   -0.002957628109,  -0.004180506337,
    -0.005241762381,   -0.006138926838,  -0.006871134974,  -0.007440029643,
    -0.007848716341,   -0.008102368563,  -0.008207269013,  -0.008171834052,
    -0.008005576208,   -0.007719407789,  -0.007324781734,  -0.0068346099,
    -0.006261858623,   -0.005620298441,  -0.004923689179,  -0.00418635644,
    -0.003421482397,   -0.002643183107,  -0.001863716752,  -0.001095934655,
    -0.0003507778456,  0.0003607070539,  0.001029419829,   0.001646643854,
    0.002205569064,    0.002699935576,   0.003125529503,   0.003478640225,
    0.003757563187,    0.003961127717,   0.004090175498,   0.004145852756,
    0.00413125474,     0.004049625248,   0.003905905643,   0.003704900853,
    0.00345301209,     0.003156311577,   0.002822061069,   0.002457238268,
    0.002068829956,    0.001665653544,   0.001251969952,   0.0008386967238,
    0.0004301121517,   0.00003207824193, -0.0003474208061, -0.00070385047,
    -0.001032887027,   -0.001330877072,  -0.001593292691,  -0.001817821641,
    -0.002002278809,   -0.002146301325,  -0.002248884644,  -0.002310532611,
    -0.002331318334,   -0.002313192002,  -0.002257786226,  -0.002168231178,
    -0.002046865411,   -0.001897459268,  -0.001722955029,  -0.001527603134,
    -0.001314683235,   -0.001088955207,  -0.000853834732,  -0.0006139134057,
    -0.0003724064736,  -0.0001335729467, 0.00009991019033, 0.0003243022948,
    0.0005372641608,   0.0007358249859,  0.0009182369104,  0.001082253992,
    0.001226948923,    0.001350906445,   0.001454023994,   0.00153552962,
    0.001596169546,    0.001635767752,   0.001655626693,   0.001656142762,
    0.001639141934,    0.001605384634,   0.001557081006,   0.001495413715,
    0.001422821078,    0.001340582385,   0.001251217443,   0.001156042097,
    0.00105746265,     0.0009566077497,  0.0008557052352,  0.0007556059863,
    0.0006585246301,   0.0005492189666,  0.001551611349,   0.00002171816595
};
