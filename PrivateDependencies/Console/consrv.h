/***************************** Module Header ******************************\
* Module Name: consrv.h
*
* Copyright (c) 2013, Microsoft Corporation
*
* Console server CSR messages
*
* 11-09-13 IanHan 	Created.
\**************************************************************************/

#ifndef _CONSRV_H_
#define _CONSRV_H_

#include <ntcsrmsg.h>

typedef enum _CON_API_NUMBER {
    ConpEnableConsole = CONSRV_FIRST_API_NUMBER,
    ConpMaxApiNumber
} CON_API_NUMBER, *PCON_API_NUMBER;

typedef struct _ENABLECONKBDMSG {
    BOOL fEnable;
} ENABLECONKBDMSG, *PENABLECONKBDMSG;

typedef struct _CON_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
        ENABLECONKBDMSG EnableConKbdMsg;
        ULONG_PTR ApiMessageData[46]; // Size must match the sizeof(CSR_API_MSG)
    } u;
} CON_API_MSG, *PCON_API_MSG;

#endif
