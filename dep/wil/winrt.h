
#ifndef __WIL_WINRT_INCLUDED
#define __WIL_WINRT_INCLUDED

#include <hstring.h>
#include <wrl\client.h>
#include <wrl\implements.h>
#include <wrl\async.h>
#include <wrl\wrappers\corewrappers.h>
#include "result.h"
#include "com.h"
#include "resource.h"
#include <windows.foundation.collections.h>

#ifdef __cplusplus_winrt
#include <collection.h> // bring in the CRT iterator for support for C++ CX code
#endif

#ifdef WIL_ENABLE_EXCEPTIONS
/// @cond
namespace std
{
    template<class _Elem, class _Traits, class _Alloc>
    class basic_string;

    template<class _Ty>
    struct less;
}
/// @endcond
#endif

// This enables this code to be used in code that uses the ABI prefix or not.
// Code using the public SDK and C++ CX code has the ABI prefix, windows internal
// is built in a way that does not.
#if !defined(MIDL_NS_PREFIX) && !defined(____x_ABI_CWindows_CFoundation_CIClosable_FWD_DEFINED__)
// Internal .idl files use the namespace without the ABI prefix. Macro out ABI for that case
#pragma push_macro("ABI")
#undef ABI
#define ABI
#endif

namespace wil
{
#ifdef _INC_TIME
    // time_t is the number of 1 - second intervals since January 1, 1970.
    long long const SecondsToStartOf1970 = 0x2b6109100;
    long long const HundredNanoSecondsInSecond = 10000000LL;

    inline __time64_t DateTime_to_time_t(ABI::Windows::Foundation::DateTime dateTime)
    {
        // DateTime is the number of 100 - nanosecond intervals since January 1, 1601.
        return (dateTime.UniversalTime / HundredNanoSecondsInSecond - SecondsToStartOf1970);
    }

    inline ABI::Windows::Foundation::DateTime time_t_to_DateTime(__time64_t timeT)
    {
        ABI::Windows::Foundation::DateTime dateTime;
        dateTime.UniversalTime = (timeT + SecondsToStartOf1970) * HundredNanoSecondsInSecond;
        return dateTime;
    }
#endif // _INC_TIME

#pragma region HSTRING Helpers
    /// @cond
    namespace details
    {
        // hstring_compare is used to assist in HSTRING comparison of two potentially non-similar string types. E.g.
        // comparing a raw HSTRING with WRL's HString/HStringReference/etc. The consumer can optionally inhibit the
        // deduction of array sizes by providing 'true' for the 'InhibitStringArrays' template argument. This is
        // generally done in scenarios where the consumer cannot guarantee that the input argument types are perfectly
        // preserved from end-to-end. E.g. if a single function in the execution path captures an array as const T&,
        // then it is impossible to differentiate const arrays (where we generally do want to deduce length) from
        // non-const arrays (where we generally do not want to deduce length). The consumer can also optionally choose
        // to perform case-insensitive comparison by providing 'true' for the 'IgnoreCase' template argument.
        template <bool InhibitStringArrays, bool IgnoreCase>
        struct hstring_compare
        {
            template <typename LhsT, typename RhsT>
            static auto compare(LhsT&& lhs, RhsT&& rhs) ->
                decltype(get_buffer(lhs, wistd::declval<UINT32*>()), get_buffer(rhs, wistd::declval<UINT32*>()), int())
            {
                UINT32 lhsLength;
                UINT32 rhsLength;
                auto lhsBuffer = get_buffer(wistd::forward<LhsT>(lhs), &lhsLength);
                auto rhsBuffer = get_buffer(wistd::forward<RhsT>(rhs), &rhsLength);

                const auto result = ::CompareStringOrdinal(
                    lhsBuffer,
                    lhsLength,
                    rhsBuffer,
                    rhsLength,
                    IgnoreCase ? TRUE : FALSE);
                NT_ASSERT(result != 0);

                return result;
            }

            template <typename LhsT, typename RhsT>
            static auto equals(LhsT&& lhs, RhsT&& rhs) WI_NOEXCEPT ->
                decltype(compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)), bool())
            {
                return compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)) == CSTR_EQUAL;
            }

            template <typename LhsT, typename RhsT>
            static auto not_equals(LhsT&& lhs, RhsT&& rhs) WI_NOEXCEPT ->
                decltype(compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)), bool())
            {
                return compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)) != CSTR_EQUAL;
            }

            template <typename LhsT, typename RhsT>
            static auto less(LhsT&& lhs, RhsT&& rhs) WI_NOEXCEPT ->
                decltype(compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)), bool())
            {
                return compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)) == CSTR_LESS_THAN;
            }

            template <typename LhsT, typename RhsT>
            static auto less_equals(LhsT&& lhs, RhsT&& rhs) WI_NOEXCEPT ->
                decltype(compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)), bool())
            {
                return compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)) != CSTR_GREATER_THAN;
            }

            template <typename LhsT, typename RhsT>
            static auto greater(LhsT&& lhs, RhsT&& rhs) WI_NOEXCEPT ->
                decltype(compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)), bool())
            {
                return compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)) == CSTR_GREATER_THAN;
            }

            template <typename LhsT, typename RhsT>
            static auto greater_equals(LhsT&& lhs, RhsT&& rhs) WI_NOEXCEPT ->
                decltype(compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)), bool())
            {
                return compare(wistd::forward<LhsT>(lhs), wistd::forward<RhsT>(rhs)) != CSTR_LESS_THAN;
            }

            // get_buffer returns the string buffer and length for the supported string types
            static const wchar_t* get_buffer(HSTRING hstr, UINT32* length) WI_NOEXCEPT
            {
                return ::WindowsGetStringRawBuffer(hstr, length);
            }

            static const wchar_t* get_buffer(const Microsoft::WRL::Wrappers::HString& hstr, UINT32* length) WI_NOEXCEPT
            {
                return hstr.GetRawBuffer(length);
            }

            static const wchar_t* get_buffer(
                const Microsoft::WRL::Wrappers::HStringReference& hstr,
                UINT32* length) WI_NOEXCEPT
            {
                return hstr.GetRawBuffer(length);
            }

            static const wchar_t* get_buffer(const unique_hstring& str, UINT32* length) WI_NOEXCEPT
            {
                return ::WindowsGetStringRawBuffer(str.get(), length);
            }

            template <bool..., bool Enable = InhibitStringArrays>
            static wistd::enable_if_t<Enable, const wchar_t*> get_buffer(const wchar_t* str, UINT32* length) WI_NOEXCEPT
            {
                str = (str != nullptr) ? str : L"";
                *length = static_cast<UINT32>(wcslen(str));
                return str;
            }

            template <typename StringT, bool..., bool Enable = !InhibitStringArrays>
            static wistd::enable_if_t<
                wistd::conjunction<
                    wistd::is_pointer<StringT>,
                    wistd::is_same<wistd::decay_t<wistd::remove_pointer_t<StringT>>, wchar_t>,
                    wistd::bool_constant<Enable>
                >::value,
            const wchar_t*> get_buffer(StringT str, UINT32* length) WI_NOEXCEPT
            {
                str = (str != nullptr) ? str : L"";
                *length = static_cast<UINT32>(wcslen(str));
                return str;
            }

            template <size_t Size, bool..., bool Enable = !InhibitStringArrays>
            static wistd::enable_if_t<Enable, const wchar_t*> get_buffer(
                const wchar_t (&str)[Size],
                UINT32* length) WI_NOEXCEPT
            {
                *length = Size - 1;
                return str;
            }

            template <size_t Size, bool..., bool Enable = !InhibitStringArrays>
            static wistd::enable_if_t<Enable, const wchar_t*> get_buffer(wchar_t (&str)[Size], UINT32* length) WI_NOEXCEPT
            {
                *length = static_cast<UINT32>(wcslen(str));
                return str;
            }

#ifdef WIL_ENABLE_EXCEPTIONS
            template<class TraitsT, class AllocT>
            static const wchar_t* get_buffer(
                const std::basic_string<wchar_t, TraitsT, AllocT>& str,
                UINT32* length) WI_NOEXCEPT
            {
                *length = static_cast<UINT32>(str.length());
                return str.c_str();
            }
#endif
        };
    }
    /// @endcond

    //! Detects if one or more embedded null is present in an HSTRING.
    inline bool HasEmbeddedNull(_In_opt_ HSTRING value)
    {
        BOOL hasEmbeddedNull;
        WindowsStringHasEmbeddedNull(value, &hasEmbeddedNull);
        return hasEmbeddedNull != FALSE;
    }

    /** TwoPhaseHStringConstructor help using the 2 phase constructor pattern for HSTRINGs.
    ~~~
    auto stringConstructor = wil::TwoPhaseHStringConstructor::Preallocate(size);
    RETURN_IF_NULL_ALLOC(stringConstructor.Get());

    RETURN_IF_FAILED(stream->Read(stringConstructor.Get(), stringConstructor.ByteSize(), &bytesRead));

    // Validate stream contents, sizes must match, string must be null terminated.
    RETURN_IF_FAILED(stringConstructor.Validate(bytesRead));

    Microsoft::WRL::Wrappers::HString string;
    string.Attach(stringConstructor.Promote());
    ~~~
    */
    struct TwoPhaseHStringConstructor
    {
        TwoPhaseHStringConstructor() = delete;
        TwoPhaseHStringConstructor(const TwoPhaseHStringConstructor&) = delete;
        void operator=(const TwoPhaseHStringConstructor&) = delete;

        TwoPhaseHStringConstructor(TwoPhaseHStringConstructor&& other)
        {
            m_characterLength = other.m_characterLength;
            m_charBuffer = other.m_charBuffer;
            m_bufferHandle = other.m_bufferHandle;
            other.m_bufferHandle = nullptr;
        }

        static TwoPhaseHStringConstructor Preallocate(UINT32 characterLength)
        {
            TwoPhaseHStringConstructor result(characterLength);
            // Client test for allocation failure by testing the result with .Get()
            WindowsPreallocateStringBuffer(result.m_characterLength, &result.m_charBuffer, &result.m_bufferHandle);
            return result;
        }

        //! Returns the HSTRING after it has been populated like Detatch() or release(); be sure to put this in a RAII type to manage its lifetime.
        HSTRING Promote()
        {
            HSTRING result;
            const auto hr = WindowsPromoteStringBuffer(m_bufferHandle, &result);
            FAIL_FAST_IF_FAILED(hr);  // Failure here is only due to invalid input, nul terminator overwritten, a bug in the usage.
            m_bufferHandle = nullptr; // after promotion must not delete
            return result;
        }

        ~TwoPhaseHStringConstructor()
        {
            WindowsDeleteStringBuffer(m_bufferHandle); // ok to call with null
        }

        explicit operator PCWSTR() const
        {
            // This is set by WindowsPromoteStringBuffer() which must be called to
            // construct this object via the static method Preallocate().
            return m_charBuffer;
        }

        //! Returns a pointer for the buffer so it can be populated
        wchar_t* Get() const { return m_charBuffer; }
        //! Used to validate range of buffer when populating.
        ULONG ByteSize() const { return m_characterLength * sizeof(*m_charBuffer); }

        /** Ensure that the size of the data provided is consistent with the pre-allocated buffer.
        It seems that WindowsPreallocateStringBuffer() provides the null terminator in the buffer
        (based on testing) so this can be called before populating the buffer.
        */
        HRESULT Validate(ULONG bytesRead) const
        {
            // Null termination is required for the buffer before calling WindowsPromoteStringBuffer().
            RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                (bytesRead != ByteSize()) ||
                (m_charBuffer[m_characterLength] != L'\0'));
            return S_OK;
        }

    private:
        TwoPhaseHStringConstructor(UINT32 characterLength) : m_characterLength(characterLength)
        {
        }

        UINT32 m_characterLength;
        wchar_t *m_charBuffer;
        HSTRING_BUFFER m_bufferHandle = nullptr;
    };

    //! A transparent less-than comparison function object that enables comparison of various string types intended for
    //! use with associative containers (such as `std::set`, `std::map`, etc.) that use
    //! `Microsoft::WRL::Wrappers::HString` as the key type. This removes the need for the consumer to explicitly
    //! create an `HString` object when using lookup functions such as `find`, `lower_bound`, etc. For example, the
    //! following scenarios would all work exactly as you would expect them to:
    //! ~~~
    //! std::map<HString, int, wil::hstring_less> map;
    //! const wchar_t constArray[] = L"foo";
    //! wchar_t nonConstArray[MAX_PATH] = L"foo";
    //!
    //! HString key;
    //! THROW_IF_FAILED(key.Set(constArray));
    //! map.emplace(std::move(key), 42);
    //!
    //! HString str;
    //! wil::unique_hstring uniqueStr;
    //! THROW_IF_FAILED(str.Set(L"foo"));
    //! THROW_IF_FAILED(str.CopyTo(&uniqueStr));
    //!
    //! // All of the following return an iterator to the pair { L"foo", 42 }
    //! map.find(str);
    //! map.find(str.Get());
    //! map.find(HStringReference(constArray));
    //! map.find(uniqueStr);
    //! map.find(std::wstring(constArray));
    //! map.find(constArray);
    //! map.find(nonConstArray);
    //! map.find(static_cast<const wchar_t*>(constArray));
    //! ~~~
    //! The first four calls in the example above use `WindowsGetStringRawBuffer` (or equivalent) to get the string
    //! buffer and length for the comparison. The fifth example uses `std::wstring::c_str` and `std::wstring::length`
    //! for getting these two values. The remaining three examples use only the string buffer and call `wcslen` for the
    //! length. That is, the length is *not* deduced for either array. This is because argument types are not always
    //! perfectly preserved by container functions and in fact are often captured as const references making it
    //! impossible to differentiate const arrays - where we can safely deduce length - from non const arrays - where we
    //! cannot safely deduce length since the buffer may be larger than actually needed (e.g. creating a
    //! `char[MAX_PATH]` array, but only filling it with 10 characters). The implications of this behavior is that
    //! string literals that contain embedded null characters will only include the part of the buffer up to the first
    //! null character. For example, the following example will result in all calls to `find` returning an end
    //! iterator.
    //! ~~~
    //! std::map<HString, int, wil::hstring_less> map;
    //! const wchar_t constArray[] = L"foo\0bar";
    //! wchar_t nonConstArray[MAX_PATH] = L"foo\0bar";
    //!
    //! // Create the key with the embedded null character
    //! HString key;
    //! THROW_IF_FAILED(key.Set(constArray));
    //! map.emplace(std::move(key), 42);
    //!
    //! // All of the following return map.end() since they look for the string "foo"
    //! map.find(constArray);
    //! map.find(nonConstArray);
    //! map.find(static_cast<const wchar_t*>(constArray));
    //! ~~~
    //! In order to search using a string literal that contains embedded null characters, a simple alternative is to
    //! first create an `HStringReference` and use that for the function call:
    //! ~~~
    //! // HStringReference's constructor *will* deduce the length of const arrays
    //! map.find(HStringReference(constArray));
    //! ~~~
    struct hstring_less
    {
        using is_transparent = void;

        template <typename LhsT, typename RhsT>
        auto operator()(const LhsT& lhs, const RhsT& rhs) const WI_NOEXCEPT ->
            decltype(details::hstring_compare<true, false>::less(lhs, rhs))
        {
            return details::hstring_compare<true, false>::less(lhs, rhs);
        }
    };

    //! A transparent less-than comparison function object whose behavior is equivalent to that of @ref hstring_less
    //! with the one difference that comparisons are case-insensitive. That is, the following example will correctly
    //! find the inserted value:
    //! ~~~
    //! std::map<HString, int, wil::hstring_insensitive_less> map;
    //!
    //! HString key;
    //! THROW_IF_FAILED(key.Set(L"foo"));
    //! map.emplace(std::move(key), 42);
    //!
    //! // All of the following return an iterator to the pair { L"foo", 42 }
    //! map.find(L"FOo");
    //! map.find(HStringReference(L"fOo"));
    //! map.find(HStringReference(L"fOO").Get());
    //! ~~~
    struct hstring_insensitive_less
    {
        using is_transparent = void;

        template <typename LhsT, typename RhsT>
        auto operator()(const LhsT& lhs, const RhsT& rhs) const WI_NOEXCEPT ->
            decltype(details::hstring_compare<true, true>::less(lhs, rhs))
        {
            return details::hstring_compare<true, true>::less(lhs, rhs);
        }
    };

#pragma endregion

    /// @cond
    namespace details
    {
        // MapToSmartType<T>::type is used to map a raw type into an RAII expression
        // of it. This is needed when lifetime management of the type is needed, for example
        // when holding them as a value produced in an iterator.
        // This type has a common set of methods used to abstract the access to the value
        // that is similar to ComPtr<> and the WRL Wrappers: Get(), GetAddressOf() and other operators.
        // Clients of the smart type must use those to access the value.

        // TODO: Having the base definition defined will result in creating leaks if a type
        // that needs resource management (e.g. PROPVARIANT) that has not specialized is used.
        //
        // One fix is to use std::is_enum to cover that case and leave the base definition undefined.
        // That base should use static_assert to inform clients how to fix the lack of specialization.
        template<typename T, typename Enable = void> struct MapToSmartType
        {
            #pragma warning(push)
            #pragma warning(disable:4702) /* https://microsoft.visualstudio.com/OS/_workitems?id=15917057&fullScreen=false&_a=edit */
            struct type // T holder
            {
                type() {};
                type(T&& value) : m_value(wistd::forward<T>(value)) {};
                operator T() const { return m_value; }
                type& operator=(T&& value) { m_value = wistd::forward<T>(value); return *this; }
                T Get() const { return m_value; }

                // Returning T&& to support move only types
                // In case of absense of T::operator=(T&&) a call to T::operator=(const T&) will happen
                T&& Get()          { return wistd::move(m_value); }

                T* GetAddressOf()  { return &m_value; }
                T* ReleaseAndGetAddressOf() { return &m_value; }
                T* operator&()     { return &m_value; }
                T m_value;
            };
            #pragma warning(pop)
        };

        // IUnknown * derived -> Microsoft::WRL::ComPtr<>
        template <typename T>
        struct MapToSmartType<T, typename wistd::enable_if<wistd::is_base_of<IUnknown, typename wistd::remove_pointer<T>::type>::value>::type>
        {
            typedef Microsoft::WRL::ComPtr<typename wistd::remove_pointer<T>::type> type;
        };

        // HSTRING -> Microsoft::WRL::Wrappers::HString
        template <> struct MapToSmartType<HSTRING, void>
        {
            class HStringWithRelease : public Microsoft::WRL::Wrappers::HString
            {
            public:
                // Unlike all other WRL types HString does not have ReleaseAndGetAddressOf and
                // GetAddressOf() has non-standard behavior, calling Release().
                HSTRING* ReleaseAndGetAddressOf() throw()
                {
                    Release();
                    return &hstr_;
                }
            };
            typedef HStringWithRelease type;
        };

        // WinRT interfaces like IVector<>, IAsyncOperation<> and IIterable<> can be templated
        // on a runtime class (instead of an interface or primitive type). In these cases the objects
        // produced by those interfaces implement an interface defined by the runtime class default interface.
        //
        // These templates deduce the type of the produced interface or pass through
        // the type unmodified in the non runtime class case.
        //
        // for example:
        //      IAsyncOperation<StorageFile*> -> IAsyncOperation<IStorageFile*>

        // For IVector<T>, IVectorView<T>.
        template<typename VectorType> struct MapVectorResultType
        {
            template<typename TResult>
            static TResult PeekGetAtType(HRESULT(STDMETHODCALLTYPE VectorType::*)(unsigned, TResult*));
            typedef decltype(PeekGetAtType(&VectorType::GetAt)) type;
        };

        // For IIterator<T>.
        template<typename T> struct MapIteratorResultType
        {
            template<typename TResult>
            static TResult PeekCurrentType(HRESULT(STDMETHODCALLTYPE ABI::Windows::Foundation::Collections::IIterator<T>::*)(TResult*));
            typedef decltype(PeekCurrentType(&ABI::Windows::Foundation::Collections::IIterator<T>::get_Current)) type;
        };

        // For IAsyncOperation<T>.
        template<typename T> struct MapAsyncOpResultType
        {
            template<typename TResult>
            static TResult PeekGetResultsType(HRESULT(STDMETHODCALLTYPE ABI::Windows::Foundation::IAsyncOperation<T>::*)(TResult*));
            typedef decltype(PeekGetResultsType(&ABI::Windows::Foundation::IAsyncOperation<T>::GetResults)) type;
        };

        // For IAsyncOperationWithProgress<T, P>.
        template<typename T, typename P> struct MapAsyncOpProgressResultType
        {
            template<typename TResult>
            static TResult PeekGetResultsType(HRESULT(STDMETHODCALLTYPE ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>::*)(TResult*));
            typedef decltype(PeekGetResultsType(&ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>::GetResults)) type;
        };

        // No support for IAsyncActionWithProgress<P> none of these (currently) use
        // a runtime class for the progress type.
    }
    /// @endcond
#pragma region C++ iterators for WinRT collections for use with range based for and STL algorithms

    /** Range base for and STL algorithms support for WinRT ABI collection types, IVector<T>, IVectorView<T>, IIterable<T>
    similar to support provided by <collection.h> for C++ CX. Three error handling policies are supported.
    ~~~
    ComPtr<CollectionType> collection = GetCollection(); // can be IVector<HSTRING>, IVectorView<HSTRING> or IIterable<HSTRING>

    for (auto const& element : wil::GetRange(collection.Get()))               // exceptions
    for (auto const& element : wil::GetRangeNoThrow(collection.Get(), &hr))   // error code
    for (auto const& element : wil::GetRangeFailFast(collection.Get()))       // fail fast
    {
       // use element
    }
    ~~~
    Standard algorithm example:
    ~~~
    ComPtr<IVectorView<StorageFile*>> files = GetFiles();
    auto fileRange = wil::GetRangeNoThrow(files.Get());
    auto itFound = std::find_if(fileRange.begin(), fileRange.end(), [](ComPtr<IStorageFile> file) -> bool
    {
         return true; // first element in range
    });
    ~~~
    */
#pragma region exception and fail fast based IVector<>/IVectorView<>

    template <typename VectorType, typename err_policy = err_exception_policy>
    class VectorRange
    {
    public:
        typedef typename details::MapVectorResultType<VectorType>::type TResult;
        typedef typename details::MapToSmartType<TResult>::type TSmart;

        VectorRange() = delete;

        explicit VectorRange(_In_ VectorType *vector) : m_v(vector)
        {
        }

        class VectorIterator
        {
        public:
#ifdef _XUTILITY_
            // could be random_access_iterator_tag but missing some features
            typedef ::std::bidirectional_iterator_tag iterator_category;
#endif
            typedef TSmart value_type;
            typedef ptrdiff_t difference_type;
            typedef const TSmart* pointer;
            typedef const TSmart& reference;

            // for begin()
            VectorIterator(VectorType* v, unsigned int pos)
                : m_v(v), m_i(pos)
            {
            }

            // for end()
            VectorIterator() : m_v(nullptr), m_i(-1) {}

            VectorIterator(const VectorIterator& other)
            {
                m_v = other.m_v;
                m_i = other.m_i;
                err_policy::HResult(other.m_element.CopyTo(m_element.GetAddressOf()));
            }

            VectorIterator& operator=(const VectorIterator& other)
            {
                m_v = other.m_v;
                m_i = other.m_i;
                err_policy::HResult(other.m_element.CopyTo(m_element.ReleaseAndGetAddressOf()));
                return *this;
            }

            reference operator*()
            {
                err_policy::HResult(m_v->GetAt(m_i, m_element.ReleaseAndGetAddressOf()));
                return m_element;
            }

            pointer operator->()
            {
                err_policy::HResult(m_v->GetAt(m_i, m_element.ReleaseAndGetAddressOf()));
                return &m_element;
            }

            VectorIterator& operator++()
            {
                ++m_i;
                return *this;
            }

            VectorIterator& operator--()
            {
                --m_i;
                return *this;
            }

            VectorIterator operator++(int)
            {
                VectorIterator old(*this);
                ++*this;
                return old;
            }

            VectorIterator operator--(int)
            {
                VectorIterator old(*this);
                --*this;
                return old;
            }

            VectorIterator& operator+=(int n)
            {
                m_i += n;
                return *this;
            }

            VectorIterator& operator-=(int n)
            {
                m_i -= n;
                return *this;
            }

            VectorIterator operator+(int n) const
            {
                VectorIterator ret(*this);
                ret += n;
                return ret;
            }

            VectorIterator operator-(int n) const
            {
                VectorIterator ret(*this);
                ret -= n;
                return ret;
            }

            ptrdiff_t operator-(const VectorIterator& other) const
            {
                return m_i - other.m_i;
            }

            bool operator==(const VectorIterator& other) const
            {
                return m_i == other.m_i;
            }

            bool operator!=(const VectorIterator& other) const
            {
                return m_i != other.m_i;
            }

            bool operator<(const VectorIterator& other) const
            {
                return m_i < other.m_i;
            }

            bool operator>(const VectorIterator& other) const
            {
                return m_i > other.m_i;
            }

            bool operator<=(const VectorIterator& other) const
            {
                return m_i <= other.m_i;
            }

            bool operator>=(const VectorIterator& other) const
            {
                return m_i >= other.m_i;
            }

        private:
            VectorType* m_v; // weak, collection must outlive iterators.
            unsigned int m_i;
            TSmart m_element;
        };

        VectorIterator begin()
        {
            return VectorIterator(m_v, 0);
        }

        VectorIterator end()
        {
            unsigned int size;
            err_policy::HResult(m_v->get_Size(&size));
            return VectorIterator(m_v, size);
        }
    private:
        VectorType* m_v; // weak, collection must outlive iterators.
    };
#pragma endregion

#pragma region error code based IVector<>/IVectorView<>

    template <typename VectorType>
    class VectorRangeNoThrow
    {
    public:
        typedef typename details::MapVectorResultType<VectorType>::type TResult;
        typedef typename details::MapToSmartType<TResult>::type TSmart;

        VectorRangeNoThrow() = delete;
        VectorRangeNoThrow(const VectorRangeNoThrow&) = delete;
        VectorRangeNoThrow& operator=(const VectorRangeNoThrow&) = delete;

        VectorRangeNoThrow(VectorRangeNoThrow&& other) :
            m_v(other.m_v), m_result(other.m_result), m_resultStorage(other.m_resultStorage),
            m_size(other.m_size), m_currentElement(wistd::move(other.m_currentElement))
        {
        }

        VectorRangeNoThrow(_In_ VectorType *vector, HRESULT* result = nullptr)
            : m_v(vector), m_result(result ? result : &m_resultStorage)
        {
            *m_result = m_v->get_Size(&m_size);
        }

        class VectorIteratorNoThrow
        {
        public:
#ifdef _XUTILITY_
            // must be input_iterator_tag as use (via ++, --, etc.) of one invalidates the other.
            typedef ::std::input_iterator_tag iterator_category;
#endif
            typedef TSmart value_type;
            typedef ptrdiff_t difference_type;
            typedef const TSmart* pointer;
            typedef const TSmart& reference;

            VectorIteratorNoThrow() = delete;
            VectorIteratorNoThrow(VectorRangeNoThrow<VectorType>* range, unsigned int pos)
                : m_range(range), m_i(pos)
            {
            }

            reference operator*() const
            {
                return m_range->m_currentElement;
            }

            pointer operator->() const
            {
                return &m_range->m_currentElement;
            }

            VectorIteratorNoThrow& operator++()
            {
                ++m_i;
                m_range->GetAtCurrent(m_i);
                return *this;
            }

            VectorIteratorNoThrow& operator--()
            {
                --m_i;
                m_range->GetAtCurrent(m_i);
                return *this;
            }

            VectorIteratorNoThrow operator++(int)
            {
                VectorIteratorNoThrow old(*this);
                ++*this;
                return old;
            }

            VectorIteratorNoThrow operator--(int)
            {
                VectorIteratorNoThrow old(*this);
                --*this;
                return old;
            }

            VectorIteratorNoThrow& operator+=(int n)
            {
                m_i += n;
                m_range->GetAtCurrent(m_i);
                return *this;
            }

            VectorIteratorNoThrow& operator-=(int n)
            {
                m_i -= n;
                m_range->GetAtCurrent(m_i);
                return *this;
            }

            bool operator==(VectorIteratorNoThrow const& other) const
            {
                return FAILED(*m_range->m_result) || (m_i == other.m_i);
            }

            bool operator!=(VectorIteratorNoThrow const& other) const
            {
                return !operator==(other);
            }

        private:
            VectorRangeNoThrow<VectorType>* m_range;
            unsigned int m_i = 0;
        };

        VectorIteratorNoThrow begin()
        {
            GetAtCurrent(0);
            return VectorIteratorNoThrow(this, 0);
        }

        VectorIteratorNoThrow end()
        {
            return VectorIteratorNoThrow(this, m_size);
        }

        // Note, the error code is observed in operator!= and operator==, it always
        // returns "equal" in the failed state to force the compare to the end
        // iterator to return false and stop the loop.
        //
        // Is this ok for the general case?
        void GetAtCurrent(unsigned int i)
        {
            if (SUCCEEDED(*m_result) && (i < m_size))
            {
                *m_result = m_v->GetAt(i, m_currentElement.ReleaseAndGetAddressOf());
            }
        }

    private:
        VectorType* m_v; // weak, collection must outlive iterators.
        unsigned int m_size;

        // This state is shared by VectorIteratorNoThrow instances. this means
        // use of one iterator invalidates the other.
        HRESULT* m_result;
        HRESULT m_resultStorage = S_OK; // for the case where the caller does not provide the location to store the result
        TSmart m_currentElement;
    };

#pragma endregion

#pragma region exception and fail fast based IIterable<>

    template <typename T, typename err_policy = err_exception_policy>
    class IterableRange
    {
    public:
        typedef typename details::MapIteratorResultType<T>::type TResult;
        typedef typename details::MapToSmartType<TResult>::type TSmart;

        explicit IterableRange(_In_ ABI::Windows::Foundation::Collections::IIterable<T>* iterable)
            : m_iterable(iterable)
        {
        }

        class IterableIterator
        {
        public:
#ifdef _XUTILITY_
            typedef ::std::forward_iterator_tag iterator_category;
#endif
            typedef TSmart value_type;
            typedef ptrdiff_t difference_type;
            typedef const TSmart* pointer;
            typedef const TSmart& reference;

            IterableIterator() : m_i(-1) {}

            // for begin()
            explicit IterableIterator(_In_ ABI::Windows::Foundation::Collections::IIterable<T>* iterable)
            {
                err_policy::HResult(iterable->First(&m_iterator));
                boolean hasCurrent;
                err_policy::HResult(m_iterator->get_HasCurrent(&hasCurrent));
                m_i = hasCurrent ? 0 : -1;
            }

            // for end()
            IterableIterator(int currentIndex) : m_i(-1)
            {
            }

            IterableIterator(const IterableIterator& other)
            {
                m_iterator = other.m_iterator;
                m_i = other.m_i;
                err_policy::HResult(other.m_element.CopyTo(m_element.GetAddressOf()));
            }

            IterableIterator& operator=(const IterableIterator& other)
            {
                m_iterator = other.m_iterator;
                m_i = other.m_i;
                err_policy::HResult(other.m_element.CopyTo(m_element.ReleaseAndGetAddressOf()));
                return *this;
            }

            bool operator==(IterableIterator const& other) const
            {
                return m_i == other.m_i;
            }

            bool operator!=(IterableIterator const& other) const
            {
                return !operator==(other);
            }

            reference operator*()
            {
                err_policy::HResult(m_iterator->get_Current(m_element.ReleaseAndGetAddressOf()));
                return m_element;
            }

            pointer operator->()
            {
                err_policy::HResult(m_iterator->get_Current(m_element.ReleaseAndGetAddressOf()));
                return &m_element;
            }

            IterableIterator& operator++()
            {
                boolean hasCurrent;
                err_policy::HResult(m_iterator->MoveNext(&hasCurrent));
                if (hasCurrent)
                {
                    m_i++;
                }
                else
                {
                    m_i = -1;
                }
                return *this;
            }

            IterableIterator operator++(int)
            {
                IterableIterator old(*this);
                ++*this;
                return old;
            }

        private:
            Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Collections::IIterator<T>> m_iterator;
            int m_i;
            TSmart m_element;
        };

        IterableIterator begin()
        {
            return IterableIterator(m_iterable);
        }

        IterableIterator end()
        {
            return IterableIterator();
        }
    private:
        // weak, collection must outlive iterators.
        ABI::Windows::Foundation::Collections::IIterable<T>* m_iterable;
    };
#pragma endregion

#pragma region error code base IIterable<>
    template <typename T>
    class IterableRangeNoThrow
    {
    public:
        typedef typename details::MapIteratorResultType<T>::type TResult;
        typedef typename details::MapToSmartType<TResult>::type TSmart;

        IterableRangeNoThrow() = delete;
        IterableRangeNoThrow(const IterableRangeNoThrow&) = delete;
        IterableRangeNoThrow& operator=(const IterableRangeNoThrow&) = delete;
        IterableRangeNoThrow& operator=(IterableRangeNoThrow &&) = delete;

        IterableRangeNoThrow(IterableRangeNoThrow&& other) :
            m_iterator(wistd::move(other.m_iterator)), m_element(wistd::move(other.m_element)),
            m_resultStorage(other.m_resultStorage)
        {
            if (other.m_result == &other.m_resultStorage)
            {
                m_result = &m_resultStorage;
            }
            else
            {
                m_result = other.m_result;
            }
        }

        IterableRangeNoThrow(_In_ ABI::Windows::Foundation::Collections::IIterable<T>* iterable, HRESULT* result = nullptr)
            : m_result(result ? result : &m_resultStorage)
        {
            *m_result = iterable->First(&m_iterator);
            if (SUCCEEDED(*m_result))
            {
                boolean hasCurrent;
                *m_result = m_iterator->get_HasCurrent(&hasCurrent);
                if (SUCCEEDED(*m_result) && hasCurrent)
                {
                    *m_result = m_iterator->get_Current(m_element.ReleaseAndGetAddressOf());
                    if (FAILED(*m_result))
                    {
                        m_iterator = nullptr; // release the iterator if no elements are found
                    }
                }
                else
                {
                    m_iterator = nullptr; // release the iterator if no elements are found
                }
            }
        }

        class IterableIteratorNoThrow
        {
        public:
#ifdef _XUTILITY_
            // muse be input_iterator_tag as use of one instance invalidates the other.
            typedef ::std::input_iterator_tag iterator_category;
#endif
            typedef TSmart value_type;
            typedef ptrdiff_t difference_type;
            typedef const TSmart* pointer;
            typedef const TSmart& reference;

            IterableIteratorNoThrow(_In_ IterableRangeNoThrow* range, int currentIndex) :
                m_range(range), m_i(currentIndex)
            {
            }

            bool operator==(IterableIteratorNoThrow const& other) const
            {
                return FAILED(*m_range->m_result) || (m_i == other.m_i);
            }

            bool operator!=(IterableIteratorNoThrow const& other) const
            {
                return !operator==(other);
            }

            reference operator*() const WI_NOEXCEPT
            {
                return m_range->m_element;
            }

            pointer operator->() const WI_NOEXCEPT
            {
                return &m_range->m_element;
            }

            IterableIteratorNoThrow& operator++()
            {
                boolean hasCurrent;
                *m_range->m_result = m_range->m_iterator->MoveNext(&hasCurrent);
                if (SUCCEEDED(*m_range->m_result) && hasCurrent)
                {
                    *m_range->m_result = m_range->m_iterator->get_Current(m_range->m_element.ReleaseAndGetAddressOf());
                    if (SUCCEEDED(*m_range->m_result))
                    {
                        m_i++;
                    }
                    else
                    {
                        m_i = -1;
                    }
                }
                else
                {
                    m_i = -1;
                }
                return *this;
            }

            IterableRangeNoThrow operator++(int)
            {
                IterableRangeNoThrow old(*this);
                ++*this;
                return old;
            }

        private:
            IterableRangeNoThrow* m_range;
            int m_i;
        };

        IterableIteratorNoThrow begin()
        {
            return IterableIteratorNoThrow(this, this->m_iterator ? 0 : -1);
        }

        IterableIteratorNoThrow end()
        {
            return IterableIteratorNoThrow(this, -1);
        }

    private:
        Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Collections::IIterator<T>> m_iterator;
        // This state is shared by all iterator instances
        // so use of one iterator can invalidate another's ability to dereference
        // that is allowed for input iterators.
        TSmart m_element;
        HRESULT* m_result;
        HRESULT m_resultStorage = S_OK;
    };

#pragma endregion

#ifdef WIL_ENABLE_EXCEPTIONS
    template <typename T> VectorRange<ABI::Windows::Foundation::Collections::IVector<T>> GetRange(ABI::Windows::Foundation::Collections::IVector<T> *v)
    {
        return VectorRange<ABI::Windows::Foundation::Collections::IVector<T>>(v);
    }

    template <typename T> VectorRange<ABI::Windows::Foundation::Collections::IVectorView<T>> GetRange(ABI::Windows::Foundation::Collections::IVectorView<T> *v)
    {
        return VectorRange<ABI::Windows::Foundation::Collections::IVectorView<T>>(v);
    }
#endif // WIL_ENABLE_EXCEPTIONS

    template <typename T> VectorRange<ABI::Windows::Foundation::Collections::IVector<T>, err_failfast_policy> GetRangeFailFast(ABI::Windows::Foundation::Collections::IVector<T> *v)
    {
        return VectorRange<ABI::Windows::Foundation::Collections::IVector<T>, err_failfast_policy>(v);
    }

    template <typename T> VectorRange<ABI::Windows::Foundation::Collections::IVectorView<T>, err_failfast_policy> GetRangeFailFast(ABI::Windows::Foundation::Collections::IVectorView<T> *v)
    {
        return VectorRange<ABI::Windows::Foundation::Collections::IVectorView<T>, err_failfast_policy>(v);
    }

    template <typename T> VectorRangeNoThrow<ABI::Windows::Foundation::Collections::IVector<T>> GetRangeNoThrow(ABI::Windows::Foundation::Collections::IVector<T> *v, HRESULT* result = nullptr)
    {
        return VectorRangeNoThrow<ABI::Windows::Foundation::Collections::IVector<T>>(v, result);
    }

    template <typename T> VectorRangeNoThrow<ABI::Windows::Foundation::Collections::IVectorView<T>> GetRangeNoThrow(ABI::Windows::Foundation::Collections::IVectorView<T> *v, HRESULT* result = nullptr)
    {
        return VectorRangeNoThrow<ABI::Windows::Foundation::Collections::IVectorView<T>>(v, result);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    template <typename T> IterableRange<T> GetRange(ABI::Windows::Foundation::Collections::IIterable<T> *v)
    {
        return IterableRange<T>(v);
    }
#endif // WIL_ENABLE_EXCEPTIONS

    template <typename T> IterableRange<T, err_failfast_policy> GetRangeFailFast(ABI::Windows::Foundation::Collections::IIterable<T> *v)
    {
        return IterableRange<T, err_failfast_policy>(v);
    }

    template <typename T> IterableRangeNoThrow<T> GetRangeNoThrow(ABI::Windows::Foundation::Collections::IIterable<T> *v, HRESULT* result = nullptr)
    {
        return IterableRangeNoThrow<T>(v, result);
    }
}

#pragma endregion

#ifdef WIL_ENABLE_EXCEPTIONS

#pragma region Global operator functions
#if defined(MIDL_NS_PREFIX) || defined(____x_ABI_CWindows_CFoundation_CIClosable_FWD_DEFINED__)
namespace ABI {
#endif
    namespace Windows {
        namespace Foundation {
            namespace Collections {
                template <typename X> typename wil::VectorRange<IVector<X>>::VectorIterator begin(IVector<X>* v)
                {
                    return wil::VectorRange<IVector<X>>::VectorIterator(v, 0);
                }

                template <typename X> typename wil::VectorRange<IVector<X>>::VectorIterator end(IVector<X>* v)
                {
                    unsigned int size;
                    THROW_IF_FAILED(v->get_Size(&size));
                    return wil::VectorRange<IVector<X>>::VectorIterator(v, size);
                }

                template <typename X> typename wil::VectorRange<IVectorView<X>>::VectorIterator begin(IVectorView<X>* v)
                {
                    return wil::VectorRange<IVectorView<X>>::VectorIterator(v, 0);
                }

                template <typename X> typename wil::VectorRange<IVectorView<X>>::VectorIterator end(IVectorView<X>* v)
                {
                    unsigned int size;
                    THROW_IF_FAILED(v->get_Size(&size));
                    return wil::VectorRange<IVectorView<X>>::VectorIterator(v, size);
                }

                template <typename X> typename wil::IterableRange<X>::IterableIterator begin(IIterable<X>* i)
                {
                    return wil::IterableRange<X>::IterableIterator(i);
                }

                template <typename X> typename wil::IterableRange<X>::IterableIterator end(IIterable<X>*)
                {
                    return wil::IterableRange<X>::IterableIterator();
                }
            } // namespace Collections
        } // namespace Foundation
    } // namespace Windows
#if defined(MIDL_NS_PREFIX) || defined(____x_ABI_CWindows_CFoundation_CIClosable_FWD_DEFINED__)
} // namespace ABI
#endif

#endif // WIL_ENABLE_EXCEPTIONS

#pragma endregion

namespace wil
{
#pragma region WinRT Async API helpers

/// @cond
namespace details
{
    template <typename TResult, typename TFunc, typename ...Args,
        typename wistd::enable_if<wistd::is_same<HRESULT, TResult>::value, int>::type = 0>
        HRESULT CallAndHandleErrorsWithReturnType(const TFunc& func, Args&&... args)
    {
        return func(wistd::forward<Args>(args)...);
    }

    template <typename TResult, typename TFunc, typename ...Args,
        typename wistd::enable_if<wistd::is_same<void, TResult>::value, int>::type = 0>
        HRESULT CallAndHandleErrorsWithReturnType(const TFunc& func, Args&&... args)
    {
        try
        {
            func(wistd::forward<Args>(args)...);
        }
        CATCH_RETURN();
        return S_OK;
    }

    template <typename TFunc, typename ...Args>
    HRESULT CallAndHandleErrors(const TFunc& func, Args&&... args)
    {
        return CallAndHandleErrorsWithReturnType<decltype(func(wistd::forward<Args>(args)...)), TFunc>(
            func, wistd::forward<Args>(args)...);
    }

    // Get the last type of a template parameter pack.
    // usage:
    //     LastType<int, bool>::type boolValue;
    template <typename... Ts> struct LastType
    {
        template<typename T, typename... Ts> struct LastTypeOfTs
        {
            typedef typename LastTypeOfTs<Ts...>::type type;
        };

        template<typename T> struct LastTypeOfTs<T>
        {
            typedef T type;
        };

        template<typename... Ts>
        static typename LastTypeOfTs<Ts...>::type LastTypeOfTsFunc() {}
        typedef decltype(LastTypeOfTsFunc<Ts...>()) type;
    };

    // Takes a member function that has an out param like F(..., IAsyncAction**) or F(..., IAsyncOperation<bool>**)
    // and returns IAsyncAction* or IAsyncOperation<bool>*.
    template<typename I, typename ...P>
    typename wistd::remove_pointer<typename LastType<P...>::type>::type GetReturnParamPointerType(HRESULT(STDMETHODCALLTYPE I::*)(P...));

    // Use to determine the result type of the async action/operation interfaces or example
    // decltype(GetAsyncResultType(action.get())) returns void
    void GetAsyncResultType(ABI::Windows::Foundation::IAsyncAction*);
    template <typename P> void GetAsyncResultType(ABI::Windows::Foundation::IAsyncActionWithProgress<P>*);
    template <typename T> typename wil::details::MapAsyncOpResultType<T>::type GetAsyncResultType(ABI::Windows::Foundation::IAsyncOperation<T>*);
    template <typename T, typename P> typename wil::details::MapAsyncOpProgressResultType<T, P>::type GetAsyncResultType(ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*);

    // Use to determine the result type of the async action/operation interfaces or example
    // decltype(GetAsyncDelegateType(action.get())) returns void
    ABI::Windows::Foundation::IAsyncActionCompletedHandler* GetAsyncDelegateType(ABI::Windows::Foundation::IAsyncAction*);
    template <typename P> ABI::Windows::Foundation::IAsyncActionWithProgressCompletedHandler<P>* GetAsyncDelegateType(ABI::Windows::Foundation::IAsyncActionWithProgress<P>*);
    template <typename T> ABI::Windows::Foundation::IAsyncOperationCompletedHandler<T>* GetAsyncDelegateType(ABI::Windows::Foundation::IAsyncOperation<T>*);
    template <typename T, typename P> ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<T, P>* GetAsyncDelegateType(ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*);

    template <typename TBaseAgility, typename TIOperation, typename TFunction>
    HRESULT RunWhenCompleteAction(_In_ TIOperation operation, TFunction&& func) WI_NOEXCEPT
    {
        using namespace Microsoft::WRL;
        typedef wistd::remove_pointer<decltype(GetAsyncDelegateType(operation))>::type TIDelegate;

        auto callback = Callback<Implements<RuntimeClassFlags<ClassicCom>, TIDelegate, TBaseAgility>>(
            [func /* = wistd::move(func) */](TIOperation operation, AsyncStatus status) -> HRESULT
        {
            HRESULT hr = S_OK;
            if (status != AsyncStatus::Completed)   // avoid a potentially costly marshaled QI / call if we completed successfully
            {
                ComPtr<IAsyncInfo> asyncInfo;
                operation->QueryInterface(IID_PPV_ARGS(&asyncInfo)); // All must implement IAsyncInfo
                asyncInfo->get_ErrorCode(&hr);
            }

            return CallAndHandleErrors(func, hr);
        });
        RETURN_IF_NULL_ALLOC(callback);
        return operation->put_Completed(callback.Get());
    }

    template <typename TBaseAgility, typename TIOperation, typename TFunction>
    HRESULT RunWhenComplete(_In_ TIOperation operation, TFunction&& func) WI_NOEXCEPT
    {
        using namespace Microsoft::WRL;
        using namespace ABI::Windows::Foundation::Internal;

        typedef wistd::remove_pointer<decltype(GetAsyncDelegateType(operation))>::type TIDelegate;

        auto callback = Callback<Implements<RuntimeClassFlags<ClassicCom>, TIDelegate, TBaseAgility>>(
            [func](TIOperation operation, AsyncStatus status) -> HRESULT
        {
            typename details::MapToSmartType<typename GetAbiType<typename wistd::remove_pointer<TIOperation>::type::TResult_complex>::type>::type result;

            HRESULT hr = S_OK;
            if (status == AsyncStatus::Completed)
            {
                hr = operation->GetResults(result.GetAddressOf());
            }
            else
            {
                // avoid a potentially costly marshaled QI / call if we completed successfully
                ComPtr<IAsyncInfo> asyncInfo;
                operation->QueryInterface(IID_PPV_ARGS(&asyncInfo)); // all must implement this
                asyncInfo->get_ErrorCode(&hr);
            }

            return CallAndHandleErrors(func, hr, result.Get());
        });
        RETURN_IF_NULL_ALLOC(callback);
        return operation->put_Completed(callback.Get());
    }

    template <typename TIOperation>
    HRESULT WaitForCompletion(_In_ TIOperation operation, COWAIT_FLAGS flags, DWORD timeoutValue, _Out_opt_ bool* timedOut) WI_NOEXCEPT
    {
        typedef wistd::remove_pointer<decltype(GetAsyncDelegateType(operation))>::type TIDelegate;

        class CompletionDelegate : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::Delegate>,
            TIDelegate, Microsoft::WRL::FtmBase>
        {
        public:
            HRESULT RuntimeClassInitialize()
            {
                RETURN_HR(m_completedEventHandle.create());
            }

            HRESULT STDMETHODCALLTYPE Invoke(_In_ TIOperation, AsyncStatus status) override
            {
                m_status = status;
                m_completedEventHandle.SetEvent();
                return S_OK;
            }

            HANDLE GetEvent() const
            {
                return m_completedEventHandle.get();
            }

            AsyncStatus GetStatus() const
            {
                return m_status;
            }

        private:
            volatile AsyncStatus m_status = AsyncStatus::Started;
            wil::unique_event_nothrow m_completedEventHandle;
        };

        WI_ASSERT(timedOut || (timeoutValue == INFINITE));
        assign_to_opt_param(timedOut, false);

        Microsoft::WRL::ComPtr<CompletionDelegate> completedDelegate;
        RETURN_IF_FAILED(Microsoft::WRL::MakeAndInitialize<CompletionDelegate>(&completedDelegate));
        RETURN_IF_FAILED(operation->put_Completed(completedDelegate.Get()));

        HANDLE handles[] = { completedDelegate->GetEvent() };
        DWORD dwHandleIndex;
        HRESULT hr = CoWaitForMultipleHandles(flags, timeoutValue, ARRAYSIZE(handles), handles, &dwHandleIndex);

        // If the caller is listening for timedOut, and we actually timed out, set the bool and return S_OK. Otherwise, fail.
        if (timedOut && (hr == RPC_S_CALLPENDING))
        {
            *timedOut = true;
            return S_OK;
        }
        RETURN_IF_FAILED(hr);

        if (completedDelegate->GetStatus() != AsyncStatus::Completed)
        {
            Microsoft::WRL::ComPtr<IAsyncInfo> asyncInfo;
            operation->QueryInterface(IID_PPV_ARGS(&asyncInfo)); // all must implement this
            hr = E_UNEXPECTED;
            asyncInfo->get_ErrorCode(&hr); // error return ignored, ok?
            return hr; // leave it to the caller to log failures.
        }
        return S_OK;
    }

    template <typename TIOperation, typename TIResults>
    HRESULT WaitForCompletion(_In_ TIOperation operation, _Out_ TIResults result, COWAIT_FLAGS flags,
        DWORD timeoutValue, _Out_opt_ bool* timedOut) WI_NOEXCEPT
    {
        RETURN_IF_FAILED_EXPECTED(details::WaitForCompletion(operation, flags, timeoutValue, timedOut));
        return operation->GetResults(result);
    }
}
/// @endcond

/** Set the completion callback for an async operation to run a caller provided function.
Once complete the function is called with the error code result of the operation
and the async operation object. That can be used to retrieve the result of the operation
if there is one.
The function parameter list must be (HRESULT hr, IResultInterface* operation)
~~~
RunWhenComplete<StorageFile*>(getFileOp.Get(), [](HRESULT result, IStorageFile* file) -> void
{

});
~~~
for an agile callback use Microsoft::WRL::FtmBase
~~~
RunWhenComplete<StorageFile*, FtmBase>(getFileOp.Get(), [](HRESULT result, IStorageFile* file) -> void
{

});
~~~
Using the non throwing form:
~~~
hr = RunWhenCompleteNoThrow<StorageFile*>(getFileOp.Get(), [](HRESULT result, IStorageFile* file) -> HRESULT
{

});
~~~
*/

//! Run a fuction when an async operation completes. Use Microsoft::WRL::FtmBase for TAgility to make the completion handler agile and run on the async thread.
template<typename TAgility = IUnknown, typename TFunc>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncAction* operation, TFunc&& func) WI_NOEXCEPT
{
    return details::RunWhenCompleteAction<TAgility>(operation, wistd::function<decltype(func(S_OK))(HRESULT)>(wistd::forward<TFunc>(func)));
}

template<typename TAgility = IUnknown, typename TResult, typename TFunc, typename TAsyncResult = typename wil::details::MapAsyncOpResultType<TResult>::type>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation, TFunc&& func) WI_NOEXCEPT
{
    return details::RunWhenComplete<TAgility>(operation, wistd::function<decltype(func(S_OK, TAsyncResult()))(HRESULT, TAsyncResult)>(wistd::forward<TFunc>(func)));
}

template<typename TAgility = IUnknown, typename TResult, typename TProgress, typename TFunc, typename TAsyncResult = typename wil::details::MapAsyncOpProgressResultType<TResult, TProgress>::type>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation, TFunc&& func) WI_NOEXCEPT
{
    return details::RunWhenComplete<TAgility>(operation, wistd::function<decltype(func(S_OK, TAsyncResult()))(HRESULT, TAsyncResult)>(wistd::forward<TFunc>(func)));
}

template<typename TAgility = IUnknown, typename TProgress, typename TFunc>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncActionWithProgress<TProgress>* operation, TFunc&& func) WI_NOEXCEPT
{
    return details::RunWhenCompleteAction<TAgility>(operation, wistd::function<decltype(func(S_OK))(HRESULT)>(wistd::forward<TFunc>(func)));
}

#ifdef WIL_ENABLE_EXCEPTIONS
//! Run a fuction when an async operation completes. Use Microsoft::WRL::FtmBase for TAgility to make the completion handler agile and run on the async thread.
template<typename TAgility = IUnknown, typename TFunc>
void RunWhenComplete(_In_ ABI::Windows::Foundation::IAsyncAction* operation, TFunc&& func)
{
    THROW_IF_FAILED((details::RunWhenCompleteAction<TAgility>(operation, wistd::function<decltype(func(S_OK))(HRESULT)>(wistd::forward<TFunc>(func)))));
}

template<typename TAgility = IUnknown, typename TResult, typename TFunc, typename TAsyncResult = typename wil::details::MapAsyncOpResultType<TResult>::type>
void RunWhenComplete(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation, TFunc&& func)
{
    THROW_IF_FAILED((details::RunWhenComplete<TAgility>(operation, wistd::function<decltype(func(S_OK, TAsyncResult()))(HRESULT, TAsyncResult)>(wistd::forward<TFunc>(func)))));
}

template<typename TAgility = IUnknown, typename TResult, typename TProgress, typename TFunc, typename TAsyncResult = typename wil::details::MapAsyncOpProgressResultType<TResult, TProgress>::type>
void RunWhenComplete(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation, TFunc&& func)
{
    THROW_IF_FAILED((details::RunWhenComplete<TAgility>(operation, wistd::function<decltype(func(S_OK, TAsyncResult()))(HRESULT, TAsyncResult)>(wistd::forward<TFunc>(func)))));
}

template<typename TAgility = IUnknown, typename TProgress, typename TFunc>
void RunWhenComplete(_In_ ABI::Windows::Foundation::IAsyncActionWithProgress<TProgress>* operation, TFunc&& func)
{
    THROW_IF_FAILED((details::RunWhenCompleteAction<TAgility>(operation, wistd::function<decltype(func(S_OK))(HRESULT)>(wistd::forward<TFunc>(func)))));
}
#endif

/** Wait for an asynchronous operation to complete (or be canceled).
Use to synchronously wait on async operations on background threads.
Do not call from UI threads or STA threads as reentrancy will result.
~~~
ComPtr<IAsyncOperation<StorageFile*>> op;
THROW_IF_FAILED(storageFileStatics->GetFileFromPathAsync(HStringReference(path).Get(), &op));
auto file = wil::WaitForCompletion(op.Get());
~~~
*/
template <typename TAsync = ABI::Windows::Foundation::IAsyncAction>
inline HRESULT WaitForCompletionNoThrow(_In_ TAsync* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    return details::WaitForCompletion(operation, flags, INFINITE, nullptr);
}

// These forms return the result from the async operation

template <typename TResult>
HRESULT WaitForCompletionNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation,
    _Out_ typename wil::details::MapAsyncOpResultType<TResult>::type* result,
    COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    return details::WaitForCompletion(operation, result, flags, INFINITE, nullptr);
}

template <typename TResult, typename TProgress>
HRESULT WaitForCompletionNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation,
    _Out_ typename wil::details::MapAsyncOpProgressResultType<TResult, TProgress>::type* result,
    COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    return details::WaitForCompletion(operation, result, flags, INFINITE, nullptr);
}

// Same as above, but allows caller to specify a timeout value.
// On timeout, S_OK is returned, with timedOut set to true.

template <typename TAsync = ABI::Windows::Foundation::IAsyncAction>
inline HRESULT WaitForCompletionOrTimeoutNoThrow(_In_ TAsync* operation,
    DWORD timeoutValue, _Out_ bool* timedOut, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    return details::WaitForCompletion(operation, flags, timeoutValue, timedOut);
}

template <typename TResult>
HRESULT WaitForCompletionOrTimeoutNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation,
    _Out_ typename wil::details::MapAsyncOpResultType<TResult>::type* result,
    DWORD timeoutValue, _Out_ bool* timedOut, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    return details::WaitForCompletion(operation, result, flags, timeoutValue, timedOut);
}

template <typename TResult, typename TProgress>
HRESULT WaitForCompletionOrTimeoutNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation,
    _Out_ typename wil::details::MapAsyncOpProgressResultType<TResult, TProgress>::type* result,
    DWORD timeoutValue, _Out_ bool* timedOut, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    return details::WaitForCompletion(operation, result, flags, timeoutValue, timedOut);
}

#ifdef WIL_ENABLE_EXCEPTIONS
//! Wait for an asynchronous operation to complete (or be canceled).
template <typename TAsync = ABI::Windows::Foundation::IAsyncAction>
inline void WaitForCompletion(_In_ TAsync* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS)
{
    THROW_IF_FAILED(details::WaitForCompletion(operation, flags, INFINITE, nullptr));
}

template <typename TResult, typename TReturn = typename wil::details::MapToSmartType<typename wil::details::MapAsyncOpResultType<TResult>::type>::type>
TReturn
WaitForCompletion(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS)
{
    TReturn result;
    THROW_IF_FAILED(details::WaitForCompletion(operation, result.GetAddressOf(), flags, INFINITE, nullptr));
    return result;
}

template <typename TResult, typename TProgress, typename TReturn = typename wil::details::MapToSmartType<typename wil::details::MapAsyncOpResultType<TResult>::type>::type>
TReturn
WaitForCompletion(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS)
{
    TReturn result;
    THROW_IF_FAILED(details::WaitForCompletion(operation, result.GetAddressOf(), flags, INFINITE, nullptr));
    return result;
}

/** Similar to WaitForCompletion but this function encapsulates the creation of the async operation
making usage simpler.
~~~
ComPtr<ILauncherStatics> launcher; // inited somewhere
auto result = CallAndWaitForCompletion(launcher.Get(), &ILauncherStatics::LaunchUriAsync, uri.Get());
~~~
*/
template<typename I, typename ...P, typename ...Args>
auto CallAndWaitForCompletion(I* object, HRESULT(STDMETHODCALLTYPE I::*func)(P...), Args&&... args)
{
    Microsoft::WRL::ComPtr<typename wistd::remove_pointer<typename wistd::remove_pointer<typename details::LastType<P...>::type>::type>::type> op;
    THROW_IF_FAILED((object->*func)(wistd::forward<Args>(args)..., &op));
    return wil::WaitForCompletion(op.Get());
}
#endif

#pragma endregion

#pragma region WinRT object construction
#ifdef WIL_ENABLE_EXCEPTIONS
//! Get a WinRT activation factory object, usually using a IXXXStatics interface.
template <typename TInterface>
com_ptr<TInterface> GetActivationFactory(PCWSTR runtimeClass)
{
    com_ptr<TInterface> result;
    THROW_IF_FAILED(RoGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(runtimeClass).Get(), IID_PPV_ARGS(&result)));
    return result;
}

//! Get a WinRT object.
template <typename TInterface>
com_ptr<TInterface> ActivateInstance(PCWSTR runtimeClass)
{
    com_ptr<IInspectable> result;
    THROW_IF_FAILED(RoActivateInstance(Microsoft::WRL::Wrappers::HStringReference(runtimeClass).Get(), &result));
    return result.query<TInterface>();
}
#endif
#pragma endregion

#pragma region Async production helpers

/// @cond
namespace details
{
    template <typename TResult>
    class SyncAsyncOp WrlFinal : public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::IAsyncOperation<TResult>,
        Microsoft::WRL::AsyncBase<ABI::Windows::Foundation::IAsyncOperationCompletedHandler<TResult>>>
    {
        // typedef typename MapToSmartType<TResult>::type TSmart;
        using RuntimeClassT = typename SyncAsyncOp::RuntimeClassT;
        InspectableClass(__super::z_get_rc_name_impl(), TrustLevel::BaseTrust);
    public:
        HRESULT RuntimeClassInitialize(const TResult& op)
        {
            m_result = op;
            return S_OK;
        }

        IFACEMETHODIMP put_Completed(ABI::Windows::Foundation::IAsyncOperationCompletedHandler<TResult>* competed) override
        {
            competed->Invoke(this, AsyncStatus::Completed);
            return S_OK;
        }

        IFACEMETHODIMP get_Completed(ABI::Windows::Foundation::IAsyncOperationCompletedHandler<TResult>** competed) override
        {
            *competed = nullptr;
            return S_OK;
        }

        IFACEMETHODIMP GetResults(TResult* result) override
        {
            *result = m_result;
            return S_OK;
        }

        HRESULT OnStart() override { return S_OK; }
        void OnClose() override { }
        void OnCancel() override { }
    private:
        // needs to be MapToSmartType<TResult>::type to hold non trial types
        TResult m_result;
    };

    extern const __declspec(selectany) wchar_t SyncAsyncActionName[] = L"SyncActionAction";

    class SyncAsyncActionOp WrlFinal : public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::IAsyncAction,
        Microsoft::WRL::AsyncBase<ABI::Windows::Foundation::IAsyncActionCompletedHandler,
        Microsoft::WRL::Details::Nil,
        Microsoft::WRL::AsyncResultType::SingleResult
#ifndef _WRL_DISABLE_CAUSALITY_
        ,Microsoft::WRL::AsyncCausalityOptions<SyncAsyncActionName>
#endif
        >>
    {
        InspectableClass(InterfaceName_Windows_Foundation_IAsyncAction, TrustLevel::BaseTrust);
    public:
        IFACEMETHODIMP put_Completed(ABI::Windows::Foundation::IAsyncActionCompletedHandler* competed) override
        {
            competed->Invoke(this, AsyncStatus::Completed);
            return S_OK;
        }

        IFACEMETHODIMP get_Completed(ABI::Windows::Foundation::IAsyncActionCompletedHandler** competed) override
        {
            *competed = nullptr;
            return S_OK;
        }

        IFACEMETHODIMP GetResults() override
        {
            return S_OK;
        }

        HRESULT OnStart() override { return S_OK; }
        void OnClose() override { }
        void OnCancel() override { }
    };
}

/// @endcond
//! Creates a WinRT async operation object that implements IAsyncOperation<TResult>. Use mostly for testing and for mocking APIs.
template <typename TResult>
HRESULT MakeSynchronousAsyncOperationNoThrow(ABI::Windows::Foundation::IAsyncOperation<TResult>** result, const TResult& value)
{
    return Microsoft::WRL::MakeAndInitialize<details::SyncAsyncOp<TResult>>(result, value);
}

//! Creates a WinRT async operation object that implements IAsyncOperation<TResult>. Use mostly for testing and for mocking APIs.
inline HRESULT MakeSynchronousAsyncActionNoThrow(ABI::Windows::Foundation::IAsyncAction** result)
{
    return Microsoft::WRL::MakeAndInitialize<details::SyncAsyncActionOp>(result);
}

#ifdef WIL_ENABLE_EXCEPTIONS
//! Creates a WinRT async operation object that implements IAsyncOperation<TResult>. Use mostly for testing and for mocking APIs.
// TODO: map TRealResult and TSmartResult into SyncAsyncOp.
template <typename TResult, typename TRealResult = typename details::MapAsyncOpResultType<TResult>::type, typename TSmartResult = typename details::MapToSmartType<TRealResult>::type>
void MakeSynchronousAsyncOperation(ABI::Windows::Foundation::IAsyncOperation<TResult>** result, const TResult& value)
{
    THROW_IF_FAILED((Microsoft::WRL::MakeAndInitialize<details::SyncAsyncOp<TResult>>(result, value)));
}

//! Creates a WinRT async operation object that implements IAsyncOperation<TResult>. Use mostly for testing and for mocking APIs.
inline void MakeSynchronousAsyncAction(ABI::Windows::Foundation::IAsyncAction** result)
{
    THROW_IF_FAILED((Microsoft::WRL::MakeAndInitialize<details::SyncAsyncActionOp>(result)));
}
#endif
#pragma endregion

#pragma region EventRegistrationToken RAII wrapper

// unique_winrt_event_token[_cx] is an RAII wrapper around EventRegistrationToken. When the unique_winrt_event_token[_cx] is
// destroyed, the event is automatically unregistered. Declare a wil::unique_winrt_event_token[_cx]<T> at the scope the event
// should be registered for (often they are tied to object lifetime), where T is the type of the event sender
//     wil::unique_winrt_event_token_cx<Windows::UI::Xaml::Controls::Button> m_token;
//
// Macros have been defined to register for handling the event and then returning an unique_winrt_event_token[_cx]. These
// macros simply hide the function references for adding and removing the event.
//     C++/CX  m_token = WI_MakeUniqueWinRtEventTokenCx(ExampleEventName, sender, handler);
//     ABI     m_token = WI_MakeUniqueWinRtEventToken(ExampleEventName, sender, handler, &m_token);                 // Exception and failfast
//     ABI     RETURN_IF_FAILED(WI_MakeUniqueWinRtEventTokenNoThrow(ExampleEventName, sender, handler, &m_token));  // No throw variant
//
// When the wrapper is destroyed, the handler will be unregistered. You can explicitly unregister the handler prior.
//     m_token.reset();
//
// You can release the EventRegistrationToken from being managed by the wrapper by calling .release()
//     m_token.release();  // DANGER: no longer being managed
//
// If you just need the value of the EventRegistrationToken you can call .get()
//     m_token.get();
//
// See "onecore\shell\tests\wil\UniqueWinRTEventTokenTests.cpp" for more examples of usage in ABI and C++/CX.

#ifdef __cplusplus_winrt
namespace details
{
    template<typename T> struct remove_reference { typedef T type; };
    template<typename T> struct remove_reference<T^> { typedef T type; };
}

template<typename T>
class unique_winrt_event_token_cx
{
    using removal_func = void(T::*)(Windows::Foundation::EventRegistrationToken);
    using static_removal_func = void(__cdecl *)(Windows::Foundation::EventRegistrationToken);

public:
    unique_winrt_event_token_cx() = default;

    unique_winrt_event_token_cx(Windows::Foundation::EventRegistrationToken token, T^ sender, removal_func removalFunction) WI_NOEXCEPT :
        m_token(token),
        m_weakSender(sender),
        m_removalFunction(removalFunction)
    {}

    unique_winrt_event_token_cx(Windows::Foundation::EventRegistrationToken token, static_removal_func removalFunction) WI_NOEXCEPT :
        m_token(token),
        m_staticRemovalFunction(removalFunction)
    {}

    unique_winrt_event_token_cx(const unique_winrt_event_token_cx&) = delete;
    unique_winrt_event_token_cx& operator=(const unique_winrt_event_token_cx&) = delete;

    unique_winrt_event_token_cx(unique_winrt_event_token_cx&& other) WI_NOEXCEPT :
        m_token(other.m_token),
        m_weakSender(wistd::move(other.m_weakSender)),
        m_removalFunction(other.m_removalFunction),
        m_staticRemovalFunction(other.m_staticRemovalFunction)
    {
        other.m_token = {};
        other.m_weakSender = nullptr;
        other.m_removalFunction = nullptr;
        other.m_staticRemovalFunction = nullptr;
    }

    unique_winrt_event_token_cx& operator=(unique_winrt_event_token_cx&& other) WI_NOEXCEPT
    {
        if (this != wistd::addressof(other))
        {
            reset();

            wistd::swap_wil(m_token, other.m_token);
            wistd::swap_wil(m_weakSender, other.m_weakSender);
            wistd::swap_wil(m_removalFunction, other.m_removalFunction);
            wistd::swap_wil(m_staticRemovalFunction, other.m_staticRemovalFunction);
        }

        return *this;
    }

    ~unique_winrt_event_token_cx() WI_NOEXCEPT
    {
        reset();
    }

    explicit operator bool() const WI_NOEXCEPT
    {
        return (m_token.Value != 0);
    }

    Windows::Foundation::EventRegistrationToken get() const WI_NOEXCEPT
    {
        return m_token;
    }

    void reset() noexcept
    {
        if (m_token.Value != 0)
        {
            if (m_staticRemovalFunction)
            {
                (*m_staticRemovalFunction)(m_token);
            }
            else
            {
                auto resolvedSender = m_weakSender.Resolve<T>();
                if (resolvedSender)
                {
                    (resolvedSender->*m_removalFunction)(m_token);
                }
            }
            release();
        }
    }

    // Stops the wrapper from managing resource and returns the EventRegistrationToken.
    Windows::Foundation::EventRegistrationToken release() WI_NOEXCEPT
    {
        auto token = m_token;
        m_token = {};
        m_weakSender = nullptr;
        m_removalFunction = nullptr;
        m_staticRemovalFunction = nullptr;
        return token;
    }

private:
    Windows::Foundation::EventRegistrationToken m_token = {};
    Platform::WeakReference m_weakSender;
    removal_func m_removalFunction = nullptr;
    static_removal_func m_staticRemovalFunction = nullptr;
};

#endif

template<typename T>
class unique_winrt_event_token
{
    using removal_func = HRESULT(__stdcall T::*)(::EventRegistrationToken);

public:
    unique_winrt_event_token() = default;

    unique_winrt_event_token(::EventRegistrationToken token, T* sender, removal_func removalFunction) WI_NOEXCEPT :
        m_token(token),
        m_removalFunction(removalFunction)
    {
        m_weakSender = wil::com_weak_query_failfast(sender);
    }

    unique_winrt_event_token(const unique_winrt_event_token&) = delete;
    unique_winrt_event_token& operator=(const unique_winrt_event_token&) = delete;

    unique_winrt_event_token(unique_winrt_event_token&& other) WI_NOEXCEPT :
        m_token(other.m_token),
        m_weakSender(wistd::move(other.m_weakSender)),
        m_removalFunction(other.m_removalFunction)
    {
        other.m_token = {};
        other.m_removalFunction = nullptr;
    }

    unique_winrt_event_token& operator=(unique_winrt_event_token&& other) WI_NOEXCEPT
    {
        if (this != wistd::addressof(other))
        {
            reset();

            wistd::swap_wil(m_token, other.m_token);
            wistd::swap_wil(m_weakSender, other.m_weakSender);
            wistd::swap_wil(m_removalFunction, other.m_removalFunction);
        }

        return *this;
    }

    ~unique_winrt_event_token() WI_NOEXCEPT
    {
        reset();
    }

    explicit operator bool() const WI_NOEXCEPT
    {
        return (m_token.value != 0);
    }

    ::EventRegistrationToken get() const WI_NOEXCEPT
    {
        return m_token;
    }

    void reset() WI_NOEXCEPT
    {
        if (m_token.value != 0)
        {
            // If T cannot be QI'ed from the weak object then T is not a COM interface.
            auto resolvedSender = m_weakSender.try_query<T>();
            if (resolvedSender)
            {
                FAIL_FAST_IF_FAILED((resolvedSender.get()->*m_removalFunction)(m_token));
            }
            release();
        }
    }

    // Stops the wrapper from managing resource and returns the EventRegistrationToken.
    ::EventRegistrationToken release() WI_NOEXCEPT
    {
        auto token = m_token;
        m_token = {};
        m_weakSender = nullptr;
        m_removalFunction = nullptr;
        return token;
    }

private:
    ::EventRegistrationToken m_token = {};
    wil::com_weak_ref_failfast m_weakSender;
    removal_func m_removalFunction = nullptr;
};

namespace details
{
#ifdef __cplusplus_winrt

    // Handles registration of the event handler to the subscribing object and then wraps the EventRegistrationToken in unique_winrt_event_token.
    // Not intended to be directly called. Use the WI_MakeUniqueWinRtEventTokenCx macro to abstract away specifying the functions that handle addition and removal.
    template<typename T, typename addition_func, typename removal_func, typename handler>
    inline wil::unique_winrt_event_token_cx<T> make_unique_winrt_event_token_cx(T^ sender, addition_func additionFunc, removal_func removalFunc, handler^ h)
    {
        auto rawToken = (sender->*additionFunc)(h);
        wil::unique_winrt_event_token_cx<T> temp(rawToken, sender, removalFunc);
        return temp;
    }

    template<typename T, typename addition_func, typename removal_func, typename handler>
    inline wil::unique_winrt_event_token_cx<T> make_unique_winrt_static_event_token_cx(addition_func additionFunc, removal_func removalFunc, handler^ h)
    {
        auto rawToken = (*additionFunc)(h);
        wil::unique_winrt_event_token_cx<T> temp(rawToken, removalFunc);
        return temp;
    }

#endif // __cplusplus_winrt

    // Handles registration of the event handler to the subscribing object and then wraps the EventRegistrationToken in unique_winrt_event_token.
    // Not intended to be directly called. Use the WI_MakeUniqueWinRtEventToken macro to abstract away specifying the functions that handle addition and removal.
    template<typename err_policy = wil::err_returncode_policy, typename T, typename addition_func, typename removal_func, typename handler>
    inline auto make_unique_winrt_event_token(T* sender, addition_func additionFunc, removal_func removalFunc, handler h, wil::unique_winrt_event_token<T>* token_reference)
    {
        ::EventRegistrationToken rawToken;
        err_policy::HResult((sender->*additionFunc)(h, &rawToken));
        *token_reference = wil::unique_winrt_event_token<T>(rawToken, sender, removalFunc);
        return err_policy::OK();
    }

    // Overload make function to allow for returning the constructed object when not using HRESULT based code.
    template<typename err_policy = wil::err_returncode_policy, typename T, typename addition_func, typename removal_func, typename handler>
    inline typename wistd::enable_if<!wistd::is_same<err_policy, wil::err_returncode_policy>::value, wil::unique_winrt_event_token<T>>::type
    make_unique_winrt_event_token(T* sender, addition_func additionFunc, removal_func removalFunc, handler h)
    {
        ::EventRegistrationToken rawToken;
        err_policy::HResult((sender->*additionFunc)(h, &rawToken));
        return wil::unique_winrt_event_token<T>(rawToken, sender, removalFunc);
    }

} // namespace details

// Helper macros to abstract function names for event addition and removal.
#ifdef __cplusplus_winrt

#define WI_MakeUniqueWinRtEventTokenCx(_event, _object, _handler) \
    wil::details::make_unique_winrt_event_token_cx( \
        _object, \
        &wil::details::remove_reference<decltype(_object)>::type::##_event##::add, \
        &wil::details::remove_reference<decltype(_object)>::type::##_event##::remove, \
        _handler)

#define WI_MakeUniqueWinRtStaticEventTokenCx(_event, _baseType, _handler) \
    wil::details::make_unique_winrt_static_event_token_cx<_baseType>( \
        &##_baseType##::##_event##::add, \
        &##_baseType##::##_event##::remove, \
        _handler)

#endif // __cplusplus_winrt

#ifdef WIL_ENABLE_EXCEPTIONS

#define WI_MakeUniqueWinRtEventToken(_event, _object, _handler) \
    wil::details::make_unique_winrt_event_token<wil::err_exception_policy>( \
        _object, \
        &wistd::remove_pointer<decltype(##_object##)>::type::add_##_event, \
        &wistd::remove_pointer<decltype(##_object##)>::type::remove_##_event, \
        _handler)

#endif // WIL_ENABLE_EXCEPTIONS

#define WI_MakeUniqueWinRtEventTokenNoThrow(_event, _object, _handler, _token_reference) \
    wil::details::make_unique_winrt_event_token( \
        _object, \
        &wistd::remove_pointer<decltype(##_object##)>::type::add_##_event, \
        &wistd::remove_pointer<decltype(##_object##)>::type::remove_##_event, \
        _handler, \
        _token_reference)

#define WI_MakeUniqueWinRtEventTokenFailFast(_event, _object, _handler) \
    wil::details::make_unique_winrt_event_token<wil::err_failfast_policy>( \
        _object, \
        &wistd::remove_pointer<decltype(##_object##)>::type::add_##_event, \
        &wistd::remove_pointer<decltype(##_object##)>::type::remove_##_event, \
        _handler)

#pragma endregion // EventRegistrationToken RAII wrapper

#pragma region Continue*With async helpers

#if (NTDDI_VERSION >= NTDDI_WINBLUE)

//! Continue*With helper executes provided lambda after specified IAsyncAction\Operation.
//! Lambda gets called on the same thread where previous Async was completed.
//! Continue*With returns new IAsyncAction\Operation that could be tracked for completion.
//! It's quiet useful for scenarios where one needs to build sequence of async operations.
//! When lambda returns another asyncoperation\action next lambda will be called when nested async is completed.
//! AsyncAction\Operation returned from Continue*With has several limitations:
//! -- Doesn't support cancellation
//! -- Doesn't create AsyncAction\OperationWithProgress
//! Examples could be found in unit tests: %SDXROOT%\onecore\shell\tests\wil\WinRTTests.cpp

/// @cond
namespace details
{
    template<typename T>
    struct InnermostTrait
    {
        using type = T;
    };

    template<typename T>
    struct InnermostTrait<ABI::Windows::Foundation::IAsyncOperation<T>>
    {
        using type = typename InnermostTrait<T>::type;
    };

    template<typename T>
    struct InnermostTrait<ABI::Windows::Foundation::IAsyncOperation<T>*>
    {
        using type = typename InnermostTrait<T>::type;
    };

    template<typename T, typename U>
    struct InnermostTrait<ABI::Windows::Foundation::IAsyncOperationWithProgress<T,U>>
    {
        using type = typename InnermostTrait<T>::type;
    };

    template<typename T, typename U>
    struct InnermostTrait<ABI::Windows::Foundation::IAsyncOperationWithProgress<T,U>*>
    {
        using type = typename InnermostTrait<T>::type;
    };

    template <typename T>
    struct AsyncOperationTrait
    {
        AsyncOperationTrait() {
            static_assert(0, "Only IAsyncOperation<T> and IAsyncOperationWithProgress<T,U> are supported.");
        }
    };

    template <typename T>
    struct AsyncOperationTrait<ABI::Windows::Foundation::IAsyncOperation<T>>
    {
        typedef ABI::Windows::Foundation::IAsyncOperation<T> type;
        typedef T result_type;
        typedef typename InnermostTrait<T>::type innermost_type;
    };

    template <typename T, typename U>
    struct AsyncOperationTrait<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, U>>
    {
        typedef ABI::Windows::Foundation::IAsyncOperationWithProgress<T, U> type;
        typedef T result_type;
        typedef typename InnermostTrait<T>::type innermost_type;
    };

    template <typename T>
    struct AsyncActionTrait
    {
        AsyncActionTrait() {
            static_assert(0, "Only IAsyncAction and IAsyncActionWithProgress are supported.");
        }
    };

    template <>
    struct AsyncActionTrait<ABI::Windows::Foundation::IAsyncAction>
    {
        typedef ABI::Windows::Foundation::IAsyncAction type;
    };

    template <typename T>
    struct AsyncActionTrait<ABI::Windows::Foundation::IAsyncActionWithProgress<T>>
    {
        typedef ABI::Windows::Foundation::IAsyncActionWithProgress<T> type;
    };

    template <class TAsync, class TIDelegate, class TCallback>
    class ColdAsyncBase :
        public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRt | Microsoft::WRL::RuntimeClassType::InhibitWeakReference>,
            TAsync,
            ABI::Windows::Foundation::IAsyncInfo,
            Microsoft::WRL::FtmBase>
    {
    public:
        ColdAsyncBase(TCallback&& callback) :
            m_callback(wistd::forward<TCallback>(callback))
        {}

        HRESULT STDMETHODCALLTYPE put_Completed(_In_ TIDelegate *value) override
        {
            RETURN_HR_IF(E_INVALIDARG, value == nullptr);

            TIDelegate* tempHandler = nullptr;
            ABI::Windows::Foundation::AsyncStatus status;
            {
                // Making sure to avoid not calling handler twice or not at all
                auto lock = m_lock.Lock();

                RETURN_HR_IF(E_ILLEGAL_DELEGATE_ASSIGNMENT, m_handler != nullptr);
                m_handler = value;
                tempHandler = value;
                status = m_status;
            }

            if (status != ABI::Windows::Foundation::AsyncStatus::Started)
            {
                RETURN_IF_FAILED(tempHandler->Invoke(this, m_status));
            }
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_Completed(_Out_ TIDelegate **value) override
        {
            auto lock = m_lock.Lock();
            RETURN_IF_FAILED(m_handler.CopyTo(value));
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_Id(_Out_ unsigned __int32 *value) override
        {
            *value = 0;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_Status(_Out_ ABI::Windows::Foundation::AsyncStatus *value) override
        {
            auto lock = m_lock.Lock();
            *value = m_status;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_ErrorCode(_Out_ HRESULT *value) override
        {
            if (m_status == ABI::Windows::Foundation::AsyncStatus::Completed ||
                m_status == ABI::Windows::Foundation::AsyncStatus::Error)
            {
                *value = m_hr;
            }
            else
            {
                return E_ILLEGAL_METHOD_CALL;
            }

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Cancel() override
        {
            // http://osgvsowi/14149514 will add this feature
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE Close() override
        {
            // http://osgvsowi/14149514 will add this feature
            return E_NOTIMPL;
        }
    protected:
        HRESULT m_hr = S_OK;
        ABI::Windows::Foundation::AsyncStatus m_status = ABI::Windows::Foundation::AsyncStatus::Started;
        Microsoft::WRL::Wrappers::CriticalSection m_lock;
        typename wistd::remove_reference<TCallback>::type m_callback;
        Microsoft::WRL::ComPtr<TIDelegate> m_handler;
    };

    template <
        typename TCallback,
        typename TCallbackResult,
        class TIDelegate = ABI::Windows::Foundation::IAsyncActionCompletedHandler>
    class ColdAsyncAction WrlFinal :
        public ColdAsyncBase<ABI::Windows::Foundation::IAsyncAction, TIDelegate, TCallback>
    {
    public:
        ColdAsyncAction(TCallback&& callback) :
            ColdAsyncBase<ABI::Windows::Foundation::IAsyncAction, TIDelegate, TCallback>(wistd::forward<TCallback>(callback))
        {}

        HRESULT STDMETHODCALLTYPE GetResults() override
        {
            return (this->m_status == ABI::Windows::Foundation::AsyncStatus::Completed ||
                this->m_status == ABI::Windows::Foundation::AsyncStatus::Error) ? this->m_hr : E_ILLEGAL_METHOD_CALL;
        }

        template <typename... PrevAsyncResult>
        HRESULT Run(HRESULT hr, _In_ PrevAsyncResult&&... prevAsyncResult)
        {
            FAIL_FAST_IF(this->m_status != ABI::Windows::Foundation::AsyncStatus::Started);

            // Injecting TCallbackResult type to engage SFINAE to reject function overload
            this->m_hr = RunCallback<TCallbackResult>(hr, wistd::forward<PrevAsyncResult>(prevAsyncResult)...);
            RETURN_IF_FAILED(HandleResult());
            return S_OK;
        }
    private:

        template <
            typename TCallbackResult,
            typename... PrevAsyncResult,
            typename wistd::enable_if<wistd::is_same<HRESULT, TCallbackResult>::value, int>::type = 0
        >
        HRESULT RunCallback(HRESULT hr, PrevAsyncResult&&... prevAsyncResult)
        {
            return m_callback(hr, wistd::forward<PrevAsyncResult>(prevAsyncResult)...);
        }

#ifdef WIL_ENABLE_EXCEPTIONS
        template <
            typename TCallbackResult,
            typename... PrevAsyncResult,
            typename wistd::enable_if<wistd::is_same<void, TCallbackResult>::value, int>::type = 0
        >
        HRESULT RunCallback(HRESULT hr, PrevAsyncResult&&... prevAsyncResult)
        {
            try
            {
                m_callback(hr, wistd::forward<PrevAsyncResult>(prevAsyncResult)...);
            }
            CATCH_RETURN();
            return S_OK;
        }
#endif

        HRESULT HandleResult()
        {
            TIDelegate* tempHandler = nullptr;
            {
                auto lock = this->m_lock.Lock();
                this->m_status = SUCCEEDED(this->m_hr) ? ABI::Windows::Foundation::AsyncStatus::Completed : ABI::Windows::Foundation::AsyncStatus::Error;
                tempHandler = this->m_handler.Get();
            }

            if (tempHandler != nullptr)
            {
                RETURN_IF_FAILED(tempHandler->Invoke(this, this->m_status));
            }
            return S_OK;
        }
    };

    template <
        typename TResult,
        typename TCallback,
        typename TCallbackResult,
        class TIDelegate = ABI::Windows::Foundation::IAsyncOperationCompletedHandler<TResult>>
    class ColdAsyncOperation WrlFinal :
        public ColdAsyncBase<ABI::Windows::Foundation::IAsyncOperation<TResult>, TIDelegate, TCallback>
    {
        typedef typename MapAsyncOpResultType<TResult>::type TAbiType;
    public:
        ColdAsyncOperation(TCallback&& callback) :
            ColdAsyncBase<ABI::Windows::Foundation::IAsyncOperation<TResult>, TIDelegate, TCallback>(wistd::forward<TCallback>(callback))
        {}

        HRESULT STDMETHODCALLTYPE GetResults(_Out_ TAbiType *results) override
        {
            if (SUCCEEDED(this->m_hr) && this->m_status == ABI::Windows::Foundation::AsyncStatus::Completed)
            {
                const HRESULT hr = HandleGetResults(results, wistd::is_base_of<IUnknown, typename wistd::remove_pointer<TAbiType>::type>{});
                RETURN_IF_FAILED(hr);
            }
            return (this->m_status == ABI::Windows::Foundation::AsyncStatus::Completed ||
                this->m_status == ABI::Windows::Foundation::AsyncStatus::Error) ? this->m_hr : E_ILLEGAL_METHOD_CALL;
        }

        template <typename... PrevAsyncResult>
        HRESULT Run(HRESULT hr, _In_ PrevAsyncResult&&... prevAsyncResult)
        {
            FAIL_FAST_IF(this->m_status != ABI::Windows::Foundation::AsyncStatus::Started);

            // Injecting TCallbackResult type to engage SFINAE to reject function overload
            this->m_hr = RunCallback<TCallbackResult>(hr, wistd::forward<PrevAsyncResult>(prevAsyncResult)...);
            RETURN_IF_FAILED(HandleResult());
            return S_OK;
        }

    private:

        template <
            typename TCallbackResult,
            typename... PrevAsyncResult,
            typename wistd::enable_if<wistd::is_same<HRESULT, TCallbackResult>::value, int>::type = 0
        >
        HRESULT RunCallback(HRESULT hr, PrevAsyncResult&&... prevAsyncResult)
        {
            return m_callback(hr, wistd::forward<PrevAsyncResult>(prevAsyncResult)..., m_result.GetAddressOf());
        }

#ifdef WIL_ENABLE_EXCEPTIONS
        template <
            typename TCallbackResult,
            typename... PrevAsyncResult,
            typename wistd::enable_if<wistd::is_same<TAbiType, TCallbackResult>::value, int>::type = 0
        >
        HRESULT RunCallback(HRESULT hr, PrevAsyncResult&&... prevAsyncResult)
        {
            try
            {
                m_result = m_callback(hr, wistd::forward<PrevAsyncResult>(prevAsyncResult)...);
            }
            CATCH_RETURN();
            return S_OK;
        }
#endif

        HRESULT HandleResult()
        {
            TIDelegate* tempHandler = nullptr;
            {
                auto lock = this->m_lock.Lock();
                tempHandler = this->m_handler.Get();
                if (FAILED(this->m_hr))
                {
                    this->m_status = ABI::Windows::Foundation::AsyncStatus::Error;
                }
                else
                {
                    // Wrap into AgileRef only types derived from IUnknown
                    HRESULT hr = HandleResultInternal(wistd::is_base_of<IUnknown, typename wistd::remove_pointer<TAbiType>::type>{});
                    this->m_status = ABI::Windows::Foundation::AsyncStatus::Completed;
                    RETURN_IF_FAILED(hr);
                }
            }

            if (tempHandler != nullptr)
            {
                RETURN_IF_FAILED(tempHandler->Invoke(this, this->m_status));
            }
            return S_OK;
        }

        HRESULT HandleResultInternal(wistd::false_type)
        {
            return S_OK;
        }

        HRESULT HandleResultInternal(wistd::true_type)
        {
            RETURN_IF_FAILED(AsAgile(m_result.Get(), &m_agileRef));
            return S_OK;
        }

        HRESULT HandleGetResults(TAbiType *results, wistd::false_type)
        {
            *results = m_result.Get();
            return S_OK;
        }

        HRESULT HandleGetResults(TAbiType *results, wistd::true_type)
        {
            RETURN_IF_FAILED(m_agileRef.CopyTo(results));
            return S_OK;
        }

        typename MapToSmartType<TAbiType>::type m_result;
        Microsoft::WRL::AgileRef m_agileRef;
    };

    // IAsyncAction
    template<typename TAsyncType1, typename = AsyncActionTrait<TAsyncType1>, typename TCold>
    HRESULT ContinueActionWith(TAsyncType1* async1, TCold* async2)
    {
        typedef wistd::remove_pointer<decltype(GetAsyncDelegateType(async1))>::type TIDelegate;

        auto callback = Microsoft::WRL::Callback<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, TIDelegate, Microsoft::WRL::FtmBase>>(
            [async = Microsoft::WRL::ComPtr<TCold>(async2)](TAsyncType1* action, ABI::Windows::Foundation::AsyncStatus status) -> HRESULT
        {
            HRESULT hr = S_OK;
            if (status == ABI::Windows::Foundation::AsyncStatus::Completed)
            {
                hr = action->GetResults();
            }
            else
            {
                Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncInfo> asyncInfo;
                RETURN_IF_FAILED(action->QueryInterface(IID_PPV_ARGS(&asyncInfo)));
                RETURN_IF_FAILED(asyncInfo->get_ErrorCode(&hr));
            }
            RETURN_IF_FAILED(async->Run(hr));
            return S_OK;
        });

        RETURN_IF_NULL_ALLOC(callback);
        RETURN_IF_FAILED(async1->put_Completed(callback.Get()));
        return S_OK;
    }

    // IAsyncOperation
    template<typename TCold>
    HRESULT ContinueOperationInternal(HRESULT hr, ABI::Windows::Foundation::IAsyncAction* async1, TCold* async2)
    {
        if (FAILED(hr))
        {
            // Short-circuiting
            RETURN_IF_FAILED(async2->Run(hr));
            return hr;
        }

        RETURN_IF_FAILED(ContinueActionWith(async1, async2));
        return S_OK;
    }

    template<typename TAsyncType1, typename TAsyncResult1 = typename AsyncOperationTrait<TAsyncType1>::result_type, typename TCold>
    HRESULT ContinueOperationInternal(HRESULT hr, TAsyncType1* async1, TCold* async2)
    {
        if (FAILED(hr))
        {
            // Short-circuiting
            typedef typename MapAsyncOpResultType<TAsyncResult1>::type TAbiType;
            RETURN_IF_FAILED(async2->Run(hr, TAbiType()));
            return hr;
        }

        RETURN_IF_FAILED(ContinueOperationWith(async1, async2));
        return S_OK;
    }

    template<typename TAsyncResult1, typename TCold>
    HRESULT ContinueOperationInternal(HRESULT hr, TAsyncResult1&& result, TCold* async2)
    {
        RETURN_IF_FAILED(async2->Run(hr, wistd::forward<TAsyncResult1>(result)));
        return S_OK;
    }

    template<typename TAsyncType1, typename TAsyncResult1 = typename AsyncOperationTrait<TAsyncType1>::result_type, typename TCold>
    HRESULT ContinueOperationWith(TAsyncType1* async1, TCold* async2)
    {
        typedef wistd::remove_pointer<decltype(GetAsyncDelegateType(async1))>::type TIDelegate;

        auto callback = Microsoft::WRL::Callback<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, TIDelegate, Microsoft::WRL::FtmBase>>(
            [async = Microsoft::WRL::ComPtr<TCold>(async2)](TAsyncType1* operation, ABI::Windows::Foundation::AsyncStatus status) -> HRESULT
        {
            typename MapToSmartType<typename MapAsyncOpResultType<TAsyncResult1>::type>::type result;

            HRESULT hr = S_OK;
            if (status == ABI::Windows::Foundation::AsyncStatus::Completed)
            {
                hr = operation->GetResults(result.GetAddressOf());
            }
            else
            {
                Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncInfo> asyncInfo;
                RETURN_IF_FAILED(operation->QueryInterface(IID_PPV_ARGS(&asyncInfo)));
                RETURN_IF_FAILED(asyncInfo->get_ErrorCode(&hr));
            }
            RETURN_IF_FAILED(ContinueOperationInternal(hr, result.Get(), async.Get()));
            return S_OK;
        });

        RETURN_IF_NULL_ALLOC(callback);
        RETURN_IF_FAILED(async1->put_Completed(callback.Get()));
        return S_OK;
    }

} // details
/// @endcond

/**
* @brief Creates IAsyncAction that will be executed on the same thread where previous IAsyncAction was completed.
*/
template<
    typename TAsyncType1,
    typename = typename details::AsyncActionTrait<TAsyncType1>::type,
    typename Func>
HRESULT ContinueActionWithNoThrow(TAsyncType1* async1, ABI::Windows::Foundation::IAsyncAction** asyncAction, Func&& func)
{
    FAIL_FAST_IF_NULL(async1);
    auto action = Microsoft::WRL::Make<details::ColdAsyncAction<wistd::function<HRESULT(HRESULT)>, HRESULT>>(wistd::forward<Func>(func));
    RETURN_IF_NULL_ALLOC(action);
    RETURN_IF_FAILED(details::ContinueActionWith(async1, action.Get()));

    *asyncAction = action.Detach();
    return S_OK;
}

/**
* @brief Creates IAsyncOperation<T> that will be executed on the same thread where previous IAsyncAction was completed.
*/
template<
    typename TAsyncType1,
    typename = typename details::AsyncActionTrait<TAsyncType1>::type,
    typename TAsyncResult2,
    typename Func>
HRESULT ContinueActionWithNoThrow(TAsyncType1* async1, ABI::Windows::Foundation::IAsyncOperation<TAsyncResult2>** asyncOperation, Func&& func)
{
    FAIL_FAST_IF_NULL(async1);
    typedef typename details::MapAsyncOpResultType<TAsyncResult2>::type TAbiType2;
    typedef wistd::function<HRESULT(HRESULT, TAbiType2*)> FunctorType;
    auto operation = Microsoft::WRL::Make<details::ColdAsyncOperation<TAsyncResult2, FunctorType, HRESULT>>(wistd::forward<Func>(func));
    RETURN_IF_NULL_ALLOC(operation);
    RETURN_IF_FAILED(details::ContinueActionWith(async1, operation.Get()));

    *asyncOperation = operation.Detach();
    return S_OK;
}

/**
* @brief Creates IAsyncAction that will be executed on the same thread where previous IAsyncOperation<T> was completed.
*/
template<
    typename TAsyncType1,
    typename TAsyncResult1 = typename details::AsyncOperationTrait<TAsyncType1>::result_type,
    typename Func>
HRESULT ContinueOperationWithNoThrow(TAsyncType1* async1, ABI::Windows::Foundation::IAsyncAction** asyncAction, Func&& func)
{
    FAIL_FAST_IF_NULL(async1);
    typedef typename details::InnermostTrait<TAsyncResult1>::type InnermostType;
    typedef typename wistd::conditional<
        wistd::is_same<InnermostType, ABI::Windows::Foundation::IAsyncAction*>::value,
        wistd::function<HRESULT(HRESULT)>,
        wistd::function<HRESULT(HRESULT, typename details::MapAsyncOpResultType<InnermostType>::type)>
    >::type FunctorType;
    auto action = Microsoft::WRL::Make<details::ColdAsyncAction<FunctorType, HRESULT>>(wistd::forward<Func>(func));
    RETURN_IF_NULL_ALLOC(action);
    RETURN_IF_FAILED(details::ContinueOperationWith(async1, action.Get()));

    *asyncAction = action.Detach();
    return S_OK;
}

/**
* @brief Creates IAsyncOperation<T> that will be executed on the same thread where previous IAsyncOperation<T> was completed.
*/
template<
    typename TAsyncType1,
    typename TAsyncResult1 = typename details::AsyncOperationTrait<TAsyncType1>::result_type,
    typename TAsyncResult2,
    typename Func>
HRESULT ContinueOperationWithNoThrow(TAsyncType1* async1, ABI::Windows::Foundation::IAsyncOperation<TAsyncResult2>** asyncOperation, Func&& func)
{
    FAIL_FAST_IF_NULL(async1);
    typedef typename details::MapAsyncOpResultType<TAsyncResult2>::type TAbiType2;
    typedef typename details::InnermostTrait<TAsyncResult1>::type InnermostType;
    typedef typename wistd::conditional<
        wistd::is_same<InnermostType, ABI::Windows::Foundation::IAsyncAction*>::value,
        wistd::function<HRESULT(HRESULT, TAbiType2*)>,
        wistd::function<HRESULT(HRESULT, typename details::MapAsyncOpResultType<InnermostType>::type, TAbiType2*)>
    >::type FunctorType;
    auto operation = Microsoft::WRL::Make<details::ColdAsyncOperation<TAsyncResult2, FunctorType, HRESULT>>(wistd::forward<Func>(func));
    RETURN_IF_NULL_ALLOC(operation);
    RETURN_IF_FAILED(details::ContinueOperationWith(async1, operation.Get()));

    *asyncOperation = operation.Detach();
    return S_OK;
}

#ifdef WIL_ENABLE_EXCEPTIONS
template<
    typename TAsyncType1,
    typename = typename details::AsyncActionTrait<TAsyncType1>::type,
    typename Func
>
Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncAction> ContinueActionWith(TAsyncType1* async1, Func&& func) throw()
{
    FAIL_FAST_IF_NULL(async1);
    auto action = Microsoft::WRL::Make<details::ColdAsyncAction<wistd::function<void(HRESULT)>, void>>(wistd::forward<Func>(func));
    THROW_IF_NULL_ALLOC(action);
    THROW_IF_FAILED(details::ContinueActionWith(async1, action.Get()));
    return action;
}

template<
    typename TAsyncResult2,
    typename TAsyncType1,
    typename = typename details::AsyncActionTrait<TAsyncType1>::type,
    typename Func
>
Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncOperation<TAsyncResult2>> ContinueActionWith(TAsyncType1* async1, Func&& func) throw()
{
    FAIL_FAST_IF_NULL(async1);
    typedef typename details::MapAsyncOpResultType<TAsyncResult2>::type TAbiType2;
    typedef wistd::function<typename details::MapToSmartType<TAbiType2>::type(HRESULT)> FunctorType;
    auto operation = Microsoft::WRL::Make<details::ColdAsyncOperation<TAsyncResult2, FunctorType, TAbiType2>>(wistd::forward<Func>(func));
    THROW_IF_NULL_ALLOC(operation);
    THROW_IF_FAILED(details::ContinueActionWith(async1, operation.Get()));
    return operation;
}

template<
    typename TAsyncType1,
    typename TAsyncResult1 = typename details::AsyncOperationTrait<TAsyncType1>::type,
    typename Func
>
Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncAction> ContinueOperationWith(TAsyncType1* async1, Func&& func) throw()
{
    FAIL_FAST_IF_NULL(async1);
    typedef typename details::InnermostTrait<TAsyncResult1>::type InnermostType;
    typedef typename wistd::conditional<
        wistd::is_same<InnermostType, ABI::Windows::Foundation::IAsyncAction*>::value,
        wistd::function<void(HRESULT)>,
        wistd::function<void(HRESULT, typename details::MapAsyncOpResultType<InnermostType>::type)>
    >::type FunctorType;
    auto action = Microsoft::WRL::Make<details::ColdAsyncAction<FunctorType, void>>(wistd::forward<Func>(func));
    THROW_IF_NULL_ALLOC(action);
    THROW_IF_FAILED(details::ContinueOperationWith(async1, action.Get()));
    return action;
}

template<
    typename TAsyncResult2,
    typename TAsyncType1,
    typename TAsyncResult1 = typename details::AsyncOperationTrait<TAsyncType1>::type,
    typename Func
>
Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncOperation<TAsyncResult2>> ContinueOperationWith(TAsyncType1* async1, Func&& func) throw()
{
    FAIL_FAST_IF_NULL(async1);
    typedef typename details::MapAsyncOpResultType<TAsyncResult2>::type TAbiType2;
    typedef typename details::InnermostTrait<TAsyncResult1>::type InnermostType;
    typedef typename wistd::conditional<
        wistd::is_same<InnermostType, ABI::Windows::Foundation::IAsyncAction*>::value,
        wistd::function<typename details::MapToSmartType<TAbiType2>::type(HRESULT)>,
        wistd::function<typename details::MapToSmartType<TAbiType2>::type(HRESULT, typename details::MapAsyncOpResultType<InnermostType>::type)>
    >::type FunctorType;
    auto operation = Microsoft::WRL::Make<details::ColdAsyncOperation<TAsyncResult2, FunctorType, TAbiType2>>(wistd::forward<Func>(func));
    THROW_IF_NULL_ALLOC(operation);
    THROW_IF_FAILED(details::ContinueOperationWith(async1, operation.Get()));
    return operation;
}
#endif // WIL_ENABLE_EXCEPTIONS

#endif // NTDDI_VERSION >= NTDDI_WINBLUE

#pragma endregion

} // namespace wil

#if (NTDDI_VERSION >= NTDDI_WINBLUE)

template <>
struct ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Foundation::IAsyncAction*> :
    ABI::Windows::Foundation::IAsyncOperation_impl<ABI::Windows::Foundation::IAsyncAction*>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperation<IAsyncAction*>";
    }
};

template <typename P>
struct ABI::Windows::Foundation::IAsyncOperationWithProgress<ABI::Windows::Foundation::IAsyncAction*,P> :
    ABI::Windows::Foundation::IAsyncOperationWithProgress_impl<ABI::Windows::Foundation::IAsyncAction*, P>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationWithProgress<IAsyncAction*,P>";
    }
};

template <typename T>
struct ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Foundation::IAsyncOperation<T>*> :
    ABI::Windows::Foundation::IAsyncOperation_impl<ABI::Windows::Foundation::IAsyncOperation<T>*>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperation<IAsyncOperation<T>*>";
    }
};

template <typename T, typename P>
struct ABI::Windows::Foundation::IAsyncOperationWithProgress<ABI::Windows::Foundation::IAsyncOperation<T>*, P> :
    ABI::Windows::Foundation::IAsyncOperationWithProgress_impl<ABI::Windows::Foundation::IAsyncOperation<T>*, P>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationWithProgress<IAsyncOperation<T>*,P>";
    }
};

template <typename T, typename P>
struct ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*> :
    ABI::Windows::Foundation::IAsyncOperation_impl<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperation<IAsyncOperationWithProgress<T,P>*>";
    }
};

template <typename T, typename P, typename Z>
struct ABI::Windows::Foundation::IAsyncOperationWithProgress<ABI::Windows::Foundation::IAsyncOperationWithProgress<T,P>*, Z> :
    ABI::Windows::Foundation::IAsyncOperationWithProgress_impl<ABI::Windows::Foundation::IAsyncOperationWithProgress<T,P>*, Z>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationWithProgress<IAsyncOperationWithProgress<T,P>*,Z>";
    }
};

template <>
struct ABI::Windows::Foundation::IAsyncOperationCompletedHandler<ABI::Windows::Foundation::IAsyncAction*> :
    ABI::Windows::Foundation::IAsyncOperationCompletedHandler_impl<ABI::Windows::Foundation::IAsyncAction*>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationCompletedHandler<IAsyncAction*>";
    }
};

template <typename P>
struct ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<ABI::Windows::Foundation::IAsyncAction*, P> :
    ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler_impl<ABI::Windows::Foundation::IAsyncAction*, P>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationWithProgressCompletedHandler<IAsyncAction*,P>";
    }
};

template <typename T>
struct ABI::Windows::Foundation::IAsyncOperationCompletedHandler<ABI::Windows::Foundation::IAsyncOperation<T>*> :
    ABI::Windows::Foundation::IAsyncOperationCompletedHandler_impl<ABI::Windows::Foundation::IAsyncOperation<T>*>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationCompletedHandler<IAsyncOperation<T>*>";
    }
};

template <typename T, typename P>
struct ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<ABI::Windows::Foundation::IAsyncOperation<T>*, P> :
    ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler_impl<ABI::Windows::Foundation::IAsyncOperation<T>*, P>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationWithProgressCompletedHandler<IAsyncOperation<T>*,P>";
    }
};

template <typename T, typename P>
struct ABI::Windows::Foundation::IAsyncOperationCompletedHandler<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*> :
    ABI::Windows::Foundation::IAsyncOperationCompletedHandler_impl<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationCompletedHandler<IAsyncOperationWithProgress<T>*>";
    }
};

template <typename T, typename P, typename Z>
struct ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*, Z> :
    ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler_impl<ABI::Windows::Foundation::IAsyncOperationWithProgress<T, P>*, Z>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"IAsyncOperationWithProgressCompletedHandler<IAsyncOperationWithProgress<T,P>*,Z>";
    }
};
#endif // NTDDI_VERSION >= NTDDI_WINBLUE

#if !defined(MIDL_NS_PREFIX) && !defined(____x_ABI_CWindows_CFoundation_CIClosable_FWD_DEFINED__)
// Internal .idl files use the namespace without the ABI prefix. Macro out ABI for that case
#pragma pop_macro("ABI")
#endif

#ifdef WIL_ENABLE_EXCEPTIONS

namespace std
{
    //! Specialization of `std::less` for `Microsoft::WRL::Wrappers::HString` that uses `hstring_less` for the
    //! comparison function object.
    template <>
    struct less<Microsoft::WRL::Wrappers::HString> :
        public wil::hstring_less
    {
    };

    //! Specialization of `std::less` for `wil::unique_hstring` that uses `hstring_less` for the comparison function
    //! object.
    template <>
    struct less<wil::unique_hstring> :
        public wil::hstring_less
    {
    };
}

#endif

#endif // __WIL_WINRT_INCLUDED
