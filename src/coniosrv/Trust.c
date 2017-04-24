/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "Trust.h"

BOOLEAN
TstIsFileTrusted(
    _In_reads_z_(MAX_PATH) LPCWSTR FilePath
    )
{
    //
    // Final return value.
    //

    BOOLEAN IsTrusted = FALSE;

    //
    // Intermediate return values.
    //

    BOOL Ret;
    LONG VerificationResult;

    //
    // Signature verification and catalog contexts and information.
    //

    PVOID Context            = NULL;
    PVOID CatalogContext     = NULL;
    BOOLEAN HasCatalogInfo   = FALSE;
    PWCHAR CatalogMemberTag  = NULL;
    CATALOG_INFO CatalogInfo = { 0 };

    //
    // File-related data.
    //

    PBYTE FileHash              = NULL;
    HANDLE FileHandle           = INVALID_HANDLE_VALUE;
    DWORD FileHashLength        = 0;
    SIZE_T FileHashStringLength = 0;

    //
    // Request descriptors for the WinTrust API.
    //

    WINTRUST_DATA TrustData = { 0 };
    WINTRUST_FILE_INFO FileTrustInfo = { 0 };
    WINTRUST_CATALOG_INFO CatalogTrustInfo = { 0 };

    //
    // Initialize shared members of the verification request.
    //

    TrustData.cbStruct = sizeof(WINTRUST_DATA);
    TrustData.pPolicyCallbackData = NULL;                   // Default Code Signing EKU
    TrustData.pSIPClientData = NULL;                        // No data to pass to a Subject Interface Package (SIP) Provider
    TrustData.dwUIChoice = WTD_UI_NONE;                     // Never pop a UI
    TrustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;  // Check the whole certificate chain
    TrustData.dwStateAction = WTD_STATEACTION_VERIFY;       // Request a verification
    TrustData.hWVTStateData = NULL;                         // The API sets this
    TrustData.pwszURLReference = NULL;                      // [Reserved]
    TrustData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;   // Do not pull revocation information from the network
    TrustData.dwUIContext = WTD_UICONTEXT_EXECUTE;          // Ignored since no UI
    TrustData.pSignatureSettings = NULL;                    // No settings

    //
    // Acquire a context for signature verification.
    //

    if (!CryptCATAdminAcquireContext(&Context, NULL, 0))
    {
        goto Exit;
    }

    //
    // Open a file handle to the binary we are verifying.
    //

    FileHandle = CreateFileW(FilePath,
                             GENERIC_READ,
                             7,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        goto Exit;
    }

    //
    // Compute how many bytes the hash of the binary will take.
    //

    CryptCATAdminCalcHashFromFileHandle(FileHandle,
                                        &FileHashLength,
                                        NULL,
                                        0);
    if (FileHashLength == 0)
    {
        goto Exit;
    }

    //
    // Allocate a buffer to hold the hash of the binary.
    //

    FileHash = (PBYTE)RtlAllocateHeap(RtlProcessHeap(),
                                      0,
                                      FileHashLength);
    if (!FileHash)
    {
        goto Exit;
    }

    //
    // Compute the hash of the binary.
    //

    Ret = CryptCATAdminCalcHashFromFileHandle(FileHandle,
                                              &FileHashLength,
                                              FileHash,
                                              0);
    if (Ret == FALSE)
    {
        goto Exit;
    }

    //
    // Try to get the catalog context associated with the hash.
    //

    CatalogContext = CryptCATAdminEnumCatalogFromHash(Context,
                                                      FileHash,
                                                      FileHashLength,
                                                      0,
                                                      NULL);
    if (CatalogContext)
    {
        //
        // Try to get the catalog information from the context.
        //

        Ret = CryptCATCatalogInfoFromContext(CatalogContext, &CatalogInfo, 0);
        if (Ret != FALSE)
        {
            //
            // Convert the file hash to a string.
            //

            FileHashStringLength = sizeof(WCHAR) * ((FileHashLength * 2) + 1);
            CatalogMemberTag = (PWCHAR)RtlAllocateHeap(RtlProcessHeap(),
                                                       0,
                                                       FileHashStringLength);

            memset(CatalogMemberTag, 0, FileHashStringLength);
            for (UINT i = 0 ; i < FileHashLength ; i++)
            {
                swprintf(&CatalogMemberTag[i * 2], L"%02X", FileHash[i]);
            }

            HasCatalogInfo = TRUE;
        }
    }

    if (HasCatalogInfo)
    {
        //
        // We have the necessary catalog information.
        //

        // Fill in the required catalog information to pass to the WinTrust API.
        //
        
        CatalogTrustInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
        CatalogTrustInfo.dwCatalogVersion = 0;
        CatalogTrustInfo.pcwszCatalogFilePath = CatalogInfo.wszCatalogFile;
        CatalogTrustInfo.pcwszMemberTag = CatalogMemberTag;
        CatalogTrustInfo.pcwszMemberFilePath = FilePath;
        CatalogTrustInfo.hMemberFile = NULL;
        
        //
        // Fill in the rest of the request information.
        //

        TrustData.dwUnionChoice = WTD_CHOICE_CATALOG;  // Verify a catalog
        TrustData.pCatalog = &CatalogTrustInfo;        // Catalog information
    }
    else
    {
        //
        // No catalog found, test the signature embedded in the binary, if any.
        //
        
        //
        // Fill in the required file information to pass to the WinTrust API.
        //

        FileTrustInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
        FileTrustInfo.pcwszFilePath = FilePath;
        FileTrustInfo.hFile = NULL;
        FileTrustInfo.pgKnownSubject = NULL;

        //
        // Fill in the rest of the request information.
        //

        TrustData.dwUnionChoice = WTD_CHOICE_FILE;  // Verify a file
        TrustData.pFile = &FileTrustInfo;           // File information
    }

    //
    // Verify the file now.
    //

    GUID PolicyId = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    VerificationResult = WinVerifyTrust(NULL,         // No UI! 
                                        &PolicyId,
                                        &TrustData);
    if (VerificationResult == ERROR_SUCCESS)
    {
        //
        // Assert that the signature is tied to Microsoft.
        //

        Ret = WTHelperIsChainedToMicrosoftFromStateData(TrustData.hWVTStateData,
                                                        TRUE);  // Accept test signing
        
        //
        // Release the state data.
        //

        TrustData.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(NULL,         // Release the state data
                       &PolicyId,
                       &TrustData);
        
        //
        // If the signature is tied to Microsoft, this file is trusted.
        //

        if (Ret != FALSE)
        {
            IsTrusted = TRUE;
        }
    }

Exit:
    if (CatalogMemberTag)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, CatalogMemberTag);
    }
    if (CatalogContext)
    {
        CryptCATAdminReleaseCatalogContext(Context, CatalogContext, 0);
    }
    if (FileHash)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, FileHash);
    }
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FileHandle);
    }
    if (Context)
    {
        CryptCATAdminReleaseContext(Context, 0);
    }

    return IsTrusted;
}
