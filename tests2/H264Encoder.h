#pragma once

#ifndef __H264_ENCODER_H__
#define __H264_ENCODER_H__

#include "H264EncoderInterface.h"

class H264Encoder
{
public:
    H264Encoder();
    ~H264Encoder();
    virtual bool InitEncoder(const PENCODE_INIT_PARAM pInitParam);
    virtual void DestroyEncoder();
    virtual void EncodeFrame(PVOID pTexture);
    virtual bool QueryOutput(PBYTE pBuf, UINT32 bufSize, UINT32* pOutSize, UINT64* pFrameDuration);
    virtual bool DrainPipeline();
    virtual UINT64 QueryLatency();

    virtual UINT64 GetEncodeInputFrameIndex() { return m_inFrameIndex; }
    virtual UINT64 GetEncodeOutputFrameIndex() { return m_outFrameIndex; }

protected:
    UINT64 m_inFrameIndex;
    UINT64 m_outFrameIndex;
    void IncreaseEncodeInputFrameIndex() { m_inFrameIndex++; }
    void IncreaseEncodeOutputFrameIndex() { m_outFrameIndex++; }
};

#endif //__H264_ENCODER_H__
