#pragma once

#include <new>
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <PathCch.h>
#endif
#include <combaseapi.h> // Needed for CoTaskMemFree() used in output of some helpers.
#include "Result.h"
#include "Resource.h"

namespace wil
{
    //! Strictly a function of the file system but this is the value for all known file system, NTFS, FAT.
    //! CDFs has a limit of 254.
    size_t const maxPathSegmentLength = 255;

    //! Character length not including the null, MAX_PATH (260) includes the null.
    size_t const maxPathLength = 259;

    //! 32743 Character length not including the null. This is a system defined limit.
    //! The 24 is for the expansion of the roots from "C:" to "\Device\HarddiskVolume4"
    //! It will be 25 when there are more than 9 disks.
    size_t const maxExtendedPathLength = 0x7FFF - 24;

    //! For {guid} string form. Includes space for the null terminator.
    size_t const guidStringBufferLength = 39;

    //! For {guid} string form. Not including the null terminator.
    size_t const guidStringLength = 38;

    //! Determines if a path is an extended length path that can be used to access paths longer than MAX_PATH.
    inline bool IsExtendedLengthPath(_In_ PCWSTR path)
    {
        return wcsncmp(path, L"\\\\?\\", 4) == 0;
    }

    //! Find the last segment of a path. Matches the behavior of shlwapi!PathFindFileNameW()
    //! note, does not support streams being specified like PathFindFileNameW(), is that a bug or a feature?
    inline PCWSTR FindLastPathSegment(_In_ PCWSTR path)
    {
        auto const pathLength = wcslen(path);
        // If there is a trailing slash ignore that in the search.
        auto const limitedLength = ((pathLength > 0) && (path[pathLength - 1] == L'\\')) ? (pathLength - 1) : pathLength;

        PCWSTR result;
        auto const offset = FindStringOrdinal(FIND_FROMEND, path, static_cast<int>(limitedLength), L"\\", 1, TRUE);
        if (offset == -1)
        {
            result = path + pathLength; // null terminator
        }
        else
        {
            result = path + offset + 1; // just past the slash
        }
        return result;
    }

    //! Determine if the file name is one of the special "." or ".." names.
    inline bool PathIsDotOrDotDot(_In_ PCWSTR fileName)
    {
        return ((fileName[0] == L'.') &&
               ((fileName[1] == L'\0') || ((fileName[1] == L'.') && (fileName[2] == L'\0'))));
    }

    //! Returns the drive number, if it has one. Returns true if there is a drive number, false otherwise. Supports regular and extended length paths.
    inline bool TryGetDriveLetterNumber(_In_ PCWSTR path, _Out_ int* driveNumber)
    {
        if (path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
        {
            path += 4;
        }
        if (path[0] && (path[1] == L':'))
        {
            if ((path[0] >= L'a') && (path[0] <= L'z'))
            {
                *driveNumber = path[0] - L'a';
                return true;
            }
            else if ((path[0] >= L'A') && (path[0] <= L'Z'))
            {
                *driveNumber = path[0] - L'A';
                return true;
            }
        }
        *driveNumber = -1;
        return false;
    }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

    // PathCch.h APIs are only in desktop API for now.

    // Compute the substring in the input value that is the parent folder path.
    // returns:
    //      true + parentPathLength - path has a parent starting at the beginning path and of parentPathLength length.
    //      false, no parent path, the input is a root path.
    inline bool TryGetParentPathRange(_In_ PCWSTR path, _Out_ size_t* parentPathLength)
    {
        *parentPathLength = 0;
        bool hasParent = false;
        PCWSTR rootEnd;
        if (SUCCEEDED(PathCchSkipRoot(path, &rootEnd)) && (*rootEnd != L'\0'))
        {
            auto const lastSegment = FindLastPathSegment(path);
            *parentPathLength = lastSegment - path;
            hasParent = (*parentPathLength != 0);
        }
        return hasParent;
    }

    // Creates directories for the specified path, creating parent paths
    // as needed.
    inline HRESULT CreateDirectoryDeepNoThrow(PCWSTR path) WI_NOEXCEPT
    {
        if (::CreateDirectoryW(path, nullptr) == FALSE)
        {
            DWORD const lastError = ::GetLastError();
            if (lastError == ERROR_PATH_NOT_FOUND)
            {
                size_t parentLength;
                if (TryGetParentPathRange(path, &parentLength))
                {
                    wistd::unique_ptr<wchar_t[]> parent(new (std::nothrow) wchar_t[parentLength + 1]);
                    RETURN_IF_NULL_ALLOC(parent.get());
                    RETURN_IF_FAILED(StringCchCopyNW(parent.get(), parentLength + 1, path, parentLength));
                    CreateDirectoryDeepNoThrow(parent.get()); // recurs
                }
                RETURN_IF_WIN32_BOOL_FALSE(::CreateDirectoryW(path, nullptr));
            }
            else if (lastError != ERROR_ALREADY_EXISTS)
            {
                RETURN_WIN32(lastError);
            }
        }
        return S_OK;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    inline void CreateDirectoryDeep(PCWSTR path)
    {
        THROW_IF_FAILED(CreateDirectoryDeepNoThrow(path));
    }
#endif // WIL_ENABLE_EXCEPTIONS

    enum class RemoveDirectoryOptions
    {
        None = 0, 
        KeepRootDirectory = 0x1
    };
    DEFINE_ENUM_FLAG_OPERATORS(RemoveDirectoryOptions);

    inline HRESULT RemoveDirectoryRecursiveNoThrow(_In_ PCWSTR path, RemoveDirectoryOptions options = RemoveDirectoryOptions::None) WI_NOEXCEPT
    {
        wil::unique_hlocal_string searchPath;
        RETURN_IF_FAILED(::PathAllocCombine(path, L"*", PATHCCH_ALLOW_LONG_PATHS, &searchPath));

        WIN32_FIND_DATAW fd;
        wil::unique_hfind findHandle(::FindFirstFileW(searchPath.get(), &fd));
        RETURN_LAST_ERROR_IF_FALSE(findHandle);

        for (;;)
        {
            // skip "." and ".."
            if (!(WI_IS_FLAG_SET(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) && PathIsDotOrDotDot(fd.cFileName)))
            {
                wil::unique_hlocal_string pathToDelete;
                RETURN_IF_FAILED(::PathAllocCombine(path, fd.cFileName, PATHCCH_ALLOW_LONG_PATHS, &pathToDelete));
                // note: if pszDelete is read-only, we just fail
                if (WI_IS_FLAG_SET(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
                {
                    RemoveDirectoryOptions localOptions = options;
                    RETURN_IF_FAILED(RemoveDirectoryRecursiveNoThrow(pathToDelete.get(), WI_CLEAR_FLAG(localOptions, RemoveDirectoryOptions::KeepRootDirectory)));
                }
                else
                {
                    RETURN_LAST_ERROR_IF_FALSE(::DeleteFileW(pathToDelete.get()));
                }
            }

            if (!::FindNextFileW(findHandle.get(), &fd))
            {
                auto const err = ::GetLastError();
                if (err == ERROR_NO_MORE_FILES)
                {
                    break;
                }
                RETURN_WIN32(err);
            }
        }

        if (WI_IS_FLAG_CLEAR(options, RemoveDirectoryOptions::KeepRootDirectory))
        {
            RETURN_IF_WIN32_BOOL_FALSE(::RemoveDirectoryW(path));
        }
        return S_OK;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    inline void RemoveDirectoryRecursive(_In_ PCWSTR path, RemoveDirectoryOptions options = RemoveDirectoryOptions::None)
    {
        THROW_IF_FAILED(RemoveDirectoryRecursiveNoThrow(path, options));
    }
#endif // WIL_ENABLE_EXCEPTIONS

    // Range based for that supports Win32 structures that use NextEntryOffset as the basis of traversing
    // a result buffer that contains data. This is used in the following FileIO calls:
    // FileStreamInfo, FILE_STREAM_INFO
    // FileIdBothDirectoryInfo, FILE_ID_BOTH_DIR_INFO
    // FileFullDirectoryInfo, FILE_FULL_DIR_INFO
    // FileIdExtdDirectoryInfo, FILE_ID_EXTD_DIR_INFO
    // ReadDirectoryChangesW, FILE_NOTIFY_INFORMATION

    template <typename T>
    struct NextEntryOffsetIterator
    {
        NextEntryOffsetIterator(T *iterable = __nullptr) : current_(iterable) {}

        // range based for requires operator!=, operator++ and operator* to do its work
        // on the type returned from begin() and end(), provide those here.
        bool operator!=(const NextEntryOffsetIterator& other) const { return current_ != other.current_; }

        void operator++()
        {
            current_ = (current_->NextEntryOffset != 0) ?
                reinterpret_cast<T *>(reinterpret_cast<unsigned char*>(current_) + current_->NextEntryOffset) :
                __nullptr;
        }

        T const & operator*() const throw() { return *current_; }

        NextEntryOffsetIterator<T> begin() { return *this; }
        NextEntryOffsetIterator<T> end()   { return NextEntryOffsetIterator<T>(); }

        T* current_;
    };

    template <typename T>
    NextEntryOffsetIterator<T> CreateNextEntryOffsetIterator(T* p)
    {
        return NextEntryOffsetIterator<T>(p);
    }

#pragma region Folder Watcher
    // Example use in exception based code:
    // auto watcher = wil::make_folder_watcher(folder.Path().c_str(), true, wil::allChangeEvents, []()
    //     {
    //         // respond
    //     });
    //
    // Example use in result code based code:
    // wil::unique_folder_watcher watcher;
    // THROW_IF_FAILED(watcher.create(folder, true, wil::allChangeEvents, []()
    //     {
    //         // respond
    //     }));

    enum class FolderChangeEvent : DWORD
    {
        ChangesLost = 0, // requies special handling, reset state as events were lost
        Added = FILE_ACTION_ADDED,
        Removed = FILE_ACTION_REMOVED,
        Modified = FILE_ACTION_MODIFIED,
        RenameOldName = FILE_ACTION_RENAMED_OLD_NAME,
        RenameNewName = FILE_ACTION_RENAMED_NEW_NAME,
    };

    enum class FolderChangeEvents : DWORD
    {
        None = 0,
        FileName = FILE_NOTIFY_CHANGE_FILE_NAME,
        DirectoryName = FILE_NOTIFY_CHANGE_DIR_NAME,
        Attributes = FILE_NOTIFY_CHANGE_ATTRIBUTES,
        FileSize = FILE_NOTIFY_CHANGE_SIZE,
        LastWriteTime = FILE_NOTIFY_CHANGE_LAST_WRITE,
        Security = FILE_NOTIFY_CHANGE_SECURITY,
        All = FILE_NOTIFY_CHANGE_FILE_NAME |
              FILE_NOTIFY_CHANGE_DIR_NAME |
              FILE_NOTIFY_CHANGE_ATTRIBUTES |
              FILE_NOTIFY_CHANGE_SIZE |
              FILE_NOTIFY_CHANGE_LAST_WRITE |
              FILE_NOTIFY_CHANGE_SECURITY
    };
    DEFINE_ENUM_FLAG_OPERATORS(FolderChangeEvents);

    /// @cond
    namespace details
    {
        struct folder_watcher_state
        {
            folder_watcher_state(wistd::function<void()> &&callback) : m_callback(wistd::move(callback))
            {
            }
            wistd::function<void()> m_callback;
            unique_hfind_change m_findChangeHandle; // order important, need to close the change before the threadpool
            unique_threadpool_wait m_threadPoolWait;
        };

        inline void delete_folder_watcher_state(_In_opt_ folder_watcher_state *watcherStorage) { delete watcherStorage; }

        typedef resource_policy<folder_watcher_state *, decltype(&details::delete_folder_watcher_state),
            details::delete_folder_watcher_state, details::pointer_access_none> folder_watcher_state_resource_policy;
    }
    /// @endcond

    template <typename storage_t, typename err_policy = err_exception_policy>
    class folder_watcher_t : public storage_t
    {
    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit folder_watcher_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // HRESULT or void error handling...
        typedef typename err_policy::result result;

        // Exception-based constructors
        folder_watcher_t(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void()> &&callback)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions; use the create method");
            create(folderToWatch, isRecursive, filter, wistd::move(callback));
        }

        result create(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void()> &&callback)
        {
            return err_policy::HResult(create_common(folderToWatch, isRecursive, filter, wistd::move(callback)));
        }
    private:
        // This function exists to avoid template expansion of this code based on err_policy.
        HRESULT create_common(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void()> &&callback)
        {
            wistd::unique_ptr<details::folder_watcher_state> watcherState(new(std::nothrow) details::folder_watcher_state(wistd::move(callback)));
            RETURN_IF_NULL_ALLOC(watcherState);

            watcherState->m_findChangeHandle.reset(FindFirstChangeNotificationW(folderToWatch, isRecursive, static_cast<DWORD>(filter)));
            RETURN_LAST_ERROR_IF_FALSE(watcherState->m_findChangeHandle);

            watcherState->m_threadPoolWait.reset(CreateThreadpoolWait(
                [](PTP_CALLBACK_INSTANCE /*Instance*/, void *context, TP_WAIT *pThreadPoolWait, TP_WAIT_RESULT /*result*/)
            {
                auto pThis = static_cast<details::folder_watcher_state *>(context);
                pThis->m_callback();

                // Rearm the wait. Should not fail with valid parameters.
                FindNextChangeNotification(pThis->m_findChangeHandle.get());
                SetThreadpoolWait(pThreadPoolWait, pThis->m_findChangeHandle.get(), __nullptr);
            }, watcherState.get(), __nullptr));
            RETURN_LAST_ERROR_IF_FALSE(watcherState->m_threadPoolWait);
            reset(watcherState.release()); // no more failures after this, pass ownership
            SetThreadpoolWait(get()->m_threadPoolWait.get(), get()->m_findChangeHandle.get(), __nullptr);
            return S_OK;
        }
    };

    typedef unique_any_t<folder_watcher_t<details::unique_storage<details::folder_watcher_state_resource_policy>, err_returncode_policy>> unique_folder_watcher_nothrow;

    inline unique_folder_watcher_nothrow make_folder_watcher_nothrow(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void()> &&callback) WI_NOEXCEPT
    {
        unique_folder_watcher_nothrow watcher;
        watcher.create(folderToWatch, isRecursive, filter, wistd::move(callback));
        return watcher; // caller must test for success using if (watcher)
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    typedef unique_any_t<folder_watcher_t<details::unique_storage<details::folder_watcher_state_resource_policy>, err_exception_policy>> unique_folder_watcher;

    inline unique_folder_watcher make_folder_watcher(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void()> &&callback)
    {
        return unique_folder_watcher(folderToWatch, isRecursive, filter, wistd::move(callback));
    }
#endif // WIL_ENABLE_EXCEPTIONS

#pragma endregion

#pragma region Folder Reader

    // Example use for throwing:
    // auto reader = wil::make_folder_change_reader(folder.Path().c_str(), true, wil::FolderChangeEvents::All, 
    //     [](wil::FolderChangeEvent event, PCWSTR fileName)
    //     {
    //          switch (event)
    //          {
    //          case wil::FolderChangeEvent::ChangesLost: break;
    //          case wil::FolderChangeEvent::Added:    break;
    //          case wil::FolderChangeEvent::Removed:  break;
    //          case wil::FolderChangeEvent::Modified: break;
    //          case wil::FolderChangeEvent::RenamedOldName: break;
    //          case wil::FolderChangeEvent::RenamedNewName: break;
    //      });
    //
    // Example use for non throwing:
    // wil::unique_folder_change_reader_nothrow reader;
    // THROW_IF_FAILED(reader.create(folder, true, wil::FolderChangeEvents::All,
    //     [](wil::FolderChangeEvent event, PCWSTR fileName)
    //     { 
    //         // handle changes
    //     }));
    //

    // @cond
    namespace details
    {
        struct folder_change_reader_state
        {
            folder_change_reader_state(bool isRecursive, FolderChangeEvents filter, wistd::function<void(FolderChangeEvent, PCWSTR)> &&callback)
                : m_isRecursive(isRecursive), m_filter(filter), m_callback(wistd::move(callback))
            {
                ZeroMemory(&m_overlapped, sizeof(m_overlapped));
            }

            ~folder_change_reader_state()
            {
                if (m_tpIo != __nullptr)
                {
                    TP_IO *tpIo = m_tpIo;

                    // Indicate to the callback function that this object is being torn
                    // down.

                    {
                        auto autoLock = m_cancelLock.lock_exclusive();
                        m_tpIo = __nullptr;
                    }

                    // Cancel IO to terminate the file system monitoring operation.

                    if (m_folderHandle)
                    {
                        CancelIoEx(m_folderHandle.get(), &m_overlapped);
                    }

                    // Wait for callbacks to complete.
                    //
                    // N.B. This is a blocking call and must not be made within a
                    //      callback or within a lock which is taken inside the
                    //      callback.

                    WaitForThreadpoolIoCallbacks(tpIo, TRUE);
                    CloseThreadpoolIo(tpIo);
                }
            }

            HRESULT StartIo()
            {
                // Unfortunately we have to handle ref-counting of IOs on behalf of the
                // thread pool.
                StartThreadpoolIo(m_tpIo);
                HRESULT hr = ReadDirectoryChangesW(m_folderHandle.get(), m_readBuffer, sizeof(m_readBuffer),
                    m_isRecursive, static_cast<DWORD>(m_filter), __nullptr, &m_overlapped, __nullptr) ?
                        S_OK : HRESULT_FROM_WIN32(::GetLastError());
                if (FAILED(hr))
                {
                    // This operation does not have the usual semantic of returning
                    // ERROR_IO_PENDING.
                    // NT_ASSERT(hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING));

                    // If the operation failed for whatever reason, ensure the TP
                    // ref counts are accurate.

                    CancelThreadpoolIo(m_tpIo);
                }
                return hr;
            }

            // void (wil::FolderChangeEvent event, PCWSTR fileName)
            wistd::function<void(FolderChangeEvent, PCWSTR)> m_callback;
            unique_handle m_folderHandle;
            BOOL m_isRecursive = FALSE;
            FolderChangeEvents m_filter = FolderChangeEvents::None;
            OVERLAPPED m_overlapped;
            TP_IO *m_tpIo = __nullptr;
            srwlock m_cancelLock;
            char m_readBuffer[4096]; // Consider alternative buffer sizes. With 512 byte buffer i was not able to observe overflow.
        };

        inline void delete_folder_change_reader_state(_In_opt_ folder_change_reader_state *watcherStorage) { delete watcherStorage; }

        typedef resource_policy<folder_change_reader_state *, decltype(&details::delete_folder_change_reader_state),
            details::delete_folder_change_reader_state, details::pointer_access_none> folder_change_reader_state_resource_policy;
    }
    /// @endcond

    template <typename storage_t, typename err_policy = err_exception_policy>
    class folder_change_reader_t : public storage_t
    {
    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit folder_change_reader_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // HRESULT or void error handling...
        typedef typename err_policy::result result;

        // Exception-based constructors
        folder_change_reader_t(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void(FolderChangeEvent, PCWSTR)> &&callback)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions; use the create method");
            create(folderToWatch, isRecursive, filter, wistd::move(callback));
        }

        result create(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void(FolderChangeEvent, PCWSTR)> &&callback)
        {
            return err_policy::HResult(create_common(folderToWatch, isRecursive, filter, wistd::move(callback)));
        }

    private:
        // This function exists to avoid template expansion of this code based on err_policy.
        HRESULT create_common(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter, wistd::function<void(FolderChangeEvent, PCWSTR)> &&callback)
        {
            wistd::unique_ptr<details::folder_change_reader_state> watcherState(new(std::nothrow) details::folder_change_reader_state(
                isRecursive, filter, wistd::move(callback)));
            RETURN_IF_NULL_ALLOC(watcherState);

            watcherState->m_folderHandle.reset(CreateFileW(folderToWatch,
                FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
                __nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, __nullptr));
            RETURN_LAST_ERROR_IF_FALSE(watcherState->m_folderHandle);

            watcherState->m_tpIo = CreateThreadpoolIo(watcherState->m_folderHandle.get(),
                [](PTP_CALLBACK_INSTANCE /* Instance */, void *context,
                void * /*overlapped*/, ULONG result, ULONG_PTR /* BytesTransferred */, TP_IO * /* Io */)
            {
                auto pThis = static_cast<details::folder_change_reader_state *>(context);

                // NT_ASSERT(overlapped == &pThis->m_overlapped);

                bool requeue = true;
                if (result == ERROR_SUCCESS)
                {
                    for (auto const& info : CreateNextEntryOffsetIterator(reinterpret_cast<FILE_NOTIFY_INFORMATION *>(pThis->m_readBuffer)))
                    {
                        wchar_t realtiveFileName[MAX_PATH];
                        StringCchCopyNW(realtiveFileName, ARRAYSIZE(realtiveFileName), info.FileName, info.FileNameLength / sizeof(info.FileName[0]));

                        pThis->m_callback(static_cast<FolderChangeEvent>(info.Action), realtiveFileName);
                    }
                }
                else if (result == ERROR_NOTIFY_ENUM_DIR)
                {
                    pThis->m_callback(FolderChangeEvent::ChangesLost, __nullptr);
                }
                else
                {
                    requeue = false;
                }

                if (requeue)
                {
                    // If the lock is held non-shared or the TP IO is nullptr, this
                    // structure is being torn down. Otherwise, monitor for further
                    // changes.
                    auto autoLock = pThis->m_cancelLock.try_lock_shared();
                    if (autoLock && pThis->m_tpIo)
                    {
                        pThis->StartIo(); // ignoring failure here
                    }
                }
            }, watcherState.get(), __nullptr);
            RETURN_LAST_ERROR_IF_FALSE(watcherState->m_tpIo != __nullptr);
            RETURN_IF_FAILED(watcherState->StartIo());
            reset(watcherState.release());
            return S_OK;
        }
    };

    typedef unique_any_t<folder_change_reader_t<details::unique_storage<details::folder_change_reader_state_resource_policy>, err_returncode_policy>> unique_folder_change_reader_nothrow;

    inline unique_folder_change_reader_nothrow make_folder_change_reader_nothrow(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter,
        wistd::function<void(FolderChangeEvent, PCWSTR)> &&callback) WI_NOEXCEPT
    {
        unique_folder_change_reader_nothrow watcher;
        watcher.create(folderToWatch, isRecursive, filter, wistd::move(callback));
        return watcher; // caller must test for success using if (watcher)
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    typedef unique_any_t<folder_change_reader_t<details::unique_storage<details::folder_change_reader_state_resource_policy>, err_exception_policy>> unique_folder_change_reader;

    inline unique_folder_change_reader make_folder_change_reader(PCWSTR folderToWatch, bool isRecursive, FolderChangeEvents filter,
        wistd::function<void(FolderChangeEvent, PCWSTR)> &&callback)
    {
        return unique_folder_change_reader(folderToWatch, isRecursive, filter, wistd::move(callback));
    }
#endif // WIL_ENABLE_EXCEPTIONS
#pragma endregion

    /** Helper to convert APIs that return strings in a caller specified buffer
    into an allocated buffer of the size needed for the value.

    This is useful for converting code to support extended length paths
    where the buffer sizes can be too big to fit on the stack.

    This works for APIs that return the needed buffer size not including the null
    and those that include the null.
    */
    template <typename TLambda>
    inline HRESULT AdaptFixedSizeToAllocatedResult(PWSTR* result, TLambda&& callback)
    {
        *result = nullptr;
        unique_cotaskmem_string tempResult;
        wchar_t path[MAX_PATH]; // make smaller for testing
        DWORD const pathLength = callback(path, ARRAYSIZE(path));
        RETURN_LAST_ERROR_IF(pathLength == 0);
        if (pathLength < ARRAYSIZE(path)) // ResultFromWin32Length, but works for ResultFromWin32Count case too
        {
            tempResult = make_cotaskmem_string_nothrow(path, pathLength);
            RETURN_IF_NULL_ALLOC(tempResult);
        }
        else
        {
            tempResult = make_cotaskmem_string_nothrow(nullptr, pathLength);
            RETURN_IF_NULL_ALLOC(tempResult);

            DWORD const secondPathLength = callback(tempResult.get(), pathLength);
            RETURN_LAST_ERROR_IF(secondPathLength == 0);
            // WI_ASSERT((pathLength - secondPathLength) == 1);
        }
        *result = tempResult.release();
        return S_OK;
    }

    //! These are always extended length paths with the \\?\ prefix.
    enum class VolumePrefix
    {
        Dos = VOLUME_NAME_DOS,          // \\?\C:\Users\Chris\AppData\Local\Temp\wil8C31.tmp
        VolumeGuid = VOLUME_NAME_GUID,  // \\?\Volume{588fb606-b95b-4eae-b3cb-1e49861aaf18}\Users\Chris\AppData\Local\Temp\wil8C31.tmp
                                        // These are supported by the API but are not really paths, they can't be used with Win32 APIs.
                                        // None = VOLUME_NAME_NONE,        // \Users\Chris\AppData\Local\Temp\wil8C31.tmp
                                        // VolumeDevice = VOLUME_NAME_NT,  // \Device\HarddiskVolume4\Users\Chris\AppData\Local\Temp\wil8C31.tmp
    };
    enum class PathOptions
    {
        Normalized = FILE_NAME_NORMALIZED,
        Opened = FILE_NAME_OPENED,
    };
    DEFINE_ENUM_FLAG_OPERATORS(PathOptions);

    /** Get the full path name in different forms
    Use this instead + VolumePrefix::None instead of GetFileInformationByHandleEx(FileNameInfo) to
    get that path form.
    */
    inline HRESULT GetFinalPathNameByHandleW(HANDLE fileHandle, _Outptr_opt_result_nullonfailure_ PWSTR* path,
        wil::VolumePrefix volumePrefix = wil::VolumePrefix::Dos, wil::PathOptions options = wil::PathOptions::Normalized)
    {
        return AdaptFixedSizeToAllocatedResult(path, [&](_Out_writes_(pathLength) PWSTR path, DWORD pathLength)
        {
            return ::GetFinalPathNameByHandleW(fileHandle, path, pathLength,
                static_cast<DWORD>(volumePrefix) | static_cast<DWORD>(options));
        });
    }

    //! Return an path in an allocated buffer for handling long paths.
    //! Optionally return the pointer to the file name part.
    inline HRESULT GetFullPathNameW(PCWSTR file, _Outptr_opt_result_nullonfailure_ PWSTR* path, _Outptr_opt_ PCWSTR* filePart)
    {
        const auto hr = wil::AdaptFixedSizeToAllocatedResult(path, [&](_Out_writes_(pathLength) PWSTR path, DWORD pathLength)
        {
            // Note that GetFullPathNameW() is not limmited to MAX_PATH
            // but it does take a fixed size buffer.
            return ::GetFullPathNameW(file, pathLength, path, nullptr);
        });
        if (SUCCEEDED(hr) && filePart)
        {
            *filePart = wil::FindLastPathSegment(*path);
        }
        return hr;
    }

    // TODO: add support for these and other similar APIs.
    // GetShortPathNameW()
    // GetLongPathNameW()
    // GetWindowsDirectory()
    // GetTempDirectory()

    /// @cond
    namespace details
    {
        template <FILE_INFO_BY_HANDLE_CLASS infoClass> struct MapInfoClassToInfoStruct; // failure to map is a usage error caught by the compiler
#define MAP_INFOCLASS_TO_STRUCT(InfoClass, InfoStruct, IsFixed, Extra) \
        template <> struct MapInfoClassToInfoStruct<InfoClass> \
        { \
            typedef InfoStruct type; \
            static bool const isFixed = IsFixed; \
            static size_t const extraSize = Extra; \
        };

        MAP_INFOCLASS_TO_STRUCT(FileBasicInfo, FILE_BASIC_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileStandardInfo, FILE_STANDARD_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileNameInfo, FILE_NAME_INFO, false, 32);
        MAP_INFOCLASS_TO_STRUCT(FileRenameInfo, FILE_RENAME_INFO, false, 32);
        MAP_INFOCLASS_TO_STRUCT(FileDispositionInfo, FILE_DISPOSITION_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileAllocationInfo, FILE_ALLOCATION_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileEndOfFileInfo, FILE_END_OF_FILE_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileStreamInfo, FILE_STREAM_INFO, false, 32);
        MAP_INFOCLASS_TO_STRUCT(FileCompressionInfo, FILE_COMPRESSION_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileAttributeTagInfo, FILE_ATTRIBUTE_TAG_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileIdBothDirectoryInfo, FILE_ID_BOTH_DIR_INFO, false, 4096);
        MAP_INFOCLASS_TO_STRUCT(FileIdBothDirectoryRestartInfo, FILE_ID_BOTH_DIR_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileIoPriorityHintInfo, FILE_IO_PRIORITY_HINT_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileRemoteProtocolInfo, FILE_REMOTE_PROTOCOL_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileFullDirectoryInfo, FILE_FULL_DIR_INFO, false, 4096);
        MAP_INFOCLASS_TO_STRUCT(FileFullDirectoryRestartInfo, FILE_FULL_DIR_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileStorageInfo, FILE_STORAGE_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileAlignmentInfo, FILE_ALIGNMENT_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileIdInfo, FILE_ID_INFO, true, 0);
        MAP_INFOCLASS_TO_STRUCT(FileIdExtdDirectoryInfo, FILE_ID_EXTD_DIR_INFO, false, 4096);
        MAP_INFOCLASS_TO_STRUCT(FileIdExtdDirectoryRestartInfo, FILE_ID_EXTD_DIR_INFO, true, 0);

        // Type unsafe version used in the implementation to avoid template bloat.
        inline HRESULT GetFileInfo(HANDLE fileHandle, FILE_INFO_BY_HANDLE_CLASS infoClass, size_t allocationSize,
            _Outptr_result_nullonfailure_ void **result)
        {
            *result = nullptr;

            wistd::unique_ptr<char[]> resultHolder(new(std::nothrow) char[allocationSize]);
            RETURN_IF_NULL_ALLOC(resultHolder);

            for (;;)
            {
                if (GetFileInformationByHandleEx(fileHandle, infoClass, resultHolder.get(), static_cast<DWORD>(allocationSize)))
                {
                    *result = resultHolder.release();
                    break;
                }
                else
                {
                    DWORD const lastError = ::GetLastError();
                    if (lastError == ERROR_MORE_DATA)
                    {
                        allocationSize *= 2;
                        resultHolder.reset(new(std::nothrow) char[allocationSize]);
                        RETURN_IF_NULL_ALLOC(resultHolder);
                    }
                    else if (lastError == ERROR_NO_MORE_FILES) // for folder enumeration cases
                    {
                        resultHolder.reset();
                        break;
                    }
                    else
                    {
                        RETURN_WIN32(lastError);
                    }
                }
            }
            return S_OK;
        }
    }
    /// @endcond

    /** Get file information for a variable sized structure, returns an HRESULT.
    ~~~
    wistd::unique_ptr<FILE_NAME_INFO> fileNameInfo;
    RETURN_IF_FAILED(GetFileInfoNoThrow<FileNameInfo>(fileHandle, fileNameInfo);
    ~~~
    */
    template <FILE_INFO_BY_HANDLE_CLASS infoClass, typename wistd::enable_if<!details::MapInfoClassToInfoStruct<infoClass>::isFixed, int>::type = 0>
    HRESULT GetFileInfoNoThrow(HANDLE fileHandle, wistd::unique_ptr<typename details::MapInfoClassToInfoStruct<infoClass>::type> &result) WI_NOEXCEPT
    {
        void *rawResult;
        HRESULT hr = details::GetFileInfo(fileHandle, infoClass,
            sizeof(details::MapInfoClassToInfoStruct<infoClass>::type) + details::MapInfoClassToInfoStruct<infoClass>::extraSize,
            &rawResult);
        result.reset(static_cast<details::MapInfoClassToInfoStruct<infoClass>::type*>(rawResult));
        RETURN_HR(hr);
    }

    /** Get file information for a fixed sized structure, returns an HRESULT.
    ~~~
    FILE_BASIC_INFO fileBasicInfo;
    RETURN_IF_FAILED(GetFileInfoNoThrow<FileBasicInfo>(fileHandle, &fileBasicInfo);
    ~~~
    */
    template <FILE_INFO_BY_HANDLE_CLASS infoClass, typename wistd::enable_if<details::MapInfoClassToInfoStruct<infoClass>::isFixed, int>::type = 0>
    HRESULT GetFileInfoNoThrow(HANDLE fileHandle, _Out_ typename details::MapInfoClassToInfoStruct<infoClass>::type *result) WI_NOEXCEPT
    {
        RETURN_LAST_ERROR_IF_FALSE(GetFileInformationByHandleEx(fileHandle, infoClass, result, sizeof(*result)));
        return S_OK;
    }

#ifdef _CPPUNWIND
    /** Get file information for a fixed sized structure, throws on failure.
    ~~~
    auto fileBasicInfo = GetFileInfo<FileBasicInfo>(fileHandle);
    ~~~
    */
    template <FILE_INFO_BY_HANDLE_CLASS infoClass, typename wistd::enable_if<details::MapInfoClassToInfoStruct<infoClass>::isFixed, int>::type = 0>
    typename details::MapInfoClassToInfoStruct<infoClass>::type GetFileInfo(HANDLE fileHandle)
    {
        typename details::MapInfoClassToInfoStruct<infoClass>::type result;
        THROW_IF_FAILED(GetFileInfoNoThrow<infoClass>(fileHandle, &result));
        return result;
    }

    /** Get file information for a variable sized structure, throws on failure.
    ~~~
    auto fileBasicInfo = GetFileInfo<FileNameInfo>(fileHandle);
    ~~~
    */
    template <FILE_INFO_BY_HANDLE_CLASS infoClass, typename wistd::enable_if<!details::MapInfoClassToInfoStruct<infoClass>::isFixed, int>::type = 0>
    wistd::unique_ptr<typename details::MapInfoClassToInfoStruct<infoClass>::type> GetFileInfo(HANDLE fileHandle)
    {
        wistd::unique_ptr<typename details::MapInfoClassToInfoStruct<infoClass>::type> result;
        THROW_IF_FAILED(GetFileInfoNoThrow<infoClass>(fileHandle, result));
        return result;
    }
#endif // _CPPUNWIND
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
}
