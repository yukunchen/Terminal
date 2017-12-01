/*++
Copyright (c) Microsoft Corporation

Module Name:
- TextAttributeRun.hpp

Abstract:
- contains data structure for run-length-encoding of text attribute data

Author(s):
- Michael Niksa (miniksa) 10-Apr-2014
- Paul Campbell (paulcam) 10-Apr-2014

Revision History:
- From components of output.h/.c
  by Therese Stowell (ThereseS) 1990-1991
- Pulled into its own file from textBuffer.hpp/cpp (AustDi, 2017)
--*/

#pragma once

#include "TextAttribute.hpp"

class TextAttributeRun final
{
public:
    UINT GetLength() const;
    void SetLength(_In_ UINT const cchLength);

    const TextAttribute GetAttributes() const;
    void SetAttributes(_In_ const TextAttribute textAttribute);
    void SetAttributesFromLegacy(_In_ const WORD wNew);

private:
    UINT _cchLength;
    TextAttribute _attributes;

#ifdef UNIT_TESTING
    friend class AttrRowTests;
#endif
};
