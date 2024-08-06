#include <iostream>
#include <memory>
#include "CudaH264.hpp"
#include <thread>
class Core
{
private:
    // std::vector<std::unique_ptr<CudaH264>> encoders;
    ComPtr<IDXGIDevice2> pDevice;
    ComPtr<IDXGIFactory3> pFactory;
    ComPtr<IDXGIAdapter> pAdapter;
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
    int nFrames;
    unsigned short displayNb;
    std::vector<std::unique_ptr<CudaH264>> encoders;
public:
    Core(){}
    Core(int, int, char *argv[]);
    
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

    ~Core();
    int Init(int argc, char *argv[]);

    void resetWaitTime(LARGE_INTEGER start, LARGE_INTEGER end, LARGE_INTEGER interval, LARGE_INTEGER freq);
    int DetectDisplays();

    int runThreads();
    int runLoop(int nFrames, unsigned short displayIndex);
};