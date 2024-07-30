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

#include "Defs.hpp"
#include "Preproc.hpp"
#include "CudaH264.hpp"
#include <memory>


int runLoop(int nFrames, int argc, char *argv[])
{
    std::unique_ptr<CudaH264> Cudah264 = std::make_unique<CudaH264>(argc, argv);
    const int WAIT_BASE = 17; // 17 ms per frames = 60 FPS
    HRESULT hr = S_OK;
    int capturedFrames = 0;
    // for the capture time
    LARGE_INTEGER start = {0};
    LARGE_INTEGER end = {0};
    LARGE_INTEGER interval = {0};
    LARGE_INTEGER freq = {0};
    int wait = WAIT_BASE;

    // for the preproc time
    // to delete later :
    LARGE_INTEGER START = {0};
    LARGE_INTEGER END = {0};
    LARGE_INTEGER INTERVAL = {0};
    int wait2 = WAIT_BASE;
    //

    QueryPerformanceFrequency(&freq);

    /// Reset waiting time for the next screen capture attempt
#define RESET_WAIT_TIME(start, end, interval, freq)                                 \
    QueryPerformanceCounter(&end);                                                  \
    interval.QuadPart = end.QuadPart - start.QuadPart;                              \
    MICROSEC_TIME(interval, freq);                                                  \
    wait = (int)((WAIT_BASE) - (interval.QuadPart / 1000));                         \
    if (wait < 0) wait = 0;                                                                 \
    // std::cout << "Interval: " << interval.QuadPart << " microseconds" << std::endl; \
    // std::cout << "Start Time: " << start.QuadPart << " ticks" << std::endl;         \
    // std::cout << "End Time: " << end.QuadPart << " ticks" << std::endl;             \
    // std::cout << "Wait Time: " << wait << " millisecconds" << std::endl;            \
    // std::cout << "Wait Time in Microseconds: " << (int)((WAIT_BASE * 1000) - (interval.QuadPart)) << " microseconds" << std::endl;

#define RESET_WAIT_TIME2(START, END, INTERVAL, freq)                                \
    QueryPerformanceCounter(&END);                                                  \
    INTERVAL.QuadPart = END.QuadPart - START.QuadPart;                              \
    MICROSEC_TIME(INTERVAL, freq);                                                  \
    wait2 = (int)((WAIT_BASE) - (INTERVAL.QuadPart / 1000));                        \
    // std::cout << "Interval: " << INTERVAL.QuadPart << " microseconds" << std::endl; \
    // std::cout << "Start Time: " << START.QuadPart << " ticks" << std::endl;         \
    // std::cout << "End Time: " << END.QuadPart << " ticks" << std::endl;             \
    // std::cout << "Wait Time: " << wait2 << " millisecconds" << std::endl;           \
    // std::cout << "Wait Time in Microseconds: " << (int)((WAIT_BASE * 1000) - (INTERVAL.QuadPart)) << " microseconds" << std::endl;

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
        RESET_WAIT_TIME(start, end, interval, freq);
        std::cout << " ---- capture took " << interval.QuadPart / 1000 << " milliseconds" << std::endl;

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
                printf("Capture failed with error 0x%08x. Re-create DDA and try again.\n", hr);
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
                Cudah264->Capture(wait); // - 1 ms
            }
            QueryPerformanceCounter(&START);
            hr = Cudah264->Preproc(); // Encode 1 frame full HD = 2-3 ms // result 1-3 ms
            RESET_WAIT_TIME2(START, END, INTERVAL, freq);
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
    int nFrames = 61;
    int ret = 0;
    bool useNvenc = true;

    ret = runLoop(nFrames, argc, argv);
    return ret;
}
