// Windows Internal Libraries (wil)
//
// wil Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Internal%20Libraries%20for%20C++%20Usage%20Guide.docx?web=1
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)
//
// This WIL staging header supports the ability to create features which are dynamically withheld or used to report health
// or other considerations through the Velocity back-end.
//
// For more information see the wiki:
// https://osgwiki.com/wiki/Coin_Velocity
//
// Velocity discussion Alias (velocitydisc):
// http://idwebelements/GroupManagement.aspx?Group=velocitydisc&Operation=join  (one-click join)

//! @file
/** Patterns to enable mixing features under development with shipping code and relating the concept of a feature to backend
services for the purposes of controlling enabled state and reporting usage or feature health. */

//#define WIL_STAGING_DLL

#ifndef __WIL_STAGING_INCLUDED
#define __WIL_STAGING_INCLUDED

#pragma warning(push)
#pragma warning(disable:4714 6262)    // __forceinline not honored, stack size

#if defined(WIL_SUPPRESS_PRIVATE_API_USE) || defined(WIL_STAGING_DLL)
#define WIL_ENABLE_STAGING_API
#pragma detect_mismatch("ODR_violation_WIL_ENABLE_STAGING_API_mismatch", "1")
#else
#pragma detect_mismatch("ODR_violation_WIL_ENABLE_STAGING_API_mismatch", "0")
#endif

#include "Result.h"

typedef enum FEATURE_CHANGE_TIME
{
    FEATURE_CHANGE_TIME_READ = 0,
    FEATURE_CHANGE_TIME_MODULE_RELOAD = 1,
    FEATURE_CHANGE_TIME_SESSION = 2,
    FEATURE_CHANGE_TIME_REBOOT = 3
} FEATURE_CHANGE_TIME;

typedef enum FEATURE_ENABLED_STATE
{
    FEATURE_ENABLED_STATE_DEFAULT = 0,
    FEATURE_ENABLED_STATE_DISABLED = 1,
    FEATURE_ENABLED_STATE_ENABLED = 2
} FEATURE_ENABLED_STATE;

typedef struct FEATURE_ERROR
{
    HRESULT hr;
    UINT16 lineNumber;
    PCSTR file;
    PCSTR process;
    PCSTR module;
    UINT32 callerReturnAddressOffset;
    PCSTR callerModule;
    PCSTR message;
    UINT16 originLineNumber;
    PCSTR originFile;
    PCSTR originModule;
    UINT32 originCallerReturnAddressOffset;
    PCSTR originCallerModule;
    PCSTR originName;
} FEATURE_ERROR;

DECLARE_HANDLE(FEATURE_STATE_CHANGE_SUBSCRIPTION);
typedef void WINAPI FEATURE_STATE_CHANGE_CALLBACK(_In_opt_ void* context);
typedef FEATURE_STATE_CHANGE_CALLBACK *PFEATURE_STATE_CHANGE_CALLBACK;

STDAPI_(FEATURE_ENABLED_STATE) GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime);
STDAPI_(void) RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, _In_ PCSTR originName);
STDAPI_(void) RecordFeatureError(UINT32 featureId, _In_ const FEATURE_ERROR* error);
STDAPI_(void) SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context);
STDAPI_(void) UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription);

namespace wil
{
    //! Represents the desired state of a feature for configuration.
    enum class FeatureEnabledState : unsigned char
    {
        Default = FEATURE_ENABLED_STATE_DEFAULT,
        Disabled = FEATURE_ENABLED_STATE_DISABLED,
        Enabled = FEATURE_ENABLED_STATE_ENABLED,
    };

    //! Represents the priority of a feature configuration change.
    enum class FeatureControlKind : unsigned char
    {
        Service = 0,    //!< use to set the feature state on behalf of the velocity service (in absense of user setting)
        User = 1,       //!< use to explicitly configure a feature on or off on behalf of the user
        Testing = 2,    //!< use to adjust the state of a feature for testing
    };

    //! Represents a requested change to a particular feature or (with id == 0) a requested change across
    //! all features representing the given rolloutId or priority.
    struct StagingControl
    {
        unsigned int id;
        FeatureEnabledState state;  //!< setting (enabled or disabled), default to bypass (delete the setting)
    };

    //! Actions to perform when calling ModifyStagingControls
    enum class StagingControlActions
    {
        Default = 0x00,                 //!< Changes the given controls without affecting others
        Replace = 0x01,                 //!< The group of controls replaces all existing controls of the given FeatureControlKind
        RevertOnReboot = 0x02,          //!< Allows applying a control that will get reverted after reboot; NOTE: does not apply to a feature that already is only enabled after reboot unless IgnoreRebootRequirement also supplied.
        IgnoreRebootRequirement = 0x04  //!< Allows applying controls that would normally require a reboot immediately
    };
    DEFINE_ENUM_FLAG_OPERATORS(StagingControlActions);

    /// @cond
    namespace details
    {
#ifdef __WIL_STAGING_UNIT_TEST
        __declspec(selectany) decltype(::NtQueryWnfStateData)* g_pfnNtQueryWnfStateData = ::NtQueryWnfStateData;
        __declspec(selectany) decltype(::RtlTestAndPublishWnfStateData)* g_pfnRtlTestAndPublishWnfStateData = ::RtlTestAndPublishWnfStateData;
        __declspec(selectany) decltype(::RtlPublishWnfStateData)* g_pfnRtlPublishWnfStateData = ::RtlPublishWnfStateData;
#define __WI_NtQueryWnfStateData(StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize) ::wil::details::g_pfnNtQueryWnfStateData(StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize)
#define __WI_RtlTestAndPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope, MatchingChangeStamp) ::wil::details::g_pfnRtlTestAndPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope, MatchingChangeStamp)
#define __WI_RtlPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope) ::wil::details::g_pfnRtlPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope)
#else
#define __WI_NtQueryWnfStateData(StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize) ::NtQueryWnfStateData(StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize)
#define __WI_RtlTestAndPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope, MatchingChangeStamp) ::RtlTestAndPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope, MatchingChangeStamp)
#define __WI_RtlPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope) ::RtlPublishWnfStateData(StateName, TypeId, StateData, StateDataLength, ExplicitScope)
#endif

        const size_t c_maxCustomUsageReporting = 50;

        enum class ServiceReportingKind : unsigned char
        {
            UniqueUsage = 0,                    // DO NOT CHANGE ANY ENUMERATION VALUE
            UniqueOpportunity = 1,              // this is a service/reporting contract
            DeviceUsage = 2,
            DeviceOpportunity = 3,
            PotentialUniqueUsage = 4,
            PotentialUniqueOpportunity = 5,
            PotentialDeviceUsage = 6,
            PotentialDeviceOpportunity = 7,
            EnabledTotalDuration = 8,
            EnabledPausedDuration = 9,
            DisabledTotalDuration = 10,
            DisabledPausedDuration = 11,
            CustomEnabledBase = 100,            // Custom data points for enabled features (100-149)
            CustomDisabledBase = 150,           // Matching custom data point for tracking disabled features (150-199)
            Store = 254,                        // Flush changes to disk
            None = 255                          // Indicate no change
        };

        // Represent the type we cache to represent a feature's current state...
        enum class CachedFeatureEnabledState : unsigned char
        {
            Unknown = 0,
            Disabled = 1,
            Enabled = 2,
            Desired = 3
            // Cannot add another state without updating FeaturePropertyCache
        };

        const unsigned int c_maxUsageCount = 0x00000FFF;            // Tied to FeatureProperties::usageCount
        const unsigned int c_maxOpportunityCount = 0x0000007F;      // Tied to FeatureProperties::opportunityCount

        struct FeatureProperties
        {
            unsigned usageCount : 12;                               // Tied to c_maxUsageCount
            unsigned usageCountRepresentsPotential : 1;
            unsigned enabledState : 2;
            unsigned reportedDeviceUsage : 1;
            unsigned reportedDevicePotential : 1;
            unsigned reportedDeviceOpportunity : 1;
            unsigned reportedDevicePotentialOpportunity : 1;
            unsigned recordedDeviceUsage : 1;
            unsigned recordedDevicePotential : 1;
            unsigned recordedDeviceOpportunity : 1;
            unsigned recordedDevicePotentialOpportunity : 1;
            unsigned opportunityCount : 7;                          // Tied to c_maxOpportunityCount
            unsigned opportunityCountRepresentsPotential : 1;
            unsigned queuedForReporting : 1;
        };

        // Stored on a per-feature level to cache various properties about a feature for fast access
        union FeaturePropertyCache
        {
            FeatureProperties cache;
            volatile long var;
        };

        // Modifies a feature's property cache in a thread-safe manner
        template <typename TLambda>
        bool ModifyFeatureData(FeaturePropertyCache& properties, TLambda&& modifyFunction)
        {
            FeaturePropertyCache data;
            long var;
            do
            {
                data = properties;
                var = data.var;
                if (!modifyFunction(data))
                {
                    return false;
                }
            }
            while (::InterlockedCompareExchange(&properties.var, data.var, var) != var);
            return true;
        }

        // Modifies the cached representation of whether a feature is enabled or not
        inline void SetEnabledStateProperty(FeaturePropertyCache& properties, CachedFeatureEnabledState state)
        {
            ModifyFeatureData(properties, [&](FeaturePropertyCache& data)
            {
                if (data.cache.enabledState == static_cast<unsigned char>(state))
                {
                    return false;
                }
                data.cache.enabledState = static_cast<unsigned char>(state);
                return true;
            });
        }

        template <typename T>
        void EnsureCoalescedTimer(wil::unique_threadpool_timer& timer, bool& timerSet, T* callbackObject)
        {
            if (!timerSet)
            {
                if (!timer)
                {
                    timer.reset(::CreateThreadpoolTimer([](PTP_CALLBACK_INSTANCE, PVOID context, PTP_TIMER)
                    {
                        reinterpret_cast<T*>(context)->OnTimer();
                    }, callbackObject, nullptr));
                }
                if (timer)
                {
                    const long long delay = 5 * 60 * 1000;  // 5 mins
                    const auto allowedWindow = static_cast<DWORD>(delay) / 4;
                    FILETIME dueTime;
                    *reinterpret_cast<PLONGLONG>(&dueTime) = -static_cast<LONGLONG>(delay * 10000);
                    SetThreadpoolTimer(timer.get(), &dueTime, 0, allowedWindow);
                    timerSet = true;
                }
            }
        }

        class StagingFailureInformation : public FEATURE_ERROR
        {
        public:
            StagingFailureInformation()
            {
                ::ZeroMemory(static_cast<FEATURE_ERROR*>(this), sizeof(FEATURE_ERROR));
            }

            StagingFailureInformation(const FailureInfo& info, const DiagnosticsInfo& diagnostics, void* returnAddress)
            {
                ::ZeroMemory(static_cast<FEATURE_ERROR*>(this), sizeof(FEATURE_ERROR));

                // Note: Currently we are (by design) dropping the return addresses (returnAddress and info.returnAddress) as 
                // ALMOST EVERYONE includes file and line information already making these redundant.
                // If it ever becomes vogue NOT to include that information, then we persist these addresses.
                returnAddress;

                hr = info.hr;
                lineNumber = static_cast<unsigned short>(info.uLineNumber);
                file = info.pszFile;
                if (GetModuleInformationFromAddress(nullptr, nullptr, m_processName, ARRAYSIZE(m_processName)))
                {
                    process = m_processName;
                }
                module = info.pszModule;
                if (GetModuleInformationFromAddress(info.callerReturnAddress, &callerReturnAddressOffset, m_callerModule, ARRAYSIZE(m_callerModule)))
                {
                    callerModule = m_callerModule;
                }
                if (info.pszMessage && *info.pszMessage)
                {
                    ::wil::details::StringCchPrintfA(m_message, ARRAYSIZE(m_message), "%ws", info.pszMessage);
                    message = m_message;
                }

                originLineNumber = diagnostics.line;
                originFile = diagnostics.file;
                if (g_pfnGetModuleName)
                {
                    originModule = g_pfnGetModuleName();
                }
                if (GetModuleInformationFromAddress(diagnostics.returnAddress, &originCallerReturnAddressOffset, m_originCallerModule, ARRAYSIZE(m_originCallerModule)))
                {
                    originCallerModule = m_originCallerModule;
                }
                originName = diagnostics.name;
            }

        private:
            char m_processName[64];
            char m_callerModule[64];
            char m_originCallerModule[64];
            char m_message[96];
        };
    }

    // WARNING: EVERYTHING in this namespace must be handled WITH CARE as the entities defined within
    //          are used as an in-proc ABI contract between binaries that utilize WIL.  Making changes
    //          that add v-tables or change the storage semantics of anything herein needs to be done
    //          with care and respect to versioning.
    namespace details_abi
    {
        struct buffer_range
        {
            unsigned char* buffer = nullptr;
            unsigned char* bufferEnd = nullptr;
            unsigned char* bufferAllocEnd = nullptr;

            buffer_range() = default;
            buffer_range(void* bufferParam, size_t allocatedSize, size_t size = 0) :
                buffer(static_cast<unsigned char*>(bufferParam)), bufferEnd(buffer + size), bufferAllocEnd(buffer + allocatedSize)
            {
                WI_ASSERT(size <= allocatedSize);
            }

            size_t size() const { return (bufferEnd - buffer); }
            size_t capacity() const { return (bufferAllocEnd - buffer); }
            size_t remaining_capacity() const { return (bufferEnd >= bufferAllocEnd) ? 0 : (bufferAllocEnd - bufferEnd); }
            void clear() { bufferEnd = buffer; }
            bool empty() { return (bufferEnd == buffer); }
        };

        struct heap_buffer : buffer_range
        {
            unique_hheap_ptr<void> allocation;

            heap_buffer() = default;

            // Note: Does not take ownership of the given buffer, so may be used to pre-seed with a stack buffer and then subsequently
            // reserve or ensure space to grow into the heap.
            heap_buffer(void* bufferParam, size_t allocatedSize, size_t size = 0) : buffer_range(bufferParam, allocatedSize, size)
            {
            }

            bool reserve(size_t newCapacity)
            {
                static_assert(sizeof(allocation) == sizeof(void*), "ABI contract, must not carry any code or be changed");
                if (capacity() < newCapacity)
                {
                    newCapacity += (64 - (newCapacity % 64));

                    auto newBuffer = static_cast<unsigned char*>(::HeapAlloc(::GetProcessHeap(), 0, newCapacity));
                    if (!newBuffer)
                    {
                        return false;
                    }

                    auto currentSize = size();
                    memcpy_s(newBuffer, newCapacity, buffer, size());
                    allocation.reset(newBuffer);
                    buffer = newBuffer;
                    bufferEnd = newBuffer + currentSize;
                    bufferAllocEnd = newBuffer + newCapacity;
                }
                return true;
            }

            bool ensure(size_t appendSize)
            {
                auto requiredSize = (size() + appendSize);
                auto currentCapacity = capacity();
                if (requiredSize < currentCapacity)
                {
                    return true;
                }
                currentCapacity *= 2;
                return reserve(appendSize < currentCapacity ? currentCapacity : appendSize);
            }

            void set_size(size_t newSize)
            {
                WI_ASSERT(buffer + newSize <= bufferAllocEnd);
                auto newEnd = buffer + newSize;
                if (WI_VERIFY(newEnd <= bufferAllocEnd))
                {
                    bufferEnd = newEnd;
                }
            }

            bool push_back(_In_reads_bytes_(appendSize) const void* data, size_t appendSize)
            {
                if (!ensure(appendSize))
                {
                    return false;
                }
                WI_ASSERT(appendSize <= remaining_capacity());
                memcpy_s(bufferEnd, remaining_capacity(), data, appendSize);
                bufferEnd += appendSize;
                return true;
            }
        };

        // NOTE: POD types only, no constructors, destrucotrs, v-tables, etc
        template <typename T>
        class heap_vector
        {
        public:
            size_t size() { return (m_data.size() / sizeof(T)); }
            T* data() { return reinterpret_cast<T*>(m_data.buffer); }
            void clear() { m_data.clear(); }

            HRESULT push_back(const T& value)
            {
                return m_data.push_back(&value, sizeof(value)) ? S_OK : E_OUTOFMEMORY;
            }
        private:
            heap_buffer m_data;
        };
    }
    /// @endcond
}

/// @cond
#ifndef __STAGING_TEST_HOOK_USAGE
#define __STAGING_TEST_HOOK_USAGE(id, kind, addend)
#define __STAGING_TEST_HOOK_ERROR(id, info, diagnostics, returnAddress)
#define __STAGING_TEST_HOOK_USAGE_UNFILTERED false
#endif
/// @endcond

#if !defined(WIL_ENABLE_STAGING_API)
#ifndef _NTURTL_
#error WIL Staging APIs require NT headers. Try including #include <nt.h>, <ntrtl.h> and <nturtl.h> before <windows.h>
#endif
#include "TraceLogging.h"
#include <WNFNamesP.h>

namespace wil
{
    namespace details
    {
        enum class ControlsStore
        {
            Normal,
            Boot
        };

        enum class DefaultStateAction
        {
            Store,      // FeatureEnabledState::Default will be preserved in the controls store (use when you use one controls 
                        // store to persist application of "default" to another at a later point in time.
            Delete      // FeatureEnabledState::Default signals that a feature control should be removed from the persistent
                        // controls store.
        };

        struct SerializedStagingControl
        {
            unsigned int id;
            FeatureEnabledState state;
            FeatureControlKind kind;

            SerializedStagingControl() = default;
            SerializedStagingControl(const StagingControl& control, FeatureControlKind kind_) :
                id(control.id),
                state(control.state),
                kind(kind_)
            {
            }
        };

        struct StagingControlHeader
        {
            unsigned char version;
            unsigned char unused1;
            unsigned short unused2;
        };

        static_assert(sizeof(SerializedStagingControl) == 8, "Size must be fixed or versioning must be planned/changed");
        static_assert(sizeof(StagingControlHeader) == 4, "Size must be fixed or versioning must be planned/changed");

        const unsigned char c_serializedStagingVersion = 1;
        const size_t c_maxStoreSizeInBytes = 4096; // Changes to this value must be accompanied by the correpsonding WNF payload size change in //minkernel/manifests/wnf/WNF-Names-Wil.man
        const size_t c_maxSerializedControls = (c_maxStoreSizeInBytes - sizeof(StagingControlHeader)) / sizeof(SerializedStagingControl);

        typedef SerializedStagingControl SerializedStagingControlArray[c_maxSerializedControls];

        class SerializedControlsStore
        {
        public:
            SerializedControlsStore(ControlsStore store) :
                m_store(store)
            {
                m_header.version = c_serializedStagingVersion;
            }

            ~SerializedControlsStore()
            {
                WriteControls();
            }

            void AdjustControl(const SerializedStagingControl& change, DefaultStateAction action = DefaultStateAction::Delete)
            {
                EnsureRead();

                for (auto& control : make_range(m_controls, m_controlsCount))
                {
                    if ((control.id == change.id) && (control.kind == change.kind))
                    {
                        // We delete 'Default' unless explicitly requested not to
                        if ((change.state == FeatureEnabledState::Default) && (action == DefaultStateAction::Delete))
                        {
                            const SerializedStagingControl* next = &control + 1;
                            ::MoveMemory(&control, next, ((m_controls + m_controlsCount) - next) * sizeof(*next));
                            m_controlsCount--;
                            m_controlsModified = true;
                        }
                        else if (control.state != change.state)
                        {
                            control.state = change.state;
                            m_controlsModified = true;
                        }

                        return;
                    }
                }

                // We avoid adding 'Default' unless explicitly requested
                if ((action == DefaultStateAction::Store) || (change.state != FeatureEnabledState::Default))
                {
                    if (m_controlsCount < ARRAYSIZE(m_controls))
                    {
                        m_controls[m_controlsCount++] = change;
                        m_controlsModified = true;
                    }
                    else
                    {
                        LOG_WIN32(ERROR_OUT_OF_STRUCTURES);
                    }
                }
            }

            void AdjustControl(const StagingControl& change, FeatureControlKind kind, DefaultStateAction action = DefaultStateAction::Delete)
            {
                AdjustControl(SerializedStagingControl(change, kind), action);
            }

            void DeleteControl(unsigned int id, FeatureControlKind kind)
            {
                AdjustControl(StagingControl{ id, FeatureEnabledState::Default }, kind, DefaultStateAction::Delete);
            }

            void Clear()
            {
                EnsureRead();
                if (m_controlsCount > 0)
                {
                    m_controlsCount = 0;
                    m_controlsModified = true;
                }
            }

            _Success_return_ bool FindControl(unsigned int id, FeatureControlKind kind, _Out_ SerializedStagingControl* result)
            {
                EnsureRead();

                for (const auto& control : make_range(m_controls, m_controlsCount))
                {
                    if ((control.id == id) && (control.kind == kind))
                    {
                        *result = control;
                        return true;
                    }
                }
                return false;
            }

            _Success_return_ bool FindControl(unsigned int id, _Out_ SerializedStagingControl* result)
            {
                EnsureRead();

                bool found = false;

                for (const auto& control : make_range(m_controls, m_controlsCount))
                {
                    if ((control.id == id) &&
                        (!found || (control.kind > result->kind)))
                    {
                        *result = control;
                        found = true;
                    }
                }

                return found;
            }

            SerializedStagingControlArray& GetControls()
            {
                EnsureRead();
                return m_controls;
            }

            size_t GetCount()
            {
                EnsureRead();
                return m_controlsCount;
            }

        private:
            inline WNF_STATE_NAME ResolveWnfStateName(ControlsStore controlStore)
            {
                if (controlStore == ControlsStore::Normal)
                {
                    return WNF_WIL_FEATURE_STORE;
                }
                WI_ASSERT(controlStore == ControlsStore::Boot);
                return WNF_WIL_BOOT_FEATURE_STORE;
            }

            inline HRESULT ReadFeatureConfigFromWNF(ControlsStore controlsStore, _Out_writes_bytes_(storeDataSize) void* storeData, _Inout_ size_t* storeDataSize)
            {
                *storeDataSize = 0;
                const WNF_STATE_NAME storeWnfStateName = ResolveWnfStateName(controlsStore);

                bool isPublished;
                *storeDataSize = static_cast<ULONG>(c_maxStoreSizeInBytes);
                return wil::wnf_query_nothrow(storeWnfStateName, &isPublished, storeData, static_cast<size_t>(c_maxStoreSizeInBytes), storeDataSize);
            }

            inline HRESULT WriteFeatureConfigToWNF(ControlsStore controlsStore, _In_reads_bytes_(storeDataSize) void* storeData, size_t storeDataSize)
            {
                const WNF_STATE_NAME storeWnfStateName = ResolveWnfStateName(controlsStore);

                return wil::wnf_publish_nothrow(storeWnfStateName, storeData, storeDataSize);
            }

            // Note that these two data members are intentionally placed together so they can be read and written as a block
            StagingControlHeader m_header;
            SerializedStagingControlArray m_controls;

            _Field_range_(0, c_maxSerializedControls) size_t m_controlsCount = 0;

            bool m_controlsModified = false;
            bool m_controlsRead = false;
            ControlsStore m_store = ControlsStore::Normal;

            inline void EnsureRead()
            {
                if (!m_controlsRead)
                {
                    ReadControls();
                }
            }

            void RemoveStaleControls()
            {
                // TODO: Hook up knowledge of all controls to eliminate stale ones
            }

            inline void ReadControls()
            {
                m_controlsModified = false;
                m_controlsRead = true;

                auto size = static_cast<size_t>(c_maxStoreSizeInBytes);
                if (SUCCEEDED_LOG(ReadFeatureConfigFromWNF(m_store, &m_header, &size)) &&
                    (size > sizeof(m_header)) &&
                    (m_header.version == c_serializedStagingVersion))
                {
                    m_controlsCount = (size - sizeof(m_header)) / sizeof(m_controls[0]);
                    return;
                }

                m_header = StagingControlHeader();
                m_header.version = c_serializedStagingVersion;
                m_controlsCount = 0;
            }

            inline void WriteControls()
            {
                if (!m_controlsRead || !m_controlsModified)
                {
                    return;
                }

                RemoveStaleControls();

                auto size = static_cast<const size_t>(sizeof(m_header) + (sizeof(m_controls[0]) * m_controlsCount));
                LOG_IF_FAILED(WriteFeatureConfigToWNF(m_store, reinterpret_cast<void*>(&m_header), size));
            }
        };

        class SerializedStagingControls
        {
        public:
            void ModifyStagingControl(FeatureControlKind kind, StagingControlActions actions, const StagingControl& change)
            {
                m_controls.AdjustControl(change, kind);

                if (WI_IS_FLAG_SET(actions, StagingControlActions::RevertOnReboot))
                {
                    // We need to add the pending revert... we only do this if there's not already a pending state
                    // as the existing pending state would be the state we should revert *to*.

                    SerializedStagingControl bootCurrent;
                    if (!m_bootControls.FindControl(change.id, kind, &bootCurrent))
                    {
                        SerializedStagingControl current;
                        const bool exists = m_controls.FindControl(change.id, kind, &current);

                        // Add a pending revert to the original state
                        m_bootControls.AdjustControl(StagingControl{ change.id, exists ? current.state : FeatureEnabledState::Default }, kind, DefaultStateAction::Store);
                    }
                }
                else
                {
                    // Delete any pending changes as we've now transitioned to the desired state
                    m_bootControls.DeleteControl(change.id, kind);
                }
            }

            template <typename Callback>
            void EnumerateStagingControls(FeatureControlKind kind, Callback&& lambda)
            {
                auto original = m_controls.GetControls();
                for (auto& current : make_range(original, m_controls.GetCount()))
                {
                    if (current.kind == kind)
                    {
                        lambda(StagingControl{ current.id, current.state });
                    }
                }
            }

            void ModifyStagingControls(FeatureControlKind kind, StagingControlActions actions, size_t count, _In_reads_(count) StagingControl* changes)
            {
                if (WI_IS_FLAG_SET(actions, StagingControlActions::Replace))
                {
                    auto original = m_controls.GetControls();
                    for (auto& current : make_range(original, m_controls.GetCount()))
                    {
                        if (current.kind == kind)
                        {
                            bool found = false;
                            for (auto& change : make_range(changes, count))
                            {
                                if (current.id == change.id)
                                {
                                    found = true;
                                    break;
                                }
                            }

                            if (!found)
                            {
                                ModifyStagingControl(kind, actions, StagingControl{ current.id, FeatureEnabledState::Default });
                            }
                        }
                    }
                }

                for (auto& change : make_range(changes, count))
                {
                    ModifyStagingControl(kind, actions, change);
                }
            }

            void ModifyStagingControlsForReboot()
            {
                if (m_bootControls.GetCount() > 0)
                {
                    for (const auto& control : make_range(m_bootControls.GetControls(), m_bootControls.GetCount()))
                    {
                        m_controls.AdjustControl(control);
                    }

                    m_bootControls.Clear();
                }
            }

            bool IsEmpty()
            {
                return (m_controls.GetCount() == 0);
            }

            SerializedControlsStore& Controls() { return m_controls; }

        private:
            SerializedControlsStore m_controls{ ControlsStore::Normal };
            SerializedControlsStore m_bootControls{ ControlsStore::Boot };
        };

        class WilFeatureTelemetry : public TraceLoggingProvider
        {
            // {452b6bbd-c64b-5767-e1f2-ae8ee1027667}
            IMPLEMENT_TRACELOGGING_CLASS(WilFeatureTelemetry, "Microsoft.Windows.WilFeatureTelemetry",
                (0x452b6bbd, 0xc64b, 0x5767, 0xe1, 0xf2, 0xae, 0x8e, 0xe1, 0x02, 0x76, 0x67));
        public:

            static void ReportFeatureUsage(unsigned int featureId, ServiceReportingKind kind, size_t addend) WI_NOEXCEPT
            {
                __STAGING_TEST_HOOK_USAGE(featureId, kind, addend);

                // NOTE:  This is a temporary data point that likely will have a higher data rate than we would like.  It will
                //          be replaced with client-side aggregation and be rolled up sometime early in 2016.

                TraceLoggingWrite(Provider(), "GranularFeatureUsage", TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY),
                    TraceLoggingValue(featureId, "id"),
                    TraceLoggingValue(static_cast<unsigned long>(kind), "kind"),
                    TraceLoggingValue(static_cast<unsigned long>(addend), "addend"));
            }

            static void ReportFeatureError(const FailureInfo& info, unsigned int featureId, const DiagnosticsInfo& diagnostics, void* returnAddress) WI_NOEXCEPT
            {
                __STAGING_TEST_HOOK_ERROR(featureId, info, diagnostics, returnAddress);

                // NOTE:  This is a temporary data point that likely will have a higher data rate than we would like.  It will
                //          be replaced with client-side aggregation and be rolled up sometime early in 2016.

                void* baseAddress = nullptr;
                returnAddress = baseAddress ? reinterpret_cast<void*>(static_cast<char*>(returnAddress) - static_cast<char*>(baseAddress)) : nullptr;
                void* diagnosticsReturnAddress = baseAddress ? reinterpret_cast<void*>(static_cast<char*>(diagnostics.returnAddress) - static_cast<char*>(baseAddress)) : nullptr;

                TraceLoggingWrite(Provider(), "GranularFeatureError", TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                    TraceLoggingValue(featureId, "id"),
                    TraceLoggingHResult(info.hr, "hr"),
                    TraceLoggingUInt8(static_cast<unsigned char>(info.type), "type"),
                    TraceLoggingValue(info.pszFile, "file"),
                    TraceLoggingUInt16(static_cast<unsigned short>(info.uLineNumber), "line"),
                    TraceLoggingValue(info.pszModule, "module"),
                    TraceLoggingValue(info.pszMessage, "message"),
                    TraceLoggingPointer(info.returnAddress, "returnAddress"),
                    TraceLoggingPointer(info.callerReturnAddress, "callerReturnAddress"),
                    TraceLoggingValue(diagnostics.file, "sourceFile"),
                    TraceLoggingUInt16(diagnostics.line, "sourceLine"),
                    TraceLoggingPointer(returnAddress, "sourceReturnAddress"),
                    TraceLoggingPointer(diagnosticsReturnAddress, "sourceCallerReturnAddress"));
            }
        };
    }

    // WARNING: EVERYTHING in this namespace must be handled WITH CARE as the entities defined within
    //          are used as an in-proc ABI contract between binaries that utilize WIL.  Making changes
    //          that add v-tables or change the storage semantics of anything herein needs to be done
    //          with care and respect to versioning.
    namespace details_abi
    {
        enum class CountSize : unsigned char
        {
            None,
            UnsignedShort,
            UnsignedLong
        };

        struct UsageIndexProperty
        {
            const unsigned short c_size;
            const CountSize c_countSize;

            unsigned int count = 0;
            unsigned short size = 0;
            void* countBuffer = nullptr;
            void* value = nullptr;

            UsageIndexProperty(unsigned short size, CountSize countSize) : c_size(size), c_countSize(countSize)
            {
            }

            int Compare(_In_reads_bytes_(indexSize) void* indexData, size_t indexSize) const
            {
                return ((indexSize == size) ? memcmp(indexData, value, indexSize) : (static_cast<int>(indexSize) - size));
            }

            void Attach(unsigned int newCount, void* valueData, unsigned short valueSize)
            {
                count = newCount;
                size = valueSize;
                countBuffer = nullptr;
                value = valueData;
            }

            void UpdateCount(unsigned int newCount)
            {
                WI_ASSERT(countBuffer);
                WI_ASSERT(c_countSize != CountSize::None);
                if (count != newCount)
                {
                    count = newCount;
                    if (c_countSize == CountSize::UnsignedShort)
                    {
                        unsigned short value = static_cast<unsigned short>(count);
                        memcpy_s(countBuffer, sizeof(unsigned short), &value, sizeof(value));
                    }
                    else if (c_countSize == CountSize::UnsignedLong)
                    {
                        memcpy_s(countBuffer, sizeof(unsigned long), &count, sizeof(unsigned long));
                    }
                }
            }

            bool AddToCount(unsigned int addend)
            {
                const bool update = (c_countSize != CountSize::None);
                if (update)
                {
                    UpdateCount(count + addend);
                }
                return update;
            }

            size_t GetAndValidateCount(size_t maxCount)
            {
                if (count > maxCount)       // data is untrusted, so we truncate count if the buffer was not large enough
                {
                    UpdateCount(static_cast<unsigned int>(maxCount));
                }
                return count;
            }

            size_t GetSize() const
            {
                size_t result = (c_size == 0) ? (sizeof(size) + size) : c_size;
                if (c_countSize == CountSize::UnsignedShort)
                {
                    return (result + sizeof(unsigned short));
                }
                if (c_countSize == CountSize::UnsignedLong)
                {
                    return (result + sizeof(unsigned long));
                }
                return result;
            }

            static size_t GetFixedSize(unsigned short fixedSize, CountSize countSize)
            {
                WI_ASSERT(fixedSize != 0);
                return UsageIndexProperty(fixedSize, countSize).GetSize();
            }

            bool Write(unsigned char*& bufferParam, unsigned char* bufferEnd) const
            {
                unsigned char* buffer = bufferParam;
                if (c_countSize == CountSize::UnsignedShort)
                {
                    if ((buffer + sizeof(unsigned short)) > bufferEnd)
                    {
                        return false;
                    }
                    unsigned short value = static_cast<unsigned short>(count);
                    memcpy_s(buffer, sizeof(unsigned short), &value, sizeof(unsigned short));
                    buffer += sizeof(unsigned short);
                }
                else if (c_countSize == CountSize::UnsignedLong)
                {
                    if ((static_cast<unsigned char*>(buffer) + sizeof(unsigned long)) > bufferEnd)
                    {
                        return false;
                    }
                    memcpy_s(buffer, sizeof(unsigned long), &count, sizeof(unsigned long));
                    buffer += sizeof(unsigned long);
                }

                if (c_size == 0)
                {
                    if ((buffer + sizeof(size)) > bufferEnd)
                    {
                        return false;
                    }
                    memcpy_s(buffer, bufferEnd - buffer, &size, sizeof(size));
                    buffer += sizeof(size);
                }

                if ((buffer + size) > bufferEnd)
                {
                    return false;
                }
                memcpy_s(buffer, bufferEnd - buffer, value, size);
                buffer += size;
                bufferParam = buffer;
                return true;
            }

            bool Read(unsigned char*& bufferParam, unsigned char* bufferEnd)
            {
                unsigned char* buffer = bufferParam;
                if (c_countSize == CountSize::UnsignedShort)
                {
                    if ((buffer + sizeof(unsigned short)) > bufferEnd)
                    {
                        return false;
                    }
                    countBuffer = buffer;
                    unsigned short value;
                    memcpy_s(&value, sizeof(unsigned short), buffer, sizeof(unsigned short));
                    buffer += sizeof(unsigned short);
                    count = value;
                }
                else if (c_countSize == CountSize::UnsignedLong)
                {
                    if ((buffer + sizeof(unsigned long)) > bufferEnd)
                    {
                        return false;
                    }
                    countBuffer = buffer;
                    memcpy_s(&count, sizeof(unsigned long), buffer, sizeof(unsigned long));
                    buffer += sizeof(unsigned long);
                }

                size = c_size;
                if (c_size == 0)
                {
                    if ((buffer + sizeof(size)) > bufferEnd)
                    {
                        return false;
                    }
                    memcpy_s(&size, sizeof(size), buffer, sizeof(size));
                    buffer += sizeof(size);
                }

                if ((buffer + size) > bufferEnd)
                {
                    return false;
                }
                value = buffer;
                buffer += size;
                bufferParam = buffer;
                return true;
            }
        };

        // Manages an efficient serialized, contiguous sorted index over two properties allowing the representation
        // of various forms of usage data in WNF state (or other contiguous memory).  Format:
        //
        // Header: header
        // [Repeating until end of buffer]:
        //      IndexEntry:  UsageIndexProperty [Index]
        //      [Repeating for IndexEntry.count]
        //          ValueEntry:  UsageIndexProperty [Value]
        class RawUsageIndex
        {
        public:
            static const unsigned short c_formatVersion = 0;
            const unsigned short c_usageDataVersion;

            // Note that this implementation uses non-static constants, rather than template parameters to minimize
            // template bloat for this (fairly large) class to manage varying forms of data.
            const unsigned short c_indexSize;
            const CountSize c_indexCountSize;
            const unsigned short c_valueSize;
            const CountSize c_valueCountSize;

            RawUsageIndex(unsigned short version, unsigned short indexSize, CountSize indexCountSize, unsigned short valueSize, CountSize valueCountSize) :
                c_usageDataVersion(version), c_indexSize(indexSize), c_indexCountSize(indexCountSize), c_valueSize(valueSize), c_valueCountSize(valueCountSize),
                c_fixedValueSize((valueSize > 0) ? UsageIndexProperty::GetFixedSize(c_valueSize, c_valueCountSize) : 0)
            {
            }

            void Swap(RawUsageIndex& other)
            {
                WI_ASSERT((c_indexSize == other.c_indexSize) && (c_indexCountSize == other.c_indexCountSize) && (c_valueSize == other.c_valueSize) && (c_valueCountSize == other.c_valueCountSize));

                wistd::swap_wil(m_buffer, other.m_buffer);
                wistd::swap_wil(m_isDirty, other.m_isDirty);
                wistd::swap_wil(m_sourceNewer, other.m_sourceNewer);
            }

            bool IsDirty() const { return m_isDirty; }
            void* GetData() const { return m_buffer.buffer; }
            size_t GetDataSize() const { return m_buffer.size(); }
            unsigned short IsSourceNewer() const { return m_sourceNewer; }
            void SetAllowGrowth(bool allowGrowth) { m_allowGrowth = allowGrowth; }

            void EnsureAllocated()
            {
                if (m_buffer.buffer != m_buffer.allocation.get())
                {
                    heap_buffer newBuffer;
                    newBuffer.push_back(m_buffer.buffer, m_buffer.size());
                    m_buffer = wistd::move(newBuffer);
                }
            }

            void SetBuffer(_In_reads_bytes_(size) void* buffer, size_t size, size_t alloc)
            {
                FAIL_FAST_IF(alloc < sizeof(Header));

                m_buffer.allocation.reset();
                m_buffer.buffer = static_cast<unsigned char *>(buffer);
                m_buffer.bufferEnd = m_buffer.buffer + size;
                m_buffer.bufferAllocEnd = m_buffer.buffer + alloc;
                m_sourceNewer = false;

                auto header = static_cast<Header*>(buffer);
                bool valid = (size >= sizeof(*header));
                if (valid)
                {
                    m_sourceNewer = ((header->formatVersion > c_formatVersion) || (header->version > c_usageDataVersion));

                    if ((header->formatVersion != c_formatVersion) || (header->version != c_usageDataVersion) ||
                        (header->indexSize != c_indexSize) || (header->indexCountSize != c_indexCountSize) ||
                        (header->valueSize != c_valueSize) || (header->valueCountSize != c_valueCountSize))
                    {
                        valid = false;
                    }
                }

                if (!valid)
                {
                    header->formatVersion = c_formatVersion;
                    header->version = c_usageDataVersion;
                    header->indexSize = c_indexSize;
                    header->indexCountSize = c_indexCountSize;
                    header->valueSize = c_valueSize;
                    header->valueCountSize = c_valueCountSize;

                    m_buffer.bufferEnd = m_buffer.buffer + sizeof(Header);
                }
            }

            bool Iterate(wistd::function<bool(void*, size_t, void*, size_t, unsigned int)> callback) const
            {
                auto buffer = m_buffer.buffer + sizeof(Header);
                UsageIndexProperty index(c_indexSize, c_indexCountSize);
                UsageIndexProperty value(c_valueSize, c_valueCountSize);

                while (index.Read(buffer, m_buffer.bufferEnd))
                {
                    for (unsigned int valueEntry = 0; (valueEntry < index.count) && value.Read(buffer, m_buffer.bufferEnd); valueEntry++)
                    {
                        if (!callback(index.value, index.size, value.value, value.size, value.count))
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            template <typename TIndex, typename TValue>
            bool RecordUsage(TIndex index, TValue value, unsigned int addend = 1)
            {
                return RecordUsage(&index, sizeof(index), &value, sizeof(value), addend);
            }

            template <typename TIndex>
            bool RecordUsage(TIndex index, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                return RecordUsage(&index, sizeof(index), value, valueSize, addend);
            }

            bool RecordUsage(_In_reads_bytes_(indexSize) void* indexData, size_t indexSize, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                if (RecordUsageInternal(indexData, indexSize, valueData, valueSize, addend))
                {
                    return true;
                }

                const size_t maxRequiredSize = (indexSize + valueSize) + 32;      // 32 for overhead approximation (counts, sizes)
                if (!m_buffer.buffer)
                {
                    heap_buffer heapBuffer;
                    if (heapBuffer.ensure(maxRequiredSize + sizeof(Header)))
                    {
                        SetBuffer(heapBuffer.buffer, 0, heapBuffer.capacity());
                        m_buffer.allocation = wistd::move(heapBuffer.allocation);
                        SetAllowGrowth(true);
                    }
                }
                else if (m_allowGrowth)
                {
                    m_buffer.ensure(maxRequiredSize);
                }

                return RecordUsageInternal(indexData, indexSize, valueData, valueSize, addend);
            }

        private:
            const size_t c_fixedValueSize;

            struct Header
            {
                unsigned short formatVersion;
                unsigned short version;
                unsigned short indexSize;
                unsigned short valueSize;
                CountSize indexCountSize;
                CountSize valueCountSize;
            };

            heap_buffer m_buffer;

            bool m_isDirty = false;
            bool m_sourceNewer = false;
            bool m_allowGrowth = false;

            // NOTE: Directly adapted from STL's version only accounting for our variable size buffers
            unsigned char* LowerBound(unsigned char* buffer, size_t count, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize)
            {
                UsageIndexProperty value(c_valueSize, c_valueCountSize);
                unsigned char* first = buffer;
                while (count > 0)
                {
                    size_t count2 = count / 2;
                    auto middle = first;
                    middle += (count2 * c_fixedValueSize);

                    WI_VERIFY(value.Read(middle, m_buffer.bufferEnd));
                    if (value.Compare(valueData, valueSize) > 0)
                    {
                        first = middle;
                        count -= count2 + 1;
                    }
                    else
                    {
                        count = count2;
                    }
                }
                return first;
            }

            unsigned char* FindInsertionPointOrIncrement(UsageIndexProperty& index, unsigned char* buffer, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend)
            {
                int compare = -1;
                UsageIndexProperty value(c_valueSize, c_valueCountSize);
                if (c_fixedValueSize > 0)
                {
                    size_t count = index.GetAndValidateCount(m_buffer.size() / c_fixedValueSize);
                    unsigned char* last = buffer + (count * c_fixedValueSize);
                    buffer = LowerBound(buffer, count, valueData, valueSize);
                    if (buffer < last)
                    {
                        auto next = buffer;
                        WI_VERIFY(value.Read(next, m_buffer.bufferEnd));
                        compare = value.Compare(valueData, valueSize);
                    }
                }
                else
                {
                    for (unsigned int valuesRead = 0; (valuesRead < index.count); valuesRead++)
                    {
                        auto next = buffer;
                        if (!value.Read(next, m_buffer.bufferEnd))
                        {
                            index.UpdateCount(valuesRead);
                            break;
                        }
                        compare = value.Compare(valueData, valueSize);
                        if (compare <= 0)
                        {
                            break;
                        }
                        buffer = next;
                    }
                }

                if (compare == 0)
                {
                    // We found the match -- add to it if we need to and we're done
                    m_isDirty = (value.AddToCount(addend) || m_isDirty);
                    return nullptr;
                }

                return buffer;
            }

            unsigned char* SkipValues(UsageIndexProperty& index, unsigned char* buffer)
            {
                if (c_fixedValueSize > 0)
                {
                    auto indexCount = index.GetAndValidateCount(m_buffer.size() / c_fixedValueSize);
                    buffer += (c_fixedValueSize * indexCount);
                }
                else
                {
                    // We didn't find the index item, so continue on to the next index item after skipping the values...
                    UsageIndexProperty value(c_valueSize, c_valueCountSize);
                    unsigned int valuesRead = 0;
                    for (; (valuesRead < index.count) && value.Read(buffer, m_buffer.bufferEnd); valuesRead++)
                    {
                    }
                    index.UpdateCount(valuesRead);
                }
                return buffer;
            }

            bool RecordUsageInternal(_In_reads_bytes_(indexSize) void* indexData, size_t indexSize, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                if (!m_buffer.buffer)
                {
                    return false;
                }

                auto buffer = m_buffer.buffer + sizeof(Header);

                UsageIndexProperty index(c_indexSize, c_indexCountSize);
                bool foundIndex = false;
                bool validRead = false;
                auto prev = buffer;
                while ((validRead = index.Read(buffer, m_buffer.bufferEnd)), validRead)
                {
                    int compare = index.Compare(indexData, indexSize);
                    if (compare < 0)
                    {
                        // break so that we can do the index and value insertion below...
                        buffer = prev;
                        break;
                    }
                    if (compare == 0)
                    {
                        buffer = FindInsertionPointOrIncrement(index, buffer, valueData, valueSize, addend);
                        if (!buffer)
                        {
                            // No insertion pointer, we're done (incremented).
                            return true;
                        }

                        // Break so that we can do the value insertion below...
                        foundIndex = true;
                        break;
                    }

                    // Skip all the values in the buffer to the next index item...
                    buffer = SkipValues(index, buffer);
                    prev = buffer;
                }

                WI_ASSERT(buffer <= m_buffer.bufferEnd);
                WI_ASSERT(m_buffer.bufferEnd <= m_buffer.bufferAllocEnd);
                if (!validRead)
                {
                    // we don't trust our input data, so if we got to the end and there is any more garbage remaining then
                    // truncate at that position...
                    m_buffer.bufferEnd = buffer;
                }

                // Figure out how much space we need for insertion...
                size_t requiredIndexBytes = 0;
                if (!foundIndex)
                {
                    index.Attach(1, indexData, static_cast<unsigned short>(indexSize));
                    requiredIndexBytes = index.GetSize();
                }
                UsageIndexProperty value(c_valueSize, c_valueCountSize);
                value.Attach(addend, valueData, static_cast<unsigned short>(valueSize));
                const size_t requiredBytes = requiredIndexBytes + value.GetSize();

                // Ensure we have enough space for what we want to insert
                const size_t availableBytes = m_buffer.remaining_capacity();
                if (availableBytes < requiredBytes)
                {
                    return false;
                }

                // Move the memory and do the insertion in the leftover hole
                const size_t moveBytes = m_buffer.bufferEnd - buffer;
                memmove_s(buffer + requiredBytes, (m_buffer.bufferAllocEnd - buffer) - requiredBytes, buffer, moveBytes);
                m_buffer.bufferEnd += requiredBytes;
                if (!foundIndex)
                {
                    WI_VERIFY(index.Write(buffer, m_buffer.bufferEnd));
                }
                else
                {
                    index.AddToCount(1);
                }
                WI_VERIFY(value.Write(buffer, m_buffer.bufferEnd));
                WI_ASSERT((buffer + moveBytes) == m_buffer.bufferEnd);
                m_isDirty = true;
                return true;
            }
        };

        template <typename TIndex, typename TValue, CountSize countSize>
        class UsageIndex : public RawUsageIndex
        {
        public:
            UsageIndex(unsigned short version) : RawUsageIndex(version, sizeof(TIndex), CountSize::UnsignedShort, sizeof(TValue), countSize)
            {
            }

            __declspec(noinline) bool RecordUsage(TIndex index, TValue value, unsigned int addend = 1)
            {
                return RawUsageIndex::RecordUsage(&index, sizeof(index), &value, sizeof(value), addend);
            }

            bool Iterate(wistd::function<bool(TIndex, TValue, unsigned int)> callback) const
            {
                return RawUsageIndex::Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int count) -> bool
                {
                    if (WI_VERIFY((indexSize == sizeof(TIndex)) && (valueSize == sizeof(TValue))))
                    {
                        if (!callback(*reinterpret_cast<TIndex*>(index), *reinterpret_cast<TValue*>(value), count))
                        {
                            return false;
                        }
                    }
                    return true;
                });
            }
        };

        template <typename TIndex, CountSize countSize>
        class UsageIndexBuffer : public RawUsageIndex
        {
        public:
            UsageIndexBuffer(unsigned short version) : RawUsageIndex(version, sizeof(TIndex), CountSize::UnsignedShort, 0, countSize)
            {
            }

            bool RecordUsage(TIndex index, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                return RawUsageIndex::RecordUsage(&index, sizeof(index), valueData, valueSize, addend);
            }

            bool Iterate(wistd::function<bool(TIndex, void*, size_t, unsigned int)> callback) const
            {
                return RawUsageIndex::Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int count) -> bool
                {
                    if (WI_VERIFY(indexSize == sizeof(TIndex)))
                    {
                        if (!callback(*reinterpret_cast<TIndex*>(index), value, valueSize, count))
                        {
                            return false;
                        }
                    }
                    return true;
                });
            }
        };

        struct SerializedFailure
        {
            // NOTE: structure is serialized; variable order maintains packing efficiency
            HRESULT hr;
            unsigned short lineNumber;
            unsigned short file;
            unsigned short module;
            unsigned short callerModule;
            unsigned int callerReturnAddressOffset;
            unsigned short message;
            unsigned short originLineNumber;
            unsigned short originFile;
            unsigned short originModule;
            unsigned int originCallerReturnAddressOffset;
            unsigned short originCallerModule;
            unsigned short originName;
            unsigned short process;

            _Success_return_
            static bool Serialize(const FEATURE_ERROR& info, _Out_ size_t* requiredParam, _Out_writes_bytes_to_opt_(capacity, *requiredParam) void* bufferParam, size_t capacity)
            {
                auto bufferStart = static_cast<unsigned char*>(bufferParam);
                auto buffer = bufferStart;
                auto bufferEnd = buffer + capacity;

                SerializedFailure headerStack;
                size_t& required = *requiredParam;
                required = sizeof(SerializedFailure);
                SerializedFailure& header = (required <= capacity) ? *reinterpret_cast<SerializedFailure*>(buffer) : headerStack;
                if (required <= capacity)
                {
                    ::ZeroMemory(&header, sizeof(header));
                    header.hr = info.hr;
                    header.lineNumber = info.lineNumber;
                    header.callerReturnAddressOffset = info.callerReturnAddressOffset;
                    header.originLineNumber = info.originLineNumber;
                    header.originCallerReturnAddressOffset = info.originCallerReturnAddressOffset;
                    buffer += sizeof(header);
                }

                auto writeString = [&](_In_opt_ PCSTR string, unsigned short& offsetVar, unsigned short alternateOffset = 0)
                {
                    if (string)
                    {
                        // We may be able to collapse the string
                        if ((alternateOffset > 0) && (0 == strcmp(string, reinterpret_cast<PCSTR>(bufferStart + alternateOffset))))
                        {
                            offsetVar = alternateOffset;
                        }
                        else
                        {
                            auto stringSize = strlen(string) + 1;
                            required += stringSize;
                            if (required <= capacity)
                            {
                                memcpy_s(buffer, bufferEnd - buffer, string, stringSize);
                                offsetVar = static_cast<unsigned short>(buffer - bufferStart);
                                buffer += stringSize;
                            }
                        }
                    }
                };

                writeString(info.file, header.file);
                writeString(info.process, header.process);
                writeString(info.module, header.module, header.process);
                writeString(info.callerModule, header.callerModule, header.module);
                writeString(info.message, header.message);
                writeString(info.originFile, header.originFile, header.file);
                writeString(info.callerModule, header.callerModule, header.module);
                writeString(info.originModule, header.originModule, header.module);
                writeString(info.originCallerModule, header.originCallerModule, header.originModule);
                writeString(info.originName, header.originName);

                return (required <= capacity);
            }

            _Success_return_
            static bool Deserialize(details::StagingFailureInformation& info, _In_reads_bytes_(size) void* bufferParam, size_t size)
            {
                auto bufferStart = static_cast<char*>(bufferParam);
                auto buffer = bufferStart;
                auto bufferEnd = buffer + size;

                if ((buffer + sizeof(SerializedFailure)) < bufferEnd)
                {
                    const SerializedFailure& header = *reinterpret_cast<const SerializedFailure*>(buffer);

                    info.hr = header.hr;
                    info.lineNumber = header.lineNumber;
                    info.callerReturnAddressOffset = header.callerReturnAddressOffset;
                    info.originLineNumber = header.originLineNumber;
                    info.originCallerReturnAddressOffset = header.originCallerReturnAddressOffset;

                    auto readString = [&](PCSTR& value, unsigned short offsetVar)
                    {
                        if (offsetVar > 0)
                        {
                            auto start = buffer + offsetVar;
                            if ((start < bufferEnd) && SUCCEEDED(wil::details::StringCchLengthA(start, bufferEnd - start, nullptr)))
                            {
                                value = start;
                            }
                        }
                    };

                    readString(info.file, header.file);
                    readString(info.process, header.process);
                    readString(info.module, header.module);
                    readString(info.callerModule, header.callerModule);
                    readString(info.message, header.message);
                    readString(info.originFile, header.originFile);
                    readString(info.callerModule, header.callerModule);
                    readString(info.originModule, header.originModule);
                    readString(info.originCallerModule, header.originCallerModule);
                    readString(info.originName, header.originName);

                    return true;
                }
                return false;
            }
        };

        inline bool ReadWnfUsageBuffer(const WNF_STATE_NAME* state, _Out_writes_bytes_(countBytes) unsigned char* buffer, size_t countBytes, _Inout_ RawUsageIndex& usage, _Out_ WNF_CHANGE_STAMP* changeStamp)
        {
            ULONG size = static_cast<ULONG>(countBytes);
            const auto status = __WI_NtQueryWnfStateData(state, nullptr, nullptr, changeStamp, buffer, &size);
            const auto hr = wil::details::NtStatusToHr(status); hr;
            if (STATUS_SUCCESS != status)
            {
                size = 0;
                *changeStamp = 0;
            }

            usage.SetBuffer(buffer, size, countBytes);
            return (!usage.IsSourceNewer());
        }

        inline bool WriteWnfUsageBuffer(const WNF_STATE_NAME* state, WNF_CHANGE_STAMP match, RawUsageIndex& usage)
        {
            if (usage.IsDirty())
            {
                const auto status = __WI_RtlTestAndPublishWnfStateData(*state, nullptr, usage.GetData(), static_cast<ULONG>(usage.GetDataSize()), nullptr, match);
                const auto hr = wil::details::NtStatusToHr(status); hr;
                if (STATUS_UNSUCCESSFUL == status)
                {
                    return false;
                }
                if (FAILED(hr))
                {
                    const auto status2 = __WI_RtlPublishWnfStateData(*state, nullptr, usage.GetData(), static_cast<ULONG>(usage.GetDataSize()), nullptr);
                    const auto hr2 = wil::details::NtStatusToHr(status); hr2;
                }
            }
            return true;
        }

        inline void RecordWnfUsageIndex(_In_reads_(count) const WNF_STATE_NAME* states, size_t count, const RawUsageIndex& usage)
        {
            WI_ASSERT(count > 0);
            auto statesEnd = states + count;

            size_t updateAttempts = 0;
            size_t valuesWritten = 0;
            bool fullyWritten = false;
            do
            {
                RawUsageIndex persistedUsage(usage.c_usageDataVersion, usage.c_indexSize, usage.c_indexCountSize, usage.c_valueSize, usage.c_valueCountSize);
                unsigned char buffer[4096];
                WNF_CHANGE_STAMP readChangeStamp;
                if (!ReadWnfUsageBuffer(states, buffer, ARRAYSIZE(buffer), persistedUsage, &readChangeStamp))
                {
                    break;
                }
                WI_ASSERT(!persistedUsage.IsDirty());

                size_t read = 0;
                fullyWritten = usage.Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int addend) -> bool
                {
                    if (read >= valuesWritten)
                    {
                        if (!persistedUsage.RecordUsage(index, indexSize, value, valueSize, addend))
                        {
                            return false;
                        }
                    }
                    read++;
                    return true;
                });

                if (WriteWnfUsageBuffer(states, readChangeStamp, persistedUsage))
                {
                    states++;
                    valuesWritten = read;
                }
                else
                {
                    updateAttempts++;
                    fullyWritten = false;
                }
            }
            while (!fullyWritten && (states < statesEnd) && WI_VERIFY(updateAttempts < 50));
        }

        enum class UsageIndexesLoadOptions
        {
            None = 0x0,
            Clear = 0x1,
        };
        DEFINE_ENUM_FLAG_OPERATORS(UsageIndexesLoadOptions);

        inline void LoadWnfUsageIndex(UsageIndexesLoadOptions options, _In_reads_(count) const WNF_STATE_NAME* states, size_t count, RawUsageIndex& usage)
        {
            unsigned char rootBuffer[4096];
            WNF_CHANGE_STAMP readChangeStamp;
            details_abi::ReadWnfUsageBuffer(states, rootBuffer, ARRAYSIZE(rootBuffer), usage, &readChangeStamp);
            usage.SetAllowGrowth(true);

            if (WI_IS_FLAG_SET(options, UsageIndexesLoadOptions::Clear))
            {
                __WI_RtlPublishWnfStateData(states[0], nullptr, nullptr, 0ul, nullptr);
            }

            for (auto& name : make_range(states + 1, states + count))
            {
                details_abi::RawUsageIndex persistedUsage(usage.c_usageDataVersion, usage.c_indexSize, usage.c_indexCountSize, usage.c_valueSize, usage.c_valueCountSize);
                unsigned char buffer[4096];
                details_abi::ReadWnfUsageBuffer(&name, buffer, ARRAYSIZE(buffer), persistedUsage, &readChangeStamp);
                persistedUsage.Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int addend) -> bool
                {
                    usage.RecordUsage(index, indexSize, value, valueSize, addend);
                    return true;
                });

                if (WI_IS_FLAG_SET(options, UsageIndexesLoadOptions::Clear))
                {
                    __WI_RtlPublishWnfStateData(name, nullptr, nullptr, 0ul, nullptr);
                }
            }

            usage.EnsureAllocated();
        }

        struct UsageIndexes
        {
            UsageIndex<details::ServiceReportingKind, unsigned int, CountSize::None> device;
            UsageIndex<details::ServiceReportingKind, unsigned int, CountSize::UnsignedLong> unique;
            UsageIndexBuffer<unsigned int, CountSize::UnsignedShort> error;

            // ZEROs below are VERSIONING information for any changes to these index types
            UsageIndexes() : device(0), unique(0), error(0)
            {
            }

            void Record()
            {
                if (device.IsDirty())
                {
                    WNF_STATE_NAME names[] = { WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_1, WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_2, WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_3 };
                    RecordWnfUsageIndex(names, ARRAYSIZE(names), device);
                }

                if (unique.IsDirty())
                {
                    WNF_STATE_NAME names[] = { WNF_WIL_FEATURE_USAGE_TRACKING_1, WNF_WIL_FEATURE_USAGE_TRACKING_2, WNF_WIL_FEATURE_USAGE_TRACKING_3 };
                    RecordWnfUsageIndex(names, ARRAYSIZE(names), unique);
                }

                if (error.IsDirty())
                {
                    WNF_STATE_NAME names[] = { WNF_WIL_FEATURE_HEALTH_TRACKING_1, WNF_WIL_FEATURE_HEALTH_TRACKING_2, WNF_WIL_FEATURE_HEALTH_TRACKING_3, WNF_WIL_FEATURE_HEALTH_TRACKING_4, WNF_WIL_FEATURE_HEALTH_TRACKING_5, WNF_WIL_FEATURE_HEALTH_TRACKING_6 };
                    RecordWnfUsageIndex(names, ARRAYSIZE(names), error);
                }
            }

            void Load(UsageIndexesLoadOptions options)
            {
                WNF_STATE_NAME deviceNames[] = { WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_1, WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_2, WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_3 };
                LoadWnfUsageIndex(options, deviceNames, ARRAYSIZE(deviceNames), device);

                WNF_STATE_NAME uniqueNames[] = { WNF_WIL_FEATURE_USAGE_TRACKING_1, WNF_WIL_FEATURE_USAGE_TRACKING_2, WNF_WIL_FEATURE_USAGE_TRACKING_3 };
                LoadWnfUsageIndex(options, uniqueNames, ARRAYSIZE(uniqueNames), unique);

                WNF_STATE_NAME errorNames[] = { WNF_WIL_FEATURE_HEALTH_TRACKING_1, WNF_WIL_FEATURE_HEALTH_TRACKING_2, WNF_WIL_FEATURE_HEALTH_TRACKING_3, WNF_WIL_FEATURE_HEALTH_TRACKING_4, WNF_WIL_FEATURE_HEALTH_TRACKING_5, WNF_WIL_FEATURE_HEALTH_TRACKING_6 };
                LoadWnfUsageIndex(options, errorNames, ARRAYSIZE(errorNames), error);
            }
        };

        // This class manages a cache of the globally tracked usage and error information so that every module
        // doesn't itself end up with a cache of the same information.
        class FeatureStateData
        {
        public:
            // note: not called during process shutdown
            ~FeatureStateData()
            {
                ProcessShutdown();
            }

            // called by virtue of being held by process shared memory (final release when terminating)
            void ProcessShutdown()
            {
                UsageIndexes indexes;
                RetrieveUsageUnderLock(indexes);
                indexes.Record();
            }

            bool RecordFeatureError(unsigned int featureId, const FEATURE_ERROR& failure) WI_NOEXCEPT
            {
                unsigned char bufferStack[256];
                details_abi::heap_buffer buffer(reinterpret_cast<void*>(bufferStack), ARRAYSIZE(bufferStack));
                size_t required = 0;
                if (!SerializedFailure::Serialize(failure, &required, buffer.buffer, buffer.capacity()))
                {
                    if (!buffer.ensure(required) || !SerializedFailure::Serialize(failure, &required, buffer.buffer, buffer.capacity()))
                    {
                        // best effort -- memory failure can't record feature error
                        return false;
                    }
                }
                buffer.set_size(required);

                auto lock = m_lock.lock_exclusive();
                return m_usage.error.RecordUsage(featureId, buffer.buffer, buffer.size());
            }

            // we allow external locking for repeat callers of RecordFeatureUsageUnderLock
            rwlock_release_exclusive_scope_exit lock_exclusive() WI_NOEXCEPT
            {
                return m_lock.lock_exclusive();
            }

            bool RecordFeatureUsageUnderLock(unsigned int featureId, details::ServiceReportingKind kind, size_t addend)
            {
                __STAGING_TEST_HOOK_USAGE(featureId, kind, addend);

                switch (kind)
                {
                case details::ServiceReportingKind::DeviceUsage:
                case details::ServiceReportingKind::DeviceOpportunity:
                case details::ServiceReportingKind::PotentialDeviceUsage:
                case details::ServiceReportingKind::PotentialDeviceOpportunity:
                    m_usage.device.RecordUsage(kind, featureId);
                    return m_usage.device.IsDirty();
                default:
                    return m_usage.unique.RecordUsage(kind, featureId, static_cast<unsigned int>(addend));
                }
            }

            bool RecordFeatureUsage(unsigned int featureId, details::ServiceReportingKind kind, size_t addend)
            {
                switch (kind)
                {
                case details::ServiceReportingKind::Store:
                    RecordUsage();
                    return true;

                default:
                    if (static_cast<size_t>(kind) < (static_cast<size_t>(details::ServiceReportingKind::CustomDisabledBase) + details::c_maxCustomUsageReporting))
                    {
                        auto lock = m_lock.lock_exclusive();
                        return RecordFeatureUsageUnderLock(featureId, kind, addend);
                    }
                }
                return false;
            }

            void RecordUsage()
            {
                UsageIndexes usageToRecord;
                {
                    auto lock = m_lock.lock_exclusive();
                    RetrieveUsageUnderLock(usageToRecord);
                }
                usageToRecord.Record();
            }

        private:
            srwlock m_lock;
            UsageIndexes m_usage;

            void RetrieveUsageUnderLock(UsageIndexes& usageToRecord)
            {
                static_assert(sizeof(m_lock) == sizeof(SRWLOCK), "ABI contract, must not carry any code or be changed");

                // Now we pull any dirty records and assign them to our out pointer so that they can be
                // used to persist data to the backing store outside of our lock.

                if (m_usage.device.IsDirty())
                {
                    usageToRecord.device.Swap(m_usage.device);
                }
                if (m_usage.unique.IsDirty())
                {
                    usageToRecord.unique.Swap(m_usage.unique);
                }
                if (m_usage.error.IsDirty())
                {
                    usageToRecord.error.Swap(m_usage.error);
                }
            }
        };
    } // details_abi

    namespace details
    {
        inline void SubscribeFeaturePropertyCacheToEnabledStateChanges(_Inout_ FeaturePropertyCache& state) WI_NOEXCEPT;

        // This class manages the central relationship between staging features and reporting/eventing.
        // It keeps in-memory data structures to track usage and creates timers to occasionally push
        // that usage information into WNF state so that it can be centrally reported.  It also manages
        // the same kind of relationship for reporting feature errors and subscribing to feature notification
        // changes.
        class FeatureStateManager
        {
        public:
            FeatureStateManager() WI_NOEXCEPT : m_dataStorage("WilStaging_01")
            {
                // This object will be held in a zero-initialized global variable.  We want this line to
                // run at construction time so we can distinguish when someone is trying to call us ahead
                // of construction (CRT init order)
                m_fInitialized = true;
            }

            // note: not called during process shutdown
            ~FeatureStateManager() WI_NOEXCEPT
            {
                m_fInitialized = false;
                m_timer.reset();
            }

            // called by virtue of FeatureStateManager being held by shutdown_aware_object
            void ProcessShutdown() WI_NOEXCEPT
            {
                m_fInitialized = false;
                m_dataStorage.~ProcessLocalStorage();
            }

            void RecordFeatureError(unsigned int featureId, const FEATURE_ERROR& error) WI_NOEXCEPT
            {
                if (m_fInitialized && EnsureStateData() && m_data->RecordFeatureError(featureId, error))
                {
                    if (!ProcessShutdownInProgress())
                    {
                        auto lock = m_lock.lock_exclusive();
                        EnsureTimerUnderLock();
                    }
                }
            }

            void RecordFeatureUsage(unsigned int featureId, ServiceReportingKind kind, size_t addend) WI_NOEXCEPT
            {
                if (m_fInitialized && EnsureStateData() && m_data->RecordFeatureUsage(featureId, kind, addend))
                {
                    if (!ProcessShutdownInProgress())
                    {
                        auto lock = m_lock.lock_exclusive();
                        EnsureTimerUnderLock();
                    }
                }
            }

            inline void SubscribeToEnabledStateChanges(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
            {
                // Note:  Handle value is always (Index + 1) to retain slot ZERO as invalid (nullptr)
                *subscription = nullptr;
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    EnsureSubscribedToStateChangesUnderLock();

                    for (size_t index = 0; index < m_subscriptions.size(); index++)
                    {
                        if (m_subscriptions.data()[index].callback == nullptr)
                        {
                            m_subscriptions.data()[index] = Subscription{ callback, context };
                            *subscription = reinterpret_cast<FEATURE_STATE_CHANGE_SUBSCRIPTION>(index + 1);
                            return;
                        }
                    }
                    if (m_subscriptions.push_back(Subscription{ callback, context }))
                    {
                        *subscription = reinterpret_cast<FEATURE_STATE_CHANGE_SUBSCRIPTION>(m_subscriptions.size());
                    }
                }
            }

            inline void UnsubscribeToEnabledStateChanges(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
            {
                if (m_fInitialized && subscription)
                {
                    auto unsubscribeLock = m_unsubscribeLock.lock();
                    auto lock = m_lock.lock_exclusive();
                    size_t index = (reinterpret_cast<size_t>(subscription) - 1);
                    if (index < m_subscriptions.size())
                    {
                        m_subscriptions.data()[index] = Subscription{};
                    }
                }
            }

            void OnTimer() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    {
                        auto lock = m_lock.lock_exclusive();
                        m_timerSet = false;
                    }
                    if (m_data)
                    {
                        m_data->RecordUsage();
                    }
                }
            }

            void OnStateChange() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    size_t subscriptionCount = 0;
                    {
                        auto lock = m_lock.lock_shared();
                        subscriptionCount = m_subscriptions.size();
                    }

                    size_t subscription = 0;
                    while (subscription < subscriptionCount)
                    {
                        Subscription callback = {};
                        auto unsubscribeLock = m_unsubscribeLock.lock();

                        {
                            auto lock = m_lock.lock_exclusive();
                            for (; (subscription < subscriptionCount); subscription++)
                            {
                                if (m_subscriptions.data()[subscription].callback)
                                {
                                    callback = m_subscriptions.data()[subscription++];
                                    break;
                                }
                            }
                        }

                        if (callback.callback)
                        {
                            // Note: Reentrant unsubscribeLock allows caller to unsubscribe in their callback
                            callback.callback(callback.context);
                        }
                    }
                }
            }

        private:
            bool m_fInitialized = false;

            details_abi::ProcessLocalStorage<details_abi::FeatureStateData> m_dataStorage;
            details_abi::FeatureStateData* m_data = nullptr;

            srwlock m_lock;
            critical_section m_unsubscribeLock;     // ordering:  must always be taken ahead of m_lock

            wil::unique_threadpool_timer m_timer;
            bool m_timerSet = false;
            wil::unique_pwnf_user_subscription m_wnfSubscription;

            struct Subscription
            {
                PFEATURE_STATE_CHANGE_CALLBACK callback;
                void* context;
            };

            details_abi::heap_vector<Subscription> m_subscriptions;

            bool EnsureStateData() WI_NOEXCEPT
            {
                if (!m_data)
                {
                    auto data = m_data ? nullptr : m_dataStorage.GetShared();
                    auto lock = m_lock.lock_exclusive();
                    if (!m_data)
                    {
                        m_data = data;
                    }
                }
                return (m_data != nullptr);
            }

            void EnsureTimerUnderLock() WI_NOEXCEPT
            {
                EnsureCoalescedTimer(m_timer, m_timerSet, this);
            }

            void EnsureSubscribedToStateChangesUnderLock() WI_NOEXCEPT
            {
                if (!m_wnfSubscription)
                {
                    // Retrieve the latest changestamp and use that as the basis for change notifications
                    WNF_CHANGE_STAMP subscribeFrom = 0;
                    ULONG bufferSize = 0;
                    NtQueryWnfStateData(&WNF_WIL_FEATURE_STORE, 0, nullptr, &subscribeFrom, nullptr, &bufferSize);

                    RtlSubscribeWnfStateChangeNotification(&m_wnfSubscription, WNF_WIL_FEATURE_STORE, subscribeFrom,
                        [](WNF_STATE_NAME, WNF_CHANGE_STAMP, WNF_TYPE_ID*, void* context, const void*, ULONG) -> NTSTATUS
                    {
                        reinterpret_cast<FeatureStateManager*>(context)->OnStateChange();
                        return STATUS_SUCCESS;
                    }, this, nullptr, 0, 0);
                }
            }
        };

        __declspec(selectany) shutdown_aware_object<FeatureStateManager> g_featureStateManager;

        _Success_return_ inline bool IsFeatureConfigured(unsigned int id, _Out_ SerializedStagingControl* result)
        {
            static FeaturePropertyCache storeProbe = {};
            if (static_cast<CachedFeatureEnabledState>(storeProbe.cache.enabledState) == CachedFeatureEnabledState::Disabled)
            {
                return false;
            }

            SerializedStagingControls controls;
            if (controls.IsEmpty())
            {
                SetEnabledStateProperty(storeProbe, CachedFeatureEnabledState::Disabled);
                SubscribeFeaturePropertyCacheToEnabledStateChanges(storeProbe);
                return false;
            }

            return controls.Controls().FindControl(id, result);
        }

        // Public API Contract Implementation in WIL

        inline FEATURE_ENABLED_STATE WilApiImpl_GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime)
        {
            changeTime;
            SerializedStagingControl result;
            if (IsFeatureConfigured(featureId, &result))
            {
                return static_cast<FEATURE_ENABLED_STATE>(result.state);
            }
            return FEATURE_ENABLED_STATE_DEFAULT;
        }

        inline void WilApiImpl_RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, PCSTR originName)
        {
            originName;
            g_featureStateManager.get().RecordFeatureUsage(featureId, static_cast<ServiceReportingKind>(kind), static_cast<size_t>(addend));
        }

        inline void WilApiImpl_RecordFeatureError(UINT32 featureId, const FEATURE_ERROR* error)
        {
            g_featureStateManager.get().RecordFeatureError(featureId, *error);
        }

        inline void WilApiImpl_SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
        {
            g_featureStateManager.get().SubscribeToEnabledStateChanges(subscription, callback, context);
        }

        inline void WilApiImpl_UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
        {
            g_featureStateManager.get().UnsubscribeToEnabledStateChanges(subscription);
        }
    }

    template <typename Callback>
    inline void EnumerateStagingControls(FeatureControlKind kind, Callback&& lambda)
    {
        details::SerializedStagingControls controls;
        controls.EnumerateStagingControls(kind, wistd::forward<Callback>(lambda));
    }

    //! Adjust persistent staging controls by marking features as enabled, disabled, or default.
    inline void ModifyStagingControls(FeatureControlKind kind, size_t count, _In_reads_(count) StagingControl* changes, StagingControlActions actions = StagingControlActions::Replace)
    {
        details::SerializedStagingControls controls;
        controls.ModifyStagingControls(kind, actions, count, changes);
        // Synchronously kick our change notification callback so that setting controls and then responding to changes is predictable/stable within the same module (tests)
        details::g_featureStateManager.get().OnStateChange();
    }

    //! Adjust a single persistent staging control by marking the feature as enabled, disabled, or default.
    inline void ModifyStagingControl(unsigned int id, FeatureEnabledState state, FeatureControlKind kind = FeatureControlKind::User, StagingControlActions actions = StagingControlActions::Default)
    {
        StagingControl control{ id, state };
        ModifyStagingControls(kind, 1, &control, actions);
    }

    //! Adjust test staging controls by marking features as enabled, disabled, or default; test changes override "normal" feature controls, but are not stable (they may be reset at reboot or the request of other tests).
    //! Note: does not honor the reboot required state for making the change.
    inline void ModifyTestStagingControls(size_t count, _In_reads_(count) StagingControl* changes, StagingControlActions actions = StagingControlActions::Replace | StagingControlActions::IgnoreRebootRequirement | StagingControlActions::RevertOnReboot)
    {
        ModifyStagingControls(FeatureControlKind::Testing, count, changes, actions);
    }

    //! Adjust a single persistent staging control by marking the feature as enabled, disabled, or default.
    inline void ModifyTestStagingControl(unsigned int id, FeatureEnabledState state, StagingControlActions actions = StagingControlActions::Replace | StagingControlActions::IgnoreRebootRequirement | StagingControlActions::RevertOnReboot)
    {
        StagingControl control{ id, state };
        ModifyStagingControls(FeatureControlKind::Testing, 1, &control, actions);
    }

    //! Clears all test staging controls immediately.
    //! Note: does not honor the reboot required state for making the change.
    inline void ClearTestStagingControls()
    {
        // Default is replace, so modifying ZERO is identical to clearing
        ModifyTestStagingControls(0, nullptr);
    }

    //! Called exactly once during reboot to promote reboot settings to real settings.
    inline void ModifyStagingControlsForReboot()
    {
        details::SerializedStagingControls controls;
        controls.ModifyStagingControlsForReboot();
    }
}

#endif // WIL_ENABLE_STAGING_API

//! This macro checks whether a feature is enabled or not; use when Feature::IsEnabled is rejected due to inlining failure.
#define WI_IsFeatureEnabled(className) \
    (className::__private_IsEnabledPreCheck(), className::isAlwaysEnabled || (!className::isAlwaysDisabled && className::__private_IsEnabled()))

//! This macro checks whether a feature is enabled or not while controlling reporting; use when Feature::IsEnabled is rejected due to inlining failure.
#define WI_IsFeatureEnabled_ReportUsage(className, reportingKind) \
    (className::__private_IsEnabledPreCheck(reportingKind), className::isAlwaysEnabled || (!className::isAlwaysDisabled && className::__private_IsEnabled(reportingKind)))

/// @cond
// Do not use - will be removed
#define WI_IsFeatureEnabledWithDefault(className, isEnabledByDefaultOverride) \
    (className::isAlwaysEnabled || (!className::isAlwaysDisabled && className::__private_IsEnabledWithDefault(isEnabledByDefaultOverride)))
/// @endcond

//! This is an alias for WI_IsFeatureEnabled_ReportUsage with ReportingKind::None to check enablement without reporting; use when Feature::IsEnabled(kind) is rejected due to inlining failure.
#define WI_IsFeatureEnabled_SuppressUsage(className) \
    WI_IsFeatureEnabled_ReportUsage(className, ::wil::ReportingKind::None)


/// @cond
// Following are various staging control macros that are generated by an XML compilation tool for consumption by the
// staging header.  These settings are documented in the example XML file used to generate them.

#define __WilStaging_CompletePreviousMacro  WI_PASTE(___dummy, __COUNTER__)
#define __WilStaging_StartNewMacro          static const char WI_PASTE(___dummy, __COUNTER__)

#define WI_DEFINE_FEATURE(className_, id_, stage_, name_, description_, ...) \
    struct __WilFeatureTraits_##className_ : ::wil::details::FeatureTraitsBase \
    { \
        static const unsigned int id = id_; \
        static const ::wil::FeatureStage stage = ::wil::FeatureStage:: stage_; \
        static PCSTR GetName()          { return name_; } \
        static PCSTR GetDescription()   { return description_; } \
        __WilStaging_StartNewMacro, \
        __VA_ARGS__ ; \
    }; \
    using className_ = ::wil::Feature<__WilFeatureTraits_##className_>;

#define WilStagingGroup(name_, description_) \
    __WilStaging_CompletePreviousMacro; \
    static PCSTR GetGroupName() { return name_; } \
    static PCSTR GetGroupDescription() { return description_; } \
    __WilStaging_StartNewMacro

#define WilStagingEmail(alias_) \
    __WilStaging_CompletePreviousMacro; \
    static PCSTR GetEMail() { return alias_; } \
    __WilStaging_StartNewMacro

#define WilStagingLink(link_) \
    __WilStaging_CompletePreviousMacro; \
    static PCSTR GetLink() { return link_; } \
    __WilStaging_StartNewMacro

#define WilStagingVersion(version_) \
    __WilStaging_CompletePreviousMacro; \
    static const unsigned short version = version_; \
    __WilStaging_StartNewMacro

#define WilStagingBaseVersion(baseVersion_) \
    __WilStaging_CompletePreviousMacro; \
    static const unsigned short baseVersion = baseVersion_; \
    __WilStaging_StartNewMacro

#define WilStagingBranchOverride(stage_) \
    __WilStaging_CompletePreviousMacro; \
        static const ::wil::FeatureStage stageOverride = ::wil::FeatureStage:: stage_; \
    __WilStaging_StartNewMacro

#ifdef RESULT_DEBUG
#define WilStagingChkBranchOverride(stage_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::FeatureStage stageChkOverride = ::wil::FeatureStage:: stage_; \
    __WilStaging_StartNewMacro
#else
#define WilStagingChkBranchOverride(stage_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::FeatureStage stageChkOverride = ::wil::details::UnknownFeatureStage; \
    __WilStaging_StartNewMacro
#endif

#define WilStagingRequiredBy(...) \
    __WilStaging_CompletePreviousMacro; \
    static bool ShouldBeEnabledForDependentFeature_DefaultDisabled() \
    { \
        return ::wil::details::DependentFeatures<__VA_ARGS__>::ShouldBeEnabledForDependentFeature(false); \
    } \
    __WilStaging_StartNewMacro

#define WilStagingRequiredByEnabled(...) \
    __WilStaging_CompletePreviousMacro; \
    static bool ShouldBeEnabledForDependentFeature_DefaultEnabled() \
    { \
        return ::wil::details::DependentFeatures<__VA_ARGS__>::ShouldBeEnabledForDependentFeature(true); \
    } \
    __WilStaging_StartNewMacro

#define WilStagingRequiresFeature(...) \
    __WilStaging_CompletePreviousMacro; \
    static bool AreDependenciesEnabled() \
    { \
        return ::wil::details::RequiredFeatures<__VA_ARGS__>::IsEnabled(); \
    } \
    __WilStaging_StartNewMacro

#define WilStagingChangeTime(changeTime_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::FeatureChangeTime changeTime = ::wil::FeatureChangeTime:: changeTime_; \
    __WilStaging_StartNewMacro

/// @endcond

namespace wil
{
    //! Indicates the stage of a feature in the current production build.
    enum class FeatureStage
    {
        AlwaysDisabled,     //!< The feature is always excluded from the production build and cannot be enabled
        DisabledByDefault,  //!< The feature is included, but off by default; it can be switched on through configuration
        EnabledByDefault,   //!< The feature is included and on by default; it can be switched off through configuration
        AlwaysEnabled,      //!< The feature is always included in the production build and cannot be disabled
    };

    //! Controls precisely when feature changes are observed
    enum class FeatureChangeTime : unsigned char
    {
        OnRead = FEATURE_CHANGE_TIME_READ,              //!< Changes to the feature's enabled state are observed the next time Feature::IsEnabled() is called.
        OnReload = FEATURE_CHANGE_TIME_MODULE_RELOAD,   //!< Changes observed only after the containing binary has been reloaded.
        OnReboot = FEATURE_CHANGE_TIME_REBOOT           //!< Changes observed only after reboot.
    };

    //! Identifies what kind of reporting should be done in response to the primitive accepting this parameters as 
    enum class ReportingKind : unsigned char
    {
        None,               //!< Do not report any usage information from this primitive; use for module initialization and other unrelated actions to usage.
        UniqueUsage,        //!< Report a unique usage of this feature (when enabled) or a potential usage of the feature (when disabled).
        UniqueOpportunity,  //!< Report that the user was given an opportunity to use this feature (when enabled) or had the potential to show opportunity to use the feature (when disabled).
        DeviceUsage,        //!< [Default for most primivitves] Report that this device has used the feature (when enabled) or had the potential to use the feature (when disabled).
        DeviceOpportunity,  //!< Report that this device has presented the opportunity to use this feature (when enabled) or had the potential to show opportunity to use the feature (when disabled).
        TotalDuration,      //!< Report the overall total time (in milliseconds) spent executing the feature (including time spent waiting on user input)
        PausedDuration,     //!< Report the time (in milliseconds) spent waiting on user input while the feature is being executed
        // Custom data points start at enum value 100
    };

    inline ReportingKind GetCustomReportingKind(unsigned char index)
    {
        WI_ASSERT(index < details::c_maxCustomUsageReporting);
        return static_cast<ReportingKind>(index + 100);
    }

    //! Identifies one of several string values that can be retrieved from a feature
    enum class FeatureString
    {
        Name,
        Description,
        GroupName,
        GroupDescription,
        EMail,
        Link
    };

    //! Represents a mask of possible properties to retrieve from a feature
    enum class FeaturePropertyGroupFlags
    {
        None                = 0x0000,
        StaticProperties    = 0x0001,   //!< Retrieves all compile-time specified properties for a Feature
        FeatureEnabledState = 0x0002    //!< Retrieves the runtime enabled state of a feature
    };
    DEFINE_ENUM_FLAG_OPERATORS(FeaturePropertyGroupFlags)

    //! Holds basic information describing a feature for runtime inspection
    struct FeatureProperties
    {
        // Retrieved via FeaturePropertyGroupFlags::StaticProperties
        FeatureChangeTime changeTime;
        FeatureStage stage;
        bool isEnabledByDefault;
        unsigned short version;
        unsigned short baseVersion;

        // Retrieved via FeaturePropertyGroupFlags::FeatureEnabledState
        bool isEnabled;
    };

    /// @cond
    // Depracated: Use ReportingKind. TODO: Remove after the next RI/FI
    enum class FeatureUsageKind
    {
        ImplicitOpportunityCount = 0,
        ImplicitUsageCount       = 1,
        OpportunityCount         = 2,
        UsageCount               = 3,
        TotalDuration            = 4,
        PausedDuration           = 5,
        Custom1                  = 11,
        Custom2                  = 12,
        Custom3                  = 13,
        Custom4                  = 14,
        Custom5                  = 15,
    };

    /// @cond
    namespace details
    {
        struct RecordUsageResult
        {
            bool queueBackground;                   // true if FeaturePropertyCache should be queued recording through a background thread

            unsigned int countImmediate;            // > 0 for usage that needs immediate recording
            ServiceReportingKind kindImmediate;     // kind to report (when count > 0)
        };

        // Cache usage information if possible; return information about what couldn't be cached
        inline RecordUsageResult RecordUsageInPropertyCache(FeaturePropertyCache& properties, ServiceReportingKind kind, unsigned int addend)
        {
            RecordUsageResult result = {};
            switch (kind)
            {
            case ServiceReportingKind::DeviceUsage:
            case ServiceReportingKind::DeviceOpportunity:
            case ServiceReportingKind::PotentialDeviceUsage:
            case ServiceReportingKind::PotentialDeviceOpportunity:
                {
                    FeaturePropertyCache match = {};
                    switch (kind)
                    {
                    case ServiceReportingKind::DeviceUsage:
                        match.cache.reportedDeviceUsage = 1;
                        break;
                    case ServiceReportingKind::DeviceOpportunity:
                        match.cache.reportedDeviceOpportunity = 1;
                        break;
                    case ServiceReportingKind::PotentialDeviceUsage:
                        match.cache.reportedDevicePotential = 1;
                        break;
                    case ServiceReportingKind::PotentialDeviceOpportunity:
                        match.cache.reportedDevicePotentialOpportunity = 1;
                        break;
                    };

                    ModifyFeatureData(properties, [&](FeaturePropertyCache& data)
                    {
                        result.queueBackground = false;
                        if ((match.var & data.var) == match.var)
                        {
                            return false;
                        }
                        data.var |= match.var;
                        if (!data.cache.queuedForReporting)
                        {
                            data.cache.queuedForReporting = 1;
                            result.queueBackground = true;
                        }
                        return true;
                    });
                }
                break;

            case ServiceReportingKind::UniqueUsage:
            case ServiceReportingKind::PotentialUniqueUsage:
                {
                    ModifyFeatureData(properties, [&](FeaturePropertyCache& data)
                    {
                        result.countImmediate = 0;
                        result.queueBackground = (!data.cache.queuedForReporting);
                        data.cache.queuedForReporting = 1;
                        if ((data.cache.usageCountRepresentsPotential != 0) != (kind == ServiceReportingKind::PotentialUniqueUsage))
                        {
                            if (data.cache.usageCount > 0)
                            {
                                result.kindImmediate = (kind == ServiceReportingKind::UniqueUsage) ? ServiceReportingKind::PotentialUniqueUsage : ServiceReportingKind::UniqueUsage;
                                result.countImmediate = data.cache.usageCount;
                                data.cache.usageCount = 0;
                            }
                            data.cache.usageCountRepresentsPotential = (kind == ServiceReportingKind::PotentialUniqueUsage) ? 1 : 0;
                        }
                        unsigned int newCount = data.cache.usageCount + addend;
                        if ((newCount > c_maxUsageCount) || (newCount < data.cache.usageCount))
                        {
                            newCount = addend;
                            result.kindImmediate = kind;
                            result.countImmediate = data.cache.usageCount;
                        }
                        data.cache.usageCount = newCount;
                        return true;
                    });
                }
                break;

            case ServiceReportingKind::UniqueOpportunity:
            case ServiceReportingKind::PotentialUniqueOpportunity:
                {
                    // Note:  This code is largely duplicitive of the block in the previous case statement with different
                    // sizes and variable names.  This is intentional as refactoring would require generic property get/set
                    // functionality which isn't yet needed -- if this pattern continues, these should be collapsed.
                    ModifyFeatureData(properties, [&](FeaturePropertyCache& data)
                    {
                        result.countImmediate = 0;
                        result.queueBackground = (!data.cache.queuedForReporting);
                        data.cache.queuedForReporting = 1;
                        if ((data.cache.opportunityCountRepresentsPotential != 0) != (kind == ServiceReportingKind::PotentialUniqueOpportunity))
                        {
                            if (data.cache.opportunityCount > 0)
                            {
                                result.kindImmediate = (kind == ServiceReportingKind::UniqueOpportunity) ? ServiceReportingKind::PotentialUniqueOpportunity : ServiceReportingKind::UniqueOpportunity;
                                result.countImmediate = data.cache.opportunityCount;
                                data.cache.opportunityCount = 0;
                            }
                            data.cache.opportunityCountRepresentsPotential = (kind == ServiceReportingKind::PotentialUniqueOpportunity) ? 1 : 0;
                        }
                        unsigned int newCount = data.cache.opportunityCount + addend;
                        if ((newCount > c_maxOpportunityCount) || (newCount < data.cache.opportunityCount))
                        {
                            newCount = addend;
                            result.kindImmediate = kind;
                            result.countImmediate = data.cache.opportunityCount;
                        }
                        data.cache.opportunityCount = newCount;
                        return true;
                    });
                }
                break;

            default:
                result.kindImmediate = kind;
                result.countImmediate = addend;
                break;
            };
            return result;
        }

        struct TestFeatureState;
        __declspec(selectany) TestFeatureState* g_testStates = nullptr;
        __declspec(selectany) SRWLOCK g_testLock = SRWLOCK_INIT;

        struct TestFeatureState
        {
            unsigned int featureId;
            FeatureEnabledState state;
            TestFeatureState* next;

            static void __stdcall Reset(TestFeatureState* state)
            {
                auto lock = wil::AcquireSRWLockExclusive(&g_testLock);
                auto stateList = &g_testStates;
                while (*stateList)
                {
                    if (*stateList == state)
                    {
                        *stateList = state->next;
                        state->next = nullptr;
                        return;
                    }
                    stateList = &((*stateList)->next);
                }

                ::HeapFree(::GetProcessHeap(), 0, state);
            }
        };

        inline FEATURE_ENABLED_STATE GetFeatureTestState(UINT32 featureId, FEATURE_CHANGE_TIME)
        {
            if (g_testStates)
            {
                auto lock = wil::AcquireSRWLockShared(&g_testLock);
                auto state = g_testStates;
                while (state)
                {
                    if (state->featureId == featureId)
                    {
                        return static_cast<FEATURE_ENABLED_STATE>(state->state);
                    }
                    state = state->next;
                }
            }
            return FEATURE_ENABLED_STATE_DEFAULT;
        }

        inline FEATURE_ENABLED_STATE WilApi_GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime)
        {
            auto state = GetFeatureTestState(featureId, changeTime);
            if (state != FEATURE_ENABLED_STATE_DEFAULT)
            {
                return state;
            }

#ifdef WIL_ENABLE_STAGING_API
            return GetFeatureEnabledState(featureId, changeTime);
#else
            return WilApiImpl_GetFeatureEnabledState(featureId, changeTime);
#endif
        }

        inline void WilApi_RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, PCSTR originName)
        {
#ifdef WIL_ENABLE_STAGING_API
            RecordFeatureUsage(featureId, kind, addend, originName);
#else
            WilApiImpl_RecordFeatureUsage(featureId, kind, addend, originName);
#endif
        }

        inline void WilApi_RecordFeatureError(UINT32 featureId, const FEATURE_ERROR* error)
        {
#ifdef WIL_ENABLE_STAGING_API
            RecordFeatureError(featureId, error);
#else
            WilApiImpl_RecordFeatureError(featureId, error);
#endif
        }

        inline void WilApi_SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
        {
#ifdef WIL_ENABLE_STAGING_API
            SubscribeFeatureStateChangeNotification(subscription, callback, context);
#else
            WilApiImpl_SubscribeFeatureStateChangeNotification(subscription, callback, context);
#endif
        }

        inline void WilApi_UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
        {
#ifdef WIL_ENABLE_STAGING_API
            UnsubscribeFeatureStateChangeNotification(subscription);
#else
            WilApiImpl_UnsubscribeFeatureStateChangeNotification(subscription);
#endif
        }

        //! Manage the lifetime of a user subscription.
        typedef unique_any<FEATURE_STATE_CHANGE_SUBSCRIPTION, decltype(&::wil::details::WilApi_UnsubscribeFeatureStateChangeNotification), ::wil::details::WilApi_UnsubscribeFeatureStateChangeNotification, details::pointer_access_all> unique_feature_state_change_subscription;
      
        inline void RecordFeatureError(unsigned int featureId, const FailureInfo& info, const DiagnosticsInfo& diagnostics, void* returnAddress)
        {
            __STAGING_TEST_HOOK_ERROR(featureId, info, diagnostics, returnAddress);

            details::StagingFailureInformation failure(info, diagnostics, returnAddress);
            WilApi_RecordFeatureError(featureId, &failure);
        }

        class EnabledStateManager
        {
        public:
            EnabledStateManager() WI_NOEXCEPT
            {
                // This object will be held in a zero-initialized global variable.  We want this line to
                // run at construction time so we can distinguish when someone is trying to call us ahead
                // of construction (CRT init order)
                m_fInitialized = true;
            }

            // note: not called during process shutdown
            ~EnabledStateManager() WI_NOEXCEPT
            {
                m_fInitialized = false;
                m_timer.reset();
                ProcessShutdown();
            }

            // called by virtue of EnabledStateManager being held by shutdown_aware_object
            void ProcessShutdown() WI_NOEXCEPT
            {
                m_fInitialized = false;
                RecordCachedUsageUnderLock();
            }

            void QueueBackgroundUsageReporting(unsigned int id, _Inout_ FeaturePropertyCache& state) WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    m_cachedUsageTrackingData.push_back(CachedUsageData{ id, &state });
                    EnsureTimerUnderLock();
                }
            }

            void SubscribeFeaturePropertyCacheToEnabledStateChanges(_Inout_ FeaturePropertyCache& state) WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    m_cachedFeatureEnabledState.push_back(&state);
                    EnsureSubscribedToStateChangesUnderLock();
                }
            }

            void OnTimer() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    RecordCachedUsageUnderLock();
                    m_timerSet = false;
                }
            }

            void OnStateChange() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    for (auto& data : wil::make_range(m_cachedFeatureEnabledState.data(), m_cachedFeatureEnabledState.size()))
                    {
                        SetEnabledStateProperty(*data, CachedFeatureEnabledState::Unknown);
                    }
                    m_cachedFeatureEnabledState.clear();
                }
            }

        private:
            bool m_fInitialized = false;

            srwlock m_lock;

            wil::unique_threadpool_timer m_timer;
            bool m_timerSet = false;
            unique_feature_state_change_subscription m_subscription;

            struct CachedUsageData
            {
                unsigned int id;
                FeaturePropertyCache* data;
            };

            details_abi::heap_vector<CachedUsageData> m_cachedUsageTrackingData;
            details_abi::heap_vector<FeaturePropertyCache*> m_cachedFeatureEnabledState;

            void RecordCachedUsageUnderLock() WI_NOEXCEPT
            {
                if (m_cachedUsageTrackingData.size())
                {
                    for (auto& feature : wil::make_range(m_cachedUsageTrackingData.data(), m_cachedUsageTrackingData.size()))
                    {
                        struct Change
                        {
                            ServiceReportingKind kind;
                            unsigned int count;
                        };
                        Change changes[8] = {};
                        size_t count = 0;

                        ModifyFeatureData(*feature.data, [&](FeaturePropertyCache& change)
                        {
                            count = 0;
                            changes[count++] = Change{ ServiceReportingKind::DeviceUsage, (!change.cache.recordedDeviceUsage && change.cache.reportedDeviceUsage) ? 1ul : 0ul };
                            changes[count++] = Change{ ServiceReportingKind::PotentialDeviceUsage, (!change.cache.recordedDevicePotential && change.cache.reportedDevicePotential) ? 1ul : 0ul };
                            changes[count++] = Change{ ServiceReportingKind::DeviceOpportunity, (!change.cache.recordedDeviceOpportunity && change.cache.reportedDeviceOpportunity) ? 1ul : 0ul };
                            changes[count++] = Change{ ServiceReportingKind::PotentialDeviceOpportunity, (!change.cache.recordedDevicePotentialOpportunity && change.cache.reportedDevicePotentialOpportunity) ? 1ul : 0ul };
                            changes[count++] = Change{ ServiceReportingKind::UniqueUsage, change.cache.usageCountRepresentsPotential ? 0ul : change.cache.usageCount };
                            changes[count++] = Change{ ServiceReportingKind::PotentialUniqueUsage, change.cache.usageCountRepresentsPotential ? change.cache.usageCount : 0ul };
                            changes[count++] = Change{ ServiceReportingKind::UniqueOpportunity, change.cache.opportunityCountRepresentsPotential ? 0ul : change.cache.opportunityCount };
                            changes[count++] = Change{ ServiceReportingKind::PotentialUniqueOpportunity, change.cache.opportunityCountRepresentsPotential ? change.cache.opportunityCount : 0ul };

                            change.cache.recordedDeviceUsage = change.cache.reportedDeviceUsage;
                            change.cache.recordedDevicePotential = change.cache.reportedDevicePotential;
                            change.cache.recordedDeviceOpportunity = change.cache.reportedDeviceOpportunity;
                            change.cache.recordedDevicePotentialOpportunity = change.cache.reportedDevicePotentialOpportunity;
                            change.cache.usageCount = 0;
                            change.cache.opportunityCount = 0;
                            change.cache.queuedForReporting = 0;
                            return true;
                        });
                        WI_ASSERT(count == ARRAYSIZE(changes));

                        for (auto& change : changes)
                        {
                            if (change.count > 0)
                            {
                                WilApi_RecordFeatureUsage(feature.id, static_cast<UINT32>(change.kind), static_cast<UINT32>(change.count), nullptr);
                            }
                        }
                    }

                    m_cachedUsageTrackingData.clear();

                    WilApi_RecordFeatureUsage(0, static_cast<UINT32>(ServiceReportingKind::Store), 0, nullptr);
                }
            }

            void EnsureTimerUnderLock() WI_NOEXCEPT
            {
                EnsureCoalescedTimer(m_timer, m_timerSet, this);
            }

            void EnsureSubscribedToStateChangesUnderLock() WI_NOEXCEPT
            {
                if (!m_subscription)
                {
                    WilApi_SubscribeFeatureStateChangeNotification(&m_subscription, [](void* context)
                    {
                        reinterpret_cast<EnabledStateManager*>(context)->OnStateChange();
                    }, this);
                }
            }
        };

        __declspec(selectany) ::wil::shutdown_aware_object<EnabledStateManager> g_enabledStateManager;

        inline void SubscribeFeaturePropertyCacheToEnabledStateChanges(_Inout_ FeaturePropertyCache& state) WI_NOEXCEPT
        {
            g_enabledStateManager.get().SubscribeFeaturePropertyCacheToEnabledStateChanges(state);
        }

        inline void ReportUsageToService(unsigned int id, _Inout_ FeaturePropertyCache& state, ServiceReportingKind kind, size_t addend) WI_NOEXCEPT
        {
            RecordUsageResult result = RecordUsageInPropertyCache(state, kind, static_cast<unsigned int>(addend));

            if (result.queueBackground)
            {
                g_enabledStateManager.get().QueueBackgroundUsageReporting(id, state);
            }

            if (result.countImmediate > 0)
            {
                WilApi_RecordFeatureUsage(id, static_cast<UINT32>(result.kindImmediate), static_cast<UINT32>(result.countImmediate), nullptr);
            }
        }

        inline ServiceReportingKind MapReportingKind(ReportingKind kind, bool enabled) WI_NOEXCEPT
        {
            switch (kind)
            {
            case ReportingKind::None:
                return ServiceReportingKind::None;

            case ReportingKind::UniqueUsage:
                return enabled ? ServiceReportingKind::UniqueUsage : ServiceReportingKind::PotentialUniqueUsage;

            case ReportingKind::UniqueOpportunity:
                return enabled ? ServiceReportingKind::UniqueOpportunity : ServiceReportingKind::PotentialUniqueOpportunity;

            case ReportingKind::DeviceUsage:
                return enabled ? ServiceReportingKind::DeviceUsage : ServiceReportingKind::PotentialDeviceUsage;

            case ReportingKind::DeviceOpportunity:
                return enabled ? ServiceReportingKind::DeviceOpportunity : ServiceReportingKind::PotentialDeviceOpportunity;

            case ReportingKind::TotalDuration:
                return enabled ? ServiceReportingKind::EnabledTotalDuration : ServiceReportingKind::DisabledTotalDuration;

            case ReportingKind::PausedDuration:
                return enabled ? ServiceReportingKind::EnabledPausedDuration : ServiceReportingKind::DisabledPausedDuration;

            default:    // product of GetCustomReportingKind
                if (WI_VERIFY((static_cast<unsigned char>(kind) >= 100) && (static_cast<unsigned char>(kind) < 150)))
                {
                    unsigned char index = (static_cast<unsigned char>(kind) - 100);
                    return enabled ? static_cast<ServiceReportingKind>(static_cast<unsigned char>(ServiceReportingKind::CustomEnabledBase) + index) :
                        static_cast<ServiceReportingKind>(static_cast<unsigned char>(ServiceReportingKind::CustomDisabledBase) + index);
                }
            }

            return ServiceReportingKind::None;
        }

        inline void ReportUsageToService(unsigned int id, _Inout_ FeaturePropertyCache& state, bool enabled, ReportingKind kind, size_t addend = 1) WI_NOEXCEPT
        {
            auto serviceKind = MapReportingKind(kind, enabled);
            if (serviceKind != ServiceReportingKind::None)
            {
                ReportUsageToService(id, state, serviceKind, addend);
            }
        }

        inline void ReportFeatureCaughtException(ThreadErrorContext& context, unsigned int featureId, const DiagnosticsInfo& diagnostics, void* returnAddress) WI_NOEXCEPT
        {
            FailureInfo info = {};
            if (context.GetCaughtExceptionError(info, &diagnostics))
            {
                RecordFeatureError(featureId, info, diagnostics, returnAddress);
            }
        }

        class FeatureFunctorHost : public IFunctorHost
        {
        public:
            FeatureFunctorHost(unsigned int featureId, const DiagnosticsInfo& diagnostics) : m_featureId(featureId), m_diagnostics(diagnostics) {}

            HRESULT Run(IFunctor& functor) override
            {
                return functor.Run();
            }

            HRESULT ExceptionThrown(void* returnAddress) override
            {
                ReportFeatureCaughtException(m_context, m_featureId, m_diagnostics, returnAddress);
                RethrowCaughtException();
                FAIL_FAST_IMMEDIATE();
                return E_UNEXPECTED;
            }

        private:
            ThreadErrorContext m_context;
            const DiagnosticsInfo& m_diagnostics;
            unsigned int m_featureId;
        };

        __declspec(noinline) inline void ReportFeatureError(HRESULT hr, ThreadErrorContext& context, unsigned int featureId, const DiagnosticsInfo& diagnostics) WI_NOEXCEPT
        {
            FailureInfo info;

            if (!context.GetLastError(info, hr))
            {
                // WIL has never seen this failure, so it's being introduced here.  Let the GetLastError mechanism in WIL know about it
                // so that we're able to offer it up as the source if we have other wrapped contexts.

                ::ZeroMemory(&info, sizeof(info));
                info.uLineNumber = diagnostics.line;
                info.pszFile = diagnostics.file;
                info.returnAddress = diagnostics.returnAddress;
                info.hr = hr;

                SetLastError(info);
                context.GetLastError(info, hr);
            }

            RecordFeatureError(featureId, info, diagnostics, _ReturnAddress());
        }

        __declspec(noinline) inline void ReportFeatureCaughtException(wil::ThreadErrorContext& context, unsigned int featureId, const DiagnosticsInfo& diagnostics) WI_NOEXCEPT
        {
            ReportFeatureCaughtException(context, featureId, diagnostics, _ReturnAddress());
        }

        // WARNING: may throw an exception (even with an HRESULT return)...
        template <typename TFunctor>
        __forceinline HRESULT RunFeatureDispatch(unsigned int featureId, const DiagnosticsInfo& diagnostics, TFunctor&& functor, tag_return_HRESULT)
        {
            ThreadErrorContext context;
            HRESULT hr = functor();
            if (FAILED(hr))
            {
                ReportFeatureError(hr, context, featureId, diagnostics);
            }
            return hr;
        }

        template <typename TFunctor>
        __forceinline void RunFeatureDispatch(unsigned int featureId, const DiagnosticsInfo& diagnostics, TFunctor&& functor, tag_return_void)
        {
#if (WIL_EXCEPTION_MODE == 0)       // switchable
            functor_wrapper_void<TFunctor> wrapper(functor);
            FeatureFunctorHost host(featureId, diagnostics);
            RunFunctor(wrapper, host);
#elif (WIL_EXCEPTION_MODE == 1)     // exception-based only
            ThreadErrorContext context;
            try
            {
                functor();
            }
            catch (...)
            {
                ReportFeatureCaughtException(context, featureId, diagnostics);
                throw;
            }
#else                               // error-code only
            featureId;diagnostics;functor;
            functor();
#endif
        }

        template <typename TFunctor>
        inline auto RunFeatureDispatch(unsigned int featureId, const DiagnosticsInfo& diagnostics, TFunctor&& functor, tag_return_other) -> decltype(functor())
        {
#if (WIL_EXCEPTION_MODE == 0)       // switchable
            decltype(functor()) result;
            functor_wrapper_other<TFunctor, decltype(functor())> wrapper(functor, result);
            FeatureFunctorHost host(featureId, diagnostics);
            RunFunctor(wrapper, host);
            return result;
#elif (WIL_EXCEPTION_MODE == 1)     // exception-based only
            ThreadErrorContext context;
            try
            {
                return functor();
            }
            catch (...)
            {
                ReportFeatureCaughtException(context, featureId, diagnostics);
                throw;
            }
#else                               // error-code only
            featureId; diagnostics; functor;
            return functor();
#endif
        }

        //! Holds information necessary to report feature durations; use FeatureUsageDuration, rather than using directly.
        struct FeatureUsageDurationData
        {
            unsigned int featureId;
            bool enabled;
            ReportingKind kind;
            LARGE_INTEGER counterStart;

            FeatureUsageDurationData(unsigned int featureId_, bool enabled_, ReportingKind kind_) : featureId(featureId_), enabled(enabled_), kind(kind_)
            {
                QueryPerformanceCounter(&counterStart);
            }

            static void __stdcall Stop(FeatureUsageDurationData* data) WI_NOEXCEPT
            {
                FeatureUsageDurationData& duration = *data;

                if (duration.counterStart.QuadPart != 0)
                {
                    LARGE_INTEGER counterEnd;
                    QueryPerformanceCounter(&counterEnd);
                    WI_ASSERT(duration.counterStart.QuadPart <= counterEnd.QuadPart);
                    counterEnd.QuadPart -= duration.counterStart.QuadPart;

                    LARGE_INTEGER frequency;
                    QueryPerformanceFrequency(&frequency);

                    auto durationInMs = (counterEnd.QuadPart / (frequency.QuadPart / 1000));
                    WilApi_RecordFeatureUsage(duration.featureId, static_cast<UINT32>(MapReportingKind(duration.kind, duration.enabled)), static_cast<UINT32>(durationInMs), nullptr);

                    duration.counterStart.QuadPart = 0;
                }
            }
        };
    }
    /// @endcond

    typedef unique_any<details::TestFeatureState*, decltype(&details::TestFeatureState::Reset), &details::TestFeatureState::Reset> unique_feature_test_state;

    //! Adjust a single persistent staging control by marking the feature as enabled, disabled, or default.
    inline unique_feature_test_state TestFeatureEnabledState(unsigned int featureId, FeatureEnabledState state = FeatureEnabledState::Enabled)
    {
        unique_feature_test_state value(static_cast<details::TestFeatureState*>(::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(details::TestFeatureState))));
        if (value)
        {
            {
                auto lock = wil::AcquireSRWLockExclusive(&details::g_testLock);
                value.get()->next = details::g_testStates;
                value.get()->state = state;
                value.get()->featureId = featureId;

                details::g_testStates = value.get();
            }
            details::g_enabledStateManager.get().OnStateChange();
        }
        return value;
    }

    /** Provides the RAII class returned from a feature's ReportUsageDuration; automatically reports duration on destruction.
    You can report the duration explicitly ahead of destruction with the reset() method and abort reporting with the 
    release() method. */
    using FeatureUsageDuration = unique_struct<details::FeatureUsageDurationData, decltype(&::wil::details::FeatureUsageDurationData::Stop), ::wil::details::FeatureUsageDurationData::Stop>;

    //! A feature for use with WIL staging; features control whether or not a piece of code is available for use.
    template <typename traits>
    class Feature
    {
        //! Resulting stage after taking into account overrides
        static const FeatureStage activeStage = ((traits::stageChkOverride != details::UnknownFeatureStage) ? traits::stageChkOverride : ((traits::stageOverride != details::UnknownFeatureStage) ? traits::stageOverride : traits::stage));

    public:
        //! Globally unique integer based id for this feature.
        static const unsigned int id = traits::id;

        //! True if this feature is always disabled; this is based on the stage and whether or not there are any branch overrides.
        static const bool isAlwaysDisabled = (activeStage == FeatureStage::AlwaysDisabled);

        //! True if this feature is always enabled (it cannot be turned off)
        static const bool isAlwaysEnabled = (activeStage == FeatureStage::AlwaysEnabled);

        //! True if this feature is enabled by default (either always or conditionally based on stage or branch overrides)
        static const bool isEnabledByDefault = (isAlwaysEnabled || (activeStage == FeatureStage::EnabledByDefault));

        //! Retrieves the given string for use in tooling scenarios.
        static PCSTR GetFeatureString(FeatureString kind)
        {
            switch (kind)
            {
            case FeatureString::Name:              return traits::GetName();
            case FeatureString::Description:       return traits::GetDescription();
            case FeatureString::GroupName:         return traits::GetGroupName();
            case FeatureString::GroupDescription:  return traits::GetGroupDescription();
            case FeatureString::EMail:             return traits::GetEMail();
            case FeatureString::Link:              return traits::GetLink();
            };
            return nullptr;
        }

        //! Retrieves basic properties about the feature for use in configuration scenarios.
        static FeatureProperties GetFeatureProperties(FeaturePropertyGroupFlags featurePropertyGroups)
        {
            FeatureProperties featureProperties = {};

            if (WI_IS_FLAG_SET(featurePropertyGroups, FeaturePropertyGroupFlags::StaticProperties))
            {
                featureProperties.changeTime = traits::changeTime;
                featureProperties.stage = traits::stage;
                featureProperties.isEnabledByDefault = isEnabledByDefault;
                featureProperties.version = traits::version;
                featureProperties.baseVersion = traits::baseVersion;               
            }

            if (WI_IS_FLAG_SET(featurePropertyGroups, FeaturePropertyGroupFlags::FeatureEnabledState))
            {
                featureProperties.isEnabled = WI_IsFeatureEnabled_SuppressUsage(FeatureType);
            }

            return featureProperties;
        }

#pragma warning(push)
#pragma warning(error:4714)
        /** This is the primary primitive to determine whether a feature is enabled.
        If the feature is in the AlwaysDisabled stage, this will evaluate at compile-time to 'false' and the forceinline
        will guarantee that the optimizer just sees a static 'false' evaluation of any conditional that includes this
        primitive.  The same (opposite bool) is true for the AlwaysEnabled stage.  When in the DisabledByDefault or
        EnabledByDefault stages, this results in a runtime enabled configuration check.

        This routine also automatically records implicit usage and potential usage statistics for the device.  Use
        the overload accepting ReportingKind to suppress that usage or report unique usage, rather than device usage.

        This routine ensures that warning 4714 (failure to honor __forceinline) is an error.  If you get this error, use 
        WI_IsFeatureEnabled(Feature) rather than Feature::IsEnabled(). */
        __forceinline static bool IsEnabled()
        {
            return WI_IsFeatureEnabled(FeatureType);
        }

        /** This primitive determines whether a feature is enabled while providing control of the kind of usage that is reported.
        The default reporting of a normal IsEnabled() check is ReportingKind::ReportDeviceUsage.  This routine allows you
        to specify the kind of usage reporting that you want done in response to this call being made.  By specifing an alternate
        ReportingKind you can suppress usage information (with ReportingKind::None) or report on the unique uses of the feature
        (with ReportingKind::UniqueUsage) rather than device usage.

        This routine ensures that warning 4714 (failure to honor __forceinline) is an error.  If you get this error, use
        WI_IsFeatureEnabled_ReportUsage(Feature) rather than Feature::IsEnabled().

        See IsEnabled for more complete details. */
        __forceinline static bool IsEnabled(ReportingKind kind)
        {
            return WI_IsFeatureEnabled_ReportUsage(FeatureType, kind);
        }

        /** This primitive will run the given functor if the feature is enabled while capturing usage and error (exception) information and associating it with your feature.
        This routine give you three core capabilities rolled all into one primitive:
        
        1)  Enablement Checking:  This primitive will only run the given lambda if the feature is enabled.  If the feature isn't enabled, the 
            lambda won't be run and if the feature can't be enabled (AlwaysDisabled) the code in the lambda will be dropped from optimized builds.

        2)  Usage Reporting:  Like the IsEnabled primitive this routine will also calculate potential and actual device usage (based upon whether the feature
            is enabled).  It also has an overload accepting ReportingKind that allows you to specify that you want to track UniqueUsage, rather than
            device usage or to avoid counting this primitive towards usage at all.

        3)  Error Reporting:  This primitive also encapsualtes a lightweight model for capturing any error or exception information that happens
            while executing the lambda, aggregating that information, and attributing that to your feature.  With this information, you can know
            through your feature reporting dashboard that a specific error was hit and on what line of code that error originated and through which
            RunIfEnabled call the error originated.

        To use the routine, the first parameter should always be WI_DIAGNOSTICS_INFO as it provides the needed context (file, line and address information)
        to the error reporting when an error occurs.  The second parameter is an optional ReportingKind parameter to allow specifying the exact type of
        usage to be tracked, and the final parameter is the lambda.

        The lambda must not accept any parameters and should use [&] as the capture mechanism.  The return value of the lambda is reflected as the return
        value of the RunIfEnabled call itself.  So if your lambda returns a ComPtr<IFoo> the RunIfEnabled call returns a ComPtr<IFoo>.  In the event your
        lambda is not run, the routine will return a default constructed ComPtr<IFoo>().

        When the lambda returns an HRESULT, the return value is inspected and failures are reported based upon that HRESULT value.  For any other return
        value a try/catch is used internally to similarly report thrown failures.  Note that this routine will not block exceptions, it only observes
        them and rethrows the original exception.

        If the lambda returns a long value, that value is presumed to be an HRESULT (since unfortunately they are type equivalent).  To return an actual 
        `long` or any other typedef which maps to a `long`, use `RunIfEnabled<wil::ErrorReturn::None>(...)` so that the long isn't interpreted as an HRESULT.

        Examples:
        ~~~
        Feature_MyFeature::RunIfEnabled(WI_DIAGNOSTICS_INFO, [&]
        {
            // Your feature code
        });

        HRESULT hr = Feature_MyFeature::RunIfEnabled(WI_DIAGNOSTICS_INFO, [&]() -> HRESULT
        {
            // Your feature code
            RETURN_IF_FAILED(...);
            return S_OK;
        });

        wil::com_ptr<IFoo> foo = Feature_MyFeature::RunIfEnabled(WI_DIAGNOSTICS_INFO, [&]() -> wil::com_ptr<IFoo>
        {
            return wil::com_query<IFoo>(...);
        });
        ~~~ */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto RunIfEnabled(const DiagnosticsInfo& diagnostics, TFunctor&& functor) -> decltype(functor())
        {
            if (WI_IsFeatureEnabled(FeatureType))
            {
                return details::RunFeatureDispatch(id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
            }
            return decltype(functor())();
        }

        /** This primitive will run the given functor if the feature is enabled capturing error (exception) statistics without recording implicit usage.
        See RunIfEnabled for more details. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto RunIfEnabled(const DiagnosticsInfo& diagnostics, ReportingKind kind, TFunctor&& functor) -> decltype(functor())
        {
            if (WI_IsFeatureEnabled_ReportUsage(FeatureType, kind))
            {
                return details::RunFeatureDispatch(id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
            }
            return decltype(functor())();
        }

        /// @cond
        // Do not use - will be removed
        __forceinline static bool IsEnabled_SuppressUsage()
        {
            return WI_IsFeatureEnabled_SuppressUsage(FeatureType);
        }

        // Do not use - will be removed
        __forceinline static bool IsEnabledWithDefault(bool isEnabledByDefaultOverride)
        {
            return WI_IsFeatureEnabledWithDefault(FeatureType, isEnabledByDefaultOverride);
        }
        /// @endcond
#pragma warning(pop)

        /** Call this method at the top of feature-specific methods to avoid accidental dependencies on features. */
        static void AssertEnabled()
        {
            WI_ASSERT(WI_IsFeatureEnabled_SuppressUsage(FeatureType));
        }

        /** Returns true if the given staging feature is already enabled, or if it should be enabled, but is currently blocked
        from being enabled by a disabled dependent feature.  NOTE: this does not cover scnearios where the enabled state might 
        be pending (such as if the feature has been enabled after the next reboot); these will still return false as they shouldn't
        currently be enabled. */
        static bool ShouldBeEnabled()
        {
            if (isAlwaysEnabled)
            {
                return true;
            }
            if (isAlwaysDisabled)
            {
                return false;
            }
            auto state = GetCachedFeatureEnabledState();
            return ((state == details::CachedFeatureEnabledState::Enabled) || (state == details::CachedFeatureEnabledState::Desired));
        }

        /** Use this method to explicitly record telemetry regarding a feature's usage, such as an explicit usage signal or usage duration.
        Normally, most "IsEnabled" or "RunIfEnabled" primitives provide an override that allows you to also pass
        the ReportKind into the primitive to do this reporting as an artifact of enablement checking.  That method is
        preferred to this method when it can be used as it handles both Actual and Potential usage, but if you're not
        simultaneously controlling enablement with this primitives, this method allows you to directly report usage.

        This method also enables callers to explicitly provide additional information associated and reporting on behalf of
        a given feature.  Examples:
        ~~~
        // Report a single unique usage of the feature:
        Feature_MyFeature::ReportUsage();

        // Report a single unique offering of the feature to the user (an offered entry point such as a menu) independent
        // from actual use of the feature:
        Feature_MyFeature::ReportUsage(ReportingKind::UniqueOpportunity);
        ~~~
        You can also explicitly provide feature usage duration (though this is better done through ReportUsageDuration)
        as well as custom data points.  These data points can be usage counts, outcome counts, items per operation or
        any other count that makes sense to view in aggregate relation to usage.  */
        static void ReportUsage(ReportingKind kind = ReportingKind::UniqueUsage, size_t addend = 1)
        {
            ReportUsageToService(WI_IsFeatureEnabled_SuppressUsage(FeatureType), kind, addend);
        }

        /** Use this method to record telemetry regarding a feature's usage duration.
        This method returns an RAII class that will automatically record the duration between the call to this method
        and the destruction of the returned object and report that duration via telemetry.
        ~~~
        auto usage = Feature_MyFeature::ReportUsageDuration();
        // your feature code goes here
        // telemetry will be reported when 'usage' goes out of scope
        ~~~
        If the total duration of your feature involves some user prompting then you should capture that time via
        UsageKind::PausedDuration and keep the TotalDuration inclusive of this time.  Note that you can also use
        this method to record duration for custom data points as well. */
        static FeatureUsageDuration ReportUsageDuration(ReportingKind kind = ReportingKind::TotalDuration)
        {
            WI_ASSERT((kind != ReportingKind::None) && (kind != ReportingKind::UniqueUsage) && (kind != ReportingKind::UniqueOpportunity) &&
                (kind != ReportingKind::DeviceUsage) && (kind != ReportingKind::DeviceOpportunity));
            return FeatureUsageDuration(details::FeatureUsageDurationData(id, WI_IsFeatureEnabled_SuppressUsage(FeatureType), kind));
        }

        /// @cond
        // Depracated: Will be removed after RI/FI cycle; use ReportingKind, not FeatureUsageKind
        static void ReportUsage(FeatureUsageKind kind, size_t addend = 1)
        {
            switch (kind)
            {
            case FeatureUsageKind::ImplicitOpportunityCount:
                ReportUsage(ReportingKind::DeviceOpportunity, addend);
                break;
            case FeatureUsageKind::ImplicitUsageCount:
                ReportUsage(ReportingKind::DeviceUsage, addend);
                break;
            case FeatureUsageKind::OpportunityCount:
                ReportUsage(ReportingKind::UniqueOpportunity, addend);
                break;
            case FeatureUsageKind::UsageCount:
                ReportUsage(ReportingKind::UniqueUsage, addend);
                break;
            case FeatureUsageKind::TotalDuration:
                ReportUsage(ReportingKind::TotalDuration, addend);
                break;
            case FeatureUsageKind::PausedDuration:
                ReportUsage(ReportingKind::PausedDuration, addend);
                break;
            case FeatureUsageKind::Custom1:
            case FeatureUsageKind::Custom2:
            case FeatureUsageKind::Custom3:
            case FeatureUsageKind::Custom4:
            case FeatureUsageKind::Custom5:
                ReportUsage(GetCustomReportingKind(static_cast<unsigned char>(kind) - static_cast<unsigned char>(FeatureUsageKind::Custom1)), addend);
                break;
            }
        }

        // Depracated: Will be removed after RI/FI cycle; use ReportingKind, not FeatureUsageKind
        static FeatureUsageDuration ReportUsageDuration(FeatureUsageKind kind)
        {
            return ReportUsageDuration((kind == FeatureUsageKind::TotalDuration) ? ReportingKind::TotalDuration : ReportingKind::PausedDuration);
        }
        /// @endcond

        /** This primitive will run the given functor while automatically capturing error (exception) statistics and associating them with the feature.
        See RunIfEnabled for more information.  This routine behaves identically with the exception that there is
        no enablement checking and no usage information reported.  The feature should already be enabled when this
        routine is used. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto Run(const DiagnosticsInfo& diagnostics, TFunctor&& functor) -> decltype(functor())
        {
            AssertEnabled();
            return details::RunFeatureDispatch(id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
        }

        /** This primitive will run the given functor while automatically capturing error (exception) statistics and associating them with the feature and reporting usage.
        See RunIfEnabled for more information.  This routine behaves identically with the exception that there is
        no enablement checking.  The feature should already be enabled when this routine is used. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto Run(const DiagnosticsInfo& diagnostics, ReportingKind kind, TFunctor&& functor) -> decltype(functor())
        {
            AssertEnabled();
            ReportUsageToService(true, kind);
            return details::RunFeatureDispatch(id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
        }

        /// @cond
        // Supports WI_IsFeatureEnabled; do not use directly
        inline static void __private_IsEnabledPreCheck()
        {
            #pragma warning(suppress:4127)  // conditional expression is constant
            if (isAlwaysEnabled || isAlwaysDisabled)
            {
                ReportUsageToService(isAlwaysEnabled, ReportingKind::DeviceUsage);
            }
        }

        // Supports WI_IsFeatureEnabled; do not use directly
        inline static void __private_IsEnabledPreCheck(ReportingKind kind)
        {
            #pragma warning(suppress:4127)  // conditional expression is constant
            if (isAlwaysEnabled || isAlwaysDisabled)
            {
                WI_ASSERT((kind != ReportingKind::TotalDuration) && (kind != ReportingKind::PausedDuration));
                ReportUsageToService(isAlwaysEnabled, kind);
            }
        }

        // Supports WI_IsFeatureEnabled; do not use directly
        __declspec(noinline) static bool __private_IsEnabled()
        {
            const auto enabled = (GetCachedFeatureEnabledState() == details::CachedFeatureEnabledState::Enabled);
            ReportUsageToService(enabled, ReportingKind::DeviceUsage);
            return enabled;
        }

        // Supports WI_IsFeatureEnabled; do not use directly
        __declspec(noinline) static bool __private_IsEnabled(ReportingKind kind)
        {
            const auto enabled = (GetCachedFeatureEnabledState() == details::CachedFeatureEnabledState::Enabled);
            ReportUsageToService(enabled, kind);
            return enabled;
        }

        // Supports testing; do not use directly (will 'forget' about usage)
        static void __private_ClearCache()
        {
            auto& cache = GetFeaturePropertyCache();
            cache.var = 0;
        }

        // TODO: Remove with WI_IsEnabledWithDefault
        __declspec(noinline) static bool __private_IsEnabledWithDefault(bool isEnabledByDefaultOverride)
        {
            return (GetCachedFeatureEnabledState(isEnabledByDefaultOverride) == details::CachedFeatureEnabledState::Enabled);
        }
        /// @endcond

    private:
        typedef Feature<traits> FeatureType;

        static bool ShouldBeEnabledForDependentFeature()
        {
            return ((traits::ShouldBeEnabledForDependentFeature_DefaultDisabled() || traits::ShouldBeEnabledForDependentFeature_DefaultEnabled()));
        }

        static details::CachedFeatureEnabledState GetCurrentFeatureEnabledState(bool isEnabledByDefaultOverride)
        {
            bool shouldBeEnabled = false;

            FEATURE_ENABLED_STATE state = ::wil::details::WilApi_GetFeatureEnabledState(id, static_cast<FEATURE_CHANGE_TIME>(traits::changeTime));
            if (state != FEATURE_ENABLED_STATE_DEFAULT)
            {
                // Any explicitly configured feature controls the state (configuration wins)
                shouldBeEnabled = (state == FEATURE_ENABLED_STATE_ENABLED);
            }
            else
            {
                // We're not configured: Are there features enabled that require our feature to first be enabled?
                shouldBeEnabled = (isEnabledByDefaultOverride || ShouldBeEnabledForDependentFeature());
            }

            if (shouldBeEnabled)
            {
                return (traits::AreDependenciesEnabled() ? details::CachedFeatureEnabledState::Enabled : details::CachedFeatureEnabledState::Desired);
            }

            return details::CachedFeatureEnabledState::Disabled;
        }
        
        inline static details::FeaturePropertyCache& GetFeaturePropertyCache()
        {
            static details::FeaturePropertyCache data = {};
            return data;
        }

        static details::CachedFeatureEnabledState GetCachedFeatureEnabledState(bool isEnabledByDefaultOverride = isEnabledByDefault)
        {
            auto& cache = GetFeaturePropertyCache();
            details::CachedFeatureEnabledState cacheState = static_cast<details::CachedFeatureEnabledState>(cache.cache.enabledState);
            if (cacheState == details::CachedFeatureEnabledState::Unknown)
            {
                cacheState = GetCurrentFeatureEnabledState(isEnabledByDefaultOverride);
                SetEnabledStateProperty(cache, cacheState);

                #pragma warning(suppress:4127)  // conditional expression is constant
                if (traits::changeTime == FeatureChangeTime::OnRead)
                {
                    // When willing to change states at any point, we need to setup a subscription to invalidate our feature state in response
                    // to a configuration change.
                    SubscribeFeaturePropertyCacheToEnabledStateChanges(cache);
                }
            }
            return cacheState;
        }

        static void ReportUsageToService(bool enabled, ReportingKind kind, size_t addend = 1)
        {
            details::ReportUsageToService(id, GetFeaturePropertyCache(), enabled, kind, addend);
        }
    };

    /// @cond
    namespace details
    {
        static const FeatureStage UnknownFeatureStage = static_cast<FeatureStage>(-1);

        // Provides the defaults for optional properties that can be specified from the XML configuration
        struct FeatureTraitsBase
        {
            static const unsigned short version = 0;
            static const unsigned short baseVersion = 0;
            static const FeatureStage stageOverride = UnknownFeatureStage;
            static const FeatureStage stageChkOverride = UnknownFeatureStage;
            static const FeatureChangeTime changeTime = FeatureChangeTime::OnRead;
            static bool ShouldBeEnabledForDependentFeature_DefaultDisabled()    { return false; }
            static bool ShouldBeEnabledForDependentFeature_DefaultEnabled()     { return false;  }
            static bool AreDependenciesEnabled()    { return true; }
            static PCSTR GetEMail()                 { return nullptr; }
            static PCSTR GetLink()                  { return nullptr; }
        };

        __declspec(noinline) inline bool ShouldBeEnabledFromConfigAndDefault(unsigned int featureId, bool isEnabledByDefault)
        {
            FEATURE_ENABLED_STATE state = WilApi_GetFeatureEnabledState(featureId, FEATURE_CHANGE_TIME_READ);
            if (state != FEATURE_ENABLED_STATE_DEFAULT)
            {
                // Any explicitly configured feature controls the state (configuration wins)
                return (state == FEATURE_ENABLED_STATE_ENABLED);
            }
            return isEnabledByDefault;
        }

        // Used to provide the recursive evaluation of whether or not a feature should be enabled because there is a child feature that depends upon it that is already enabled.
        template <unsigned int... Args>
        struct DependentFeatures;

        template <unsigned int featureId>
        struct DependentFeatures<featureId>
        {
            static bool ShouldBeEnabledForDependentFeature(bool isEnabledByDefault) { return ShouldBeEnabledFromConfigAndDefault(featureId, isEnabledByDefault); }
        };

        template <unsigned int featureId, unsigned int... Args>
        struct DependentFeatures<featureId, Args...>
        {
            static bool ShouldBeEnabledForDependentFeature(bool isEnabledByDefault) { return (ShouldBeEnabledFromConfigAndDefault(featureId, isEnabledByDefault) || DependentFeatures<Args...>::ShouldBeEnabledForDependentFeature(isEnabledByDefault)); }
        };

        // Used to provide the recursive evaluation of whether or not required features are enabled so that a parent can be enabled.
        template <typename... Args>
        struct RequiredFeatures;

        template <typename TFeature>
        struct RequiredFeatures<TFeature>
        {
            static bool IsEnabled() { return WI_IsFeatureEnabled_SuppressUsage(TFeature); }
        };
        
        template <typename TFeature, typename... Args>
        struct RequiredFeatures<TFeature, Args...>
        {
            static bool IsEnabled() { return (WI_IsFeatureEnabled_SuppressUsage(TFeature) || RequiredFeatures<Args...>::IsEnabled()); }
        };
    }
    /// @endcond

} // wil

#pragma warning(pop)

#endif // __WIL_STAGING_INCLUDED
