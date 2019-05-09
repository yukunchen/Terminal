#pragma once

#ifndef __H264_ENCODERINTERFACE_H__
#define __H264_ENCODERINTERFACE_H__

#include <windows.h>
#include <memory>
#include <vector>
#include <string>

#ifdef H264ENCODER_EXPORTS
#define DLL_API __declspec(dllexport) 
#else
#define DLL_API __declspec(dllimport) 
#endif

typedef struct _ENCODE_INIT_PARAM
{
    UINT32 fps;
    UINT64 bitRate;
    PVOID pTexDesc;
    PVOID pParentDevice;
}ENCODE_INIT_PARAM, *PENCODE_INIT_PARAM;

class H264Encoder;
class H264EncoderInterface
{
public:
    DLL_API H264EncoderInterface();
    DLL_API ~H264EncoderInterface();
    DLL_API bool InitEncoder(const PENCODE_INIT_PARAM pInitParam);
    DLL_API void DestroyEncoder();
    DLL_API void EncodeFrame(PVOID pTexture);
    DLL_API bool QueryOutput(PBYTE pBuf, UINT32 bufSize, UINT32* pOutSize, UINT64* pFrameDuration);
    DLL_API bool DrainPipeline();
    DLL_API UINT64 QueryLatency();
    DLL_API UINT64 GetEncodeInputFrameIndex();
    DLL_API UINT64 GetEncodeOutputFrameIndex();

private:
    struct GPUManufacturer
    {
        std::wstring vendor;
        UINT16 venId;
        UINT64 minDrvVer;
        GPUManufacturer()
        {
            vendor = L"";
            venId = 0;
            minDrvVer = 0xFFFFFFFFFFFFFFFF;
        }
        GPUManufacturer(std::wstring ven, UINT16 vId, UINT64 ver)
        {
            vendor = ven;
            venId = vId;
            minDrvVer = ver;
        }
    };

    const std::wstring VENDOR_NAME_AMD = L"Advanced Micro Devices, Inc.";
    const UINT16 VENDOR_ID_AMD = 0x1002;
    const UINT64 MIN_VER_AMD = 0x001000c8040B03E9;//16.200.1035.1001
    const std::wstring VENDOR_NAME_NVIDIA = L"NVIDIA";
    const UINT16 VENDOR_ID_NVIDIA = 0x10DE;
    const UINT64 MIN_VER_NVIDIA = 0x00150015000d1c56;//21.21.13.7254
    const std::wstring VENDOR_NAME_INTEL = L"INTEL";
    const UINT16 VENDOR_ID_INTEL = 0x8086;
    const UINT64 MIN_VER_INTEL = 0;

    const GPUManufacturer AMD_MFG;
    const GPUManufacturer NVIDIA_MFG;
    const GPUManufacturer INTEL_MFG;

    struct GPUInfo
    {
        std::wstring vendor;
        std::wstring name;
        UINT16 venId;
        UINT16 devId;
        std::wstring hwId;
        UINT64 drvVer;
        std::wstring drvVerStr;
    };

    void InitInstalledGpuInfo();
    bool IsVendorGpuInstall(GPUManufacturer gpuMfg);
    bool ValidateGpuDriverVersion(GPUManufacturer gpuMfg);

    std::vector<GPUInfo> m_gpuInfo;
    std::unique_ptr<H264Encoder> m_pEncoderImpl;
    //std::unique_ptr<H264EncoderInterface> m_pEncoderImpl;
};

#endif //__H264_ENCODERINTERFACE_H__
