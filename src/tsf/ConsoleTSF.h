/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    TfContext.h

Abstract:

    This file defines the CConsoleTSF Interface Class.

Author:

Revision History:

Notes:

--*/

#pragma once

class CConversionArea;

class CConsoleTSF :
    public ITfContextOwner,
    public ITfContextOwnerCompositionSink,
    public ITfInputProcessorProfileActivationSink,
    public ITfUIElementSink,
    public ITfCompartmentEventSink,
    public ITfCleanupContextSink,
    public ITfCompositionSink,
    public ITfTextEditSink,
    public ITfTextLayoutSink
{
public:
    CConsoleTSF(HWND hwndConsole,
                GetSuggestionWindowPos pfnPosition) :
        _hwndConsole(hwndConsole),
        _pfnPosition(pfnPosition),
        _cRef(1)
    {
    }

    virtual ~CConsoleTSF()
    {
    }
    HRESULT Initialize();
    void    Uninitialize();

public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid,  void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfContextOwner
    STDMETHODIMP GetACPFromPoint(const POINT*, DWORD, LONG *pCP)
    {
        if (pCP)
        {
            *pCP = 0;
        }

        return S_OK;
    }


    STDMETHODIMP GetScreenExt(RECT *pRect)
    {
        if (pRect)
        {
            *pRect = _pfnPosition();
        }

        return S_OK;
    }

    STDMETHODIMP GetTextExt(LONG, LONG, RECT *pRect, BOOL *pbClipped)
    {
        if (pRect)
        {
            GetScreenExt(pRect);
        }

        if (pbClipped)
        {
            *pbClipped = FALSE;
        }

        return S_OK;
    }

    STDMETHODIMP GetStatus(TF_STATUS *pTfStatus)
    {
        if (pTfStatus)
        {
            pTfStatus->dwDynamicFlags = 0;
            pTfStatus->dwStaticFlags = TF_SS_TRANSITORY;
        }
        return pTfStatus ? S_OK : E_INVALIDARG;
    }
    STDMETHODIMP GetWnd(HWND* phwnd)
    {
        *phwnd = _hwndConsole;
        return E_NOTIMPL;
    }
    STDMETHODIMP GetAttribute(REFGUID,  VARIANT*)
    { return E_NOTIMPL; }

    // ITfContextOwnerCompositionSink methods
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk);
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
    STDMETHODIMP OnEndComposition(ITfCompositionView* pComposition);

    // ITfCompositionSink methods
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition);

    // ITfInputProcessorProfileActivationSink
    STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid,
                             REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags);

    // ITfUIElementSink methods
    STDMETHODIMP BeginUIElement(DWORD dwUIELementId, BOOL *pbShow);
    STDMETHODIMP UpdateUIElement(DWORD dwUIELementId);
    STDMETHODIMP EndUIElement(DWORD dwUIELementId);

    // ITfCompartmentEventSink
    STDMETHODIMP OnChange(REFGUID rguid);

    // ITfCleanupContextSink methods
    STDMETHODIMP OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic);

    // ITfTextEditSink methods
    STDMETHODIMP OnEndEdit(ITfContext *pInputContext, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

    // ITfTextLayoutSink methods
    STDMETHODIMP OnLayoutChange(ITfContext* /*pic*/, TfLayoutCode /*lcode*/, ITfContextView* /*pView*/)
    { return E_NOTIMPL; }

public:
    CConversionArea* CreateConversionArea();
    CConversionArea* GetConversionArea() { return _pConversionArea; }
    ITfContext* GetInputContext() { return _spITfInputContext; }
    HWND GetConsoleHwnd() { return _hwndConsole; }
    TfClientId GetTfClientId() { return _tid; }
    BOOL IsInComposition() { return (_cCompositions > 0); }
    void OnEditSession() { _fEditSessionRequested = FALSE; }
    BOOL IsPendingCompositionCleanup() { return _fCleanupSessionRequested || _fCompositionCleanupSkipped; }
    void OnCompositionCleanup(BOOL bSucceeded)
    {
        _fCleanupSessionRequested = FALSE;
        _fCompositionCleanupSkipped = !bSucceeded;
    }
    void SetModifyingDocFlag(BOOL fSet) { _fModifyingDoc = fSet; }
    BOOL HasFocus() { return _fHasFocus ? TRUE : FALSE; }
    void SetFocus(BOOL fSet)
    {
        if (!fSet && _cCompositions)
        {
            // Close (terminate) any open compositions when losing the input focus.
            CComQIPtr<ITfContextOwnerCompositionServices> spCompositionServices(_spITfInputContext);
            if (spCompositionServices)
            {
                spCompositionServices->TerminateComposition(NULL);
            }
        }
        _fHasFocus = fSet;
    }

    // A workaround for a MS Korean IME scenario where the IME appends a whitespace
    // composition programmatically right after completing a keyboard input composition.
    // Since post-composition clean-up is an async operation, the programmatic whitespace
    // composition gets completed before the previous composition cleanup happened,
    // and this results in a double insertion of the first composition. To avoid that, we'll
    // store the length of the last completed composition here until it's cleaned up.
    // (for simplicity, this patch doesn't provide a generic solution for all possible
    // scenarios with subsequent synchronous compositions, only for the known 'append').
    long GetCompletedRangeLength() const { return _cchCompleted; }
    void SetCompletedRangeLength(long cch) { _cchCompleted = cch; }

private:
    ITfUIElement *GetUIElement(DWORD dwUIElementId);
    HRESULT GetCompartment(REFGUID rguidComp, ITfCompartment **ppComp);
    HRESULT AdviseCompartmentSink(REFGUID rguidComp, DWORD* pdwCookie);
    HRESULT UnadviseCompartmentSink(REFGUID rguidComp, DWORD* pdwCookie);
    HRESULT SetCompartmentDWORD(REFGUID rguidComp, DWORD dw);
    HRESULT GetCompartmentDWORD(REFGUID rguidComp, DWORD *pdw);
    HRESULT OnUpdateComposition();
    HRESULT OnCompleteComposition();
    BOOL HasCompositionChanged(ITfContext *pInputContext, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

private:
    // ref count.
    DWORD _cRef;

    // Cicero stuff.
    TfClientId              _tid;
    CComPtr<ITfThreadMgrEx> _spITfThreadMgr;
    CComPtr<ITfDocumentMgr> _spITfDocumentMgr;
    CComPtr<ITfContext>     _spITfInputContext;
    CComPtr<ITfInputProcessorProfiles> _spITfProfiles;

    // Event sink cookies.
    DWORD   _dwContextOwnerCookie;
    DWORD   _dwUIElementSinkCookie;
    DWORD   _dwTextEditSinkCookie;
    DWORD   _dwActivationSinkCookie;

    // Conversion area object for the languages.
    CConversionArea*        _pConversionArea;

    // Console info.
    HWND    _hwndConsole;
    GetSuggestionWindowPos _pfnPosition;

    // Miscellaneous flags
    BOOL _fHasFocus : 1;
    BOOL _fModifyingDoc : 1;            // Set TRUE, when calls ITfRange::SetText
    BOOL _fCoInitialized : 1;
    BOOL _fEditSessionRequested : 1;
    BOOL _fCleanupSessionRequested : 1;
    BOOL _fCompositionCleanupSkipped : 1;

    int  _cCompositions;
    long _cchCompleted;     // length of completed composition waiting for cleanup
};

extern CConsoleTSF* g_pConsoleTSF;
