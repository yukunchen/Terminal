#pragma once

#ifndef __AMD_VCE_ENCODER_H__
#define __AMD_VCE_ENCODER_H__

#include "H264Encoder.h"
#include "DxCommon.h"
#include "AMFFactory.h"
#include "VideoEncoderVCE.h"
#include "Thread.h"

class AmdVceEncoder : public H264Encoder
{
public:
    // Public member functions
    AmdVceEncoder();
    ~AmdVceEncoder();
    bool InitEncoder(const PENCODE_INIT_PARAM pInitParam);
    void DestroyEncoder();
    bool InitSharedSurface(HANDLE hnd);
    void EncodeFrame(PVOID pTexture);
    bool QueryOutput(BYTE** ppBuf, UINT64* pSize, INT64* pFrameDuration);
    bool QueryOutput(PBYTE pBuf, UINT32 bufSize, UINT32* pOutSize, UINT64* pFrameDuration);
    bool DrainPipeline();
    UINT64 QueryLatency();

protected:
    // type define
    template<typename T> void Release(T *&obj)
    {
        if (!obj) return;
        obj->Release();
        obj = nullptr;
    }

    // Private member functions
    void InitSurfaceDXGIFormat(DXGI_FORMAT format);

    // Constant
    const wchar_t* m_PropertyStartTime = L"StartTimeProperty";
    const amf_pts m_MILLISEC_TIME = 10000;

    // Class members
    amf_uint32 m_width;
    amf_uint32 m_height;
    amf_uint32 m_fps;
    amf_uint64 m_bitRate;
    amf_uint32 m_constantQP;
    amf::AMF_SURFACE_FORMAT m_surfaceFormat;
    amf::AMF_MEMORY_TYPE m_memoryType;
    ID3D11Device* m_pParentDx11Device;
    ID3D11Device* m_pDx11Device;
    ID3D11DeviceContext* m_pDx11DeviceContext;
    D3D11_TEXTURE2D_DESC m_textureDesc;
    bool m_useSeparateDevice;
    amf::AMFContextPtr m_amfContext;
    amf::AMFComponentPtr m_amfEncoder;
    amf::AMFSurfacePtr m_amfSurface;
    
    HANDLE m_sharedTextureHandle;
    ID3D11Texture2D* m_sharedTexture;
    IDXGIKeyedMutex* m_sharedTextureMutex;
    
    amf::AMFDataPtr m_amfOutputData;
    amf::AMFBufferPtr m_amfOutputBuffer;
    amf_pts m_latency1stFrame;
};

#endif //__AMD_VCE_ENCODER_H__