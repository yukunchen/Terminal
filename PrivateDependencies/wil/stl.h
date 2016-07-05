// Windows Internal Libraries (wil)
//
// wil Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Internal%20Libraries%20for%20C++%20Usage%20Guide.docx?web=1
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)
//
//! @file
//! Windows STL helpers: custom allocators for STL containers

#pragma once

#include "Common.h"
#if defined(WIL_ENABLE_EXCEPTIONS)
namespace std
{
    template<class _Ty>
    class allocator;

    template<class _Ty, class _Alloc>
    class vector;

    template<class _Elem>
    struct char_traits;

    template<class _Elem, class _Traits, class _Alloc>
    class basic_string;
} // namespace std
#endif // WIL_ENABLE_EXCEPTIONS

namespace wil
{

#if defined(WIL_ENABLE_EXCEPTIONS)
    /** Secure allocator for STL containers.
    The `wil::secure_allocator` allocator calls `SecureZeroMemory` before deallocating
    memory. This provides a mechanism for secure STL containers such as `wil::secure_vector`,
    `wil::secure_string`, and `wil::secure_wstring`. */
    template <typename T> 
    struct secure_allocator 
        : public std::allocator<T>
    {
        typedef typename std::allocator<T>::pointer pointer;
        typedef typename std::allocator<T>::size_type size_type;

        template<typename Other> 
        struct rebind
        {
            typedef secure_allocator<Other> other;
        };

        secure_allocator()
            : std::allocator<T>()
        {
        }

        ~secure_allocator() = default;

        secure_allocator(const secure_allocator& a)
            : std::allocator<T>(a)
        {
        }

        template <class U> 
        secure_allocator(const secure_allocator<U>& a)
            : std::allocator<T>(a)
        {
        }

        pointer allocate(size_type n)
        {
            return std::allocator<T>::allocate(n);
        }

        void deallocate(pointer p, size_type n)
        {	
            SecureZeroMemory(p, sizeof(T) * n);
            std::allocator<T>::deallocate(p, n);
        }
    };
    
    //! `wil::secure_vector` will be securely zeroed before deallocation.
    template <typename Type>
    using secure_vector = std::vector<Type, secure_allocator<Type>>;
    //! `wil::secure_wstring` will be securely zeroed before deallocation.
    using secure_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, wil::secure_allocator<wchar_t>>;
    //! `wil::secure_string` will be securely zeroed before deallocation.
    using secure_string = std::basic_string<char, std::char_traits<char>, wil::secure_allocator<char>>;
#endif // WIL_ENABLE_EXCEPTIONS
} // namespace wil