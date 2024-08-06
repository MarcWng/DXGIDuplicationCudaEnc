#pragma once

#include "IEncoder.hpp"
#include <memory>
#include "DDAImpl.hpp"
#include "NvEncoder/NvEncoderCuda.h"
#include "NvEncoder/NvEncoderD3D11.h"

class CudaH264 : public IEncoder
{
    #define returnIfError(x)\
    if (FAILED(x))\
    {\
        printf("%s: Line %d, File %s Returning error 0x%08x\n",__FUNCTION__, __LINE__, __FILE__, x);\
        return x;\
    }
private:
    CUdevice cuDevice = 0;
    /// Cuda device context used for the operations demonstrated in this application
    CUcontext cuContext;

    /// DDA wrapper object, defined in DDAImpl.h
    DDAImpl *pDDAWrapper = nullptr;


    /// NVENCODE API wrapper. Defined in NvEncoderCuda.h. This class is imported from NVIDIA Video SDK
    NvEncoderCuda *pEnc;
    /// D3D11 device context used for the operations demonstrated in this application
    ID3D11Device *pD3DDev = nullptr;
    CUdeviceptr dptr;
    size_t size;
    CUarray cuArray;


    /// D3D11 RGB Texture2D object that recieves the captured image from DDA
    ID3D11Texture2D *pDupTex2D = nullptr;
    /// D3D11 RGB Texture2D object that has the correct rights to send the image to NVENCCUDA for mapping and video encoding
    ID3D11Texture2D *pEncBuf = nullptr;
    /// pEncBuf Description


    /// D3D11 device context
    ID3D11DeviceContext *pCtx = nullptr;

    /// Encoded video bitstream packet in CPU memory
    std::vector<std::vector<uint8_t>> vPacket;

    /// Arguments for Cuda
    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
    //    NvEncoderInitParam encodeCLIOptions;
    /// display index
    int displayIndex;
    int iGpu;
    std::ofstream fpOut;
    /// Failure count from Capture API
    UINT failCount = 0;

public:
    explicit CudaH264(int DisplayIndex);
    ~CudaH264() override;
    HRESULT InitEnc() override;
    HRESULT Encode() { return (S_OK); }
    HRESULT Encode(CUarray_st *cuArray);
    HRESULT WriteRawFrame(ID3D11Texture2D *pBuffer);
    void Cleanup(bool bDelete) override;

    // Desktop duplication
    /// Just a method to init everything
    HRESULT Init(ComPtr<IDXGIDevice2> &pDevice, ComPtr<IDXGIFactory3> &pFactory, ComPtr<IDXGIAdapter> &pAdapter);

    /// Initialize open the output file
    HRESULT InitOutFile();

     /// Initialize DDA handler
    HRESULT InitDup(ComPtr<IDXGIDevice2> &pDevice, ComPtr<IDXGIFactory3> &pFactory, ComPtr<IDXGIAdapter> &pAdapter);

    /// Initialize DXGI pipeline
    HRESULT InitDXGI();

    /// Capture a frame using DDA
    HRESULT Capture(int wait);

    /// Preprocess captured frame
    HRESULT Preproc();

    void WriteEncOutput();

    HRESULT SaveFrameToFile(const void* pBuffer, int width, int height);
};
