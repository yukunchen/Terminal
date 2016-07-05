// Windows Internal Libraries (wil)
// WrlEvent.h:  Helpers and classes for dealing with WRL eventing 
//
// wil Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Internal%20Libraries%20for%20C++%20Usage%20Guide.docx?web=1
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)

#pragma once

#pragma warning(push)
#pragma warning(disable: 26135 26165)   // Missing locking annotation, Failing to release lock

#include "Com.h"
#include "Functional.h"
#include "Resource.h"

// AsyncEventSource will use Microsoft::WRL::AgileEventSource rather than Windows::Internal::GitEventSourceSupportsAgile
// in the future. Until then, including winrt\GitEvent.h rather than just wrl\event.h brings in some extra
// dependencies. These are listed explicitly below.
#include <wrl\event.h>
#include <wrl\ftm.h>
#include <winrt\gitptr.h>
#include <windowsstringp.h>
#include <winrt\GitEvent.h>

namespace wil
{
    /// @cond
    namespace details
    {
        // Wraps an object that doesn't support the IUnknown ref counting contract so that its lifetime can be managed with
        // types that expect AddRef/Release lifetime management.
        // It should be possible to use wistd::shared_ptr when it becomes available for this functionality and remove
        // RefCountedContainer.
        template <typename T>
        class RefCountedContainer
        {
        public:
            RefCountedContainer(_In_ T&& value) WI_NOEXCEPT
                : m_value(wistd::move(value)),
                  m_refCount(1)
            {}

            IFACEMETHODIMP_(ULONG) AddRef() WI_NOEXCEPT
            { return InterlockedIncrement(&m_refCount); }

            IFACEMETHODIMP_(ULONG) Release() WI_NOEXCEPT
            {
                long cRef = InterlockedDecrement(&m_refCount);
                if (cRef == 0)
                {
                    delete this;
                }
                return cRef;
            }

            const T& GetValue() WI_NOEXCEPT
            { return m_value; }

        private:
            ULONG m_refCount;
            T m_value;
        };

        // Container for a null-terminated string that can be safely copied without duplicating the string it contains.
        struct NullTerminatedStringCopyableContainer
        {
            NullTerminatedStringCopyableContainer(_In_ Microsoft::WRL::ComPtr<RefCountedContainer<wil::unique_cotaskmem_string>> const& refCountedString) WI_NOEXCEPT
                : m_refCountedString(refCountedString)
            {}

            PCWSTR Get() const WI_NOEXCEPT
            { return m_refCountedString->GetValue().get(); }

        private:
            Microsoft::WRL::ComPtr<RefCountedContainer<wil::unique_cotaskmem_string>> m_refCountedString;
        };

        // Type that may be returned from MakeSafelyCopyable that carries with it the original type
        // of the value that was made copyable and contains the new copyable value.
        template <typename CopyableType, typename OriginalType>
        struct MakeSafelyCopyableResult
        {
            MakeSafelyCopyableResult(_Inout_ CopyableType&& sourceCopyableValue) WI_NOEXCEPT
                : copyableValue(wistd::move(sourceCopyableValue))
            {}

            CopyableType copyableValue;
        };

        // Type that may be returned from MakeSafelyCopyable that carries with it the original type
        // of the value that was made copyable and contains the new copyable value and a status code
        // indicating whether the MakeSafelyCopyable operation was successful. This allows for using
        // MakeSafelyCopyable with variadic argument expansions when the operation to make a value
        // copyable can fail.
        template <typename CopyableType, typename OriginalType>
        struct MakeSafelyCopyableResultWithStatus : MakeSafelyCopyableResult<CopyableType, OriginalType>
        {
            MakeSafelyCopyableResultWithStatus(
                _Inout_ CopyableType&& sourceCopyableValue,
                _In_    HRESULT sourceStatus) WI_NOEXCEPT
                : MakeSafelyCopyableResult<CopyableType, OriginalType>(wistd::move(sourceCopyableValue)),
                  status(sourceStatus)
            {}

            HRESULT status;
        };

        // Generic MakeSafelyCopyable traits describing how a given type can be made safely copyable.
        // These traits are used when no specialization below matches the type. These traits just use the
        // given type as the copyable type without any transformation.
        template <typename T>
        struct MakeSafelyCopyableTraitsDefault
        {
            typedef T const& SafelyCopyableResultType;

            static SafelyCopyableResultType MakeSafelyCopyable(_In_ T const& arg) WI_NOEXCEPT
            { return arg; }
        };

        // MakeSafelyCopyable traits describing how a null-terminated string can be made copyable.
        struct MakeSafelyCopyableTraitsNullTerminatedString
        {
            typedef MakeSafelyCopyableResultWithStatus<NullTerminatedStringCopyableContainer, PCWSTR> SafelyCopyableResultType;

            static SafelyCopyableResultType MakeSafelyCopyable(_In_ PCWSTR string) WI_NOEXCEPT
            {
                Microsoft::WRL::ComPtr<RefCountedContainer<unique_cotaskmem_string>> copyableString;
                HRESULT hr = MakeCopyableString(string, &copyableString);
                return SafelyCopyableResultType(NullTerminatedStringCopyableContainer(wistd::move(copyableString)), hr);
            }

            static PCWSTR GetRawValue(_In_ NullTerminatedStringCopyableContainer const& copyableContainer) WI_NOEXCEPT
            { return copyableContainer.Get(); }

        private:
            static HRESULT MakeCopyableString(
                _In_         PCWSTR string,
                _COM_Outptr_ RefCountedContainer<unique_cotaskmem_string>** container) WI_NOEXCEPT
            {
                *container = nullptr;
                wil::unique_cotaskmem_string movableString = make_cotaskmem_string_nothrow(string);
                RETURN_IF_NULL_ALLOC(movableString.get());
                *container = new(std::nothrow) RefCountedContainer<wil::unique_cotaskmem_string>(wistd::move(movableString));
                RETURN_IF_NULL_ALLOC(*container);
                return S_OK;
            }
        };

        template <>
        struct MakeSafelyCopyableTraitsDefault<PWSTR> : MakeSafelyCopyableTraitsNullTerminatedString
        {};

        template <>
        struct MakeSafelyCopyableTraitsDefault<PCWSTR> : MakeSafelyCopyableTraitsNullTerminatedString
        {};

        // MakeSafelyCopyable traits describing how a pointer to an IUnknown-derived type can be made safely copyable.
        template <typename T>
        struct MakeSafelyCopyablTraitsIUnknown
        {
        private:
            typedef Microsoft::WRL::ComPtr<typename wistd::remove_pointer<T>::type> ComPtrType;

        public:
            typedef MakeSafelyCopyableResult<ComPtrType, T> SafelyCopyableResultType;

            static SafelyCopyableResultType MakeSafelyCopyable(_In_ T arg) WI_NOEXCEPT
            { return SafelyCopyableResultType(ComPtrType(arg)); }

            static T GetRawValue(_In_ ComPtrType const& comPtr) WI_NOEXCEPT
            { return comPtr.Get(); }
        };

        // MakeSafelyCopyableTraits provides functionality to make a given type copyable and to extract
        // the original raw type from the returned copyable type.
        template <typename T>
        using MakeSafelyCopyableTraits = typename wistd::_If<
            wistd::is_base_of<IUnknown, typename wistd::remove_pointer<T>::type>::value,
            MakeSafelyCopyablTraitsIUnknown<T>,
            MakeSafelyCopyableTraitsDefault<T>>::type;

        // Takes a value that may or may not be safely copyable for later use and returns a safely copyable container
        // for the given value. To get the raw given value back from the safely copyable container returned by this
        // function, call RefOrGetRawValue.
        // For example, calling MakeSafelyCopyable(x) where x is an integer will result in a reference to x being returned,
        // and calling MakeSafelyCopyable(pInterface) where pInterface is a raw COM interface will result in a
        // MakeSafelyCopyableResult value containing a ComPtr containing a pointer to pInterface being returned.
        template <typename T>
        static typename MakeSafelyCopyableTraits<T>::SafelyCopyableResultType
        MakeSafelyCopyable(_In_ T const& arg) WI_NOEXCEPT
        { return MakeSafelyCopyableTraits<T>::MakeSafelyCopyable(arg); }

        // RefOrGetRawValue takes a value returned by MakeSafelyCopyable and returns the contained value of the type
        // that was passed to MakeSafelyCopyable to generate the copyable type. This is the generic version. It
        // just returns a const reference to the value it is given.
        template <typename T>
        T const& RefOrGetRawValue(_In_ T const& value)
        { return value; }

        // This specialization returns the raw value contained within the MakeSafelyCopyableResult value it is given.
        template <typename CopyableType, typename OriginalType>
        static typename OriginalType
        RefOrGetRawValue(_In_ MakeSafelyCopyableResult<CopyableType, OriginalType> const& safelyCopyableResult)
        { return MakeSafelyCopyableTraits<OriginalType>::GetRawValue(safelyCopyableResult.copyableValue); }

        // This specialization returns the raw value contained within the MakeSafelyCopyableResultWithStatus value it is given.
        template <typename CopyableType, typename OriginalType>
        static typename OriginalType
        RefOrGetRawValue(_In_ MakeSafelyCopyableResultWithStatus<CopyableType, OriginalType> const& safelyCopyableResult)
        { return MakeSafelyCopyableTraits<OriginalType>::GetRawValue(safelyCopyableResult.copyableValue); }

        // Used to check that MakeSafelyCopyable has succeeded in making a variadic argument expansion safely copyable. It
        // does this by examining each transformed entry in the expansion to see if it indicates success or not. This
        // overload/specialization is the generic case. It ignores the first item in the expansion and returns the result
        // of applying CheckExpandedSafelyCopyableArgsStatus to the rest of the list. This results in only the copyable
        // transformations that can fail affecting the final result.
        template <typename First, typename ...Rest>
        static HRESULT CheckExpandedSafelyCopyableArgsStatus(First const& /*first*/, Rest const& ...rest) WI_NOEXCEPT
        { return CheckExpandedSafelyCopyableArgsStatus(rest...); }

        // This specialization matches when the transformed expansion element is a safely copyable type that has a status. This
        // specialization returns failure if the first item in the expanion indicates that the copyable transformation failed,
        // and it returns the result of CheckExpandedSafelyCopyableArgsStatus applied to the rest of the expansion if not.
        template <typename CopyableType, typename OriginalType, typename ...Rest>
        static HRESULT CheckExpandedSafelyCopyableArgsStatus(MakeSafelyCopyableResultWithStatus<CopyableType, OriginalType> const& first, Rest const& ...rest) WI_NOEXCEPT
        { return FAILED(first.status) ? first.status : CheckExpandedSafelyCopyableArgsStatus(rest...); }

        // This overload is the base case. It returns success if there are no entries in the transformed expansion.
        static HRESULT CheckExpandedSafelyCopyableArgsStatus() WI_NOEXCEPT
        { return S_OK; }
    } // namespace details
    /// @endcond

// AsyncEventSource makes use of the threadpool APIs, which are available only in the desktop partition.
// Therefore, AsyncEventSource is only available in the desktop partition.
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

    // Controls how asynchronous events will be invoked with respect to each other.
    enum class AsyncInvokeMethod
    {
        Parallel, // events are fired in parallel possibly producing out of order delivery.
        Serial,   // events are invoked serially, in the same order in which their invocation was requested.
    };

    /// @cond
    namespace details
    {
        // This class defines a generic base class for the classes generated by the
        // AsyncEventWorkItem class template defined below.
        struct AsyncEventWorkItemBase
        {
            virtual ~AsyncEventWorkItemBase() WI_NOEXCEPT = 0 {};

            virtual void RaiseEvent() WI_NOEXCEPT = 0;

            wistd::unique_ptr<AsyncEventWorkItemBase> nextEntry;
        };

        // This class defines a work queue that manages AsyncEventWorkItemBase objects. Work items can be added to
        // the end of the queue and retrieved from the front of the queue.
        class AsyncEventWorkQueue
        {
        public:
            AsyncEventWorkQueue() WI_NOEXCEPT
                : m_tailNextPointer(&m_firstEntry)
            {}

            void AddWorkItem(_In_ wistd::unique_ptr<AsyncEventWorkItemBase>&& workItem) WI_NOEXCEPT
            {
                // Insert a new entry and update our tail next pointer so we know where to insert
                // the next new entry.
                *m_tailNextPointer = wistd::move(workItem);
                m_tailNextPointer = &(*m_tailNextPointer)->nextEntry;
            }

            // Removes the first work item from the queue and returns it. Returns null if the queue is empty.
            wistd::unique_ptr<AsyncEventWorkItemBase> GetNextWorkItem() WI_NOEXCEPT
            {
                wistd::unique_ptr<AsyncEventWorkItemBase> workItem;
                if (m_firstEntry)
                {
                    // Save the first entry so that it can be returned.
                    workItem = wistd::move(m_firstEntry);

                    // Move the entry next after the entry that will be returned into our first entry pointer.
                    m_firstEntry = wistd::move(workItem->nextEntry);
                    // If the first entry is now null, then the queue is empty, so make sure to reset
                    // the tail next pointer to our first entry pointer.
                    if (!m_firstEntry)
                    {
                        m_tailNextPointer = &m_firstEntry;
                    }
                }
                return workItem;
            }

            bool IsEmpty() const WI_NOEXCEPT
            {
                return !m_firstEntry;
            }

        private:
            wistd::unique_ptr<AsyncEventWorkItemBase> m_firstEntry;       // The first entry in the queue that will be returned when GetNextWorkItem is called.
            wistd::unique_ptr<AsyncEventWorkItemBase>* m_tailNextPointer; // Pointer to the unique_ptr that should be updated with a new entry when AddWorkItem is called.
        };

        // This class template allows for different invocation policies to be specified for use by AsyncEventSourceT.
        template <AsyncInvokeMethod>
        class AsyncEventInvocationPolicy;

        // This specialization defines an invocation policy that does not serialize event invocations.
        // Event invocations may occur in any order and at the same time.
        template <>
        class AsyncEventInvocationPolicy<AsyncInvokeMethod::Parallel>
        {
        protected:
            void HandleAsyncEventInvoke(
                _Inout_ wil::srwlock& srwLockWorkQueue,
                _Inout_ AsyncEventWorkQueue& workQueue) WI_NOEXCEPT
            {
                // Since exactly one work item will be submitted to the thread pool for each event
                // invocation work item queued, remove exactly one invocation work item from the queue and invoke it.

                wistd::unique_ptr<AsyncEventWorkItemBase> workItem;
                {
                    auto workQueueLock = srwLockWorkQueue.lock_exclusive();
                    workItem = workQueue.GetNextWorkItem();
                }
            
                workItem->RaiseEvent();
            }

            void HandleWorkItemQueuedUnderLock(_In_ PTP_WORK threadpoolAsyncEventWork) WI_NOEXCEPT
            {
                // Submit one work item to the thread pool for each event invocation work item queued.
                SubmitThreadpoolWork(threadpoolAsyncEventWork);
            }
        };

        // This specialization defines an invocation policy that serializes event invocations.
        // Events will be invoked in the order in which they were fired.
        template <>
        class AsyncEventInvocationPolicy<AsyncInvokeMethod::Serial>
        {
        protected:
            AsyncEventInvocationPolicy() WI_NOEXCEPT
                : m_fSerializedInvokeWorkerInProgress(false)
            {}

            void HandleAsyncEventInvoke(
                _Inout_ wil::srwlock& srwLockWorkQueue,
                _Inout_ AsyncEventWorkQueue& workQueue) WI_NOEXCEPT
            {
                // We will only ever have one thread pool work item invoking events at a time.
                // Invoke all queued event invocation work items before exiting.
                wistd::unique_ptr<AsyncEventWorkItemBase> workItem;
                bool fDone = false;
                while (!fDone)
                {
                    // Remove the oldest event invocation work item from the queue if there is one.
                    // If no work items remain, indicate that we no longer have a work item in progress inside the same
                    // lock context that we use to look at the work queue. This way, we ensure that we won't
                    // miss any work items and that we won't ever have two workers invoking an event handler
                    // at the same time.
                    auto workQueueLock = srwLockWorkQueue.lock_exclusive();
                    if (!workQueue.IsEmpty())
                    {
                        // If there is something in the work queue, store it and remove it from the queue,
                        // and raise its associated event after releasing the work queue lock below.
                        workItem = workQueue.GetNextWorkItem();
                    
                        workQueueLock.reset();
                        workItem->RaiseEvent();
                        workItem.reset();
                    }
                    else
                    {
                        // If nothing is left in the work queue, this worker is done. Mark that
                        // there is no longer a worker in progress (since we will be exiting and
                        // will no longer raise any events). This needs to be done atomically with
                        // checking whether there are any items in the work queue to avoid missing
                        // work queue items.
                        m_fSerializedInvokeWorkerInProgress = false;
                        fDone = true;
                    }
                }
            }

            void HandleWorkItemQueuedUnderLock(_In_ PTP_WORK threadpoolAsyncEventWork) WI_NOEXCEPT
            {
                // We only want one worker invoking event handlers at a time to ensure that all event invocations
                // are serialized.
                if (!m_fSerializedInvokeWorkerInProgress)
                {
                    SubmitThreadpoolWork(threadpoolAsyncEventWork);
                    m_fSerializedInvokeWorkerInProgress = true;
                }
            }

        private:
            bool m_fSerializedInvokeWorkerInProgress;
        };
    } // namespace details
    /// @endcond

    // AsyncEventSourceT takes the core functionality of a standard WRL EventSource implementation
    // and adds support for firing the events on a separate thread.
    template <typename DelegateInterface, template <typename, typename> class EventSourceTemplate, typename EventSourceOptions, AsyncInvokeMethod asyncInvokeMethod, typename ErrorPolicy>
    class AsyncEventSourceT :
        public details::AsyncEventInvocationPolicy<asyncInvokeMethod>,
        private EventSourceTemplate<DelegateInterface, EventSourceOptions>
    {
    private:
        typedef details::AsyncEventInvocationPolicy<asyncInvokeMethod> AsyncInvocationPolicy;
        typedef EventSourceTemplate<DelegateInterface, EventSourceOptions> EventSourceType;

    public:
        typedef typename ErrorPolicy::result result;

        AsyncEventSourceT() WI_NOEXCEPT
            : m_initOnce(INIT_ONCE_STATIC_INIT)
        {}

        ~AsyncEventSourceT() WI_NOEXCEPT
        {
            // All threadpool work items must be complete before the work queue is released.
            m_threadpoolAsyncEventWork.reset();
        }

        result Add(_In_opt_ DelegateInterface* delegateInterface, _Out_ EventRegistrationToken* token)
        {
            return ErrorPolicy::HResult(EventSourceType::Add(delegateInterface, token));
        }

        result Remove(_In_ EventRegistrationToken token)
        {
            return ErrorPolicy::HResult(EventSourceType::Remove(token));
        }

        size_t GetSize() const WI_NOEXCEPT
        {
            return EventSourceType::GetSize();
        }

        template <typename ...Args>
        result InvokeAll(Args&& ...args)
        {
            return ErrorPolicy::HResult(EventSourceType::InvokeAll(wistd::forward<Args>(args)...));
        }

        // This method accepts a variable number of arguments of any type. Any arguments that derive from IUnknown
        // will have their lifetime managed automatically. They will be AddRef'd now and Released after the event
        // invocation is actually made.
        template <typename ...Args>
        result AsyncInvokeAll(Args const& ...args)
        {
            wistd::function<HRESULT(DelegateInterface*)> callback;
            HRESULT hr = MakeAsyncCallbackCopyArgs(callback, details::MakeSafelyCopyable(args)...);
            if (FAILED(hr))
            {
                return ErrorPolicy::HResult(hr);
            }
            return ErrorPolicy::HResult(DoAsyncInvoke(wistd::move(callback)));
        }

        // This method accepts a callback that will be called for each registered event delegate
        // when the event is actually invoked.
        result AsyncInvokeAllCallback(wistd::function<HRESULT(DelegateInterface*)>&& callback)
        {
            return ErrorPolicy::HResult(DoAsyncInvoke(wistd::move(callback)));
        }

    private:
        // Represents a work item in an AsyncEventSourceT's async event invocation queue.
        class AsyncEventWorkItem : public details::AsyncEventWorkItemBase
        {
        public:
            AsyncEventWorkItem(
                _In_ AsyncEventSourceT* parent,
                _In_ Microsoft::WRL::Details::EventTargetArray* targets,
                _In_ wistd::function<HRESULT(DelegateInterface*)>&& invokeOne) WI_NOEXCEPT
                : m_asyncEventSource(parent),
                  m_targets(targets),
                  m_invokeOne(wistd::move(invokeOne))
            {}

            // AsyncEventWorkItemBase implementation
            void RaiseEvent() WI_NOEXCEPT override
            {
                // The callback given to InvokeDelegates is called synchronously, so it is safe to capture
                // the this pointer in the lambda below.
                // InvokeDelegates may return an error in the case that one of the event handler callbacks returns
                // an error, but we don't have any way of notifying our client at this point, so the result
                // of this call is ignored.
                Microsoft::WRL::InvokeTraits<EventSourceOptions::invokeMode>::InvokeDelegates(
                    [this] (_In_ Microsoft::WRL::ComPtr<IUnknown> const& delegateInterfaceUnknown) -> HRESULT
                    {
                        return m_invokeOne(static_cast<DelegateInterface*>(delegateInterfaceUnknown.Get()));
                    },
                    m_targets.Get(),
                    m_asyncEventSource);
            }

        private:
            // The parent AsyncEventSourceT ensures that it lives at least as long as all of its work items,
            // so it is safe to hold a raw pointer to the parent AsyncEventSourceT.
            AsyncEventSourceT* m_asyncEventSource;
            Microsoft::WRL::ComPtr<Microsoft::WRL::Details::EventTargetArray> m_targets;
            wistd::function<HRESULT(DelegateInterface*)> m_invokeOne;
        };

        HRESULT Initialize() WI_NOEXCEPT
        {
            // Increment the MTA usage count so we avoid having to make an MTA initialization
            // call for each work item callback.
            RETURN_IF_FAILED(CoIncrementMTAUsage(&m_mtaUsageCookie));
            m_threadpoolAsyncEventWork.reset(CreateThreadpoolWork(
                [] (_Inout_     PTP_CALLBACK_INSTANCE /*Instance*/,
                    _Inout_opt_ void* Context,
                    _Inout_     PTP_WORK /*Work*/)
                {
                    // MTA initialization is not necessary here because the MTA is kept alive via
                    // a CoIncrementMTAUsage call.

                    // Allow the chosen invocation policy to perform the necessary actions to invoke queued events
                    // from this thread pool thread.
                    auto pThis = static_cast<AsyncEventSourceT*>(Context);
                    pThis->HandleAsyncEventInvoke(pThis->m_srwLockWorkQueue, pThis->m_asyncEventWorkQueue);
                },
                this,
                nullptr));
            RETURN_LAST_ERROR_IF_NULL(m_threadpoolAsyncEventWork.get());
            return S_OK;
        }

        HRESULT DoAsyncInvoke(_In_ wistd::function<HRESULT(DelegateInterface*)>&& invokeOne) WI_NOEXCEPT
        {
            // The targetsPointerLock_ protects the acquisition of an AddRef'd pointer to
            // "current list".  An Add/Remove operation may occur during the
            // firing of events (but occurs on a copy of the list).  i.e. both
            // InvokeAll/invoke and Add/Remove are readers of the "current list".
            // NOTE:  EventSource<DelegateInterface>::InvokeAll(...) must never take the addRemoveLock_.
            Microsoft::WRL::ComPtr<Microsoft::WRL::Details::EventTargetArray> targets;
            {
                auto targetsPointerLock = targetsPointerLock_.LockExclusive();
                targets = targets_;
            }

            // The list may not exist if nobody has registered an event handler yet.
            if (targets)
            {
                // If this is the first time we've been called, set up thread pool state so that we can
                // submit work to it to call event handlers.
                RETURN_IF_WIN32_BOOL_FALSE(InitOnceExecuteOnce(
                    &m_initOnce,
                    [] (_Inout_                       PINIT_ONCE /*InitOnce*/,
                        _Inout_opt_                   PVOID Parameter,
                        _Outptr_opt_result_maybenull_ PVOID* Context) -> BOOL
                    {
                        wil::AssignNullToOptParam(Context);
                        auto pThis = static_cast<AsyncEventSourceT*>(Parameter);
                        HRESULT hr = pThis->Initialize();
                        if (FAILED(hr))
                        {
                            ::SetLastError(GetScode(hr));
                        }
                        return SUCCEEDED(hr);
                    },
                    this,
                    nullptr));

                // Generate a work item for this call.
                wistd::unique_ptr<details::AsyncEventWorkItemBase> workItem(new (std::nothrow) AsyncEventWorkItem(this, targets.Get(), wistd::move(invokeOne)));
                RETURN_IF_NULL_ALLOC(workItem);

                // Put the new work item in the work queue.
                auto workQueueLock = m_srwLockWorkQueue.lock_exclusive();
                m_asyncEventWorkQueue.AddWorkItem(wistd::move(workItem));
                AsyncInvocationPolicy::HandleWorkItemQueuedUnderLock(m_threadpoolAsyncEventWork.get());
            }
            return S_OK;
        }

        // This function takes a variable number of arguments and returns a callback that results
        // in the Invoke method on the delegate interface it is given being applied to copies
        // of the arguments this method was given.
        template <typename ...Args>
        static HRESULT MakeAsyncCallbackCopyArgs(wistd::function<HRESULT(DelegateInterface*)>& callback, Args&& ...makeCopyableResults) WI_NOEXCEPT
        {
            RETURN_IF_FAILED(details::CheckExpandedSafelyCopyableArgsStatus(makeCopyableResults...));
            callback = [makeCopyableResults...] (_In_ DelegateInterface* delegateInterface) -> HRESULT
            {
                return delegateInterface->Invoke(details::RefOrGetRawValue(makeCopyableResults)...);
            };
            return S_OK;
        }

        INIT_ONCE m_initOnce;

        unique_mta_usage_cookie m_mtaUsageCookie;
        unique_threadpool_work_nocancel m_threadpoolAsyncEventWork;

        wil::srwlock m_srwLockWorkQueue;
        details::AsyncEventWorkQueue m_asyncEventWorkQueue;
    };

    // AsyncEventSource inherits from Windows::Internal::GitEventSourceSupportsAgile now. This should be changed to
    // Microsoft::WRL::AgileEventSource.

    template<
        typename DelegateInterface,
        AsyncInvokeMethod asyncInvokeMethod = AsyncInvokeMethod::Serial,
        Microsoft::WRL::InvokeMode invokeMode = Microsoft::WRL::FireAll>
    using AsyncEventSourceNoThrow = AsyncEventSourceT<
        DelegateInterface,
        Windows::Internal::GitEventSourceSupportsAgile,
        Microsoft::WRL::InvokeModeOptions<invokeMode>,
        asyncInvokeMethod,
        err_returncode_policy>;
        
    template <
        typename DelegateInterface,
        AsyncInvokeMethod asyncInvokeMethod = AsyncInvokeMethod::Serial,
        Microsoft::WRL::InvokeMode invokeMode = Microsoft::WRL::FireAll>
    using AsyncEventSourceFailFast = AsyncEventSourceT<
        DelegateInterface,
        Windows::Internal::GitEventSourceSupportsAgile,
        Microsoft::WRL::InvokeModeOptions<invokeMode>,
        asyncInvokeMethod,
        err_failfast_policy>;

#ifdef WIL_ENABLE_EXCEPTIONS
    template<
        typename DelegateInterface,
        AsyncInvokeMethod asyncInvokeMethod = AsyncInvokeMethod::Serial,
        Microsoft::WRL::InvokeMode invokeMode = Microsoft::WRL::FireAll>
    using AsyncEventSource = AsyncEventSourceT<
        DelegateInterface,
        Windows::Internal::GitEventSourceSupportsAgile,
        Microsoft::WRL::InvokeModeOptions<invokeMode>,
        asyncInvokeMethod,
        err_exception_policy>;
#endif

#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
}

#pragma warning(pop)
