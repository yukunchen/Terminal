#include "stdafx.h"
#include "H264EncoderInterface.h"
#include "AmdVceEncoder.h"
#include "NvEncEncoder.h"
#include <initguid.h>
#include <setupapi.h>
#include <devguid.h>
#include <devpkey.h>

DLL_API H264EncoderInterface::H264EncoderInterface():
    AMD_MFG(VENDOR_NAME_AMD, VENDOR_ID_AMD, MIN_VER_AMD),
    NVIDIA_MFG(VENDOR_NAME_NVIDIA, VENDOR_ID_NVIDIA, MIN_VER_NVIDIA),
    INTEL_MFG(VENDOR_NAME_INTEL, VENDOR_ID_INTEL, MIN_VER_INTEL)
{
    InitInstalledGpuInfo();
    //Don't support hybrid graphics yet
    if (IsVendorGpuInstall(AMD_MFG) && !IsVendorGpuInstall(NVIDIA_MFG) && !IsVendorGpuInstall(INTEL_MFG))
    {
        if (ValidateGpuDriverVersion(AMD_MFG))
        {
            m_pEncoderImpl = std::unique_ptr<AmdVceEncoder>(new AmdVceEncoder());
        }
    }
    else if (IsVendorGpuInstall(NVIDIA_MFG) && !IsVendorGpuInstall(AMD_MFG) && !IsVendorGpuInstall(INTEL_MFG))
    {
        if (ValidateGpuDriverVersion(NVIDIA_MFG))
        {
            m_pEncoderImpl = std::unique_ptr<NvEncEncoder>(new NvEncEncoder());
        }
    }
    else if (IsVendorGpuInstall(INTEL_MFG) && !IsVendorGpuInstall(AMD_MFG) && !IsVendorGpuInstall(NVIDIA_MFG))
    {
        m_pEncoderImpl = std::unique_ptr<H264Encoder>(new H264Encoder());
    }
    else
    {
        m_pEncoderImpl = std::unique_ptr<H264Encoder>(new H264Encoder());
    }
}

DLL_API H264EncoderInterface::~H264EncoderInterface()
{
    m_gpuInfo.clear();
}

DLL_API bool H264EncoderInterface::InitEncoder(const PENCODE_INIT_PARAM pInitParam)
{
    return m_pEncoderImpl->InitEncoder(pInitParam);
}

DLL_API void H264EncoderInterface::DestroyEncoder()
{
    m_pEncoderImpl->DestroyEncoder();
}

DLL_API void H264EncoderInterface::EncodeFrame(PVOID pTexture)
{
    m_pEncoderImpl->EncodeFrame(pTexture);
}

DLL_API bool H264EncoderInterface::QueryOutput(PBYTE pBuf, UINT32 bufSize, UINT32* pOutSize, UINT64* pFrameDuration)
{
    return m_pEncoderImpl->QueryOutput(pBuf, bufSize, pOutSize, pFrameDuration);
}

DLL_API bool H264EncoderInterface::DrainPipeline()
{
    return m_pEncoderImpl->DrainPipeline();
}

DLL_API UINT64 H264EncoderInterface::QueryLatency()
{
    return m_pEncoderImpl->QueryLatency();
}

DLL_API UINT64 H264EncoderInterface::GetEncodeInputFrameIndex()
{
    return m_pEncoderImpl->GetEncodeInputFrameIndex();
}

DLL_API UINT64 H264EncoderInterface::GetEncodeOutputFrameIndex()
{
    return m_pEncoderImpl->GetEncodeOutputFrameIndex();
}

void H264EncoderInterface::InitInstalledGpuInfo()
{
    HDEVINFO hDevInfo = nullptr;
    SP_DEVINFO_DATA DeviceInfoData = { 0 };
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISPLAY, 0, 0, DIGCF_PRESENT);
    if (INVALID_HANDLE_VALUE != hDevInfo)
    {
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
        {
            BOOL isSuccess = TRUE;
            DWORD err;

            SP_DEVINSTALL_PARAMS deviceInstallParams;
            RtlZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
            deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (isSuccess)
            {
                isSuccess = SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &deviceInstallParams);
            }
            else
            {
                err = GetLastError();
            }

            // Only care current installed driver
            deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER);

            if (isSuccess)
            {
                isSuccess = SetupDiSetDeviceInstallParams(hDevInfo, &DeviceInfoData, &deviceInstallParams);
            }
            else
            {
                err = GetLastError();
            }

            bool drvInfoListBuilt = false;
            if (isSuccess)
            {
                isSuccess = SetupDiBuildDriverInfoList(hDevInfo, &DeviceInfoData, SPDIT_CLASSDRIVER);
            }
            else
            {
                err = GetLastError();
            }

            SP_DRVINFO_DATA DriverInfoData;
            DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
            if (isSuccess)
            {
                drvInfoListBuilt = true;
                isSuccess = SetupDiEnumDriverInfo(hDevInfo, &DeviceInfoData, SPDIT_CLASSDRIVER, 0, &DriverInfoData);
            }
            else
            {
                err = GetLastError();
            }

            PSP_DRVINFO_DETAIL_DATA pDriverInfoDetail = (PSP_DRVINFO_DETAIL_DATA)malloc(sizeof(SP_DRVINFO_DETAIL_DATA));
            pDriverInfoDetail->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            DWORD driverInfoDetailSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            while (!SetupDiGetDriverInfoDetail(
                hDevInfo,
                &DeviceInfoData,
                &DriverInfoData,
                pDriverInfoDetail,
                driverInfoDetailSize,
                &driverInfoDetailSize))
            {
                err = GetLastError();
                if (err == ERROR_INSUFFICIENT_BUFFER)
                {
                    if (pDriverInfoDetail)
                    {
                        free(pDriverInfoDetail);
                    }
                    pDriverInfoDetail = (PSP_DRVINFO_DETAIL_DATA)malloc(driverInfoDetailSize);
                    pDriverInfoDetail->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                }
                else
                {
                    break;
                }
            }

            // Get device property by DEVPKEY
            PBYTE pBuf = nullptr;
            DWORD bufSize = 0;
            DEVPROPTYPE PropType;
            while (!SetupDiGetDeviceProperty(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_HardwareIds,
                &PropType,
                pBuf,
                bufSize,
                &bufSize,
                0))
            {
                err = GetLastError();
                if (err == ERROR_INSUFFICIENT_BUFFER)
                {
                    if (pBuf)
                    {
                        free(pBuf);
                    }
                    pBuf = (PBYTE)malloc(bufSize * 2);// Consider wide char
                }
                else
                {
                    break;
                }
            }

            // Fill GPU info
            GPUInfo gpuInfo;
            gpuInfo.vendor = std::wstring(DriverInfoData.MfgName);
            gpuInfo.name = std::wstring(DriverInfoData.Description);
            gpuInfo.hwId = std::wstring((wchar_t*)pBuf);
            gpuInfo.venId = static_cast<UINT16>(std::stoul(gpuInfo.hwId.substr(gpuInfo.hwId.find(L"VEN_") + std::wstring(L"VEN_").length(), 4), 0, 16));
            gpuInfo.devId = static_cast<UINT16>(std::stoul(gpuInfo.hwId.substr(gpuInfo.hwId.find(L"DEV_") + std::wstring(L"DEV_").length(), 4), 0, 16));
            gpuInfo.drvVer = DriverInfoData.DriverVersion;
            gpuInfo.drvVerStr =
                std::to_wstring(gpuInfo.drvVer >> 48 & 0xFFFF) + L"." +
                std::to_wstring(gpuInfo.drvVer >> 32 & 0xFFFF) + L"." +
                std::to_wstring(gpuInfo.drvVer >> 16 & 0xFFFF) + L"." +
                std::to_wstring(gpuInfo.drvVer & 0xFFFF);
            m_gpuInfo.push_back(gpuInfo);

            if (pBuf)
            {
                free(pBuf);
            }

            if (pDriverInfoDetail)
            {
                free(pDriverInfoDetail);
            }

            if (drvInfoListBuilt)
            {
                isSuccess = SetupDiDestroyDriverInfoList(hDevInfo, &DeviceInfoData, SPDIT_CLASSDRIVER);
                if (!isSuccess)
                {
                    err = GetLastError();
                }
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}

bool H264EncoderInterface::IsVendorGpuInstall(GPUManufacturer gpuMfg)
{
    bool found = false;
    for (std::vector<GPUInfo>::iterator gpu = m_gpuInfo.begin();
        gpu < m_gpuInfo.end();
        gpu++)
    {
        if (gpu->vendor == gpuMfg.vendor && gpu->venId == gpuMfg.venId)
        {
            found = true;
            break;
        }
    }
    return found;
}

bool H264EncoderInterface::ValidateGpuDriverVersion(GPUManufacturer gpuMfg)
{
    bool drvPass = true;
    for (std::vector<GPUInfo>::iterator gpu = m_gpuInfo.begin();
        gpu < m_gpuInfo.end();
        gpu++)
    {
        // Check multi-adapter case
        if (gpu->vendor == gpuMfg.vendor && gpu->venId == gpuMfg.venId)
        {
            drvPass &= (gpu->drvVer >= gpuMfg.minDrvVer) ? true : false;
        }
    }
    return drvPass;
}