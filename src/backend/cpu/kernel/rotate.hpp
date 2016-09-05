/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <Array.hpp>
#include <math.hpp>
#include <err_cpu.hpp>
#include "interp.hpp"

namespace cpu
{
namespace kernel
{

template<typename T, int order>
void rotate(Array<T> output, const Array<T> input,
            const float theta, af_interp_type method)
{
    typedef typename dtype_traits<T>::base_type BT;
    typedef wtype_t<BT> WT;
    Interp2<T, WT, order> interp;

    const af::dim4 odims    = output.dims();
    const af::dim4 idims    = input.dims();
    const af::dim4 ostrides = output.strides();
    const af::dim4 istrides = input.strides();

    const float c = cos(-theta), s = sin(-theta);
    float tx, ty;
    {
        const float nx = 0.5 * (idims[0] - 1);
        const float ny = 0.5 * (idims[1] - 1);
        const float mx = 0.5 * (odims[0] - 1);
        const float my = 0.5 * (odims[1] - 1);
        const float sx = (mx * c + my *-s);
        const float sy = (mx * s + my * c);
        tx = -(sx - nx);
        ty = -(sy - ny);
    }

    const float tmat[6] = {std::round( c * 1000) / 1000.0f,
                           std::round(-s * 1000) / 1000.0f,
                           std::round(tx * 1000) / 1000.0f,
                           std::round( s * 1000) / 1000.0f,
                           std::round( c * 1000) / 1000.0f,
                           std::round(ty * 1000) / 1000.0f,
                          };

    for (int idw = 0; idw < (int)odims[3]; idw++) {
        for (int idz = 0; idz < (int)odims[2]; idz++) {

            int out_off = idz * ostrides[2] + idw * ostrides[3];
            int in_off  = idz * istrides[2] + idw * istrides[3];
            T *out = output.get() + out_off;

            // Do transform for image
            for(int idy = 0; idy < (int)odims[1]; idy++) {
                for(int idx = 0; idx < (int)odims[0]; idx++) {
                    WT xidi = idx * tmat[0] + idy * tmat[1] + tmat[2];
                    WT yidi = idx * tmat[3] + idy * tmat[4] + tmat[5];

                    // Special conditions to deal with boundaries for bilinear and bicubic
                    // FIXME: Ideally this condition should be removed or be present for all methods
                    // But tests are expecting a different behavior for bilinear and nearest
                    bool condX = xidi >= -0.0001 && xidi < idims[0];
                    bool condY = yidi >= -0.0001 && yidi < idims[1];
                    T val = scalar<T>(0);
                    if (order == 1 || (condX && condY)) {
                        // FIXME: Nearest and lower do not do clamping, but other methods do
                        // Make it consistent
                        bool clamp = order != 1;
                        val = interp(input, in_off, xidi, yidi, method, clamp);
                    }
                    out[idy * ostrides[1] + idx] = val;
                }
            }
        }
    }
}

}
}
