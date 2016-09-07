#pragma once
#include <hstring.h>
#include <wrl\client.h>
#include <wrl\implements.h>
#include <wrl\async.h>
#include <wrl\wrappers\corewrappers.h>
#include "functional.h"
#include "Result.h"
#include "com.h"
#include "Resource.h"
#include <windows.foundation.collections.h>

#ifdef __cplusplus_winrt
#include <collection.h> // bring in the CRT iterator for support for C++ CX code
#endif

// This enables this code to be used in code that uses the ABI prefix or not.
// Code using the public SDK and C++ CX code has the ABI prefix, windows internal
// is built in a way that does not.
#ifdef ____x_Windows_CFoundation_CIClosable_FWD_DEFINED__
// Internal .idl files use the namespace without the ABI prefix. Macro out ABI for that case
#pragma push_macro("ABI")
#undef ABI
#define ABI
#endif

namespace wil
{
#pragma region HSTRING Helpers
    /** Determines if an HSTRING value is a string reference, that is a value that
    has not allocated the buffer yet. This is also known as a "fast path string".
    Duplicating a string reference requires a memory allocation and can fail
    Duplicating non string references can't fail as the internal ref count is incremented.
    */
    inline bool IsStringReference(_In_opt_ HSTRING value)
    {
        return (value != nullptr) && (reinterpret_cast<UINT_PTR>(reinterpret_cast<HSTRING_HEADER*>(value)->Reserved.Reserved1) & 1);
    }

    /** Retrieve the ref count from an HSTRING. Note there are 2 special cases: 1 is returned
    for empty strings (nullptr that represents "") and 0 is returned for string references.
    */
    inline UINT32 GetHStringRefCount(_In_opt_ HSTRING value)
    {
        return IsStringReference(value) ? 0 : (value ? reinterpret_cast<UINT32*>(value)[5] : 1);
    }

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
        // that needs resource management (e.g. PROPVARIANT) that has not specalzied is used.
        //
        // One fix is to use std::is_enum to cover that case and leave the base definition undefined.
        // That base should use static_assert to inform clients how to fix the lack of specalization.
        template<typename T, typename Enable = void> struct MapToSmartType
        {
            struct type // T holder
            {
                operator T() const { return m_value; }
                T Get() const      { return m_value; }
                T* GetAddressOf()  { return &m_value; }
                T* operator&()     { return &m_value; }
                T m_value;
            };
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
            typedef Microsoft::WRL::Wrappers::HString type;
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
                err_policy::HResult(other.m_element.CopyTo(m_element.GetAddressOf()));
                return *this;
            }

            reference operator*()
            {
                err_policy::HResult(m_v->GetAt(m_i, m_element.GetAddressOf()));
                return m_element;
            }

            pointer operator->()
            {
                err_policy::HResult(m_v->GetAt(m_i, m_element.GetAddressOf()));
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
                *m_result = m_v->GetAt(i, m_currentElement.GetAddressOf());
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
                err_policy::HResult(other.m_element.CopyTo(m_element.GetAddressOf()));
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
                err_policy::HResult(m_iterator->get_Current(m_element.GetAddressOf()));
                return m_element;
            }

            pointer operator->()
            {
                err_policy::HResult(m_iterator->get_Current(m_element.GetAddressOf()));
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
                    *m_result = m_iterator->get_Current(m_element.GetAddressOf());
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
                    *m_range->m_result = m_range->m_iterator->get_Current(m_range->m_element.GetAddressOf());
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
#ifndef ____x_Windows_CFoundation_CIClosable_FWD_DEFINED__
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
#ifndef ____x_Windows_CFoundation_CIClosable_FWD_DEFINED__
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
        RETURN_HR(operation->put_Completed(callback.Get()));
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
            details::MapToSmartType<GetAbiType<wistd::remove_pointer<TIOperation>::type::TResult_complex>::type>::type result;

            HRESULT hr = S_OK;
            if (status == AsyncStatus::Completed)
            {
                hr = operation->GetResults(&result);
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
        RETURN_HR(operation->put_Completed(callback.Get()));
    }

    template <typename TIOperation>
    HRESULT WaitForCompletion(_In_ TIOperation operation, COWAIT_FLAGS flags) WI_NOEXCEPT
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

        Microsoft::WRL::ComPtr<CompletionDelegate> completedDelegate;
        RETURN_IF_FAILED(Microsoft::WRL::MakeAndInitialize<CompletionDelegate>(&completedDelegate));
        RETURN_IF_FAILED(operation->put_Completed(completedDelegate.Get()));

        HANDLE handles[] = { completedDelegate->GetEvent() };
        DWORD dwHandleIndex;
        RETURN_IF_FAILED(CoWaitForMultipleHandles(flags, INFINITE, ARRAYSIZE(handles), handles, &dwHandleIndex));

        if (completedDelegate->GetStatus() != AsyncStatus::Completed)
        {
            Microsoft::WRL::ComPtr<IAsyncInfo> asyncInfo;
            operation->QueryInterface(IID_PPV_ARGS(&asyncInfo)); // all must implement this
            HRESULT hr;
            asyncInfo->get_ErrorCode(&hr); // error return ignored, ok?
            RETURN_HR(hr);
        }
        return S_OK;
    }

    template <typename TIOperation, typename TIResults>
    HRESULT WaitForCompletion(_In_ TIOperation operation, _Out_ TIResults result, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
    {
        RETURN_IF_FAILED(details::WaitForCompletion(operation, flags));
        RETURN_HR(operation->GetResults(result));
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
    RETURN_HR((details::RunWhenCompleteAction<TAgility>(operation, wistd::function<decltype(func(S_OK))(HRESULT)>(wistd::forward<TFunc>(func)))));
}

template<typename TAgility = IUnknown, typename TResult, typename TFunc, typename TAsyncResult = typename wil::details::MapAsyncOpResultType<TResult>::type>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation, TFunc&& func) WI_NOEXCEPT
{
    RETURN_HR((details::RunWhenComplete<TAgility>(operation, wistd::function<decltype(func(S_OK, TAsyncResult()))(HRESULT, TAsyncResult)>(wistd::forward<TFunc>(func)))));
}

template<typename TAgility = IUnknown, typename TResult, typename TProgress, typename TFunc, typename TAsyncResult = typename wil::details::MapAsyncOpProgressResultType<TResult, TProgress>::type>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation, TFunc&& func) WI_NOEXCEPT
{
    RETURN_HR((details::RunWhenComplete<TAgility>(operation, wistd::function<decltype(func(S_OK, TAsyncResult()))(HRESULT, TAsyncResult)>(wistd::forward<TFunc>(func)))));
}

template<typename TAgility = IUnknown, typename TProgress, typename TFunc>
HRESULT RunWhenCompleteNoThrow(_In_ ABI::Windows::Foundation::IAsyncActionWithProgress<TProgress>* operation, TFunc&& func) WI_NOEXCEPT
{
    RETURN_HR((details::RunWhenCompleteAction<TAgility>(operation, wistd::function<decltype(func(S_OK))(HRESULT)>(wistd::forward<TFunc>(func)))));
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
    RETURN_HR(details::WaitForCompletion(operation, flags));
}

// These forms return the result from the async operation

template <typename TResult>
HRESULT WaitForCompletionNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation,
    _Out_ typename wil::details::MapAsyncOpResultType<TResult>::type* result,
    COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    RETURN_HR(details::WaitForCompletion(operation, result, flags));
}

template <typename TResult, typename TProgress>
HRESULT WaitForCompletionNoThrow(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation,
    _Out_ typename wil::details::MapAsyncOpProgressResultType<TResult, TProgress>::type* result,
    COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS) WI_NOEXCEPT
{
    RETURN_HR(details::WaitForCompletion(operation, result, flags));
}

#ifdef WIL_ENABLE_EXCEPTIONS
//! Wait for an asynchronous operation to complete (or be canceled).
template <typename TAsync = ABI::Windows::Foundation::IAsyncAction>
inline void WaitForCompletion(_In_ TAsync* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS)
{
    THROW_IF_FAILED(details::WaitForCompletion(operation, flags));
}

template <typename TResult, typename TReturn = typename wil::details::MapToSmartType<typename wil::details::MapAsyncOpResultType<TResult>::type>::type>
TReturn
WaitForCompletion(_In_ ABI::Windows::Foundation::IAsyncOperation<TResult>* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS)
{
    TReturn result;
    THROW_IF_FAILED(details::WaitForCompletion(operation, &result, flags));
    return result;
}

template <typename TResult, typename TProgress, typename TReturn = typename wil::details::MapToSmartType<typename wil::details::MapAsyncOpResultType<TResult>::type>::type>
TReturn
WaitForCompletion(_In_ ABI::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>* operation, COWAIT_FLAGS flags = COWAIT_DISPATCH_CALLS)
{
    TReturn result;
    THROW_IF_FAILED(details::WaitForCompletion(operation, &result, flags));
    return result;
}

/** Similar to WaitForCompletion but this function encapsulates the creation of the async operation
making usage simpler.
~~~
ComPtr<ILauncherStatics> launcher; // inited somewhere
auto result = CallAndWaitForCompletion(launcher.Get(), &ILauncherStatics::LaunchUriAsync, uri.Get());
~~~
*/
template<
    typename I, typename ...P, typename ...Args>
    auto CallAndWaitForCompletion(I* object, HRESULT(STDMETHODCALLTYPE I::*func)(P...), Args&&... args) ->
    typename decltype(details::GetAsyncResultType(static_cast<decltype(details::GetReturnParamPointerType(func))>(nullptr)))
{
    Microsoft::WRL::ComPtr<wistd::remove_pointer<wistd::remove_pointer<details::LastType<P...>::type>::type>::type> op;
    THROW_IF_FAILED((object->*func)(wistd::forward<Args>(args)..., &op));
    return WaitForCompletion(op.Get());
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
        InspectableClass(z_get_rc_name_impl(), TrustLevel::BaseTrust);
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
        Microsoft::WRL::AsyncBase<ABI::Windows::Foundation::IAsyncActionCompletedHandler, Microsoft::WRL::Details::Nil, Microsoft::WRL::AsyncResultType::SingleResult, Microsoft::WRL::AsyncCausalityOptions<SyncAsyncActionName>>>
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

} // namespace wil

#ifdef ____x_Windows_CFoundation_CIClosable_FWD_DEFINED__
// Internal .idl files use the namespace without the ABI prefix. Macro out ABI for that case
#pragma pop_macro("ABI")
#endif
