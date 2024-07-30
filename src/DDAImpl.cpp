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
#include "DDAImpl.hpp"
#include <iomanip>

DDAImpl::DDAImpl(ID3D11Device *pDev, ID3D11DeviceContext *pDevCtx)
    : pD3DDev(pDev), pCtx(pDevCtx)
{
    pD3DDev->AddRef();
    pCtx->AddRef();
    ofs = std::ofstream("PresentTSLog.txt");
    QueryPerformanceFrequency(&qpcFreq);
}

DDAImpl::~DDAImpl()
{
    for (auto &display : displays)
    {
        if (display.pDup)
        {
            display.pDup->ReleaseFrame();
        }
    }
}

HRESULT DDAImpl::Init()
{
    ComPtr<IDXGIDevice2> pDevice;
    ComPtr<IDXGIFactory1> pFactory;
    ComPtr<IDXGIAdapter> pAdapter;

    HRESULT hr = S_OK;

    if (FAILED(hr = pD3DDev->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void **>(pDevice.GetAddressOf()))))
        return (hr);

    if (FAILED(hr = pDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(pAdapter.GetAddressOf()))))
        return (hr);

    // Enumerate all outputs
    for (UINT i = 0; ; ++i)
    {
        ComPtr<IDXGIOutput> pOutput;
        ComPtr<IDXGIOutput1> pOut1;

        if (FAILED(hr = pAdapter->EnumOutputs(i, pOutput.GetAddressOf())))
        {
            if (hr == DXGI_ERROR_NOT_FOUND) {
                hr = S_OK;
                break; // No more outputs
            }
            return hr; // Other error
        }

        if (FAILED(hr = pOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void **>(pOut1.GetAddressOf()))))
            return (hr);

        ComPtr<IDXGIOutputDuplication> pDuplication;
        if (FAILED(hr = pOut1->DuplicateOutput(pDevice.Get(), pDuplication.GetAddressOf())))
            return (hr);

        DXGI_OUTDUPL_DESC outDesc;
        ZeroMemory(&outDesc, sizeof(outDesc));
        pDuplication->GetDesc(&outDesc);

        DisplayDuplication display = { pDuplication, outDesc.ModeDesc.Width, outDesc.ModeDesc.Height };
        displays.push_back(display);
    }

    return hr;
}

HRESULT DDAImpl::GetCapturedFrame(ID3D11Texture2D **ppTex2D, int wait, int displayIndex)
{
    if (displayIndex < 0 || displayIndex >= displays.size())
        return E_INVALIDARG;

    auto &display = displays[displayIndex];
    HRESULT hr = S_OK;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    ZeroMemory(&frameInfo, sizeof(frameInfo));
    int acquired = 0;

#define RETURN_ERR(x)                                                                 \
    {                                                                                 \
        printf("%s: %d : Line %d return 0x%x\n", __FUNCTION__, frameno, __LINE__, x); \
        return x;                                                                     \
    }

    ComPtr<IDXGIResource> pResource;
    display.pDup->ReleaseFrame();
    hr = display.pDup->AcquireNextFrame(wait, &frameInfo, pResource.GetAddressOf());
    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            printf("%s: %d : Wait for %d ms timed out\n", __FUNCTION__, frameno, wait);
        }
        if (hr == DXGI_ERROR_INVALID_CALL)
        {
            printf("%s: %d : Invalid Call, previous frame not released?\n", __FUNCTION__, frameno);
        }
        if (hr == DXGI_ERROR_ACCESS_LOST)
        {
            printf("%s: %d : Access lost, frame needs to be released?\n", __FUNCTION__, frameno);
        }
        RETURN_ERR(hr);
    }
    if (frameInfo.AccumulatedFrames == 0 || frameInfo.LastPresentTime.QuadPart == 0)
    {
        // No image update, only cursor moved.
        ofs << "frameNo: " << frameno << " | Accumulated: " << frameInfo.AccumulatedFrames << "MouseOnly?" << frameInfo.LastMouseUpdateTime.QuadPart << std::endl;
        RETURN_ERR(DXGI_ERROR_WAIT_TIMEOUT);
    }

    if (!pResource)
    {
        printf("%s: %d : Null output resource. Return error.\n", __FUNCTION__, frameno);
        return E_UNEXPECTED;
    }

    if (FAILED(hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void **)ppTex2D)))
    {
        return hr;
    }

    LARGE_INTEGER pts = frameInfo.LastPresentTime;
    MICROSEC_TIME(pts, qpcFreq);
    LONGLONG interval = pts.QuadPart - lastPTS.QuadPart;

    printf("%s: %d : Accumulated Frames %u PTS Interval %lld PTS %lld\n", __FUNCTION__, frameno, frameInfo.AccumulatedFrames, interval * 1000, frameInfo.LastPresentTime.QuadPart);
    ofs << "frameNo: " << frameno << " | Accumulated: " << frameInfo.AccumulatedFrames << " | PTS: " << frameInfo.LastPresentTime.QuadPart << " | PTSInterval: " << (interval) * 1000 << std::endl;
    lastPTS = pts; // store microsec value
    frameno += frameInfo.AccumulatedFrames;

    return hr;
}

DWORD DDAImpl::getWidth(int displayIndex)
{
    if (displayIndex < 0 || displayIndex >= displays.size())
        return 0;

    return displays[displayIndex].width;
}

DWORD DDAImpl::getHeight(int displayIndex)
{
    if (displayIndex < 0 || displayIndex >= displays.size())
        return 0;

    return displays[displayIndex].height;
}
