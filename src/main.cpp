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

// #include "Defs.hpp"
// #include "Preproc.hpp"
#include "Core.hpp"
#include <thread>
int Core::runLoop(int nFrames, unsigned short displayIndex)
{
    std::unique_ptr<CudaH264> &Cudah264 = encoders[displayIndex];
    /// Run capture loop
    do
    {
        /// get start timestamp.
        /// use this to adjust the waiting period in each capture attempt to approximately attempt 60 captures in a second
        QueryPerformanceCounter(&start);
        /// Get a frame from DDA
        hr = Cudah264->Capture(wait);
        resetWaitTime(start, end, interval, freq);
        std::cout << " ---- capture took " << interval.QuadPart / 1000 << " milliseconds" << std::endl;

        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            /// retry if there was no new update to the screen during our specific timeout interval
            /// reset our waiting time
            resetWaitTime(start, end, interval, freq);
            continue;
        }
        else
        {
            if (FAILED(hr))
            {
                /// Re-try with a new DDA object
                printf("Capture failed with error 0x%08x. Re-create DDA and try again.\n", hr);
                Cudah264->Cleanup(true);
                hr = Cudah264->Init(pDevice, pFactory, pAdapter);
                if (FAILED(hr))
                {
                    /// Could not initialize DDA, bail out/
                    printf("Failed to Init Cudah264-> return error 0x%08x\n", hr);
                    return -1;
                }
                resetWaitTime(start, end, interval, freq);
                QueryPerformanceCounter(&start);
                /// Get a frame from DDA
                Cudah264->Capture(wait); // - 1 ms
            }
            // QueryPerformanceCounter(&START);
            hr = Cudah264->Preproc(); // Encode 1 frame full HD = 2-3 ms // result 1-3 ms
            // resetWaitTime2(START, END, INTERVAL, freq);
            std::cout << " ______ Preproc took " << INTERVAL.QuadPart / 1000 << " milliseconds" << std::endl;
            std::cout << "///////////////////////////////////// " << std::endl;
            if (FAILED(hr))
            {
                printf("Preproc failed with error 0x%08x\n", hr);
                return -1;
            }
            capturedFrames++;
            // Total = 8 ms max
        }
    } while (capturedFrames <= nFrames);

    return 0;
}

int main(int argc, char *argv[])
{
    int nFrames = 200;
    int ret = 0;
    Core core(nFrames, argc, argv);
    core.runThreads();
    return ret;
}
