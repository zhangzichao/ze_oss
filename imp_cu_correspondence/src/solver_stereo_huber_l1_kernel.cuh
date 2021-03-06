// Copyright (c) 2015-2016, ETH Zurich, Wyss Zurich, Zurich Eye
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the ETH Zurich, Wyss Zurich, Zurich Eye nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL ETH Zurich, Wyss Zurich, Zurich Eye BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <cuda_runtime_api.h>
#include <imp/core/types.hpp>
#include <imp/cu_core/cu_utils.hpp>
#include <imp/cu_core/cu_k_derivative.cuh>
#include <imp/cuda_toolkit/helper_math.hpp>


namespace ze {
namespace cu {

//-----------------------------------------------------------------------------
/** restricts the udpate to +/- lin_step around the given value in lin_tex
 * @note \a d_srcdst and the return value is identical.
 * @todo (MWE) move function to common kernel def file for all stereo models
 */
template<typename Pixel>
__device__ Pixel k_linearized_update(Pixel& d_srcdst, Texture2D& lin_tex,
                                     const float lin_step,
                                     const int x, const int y)
{
  Pixel lin;
  tex2DFetch(lin, lin_tex, x, y);
  d_srcdst = ze::cu::max(lin-lin_step,
                          ze::cu::min(lin+lin_step, d_srcdst));
  return d_srcdst;
}

//-----------------------------------------------------------------------------
/**
 * @brief k_primalUpdate is the Huber-L1-Precondition model's primal update kernel
 * @note PPixel and DPixel denote for the Pixel type/dimension of primal and dual variable
 */
template<typename PPixel>
__global__ void k_primalUpdate(PPixel* d_u, PPixel* d_u_prev, const size_t stride,
                               uint32_t width, uint32_t height,
                               const float lambda, const float tau,
                               const float lin_step,
                               Texture2D u_tex, Texture2D u0_tex,
                               Texture2D pu_tex, Texture2D ix_tex, Texture2D it_tex)
{
  const int x = blockIdx.x*blockDim.x + threadIdx.x;
  const int y = blockIdx.y*blockDim.y + threadIdx.y;

  if (x<width && y<height)
  {
    float u0 = tex2DFetch<float>(u0_tex, x,y);
    float it = tex2DFetch<float>(it_tex, x, y);
    float ix = tex2DFetch<float>(ix_tex, x, y);

    // divergence operator (dpAD) of dual var
    float div = dpAd(pu_tex, x, y, width, height);

    // save current u
    float u_prev = tex2DFetch<float>(u_tex, x, y);
    float u = u_prev;
    u += tau*div;

    // prox operator
    float prox = it + (u-u0)*ix;
    prox /= max(1e-9f, ix*ix);
    float tau_lambda = tau*lambda;

    if(prox < -tau_lambda)
    {
      u += tau_lambda*ix;
    }
    else if(prox > tau_lambda)
    {
      u -= tau_lambda*ix;
    }
    else
    {
      u -= prox*ix;
    }

    // restrict update step because of linearization only valid in small neighborhood
    u = k_linearized_update(u, u0_tex, lin_step, x, y);

    d_u[y*stride+x] = u;
    d_u_prev[y*stride+x] = 2.f*u - u_prev;
  }
}

//-----------------------------------------------------------------------------
template<typename DPixel>
__global__ void k_dualUpdate(DPixel* d_pu, const size_t stride_pu,
                             uint32_t width, uint32_t height,
                             const float eps_u, const float sigma,
                             Texture2D u_prev_tex, Texture2D pu_tex)
{
  const int x = blockIdx.x*blockDim.x + threadIdx.x;
  const int y = blockIdx.y*blockDim.y + threadIdx.y;
  if (x<width && y<height)
  {
    // update pu
    float2 du = dp(u_prev_tex, x, y);
    float2 pu = tex2DFetch<float2>(pu_tex, x,y);
    pu  = (pu + sigma*du) / (1.f + sigma*eps_u);
    pu = pu / max(1.0f, length(pu));

    d_pu[y*stride_pu+x] = {pu.x, pu.y};
  }
}


} // namespace cu
} // namespace ze

