/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#define REMOTE_STRING L"–remote “npipe:pipe=foo,server=bar”\t"
#define EXPECTED_REMOTE_STRING L"-remote \"npipe:pipe=foo,server=bar\""
