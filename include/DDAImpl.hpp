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

#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <dxgi1_2.h>
#include <d3d11_2.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DDAImpl
{
private:
    /// Struct to hold information for each display
    struct DisplayDuplication
    {
        ComPtr<IDXGIOutputDuplication> pDup;
        DWORD width;
        DWORD height;
    };

    /// The D3D11 device used by the DDA session
    ComPtr<ID3D11Device> pD3DDev;
    /// The D3D11 Device Context used by the DDA session
    ComPtr<ID3D11DeviceContext> pCtx;
    /// Vector to hold duplication objects and information for each display
    DisplayDuplication display;
    /// Running count of no. of accumulated desktop updates
    int frameno = 0;
    /// output file stream to dump timestamps
    std::ofstream ofs;
    /// DXGI_OUTDUPL_FRAME_INFO::latPresentTime from the last Acquired frame
    LARGE_INTEGER lastPTS = { 0 };
    /// Clock frequency from QueryPerformaceFrequency()
    LARGE_INTEGER qpcFreq = { 0 };

public:
    /// Constructor
    DDAImpl(ID3D11Device *pDev, ID3D11DeviceContext* pDevCtx);
    /// Destructor. Release all resources before destroying the object
    ~DDAImpl();
    /// Initialize DDA
    HRESULT Init(int display, ComPtr<IDXGIDevice2> &pDevice, ComPtr<IDXGIFactory3> &pFactory, ComPtr<IDXGIAdapter> &pAdapter);
    /// Acquire a new frame from DDA, and return it as a Texture2D object.
    /// 'wait' specifies the time in milliseconds that DDA should wait for a new screen update.
    HRESULT GetCapturedFrame(ID3D11Texture2D **pTex2D, int wait);
    /// Return output width of a specific display
    const std::pair<const DWORD, const DWORD> getResolution() const;
};