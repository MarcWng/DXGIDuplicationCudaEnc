/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Defs.h"
#include "Preproc.h"
#include "CudaH264.hpp"
#include <memory>

/// Demo 60 FPS (approx.) capture
int Grab60FPS(int nFrames, int argc,char *argv[])
{
    std::unique_ptr<CudaH264> Cudah264 = std::make_unique<CudaH264>(argc, argv);
    const int WAIT_BASE = 10;
    HRESULT hr = S_OK;
    int capturedFrames = 0;
    LARGE_INTEGER start = { 0 };
    LARGE_INTEGER end = { 0 };
    LARGE_INTEGER interval = { 0 };
    LARGE_INTEGER freq = { 0 };
    int wait = WAIT_BASE;

    QueryPerformanceFrequency(&freq);

    /// Reset waiting time for the next screen capture attempt
#define RESET_WAIT_TIME(start, end, interval, freq)         \
    QueryPerformanceCounter(&end);                          \
    interval.QuadPart = end.QuadPart - start.QuadPart;      \
    MICROSEC_TIME(interval, freq);                          \
    wait = (int)(WAIT_BASE - (interval.QuadPart * 1000));

    /// Initialize Cudah264 app
    hr = Cudah264->Init();
    if (FAILED(hr))
    {
        printf("Initialization failed with error 0x%08x\n", hr);
        return -1;
    }

    /// Run capture loop
    do
    {
        /// get start timestamp. 
        /// use this to adjust the waiting period in each capture attempt to approximately attempt 60 captures in a second
        QueryPerformanceCounter(&start);
        /// Get a frame from DDA
        hr = Cudah264->Capture(wait);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) 
        {
            /// retry if there was no new update to the screen during our specific timeout interval
            /// reset our waiting time
            RESET_WAIT_TIME(start, end, interval, freq);
            continue;
        }
        else
        {
            if (FAILED(hr))
            {
                /// Re-try with a new DDA object
                printf("Captrue failed with error 0x%08x. Re-create DDA and try again.\n", hr);
                Cudah264->Cleanup(true);
                hr = Cudah264->Init();
                if (FAILED(hr))
                {
                    /// Could not initialize DDA, bail out/
                    printf("Failed to Init Cudah264-> return error 0x%08x\n", hr);
                    return -1;
                }
                RESET_WAIT_TIME(start, end, interval, freq);
                QueryPerformanceCounter(&start);
                /// Get a frame from DDA
                Cudah264->Capture(wait);
            }
            RESET_WAIT_TIME(start, end, interval, freq);
            /// Preprocess for encoding
            hr = Cudah264->Preproc(); 
            if (FAILED(hr))
            {
                printf("Preproc failed with error 0x%08x\n", hr);
                return -1;
            }


//            hr = Cudah264->Encode();
//            if (FAILED(hr))
//            {
//                printf("Encode failed with error 0x%08x\n", hr);
//                return -1;
//            }
            capturedFrames++;
        }
    } while (capturedFrames <= nFrames);

    return 0;
}

int main(int argc, char *argv[])
{
    /// The app will try to capture 20 times, by default
    int nFrames = 300;
    int ret = 0;
    bool useNvenc = true;

/// Kick off the demo
    ret = Grab60FPS(nFrames, argc, argv);
    return ret;
}