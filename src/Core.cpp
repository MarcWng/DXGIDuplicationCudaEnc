#include "Core.hpp"
#include <dxgi1_2.h>

Core::Core(int _nFrames, int argc, char *argv[])
: nFrames(_nFrames), displayNb(DetectDisplays())
{
    Init(argc, argv);
}

void Core::resetWaitTime(LARGE_INTEGER start, LARGE_INTEGER end, LARGE_INTEGER interval, LARGE_INTEGER freq)
{
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&end);
    interval.QuadPart = end.QuadPart - start.QuadPart;
    wait = WAIT_BASE - (int)(interval.QuadPart / 1000);
    if (wait < 0)
        wait = 0;
}

int Core::runThreads()
{
    // run as much threads as we have displays
    std::vector<std::thread> threads;

    // Create and launch threads for each display

    for (int i = 0; i < displayNb; ++i) {
        // Using lambda to bind the member function with 'this' pointer and arguments
        threads.emplace_back([this](int frames, unsigned short displayIndex) {
            this->runLoop(frames, displayIndex);
        }, nFrames, i);
    }

    // Join the threads to ensure they complete before exiting main
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    

    return 0;
}

int Core::DetectDisplays()
{
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);
    if (FAILED(hr))
    {
        std::cout << "Failed to create DXGIFactory1: " << std::hex << hr << std::endl;
        return -1;
    }

    UINT adapterIndex = 0;
    IDXGIAdapter1* pAdapter = nullptr;
    UINT outputCounts = 0;
    std::cout << "Display Adapters and their Outputs (Monitors):" << std::endl;
    while (pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        pAdapter->GetDesc1(&adapterDesc);

        std::wcout << L"Adapter " << adapterIndex << L": " << adapterDesc.Description << std::endl;

        UINT outputIndex = 0;
        IDXGIOutput* pOutput = nullptr;

        while (pAdapter->EnumOutputs(outputIndex, &pOutput) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_OUTPUT_DESC outputDesc;
            pOutput->GetDesc(&outputDesc);

            std::wcout << L"  Output " << outputIndex << L": " << outputDesc.DeviceName << std::endl;

            pOutput->Release();
            outputIndex++;
            outputCounts++;
        }

        pAdapter->Release();
        adapterIndex++;
    }
    std::cout << outputCounts << " displays detected" << std::endl;

    pFactory->Release();
    return outputCounts;
}

int Core::Init(int argc, char *argv[]) {
    QueryPerformanceFrequency(&freq);

    // Detect how many displays we have

    for (int displayIndex = 0; displayIndex < displayNb; ++displayIndex) {
        std::unique_ptr<CudaH264> encoder = std::make_unique<CudaH264>(displayIndex);
        HRESULT hr = encoder->Init(pDevice, pFactory, pAdapter);
        if (hr != S_OK) {
            std::cout << "Failed to initialize encoder for display " << displayIndex << std::endl;
            return -1;
        }
        encoders.push_back(std::move(encoder));
    }

    return 0;
}


Core::~Core()
{
}
