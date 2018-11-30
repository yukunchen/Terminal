/*++

Copyright (C) Microsoft. All rights reserved.

Module Name:

    enterprisecontextstructs.h

Abstract:

    The header for Enterprise Context structures.

Author:

    Jugal Kaku (jugalk) 11-Nov-2014

--*/

#pragma once

#ifndef _ENTERPRISE_CONTEXT_STRUCTS_H
#define _ENTERPRISE_CONTEXT_STRUCTS_H

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  Enterprise context PUBLIC structures definitions.                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

typedef UNICODE_STRING SRP_ENTERPRISE_ID;
typedef UNICODE_STRING *PSRP_ENTERPRISE_ID;
typedef const SRP_ENTERPRISE_ID *PCSRP_ENTERPRISE_ID;

typedef struct _SRP_ENTERPRISE_CONTEXT {

    //
    // Version. Only SRP_ENTERPRISE_CONTEXT_VERSION_1 supported currently.
    //

    USHORT Version;

    //
    // Reserved field. Pass zero in set operations and ignore on get operations.
    //

    USHORT Reserved;

    //
    // Number of enterprise IDs access allowed to.
    //
    
    ULONG AllowedEnterpriseIdCount;

    //
    // Policy enforcement related flags. See SRP_ENTERPRISE_POLICY_ values.
    //

    ULONG64 PolicyFlags;

    //
    // Enterprise IDs access allowed to.
    //

#ifdef MIDL_PASS
    [size_is(AllowedEnterpriseIdCount)]SRP_ENTERPRISE_ID* AllowedEnterpriseIds;
#else
    SRP_ENTERPRISE_ID AllowedEnterpriseIds[ANYSIZE_ARRAY];
#endif

} SRP_ENTERPRISE_CONTEXT;

typedef SRP_ENTERPRISE_CONTEXT *PSRP_ENTERPRISE_CONTEXT;

#endif // _ENTERPRISE_CONTEXT_STRUCTS_H
