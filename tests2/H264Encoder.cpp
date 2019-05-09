#include "stdafx.h"
#include "H264Encoder.h"

H264Encoder::H264Encoder():
    m_inFrameIndex(0),
    m_outFrameIndex(0)
{
}

H264Encoder::~H264Encoder()
{
}

bool H264Encoder::InitEncoder(const PENCODE_INIT_PARAM pInitParam)
{
    return false;
}

void H264Encoder::DestroyEncoder()
{
    m_inFrameIndex = 0;
    m_outFrameIndex = 0;
}

void H264Encoder::EncodeFrame(PVOID pTexture)
{
    return;
}

bool H264Encoder::QueryOutput(PBYTE pBuf, UINT32 bufSize, UINT32* pOutSize, UINT64* pFrameDuration)
{
    return false;
}

bool H264Encoder::DrainPipeline()
{
    return false;
}

UINT64 H264Encoder::QueryLatency()
{
    return 0;
}