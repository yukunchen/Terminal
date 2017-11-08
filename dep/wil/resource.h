// Windows Internal Libraries (wil)
// Resource.h: WIL Resource Wrappers (RAII) Library
//
// Usage Guidelines:
// "https://osgwiki.com/wiki/WIL_Resource_Wrappers_(RAII)"
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)
//
//! @file
//! WIL Resource Wrappers (RAII): Provides a family of smart pointer patterns and resource wrappers to enable customers to
//! consistently use RAII in all code.

#include "ResultMacros.h"
#include "wistd_memory.h"
#include "wistd_functional.h"

#pragma warning(push)
#pragma warning(disable:26135 26110)    // Missing locking annotation, Caller failing to hold lock
#pragma warning(disable:4714)           // __forceinline not honored

#ifndef __WIL_RESOURCE
#define __WIL_RESOURCE

// Forward declaration
/// @cond
namespace Microsoft
{
    namespace WRL
    {
        template <typename T>
        class ComPtr;
    }
}
/// @endcond

namespace wil
{
    //! This type copies the current value of GetLastError at construction and resets the last error
    //! to that value when it is destroyed.
    //!
    //! This is useful in library code that runs during a value's destructor. If the library code could
    //! inadvertantly change the value of GetLastError (by calling a Win32 API or similar), it should
    //! instantiate a value of this type before calling the library function in order to preserve the
    //! GetLastError value the user would expect.
    //!
    //! This construct exists to hide kernel mode/user mode differences in wil library code.
    //!
    //! Example usage:
    //!
    //!     if (!CreateFile(...))
    //!     {
    //!         auto lastError = wil::last_error_context();
    //!         WriteFile(g_hlog, logdata);
    //!     }
    //!
    class last_error_context
    {
#ifndef WIL_KERNEL_MODE
        bool m_dismissed;
        DWORD m_error;
    public:
        last_error_context() WI_NOEXCEPT :
            m_dismissed(false),
            m_error(::GetLastError())
        {
        }

        last_error_context(last_error_context&& other) WI_NOEXCEPT
        {
            operator=(wistd::move(other));
        }

        last_error_context & operator=(last_error_context&& other) WI_NOEXCEPT
        {
            m_dismissed = other.m_dismissed;
            m_error = other.m_error;

            other.m_dismissed = true;

            return *this;
        }

        ~last_error_context() WI_NOEXCEPT
        {
            if (!m_dismissed)
            {
                ::SetLastError(m_error);
            }
        }

        //! last_error_context doesn't own a concrete resource, so therefore
        //! it just disarms its destructor and returns void.
        void release() WI_NOEXCEPT
        {
            WI_ASSERT(!m_dismissed);
            m_dismissed = true;
        }
#else
    public:
        void release() WI_NOEXCEPT { }
#endif // WIL_KERNEL_MODE
    };

    /// @cond
    namespace details
    {
        typedef wistd::integral_constant<size_t, 0> pointer_access_all;             // get(), release(), addressof(), and '&' are available
        typedef wistd::integral_constant<size_t, 1> pointer_access_noaddress;       // get() and release() are available
        typedef wistd::integral_constant<size_t, 2> pointer_access_none;            // the raw pointer is not available

        template <typename pointer,                                                 // The handle type
            typename close_fn_t,                                              // The handle close function type
            close_fn_t close_fn,                                              //      * and function pointer
            typename pointer_access = pointer_access_all,                     // all, noaddress or none to control pointer method access
            typename pointer_storage = pointer,                               // The type used to store the handle (usually the same as the handle itself)
            pointer invalid = pointer(),                                      // The invalid handle value (default ZERO value)
            typename pointer_invalid = wistd::nullptr_t>                      // nullptr_t if the invalid handle value is compatible with nullptr, otherwise pointer
            struct resource_policy
        {
            typedef pointer_storage pointer_storage;
            typedef pointer pointer;
            typedef pointer_invalid pointer_invalid;
            typedef pointer_access pointer_access;
            __forceinline static pointer_storage invalid_value() WI_NOEXCEPT { return invalid; }
            __forceinline static bool is_valid(pointer_storage value) WI_NOEXCEPT { return (static_cast<pointer>(value) != static_cast<pointer>(invalid)); }
            __forceinline static void close(pointer_storage value) WI_NOEXCEPT { close_fn(value); }

            inline static void close_reset(pointer_storage value) WI_NOEXCEPT
            {
                auto preserveError = last_error_context();
                close_fn(value);
            }
        };


        // This class provides the pointer storage behind the implementation of unique_any_t utilizing the given
        // resource_policy.  It is separate from unique_any_t to allow a type-specific specialization class to plug
        // into the inheritance chain between unique_any_t and unique_storage.  This allows classes like unique_event
        // to be a unique_any formed class, but also expose methods like SetEvent directly.

        template <typename policy>
        class unique_storage
        {
        protected:
            typedef policy policy;
            typedef typename policy::pointer_storage pointer_storage;
            typedef typename policy::pointer pointer;
            typedef unique_storage<policy> base_storage;

            unique_storage() WI_NOEXCEPT :
            m_ptr(policy::invalid_value())
            {
            }

            explicit unique_storage(pointer_storage ptr) WI_NOEXCEPT :
                m_ptr(ptr)
            {
            }

            unique_storage(unique_storage &&other) WI_NOEXCEPT :
                m_ptr(wistd::move(other.m_ptr))
            {
                other.m_ptr = policy::invalid_value();
            }

            ~unique_storage() WI_NOEXCEPT
            {
                if (policy::is_valid(m_ptr))
                {
                    policy::close(m_ptr);
                }
            }

            void replace(unique_storage &&other) WI_NOEXCEPT
            {
                reset(other.m_ptr);
                other.m_ptr = policy::invalid_value();
            }

            bool is_valid() const WI_NOEXCEPT
            {
                return policy::is_valid(m_ptr);
            }

        public:
            void reset(pointer_storage ptr = policy::invalid_value()) WI_NOEXCEPT
            {
                if (policy::is_valid(m_ptr))
                {
                    policy::close_reset(m_ptr);
                }
                m_ptr = ptr;
            }

            void reset(wistd::nullptr_t) WI_NOEXCEPT
            {
                static_assert(wistd::is_same<typename policy::pointer_invalid, wistd::nullptr_t>::value, "reset(nullptr): valid only for handle types using nullptr as the invalid value");
                reset();
            }

            pointer get() const WI_NOEXCEPT
            {
                return static_cast<pointer>(m_ptr);
            }

            pointer_storage release() WI_NOEXCEPT
            {
                static_assert(!wistd::is_same<typename policy::pointer_access, pointer_access_none>::value, "release(): the raw handle value is not available for this resource class");
                auto ptr = m_ptr;
                m_ptr = policy::invalid_value();
                return ptr;
            }

            pointer_storage *addressof() WI_NOEXCEPT
            {
                static_assert(wistd::is_same<typename policy::pointer_access, pointer_access_all>::value, "addressof(): the address of the raw handle is not available for this resource class");
                return &m_ptr;
            }

        private:
            pointer_storage m_ptr;
        };
    } // details
      /// @endcond


      // This class when paired with unique_storage and an optional type-specific specialization class implements
      // the same interface as STL's unique_ptr<> for resource handle types.  It is a non-copyable, yet movable class
      // supporting attach (reset), detach (release), retrieval (get()).

    template <typename storage_t>
    class unique_any_t : public storage_t
    {
    public:
        typedef typename storage_t::policy policy;
        typedef typename policy::pointer_storage pointer_storage;
        typedef typename policy::pointer pointer;

        unique_any_t(unique_any_t const &) = delete;
        unique_any_t& operator=(unique_any_t const &) = delete;

        // Note that the default constructor really shouldn't be needed (taken care of by the forwarding constructor below), but
        // the forwarding constructor causes an internal compiler error when the class is used in a C++ array.  Defining the default
        // constructor independent of the forwarding constructor removes the compiler limitation.
        unique_any_t() = default;

        // forwarding constructor: forwards all 'explicit' and multi-arg constructors to the base class
        template <typename arg1, typename... args_t>
        explicit unique_any_t(arg1 && first, args_t&&... args) :  // should not be WI_NOEXCEPT (may forward to a throwing constructor)
            storage_t(wistd::forward<arg1>(first), wistd::forward<args_t>(args)...)
        {
            static_assert(wistd::is_same<typename policy::pointer_access, details::pointer_access_none>::value ||
                wistd::is_same<typename policy::pointer_access, details::pointer_access_all>::value ||
                wistd::is_same<typename policy::pointer_access, details::pointer_access_noaddress>::value, "pointer_access policy must be a known pointer_access* integral type");
        }

        unique_any_t(wistd::nullptr_t) WI_NOEXCEPT
        {
            static_assert(wistd::is_same<policy::pointer_invalid, wistd::nullptr_t>::value, "nullptr constructor: valid only for handle types using nullptr as the invalid value");
        }

        unique_any_t(unique_any_t &&other) WI_NOEXCEPT :
        storage_t(wistd::move(other))
        {
        }

        unique_any_t& operator=(unique_any_t &&other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                // cast to base_storage to 'skip' calling the (optional) specialization class that provides handle-specific functionality
                storage_t::replace(wistd::move(static_cast<storage_t::base_storage &>(other)));
            }
            return (*this);
        }

        unique_any_t& operator=(wistd::nullptr_t) WI_NOEXCEPT
        {
            static_assert(wistd::is_same<policy::pointer_invalid, wistd::nullptr_t>::value, "nullptr assignment: valid only for handle types using nullptr as the invalid value");
            storage_t::reset();
            return (*this);
        }

        void swap(unique_any_t &other) WI_NOEXCEPT
        {
            unique_any_t self(wistd::move(*this));
            operator=(wistd::move(other));
            other = wistd::move(self);
        }

        explicit operator bool() const WI_NOEXCEPT
        {
            return storage_t::is_valid();
        }

        pointer_storage *operator&() WI_NOEXCEPT
        {
            static_assert(wistd::is_same<typename policy::pointer_access, details::pointer_access_all>::value, "operator & is not available for this handle");
            storage_t::reset();
            return storage_t::addressof();
        }

        pointer get() const WI_NOEXCEPT
        {
            static_assert(!wistd::is_same<typename policy::pointer_access, details::pointer_access_none>::value, "get(): the raw handle value is not available for this resource class");
            return storage_t::get();
        }

        // The following functions are publicly exposed by their inclusion in the unique_storage base class

        // explicit unique_any_t(pointer_storage ptr) WI_NOEXCEPT
        // void reset(pointer_storage ptr = policy::invalid_value()) WI_NOEXCEPT
        // void reset(wistd::nullptr_t) WI_NOEXCEPT
        // pointer_storage release() WI_NOEXCEPT                                        // not exposed for some resource types
        // pointer_storage *addressof() WI_NOEXCEPT                                     // not exposed for some resource types
    };

    template <typename policy>
    void swap(unique_any_t<policy>& left, unique_any_t<policy>& right) WI_NOEXCEPT
    {
        left.swap(right);
    }

    template <typename policy>
    bool operator==(const unique_any_t<policy>& left, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        return (left.get() == right.get());
    }

    template <typename policy>
    bool operator==(const unique_any_t<policy>& left, wistd::nullptr_t) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<typename unique_any_t<policy>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !left;
    }

    template <typename policy>
    bool operator==(wistd::nullptr_t, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<typename unique_any_t<policy>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !right;
    }

    template <typename policy>
    bool operator!=(const unique_any_t<policy>& left, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        return (!(left.get() == right.get()));
    }

    template <typename policy>
    bool operator!=(const unique_any_t<policy>& left, wistd::nullptr_t) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<typename unique_any_t<policy>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !!left;
    }

    template <typename policy>
    bool operator!=(wistd::nullptr_t, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<typename unique_any_t<policy>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !!right;
    }

    template <typename policy>
    bool operator<(const unique_any_t<policy>& left, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        return (left.get() < right.get());
    }

    template <typename policy>
    bool operator>=(const unique_any_t<policy>& left, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        return (!(left < right));
    }

    template <typename policy>
    bool operator>(const unique_any_t<policy>& left, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        return (right < left);
    }

    template <typename policy>
    bool operator<=(const unique_any_t<policy>& left, const unique_any_t<policy>& right) WI_NOEXCEPT
    {
        return (!(right < left));
    }

    // unique_any provides a template alias for easily building a unique_any_t from a unique_storage class with the given
    // template parameters for resource_policy.

    template <typename pointer,                                         // The handle type
        typename close_fn_t,                                      // The handle close function type
        close_fn_t close_fn,                                      //      * and function pointer
        typename pointer_access = details::pointer_access_all,    // all, noaddress or none to control pointer method access
        typename pointer_storage = pointer,                       // The type used to store the handle (usually the same as the handle itself)
        pointer invalid = pointer(),                              // The invalid handle value (default ZERO value)
        typename pointer_invalid = wistd::nullptr_t>              // nullptr_t if the invalid handle value is compatible with nullptr, otherwise pointer
        using unique_any = unique_any_t<details::unique_storage<details::resource_policy<pointer, close_fn_t, close_fn, pointer_access, pointer_storage, invalid, pointer_invalid>>>;

    /// @cond
    namespace details
    {
        template <typename TLambda>
        class lambda_call
        {
        public:
            lambda_call(const lambda_call&) = delete;
            lambda_call& operator=(const lambda_call&) = delete;
            lambda_call& operator=(lambda_call&& other) = delete;

            explicit lambda_call(TLambda&& lambda) WI_NOEXCEPT : m_lambda(wistd::move(lambda))
            {
                static_assert(wistd::is_same<decltype(lambda()), void>::value, "scope_exit lambdas must not have a return value");
                static_assert(!wistd::is_lvalue_reference<TLambda>::value && !wistd::is_rvalue_reference<TLambda>::value,
                    "scope_exit should only be directly used with a lambda");
            }

            lambda_call(lambda_call&& other) WI_NOEXCEPT : m_lambda(wistd::move(other.m_lambda)), m_call(other.m_call)
            {
                other.m_call = false;
            }

            ~lambda_call() WI_NOEXCEPT
            {
                reset();
            }

            // Ensures the scope_exit lambda will not be called
            void release() WI_NOEXCEPT
            {
                m_call = false;
            }

            // Executes the scope_exit lambda immediately if not yet run; ensures it will not run again
            void reset() WI_NOEXCEPT
            {
                if (m_call)
                {
                    m_call = false;
                    m_lambda();
                }
            }

            // Returns true if the scope_exit lambda is still going to be executed
            explicit operator bool() const WI_NOEXCEPT
            {
                return m_call;
            }

        protected:
            TLambda m_lambda;
            bool m_call = true;
        };

#ifdef WIL_ENABLE_EXCEPTIONS
        template <typename TLambda>
        class lambda_call_log
        {
        public:
            lambda_call_log(const lambda_call_log&) = delete;
            lambda_call_log& operator=(const lambda_call_log&) = delete;
            lambda_call_log& operator=(lambda_call_log&& other) = delete;

            explicit lambda_call_log(void* address, const DiagnosticsInfo& info, TLambda&& lambda) WI_NOEXCEPT :
            m_address(address), m_info(info), m_lambda(wistd::move(lambda))
            {
                static_assert(wistd::is_same<decltype(lambda()), void>::value, "scope_exit lambdas must return 'void'");
                static_assert(!wistd::is_lvalue_reference<TLambda>::value && !wistd::is_rvalue_reference<TLambda>::value,
                    "scope_exit should only be directly used with a lambda");
            }

            lambda_call_log(lambda_call_log&& other) WI_NOEXCEPT :
            m_info(other.m_info), m_address(other.m_address), m_lambda(wistd::move(other.m_lambda)), m_call(other.m_call)
            {
                other.m_call = false;
            }

            ~lambda_call_log() WI_NOEXCEPT
            {
                reset();
            }

            // Ensures the scope_exit lambda will not be called
            void release() WI_NOEXCEPT
            {
                m_call = false;
            }

            // Executes the scope_exit lambda immediately if not yet run; ensures it will not run again
            void reset() WI_NOEXCEPT
            {
                if (m_call)
                {
                    m_call = false;
                    try
                    {
                        m_lambda();
                    }
                    catch (...)
                    {
                        ReportFailure_CaughtException(__R_DIAGNOSTICS(m_info), m_address, FailureType::Log);
                    }
                }
            }

            // Returns true if the scope_exit lambda is still going to be executed
            explicit operator bool() const WI_NOEXCEPT
            {
                return m_call;
            }

        private:
            void* m_address;
            DiagnosticsInfo m_info;
            TLambda m_lambda;
            bool m_call = true;
        };
#endif  // WIL_ENABLE_EXCEPTIONS
    }
    /// @endcond

    /** Returns an object that executes the given lambda when destroyed.
    Capture the object with 'auto'; use reset() to execute the lambda early or release() to avoid
    execution.  Exceptions thrown in the lambda will fail-fast; use scope_exit_log to avoid. */
    template <typename TLambda>
    _Check_return_ inline auto scope_exit(TLambda&& lambda) WI_NOEXCEPT
    {
        return details::lambda_call<TLambda>(wistd::forward<TLambda>(lambda));
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Returns an object that executes the given lambda when destroyed; logs exceptions.
    Capture the object with 'auto'; use reset() to execute the lambda early or release() to avoid
    execution.  Exceptions thrown in the lambda will be caught and logged without being propagated. */
    template <typename TLambda>
    _Check_return_ inline __declspec(noinline) auto scope_exit_log(const DiagnosticsInfo& diagnostics, TLambda&& lambda) WI_NOEXCEPT
    {
        return details::lambda_call_log<TLambda>(_ReturnAddress(), diagnostics, wistd::forward<TLambda>(lambda));
    }
#endif

    // Forward declaration...
    template <typename T, typename err_policy>
    class com_ptr_t;

    //! Type traits class that identifies the inner type of any smart pointer.
    template <typename Ptr>
    struct smart_pointer_details
    {
        typedef typename Ptr::pointer pointer;
    };

    /// @cond
    template <typename T>
    struct smart_pointer_details<Microsoft::WRL::ComPtr<T>>
    {
        typedef T* pointer;
    };
    /// @endcond

    /** Generically detaches a raw pointer from any smart pointer.
    Caller takes ownership of the returned raw pointer; calls the correct release(), detach(),
    or Detach() method based on the smart pointer type */
    template <typename TSmartPointer>
    _Check_return_ typename TSmartPointer::pointer detach_from_smart_pointer(TSmartPointer& smartPtr)
    {
        return smartPtr.release();
    }

    /// @cond
    // Generically detaches a raw pointer from any smart pointer
    template <typename T, typename err>
    _Check_return_ T* detach_from_smart_pointer(wil::com_ptr_t<T, err>& smartPtr)
    {
        return smartPtr.detach();
    }

    // Generically detaches a raw pointer from any smart pointer
    template <typename T>
    _Check_return_ T* detach_from_smart_pointer(Microsoft::WRL::ComPtr<T>& smartPtr)
    {
        return smartPtr.Detach();
    }

    template<typename T, typename err> class com_ptr_t; // forward
    namespace details
    {
        // The first two attach_to_smart_pointer() overloads are ambiguous when passed a com_ptr_t.
        // To solve that use this functions return type to elminate the reset form for com_ptr_t.
        template <typename T, typename err> wistd::false_type use_reset(wil::com_ptr_t<T, err>*) { return wistd::false_type(); }
        template <typename T> wistd::true_type use_reset(T*) { return wistd::true_type(); }
    }
    /// @endcond

    /** Generically attach a raw pointer to a compatible smart pointer.
    Calls the correct reset(), attach(), or Attach() method based on samrt pointer type. */
    template <typename TSmartPointer, typename EnableResetForm = wistd::enable_if_t<decltype(details::use_reset(static_cast<TSmartPointer*>(nullptr)))::value>>
    void attach_to_smart_pointer(TSmartPointer& smartPtr, typename TSmartPointer::pointer rawPtr)
    {
        smartPtr.reset(rawPtr);
    }

    /// @cond

    // Generically attach a raw pointer to a compatible smart pointer.
    template <typename T, typename err>
    void attach_to_smart_pointer(wil::com_ptr_t<T, err>& smartPtr, T* rawPtr)
    {
        smartPtr.attach(rawPtr);
    }

    // Generically attach a raw pointer to a compatible smart pointer.
    template <typename T>
    void attach_to_smart_pointer(Microsoft::WRL::ComPtr<T>& smartPtr, T* rawPtr)
    {
        smartPtr.Attach(rawPtr);
    }
    /// @endcond

    //! @ingroup outparam
    /** Detach a smart pointer resource to an optional output pointer parameter.
    Avoids cluttering code with nullptr tests; works generically for any smart pointer */
    template <typename T, typename TSmartPointer>
    inline void detach_to_opt_param(_Out_opt_ T* outParam, TSmartPointer&& smartPtr)
    {
        if (outParam)
        {
            *outParam = detach_from_smart_pointer(smartPtr);
        }
    }

    /// @cond
#ifndef WIL_HIDE_DEPRECATED_1611
    WIL_WARN_DEPRECATED_1611_PRAGMA(DetachToOptParam)

        // DEPRECATED: Use wil::detach_to_opt_param
        template <typename T, typename TSmartPointer>
    inline void DetachToOptParam(_Out_opt_ T* outParam, TSmartPointer&& smartPtr)
    {
        detach_to_opt_param(outParam, wistd::forward<TSmartPointer>(smartPtr));
    }
#endif
    /// @endcond

    /// @cond
    namespace details
    {
        template <typename T>
        struct out_param_t
        {
            typedef typename wil::smart_pointer_details<T>::pointer pointer;
            T &wrapper;
            pointer pRaw;
            bool replace = true;

            out_param_t(_Inout_ T &output) :
                wrapper(output),
                pRaw(nullptr)
            {
            }

            out_param_t(out_param_t&& other) :
                wrapper(other.wrapper),
                pRaw(other.pRaw)
            {
                WI_ASSERT(other.replace);
                other.replace = false;
            }

            operator pointer*()
            {
                WI_ASSERT(replace);
                return &pRaw;
            }

            ~out_param_t()
            {
                if (replace)
                {
                    attach_to_smart_pointer(wrapper, pRaw);
                }
            }

            out_param_t(out_param_t const &other) = delete;
            out_param_t &operator=(out_param_t const &other) = delete;
        };

        template <typename Tcast, typename T>
        struct out_param_ptr_t
        {
            typedef typename wil::smart_pointer_details<T>::pointer pointer;
            T &wrapper;
            pointer pRaw;
            bool replace = true;

            out_param_ptr_t(_Inout_ T &output) :
                wrapper(output),
                pRaw(nullptr)
            {
            }

            out_param_ptr_t(out_param_ptr_t&& other) :
                wrapper(other.wrapper),
                pRaw(other.pRaw)
            {
                WI_ASSERT(other.replace);
                other.replace = false;
            }

            operator Tcast()
            {
                WI_ASSERT(replace);
                return reinterpret_cast<Tcast>(&pRaw);
            }

            ~out_param_ptr_t()
            {
                if (replace)
                {
                    attach_to_smart_pointer(wrapper, pRaw);
                }
            }

            out_param_ptr_t(out_param_ptr_t const &other) = delete;
            out_param_ptr_t &operator=(out_param_ptr_t const &other) = delete;
        };
    } // details
      /// @endcond

      /** Use to retrieve raw out parameter pointers into smart pointers that do not support the '&' operator.
      This avoids multi-step handling of a raw resource to establish the smart pointer.
      Example: `GetFoo(out_param(foo));` */
    template <typename T>
    details::out_param_t<T> out_param(T& p)
    {
        return details::out_param_t<T>(p);
    }

    /** Use to retrieve raw out parameter pointers (with a required cast) into smart pointers that do not support the '&' operator.
    Use only when the smart pointer's &handle is not equal to the output type a function requries, necessitating a cast.
    Example: `wil::out_param_ptr<PSECURITY_DESCRIPTOR*>(securityDescriptor)` */
    template <typename Tcast, typename T>
    details::out_param_ptr_t<Tcast, T> out_param_ptr(T& p)
    {
        return details::out_param_ptr_t<Tcast, T>(p);
    }

    /** Use unique_struct to define an RAII type for a trivial struct that references resources that must be cleaned up.
    Unique_struct wraps a trivial struct using a custom clean up function and, optionally, custom initializer function. If no custom initialier function is defined in the template
    then ZeroMemory is used.
    Unique_struct is modeled off of std::unique_ptr. However, unique_struct inherits from the defined type instead of managing the struct through a private member variable.

    If the type you're wrapping is a system type, you can share the code by declaring it in this file (Resource.h). Send requests to wildisc.
    Otherwise, if the type is local to your project, declare it locally.
    @tparam struct_t The struct you want to manage
    @tparam close_fn_t The type of the function to clean up the struct. Takes one parameter: a pointer of struct_t. Return values are ignored.
    @tparam close_fn The function of type close_fn_t. This is called in the destructor and reset functions.
    @tparam init_fn_t Optional:The type of the function to initialize the struct.  Takes one parameter: a pointer of struct_t. Return values are ignored.
    @tparam init_fn Optional:The function of type init_fn_t. This is called in the constructor, reset, and release functions. The default is ZeroMemory to initialize the struct.

    Defined using the default zero memory initializer
    ~~~
    typedef wil::unique_struct<PROPVARIANT, decltype(&::PropVariantClear), ::PropVariantClear> unique_prop_variant_default_init;

    unique_prop_variant propvariant;
    SomeFunction(&propvariant);
    ~~~

    Defined using a custom initializer
    ~~~
    typedef wil::unique_struct<PROPVARIANT, decltype(&::PropVariantClear), ::PropVariantClear, decltype(&::PropVariantInit), ::PropVariantInit> unique_prop_variant;

    unique_prop_variant propvariant;
    SomeFunction(&propvariant);
    ~~~
    */
    template <typename struct_t, typename close_fn_t, close_fn_t close_fn, typename init_fn_t = wistd::nullptr_t, init_fn_t init_fn = wistd::nullptr_t()>
    class unique_struct : public struct_t
    {
    public:
        //! Initializes the managed struct using the user-provided initialization function, or ZeroMemory if no function is specified
        unique_struct()
        {
            call_init(use_default_init_fn());
        }

        //! Takes ownership of the struct by doing a shallow copy. Must explicitly be type struct_t
        explicit unique_struct(const struct_t& other) WI_NOEXCEPT :
        struct_t(other)
        {}

        //! Initializes the managed struct by taking the ownership of the other managed struct
        //! Then resets the other managed struct by calling the custom close function
        unique_struct(unique_struct&& other) WI_NOEXCEPT :
        struct_t(other.release())
        {}

        //! Resets this managed struct by calling the custom close function and takes ownership of the other managed struct
        //! Then resets the other managed struct by calling the custom close function
        unique_struct & operator=(unique_struct&& other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                reset(other.release());
            }
            return *this;
        }

        //! Calls the custom close function
        ~unique_struct() WI_NOEXCEPT
        {
            close_fn(this);
        }

        void reset(const unique_struct&) = delete;

        //! Resets this managed struct by calling the custom close function and begins management of the other struct
        void reset(const struct_t& other) WI_NOEXCEPT
        {
            {
                auto preserveError = last_error_context();
                close_fn(this);
            }
            struct_t::operator=(other);
        }

        //! Resets this managed struct by calling the custom close function
        //! Then initializes this managed struct using the user-provided initialization function, or ZeroMemory if no function is specified
        void reset() WI_NOEXCEPT
        {
            close_fn(this);
            call_init(use_default_init_fn());
        }

        void swap(struct_t&) = delete;

        //! Swaps the managed structs
        void swap(unique_struct& other) WI_NOEXCEPT
        {
            struct_t self(*this);
            struct_t::operator=(other);
            *(other.addressof()) = self;
        }

        //! Returns the managed struct
        //! Then initializes this managed struct using the user-provided initialization function, or ZeroMemory if no function is specified
        struct_t release() WI_NOEXCEPT
        {
            struct_t value(*this);
            call_init(use_default_init_fn());
            return value;
        }

        //! Returns address of the managed struct
        struct_t * addressof() WI_NOEXCEPT
        {
            return this;
        }

        //! Resets this managed struct by calling the custom close function
        //! Then initializes this managed struct using the user-provided initialization function, or ZeroMemory if no function is specified
        //! Returns address of the managed struct
        struct_t * reset_and_addressof() WI_NOEXCEPT
        {
            reset();
            return this;
        }

        unique_struct(const unique_struct&) = delete;
        unique_struct& operator=(const unique_struct&) = delete;
        unique_struct& operator=(const struct_t&) = delete;

    private:
        typedef typename wistd::is_same<init_fn_t, wistd::nullptr_t>::type use_default_init_fn;

        void call_init(wistd::true_type)
        {
            RtlZeroMemory(this, sizeof(*this));
        }

        void call_init(wistd::false_type)
        {
            init_fn(this);
        }
    };

    struct empty_deleter
    {
        template <typename T>
        void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T) const
        {
        }
    };

    /** unique_any_array_ptr is a RAII type for managing conformant arrays that need to be freed and have elements that may need to be freed.
    The intented use for this RAII type would be to capture out params from API like IPropertyValue::GetStringArray.
    This class also maintains the size of the array, so it can iterate over the members and deallocate them before it deallocates the base array pointer.

    If the type you're wrapping is a system type, you can share the code by declaring it in this file (Resource.h). Send requests to wildisc.
    Otherwise, if the type is local to your project, declare it locally.

    @tparam ValueType: The type of array you want to manage.
    @tparam ArrayDeleter: The type of the function to clean up the array. Takes one parameter of type T[] or T*. Return values are ignored. This is called in the destructor and reset functions.
    @tparam ElementDeleter: The type of the function to clean up the array elements. Takes one parameter of type T. Return values are ignored. This is called in the destructor and reset functions.

    ~~~
    void GetSomeArray(_Out_ size_t*, _Out_ NOTMYTYPE**);

    struct not_my_deleter
    {
    void operator()(NOTMYTYPE p) const
    {
    destroy(p);
    }
    };

    wil::unique_any_array_ptr<NOTMYTYPE, ::CoTaskMemFree, not_my_deleter> myArray;
    GetSomeArray(myArray.size_address(), &myArray);
    ~~~ */
    template <typename ValueType, typename ArrayDeleter, typename ElementDeleter = empty_deleter>
    class unique_any_array_ptr
    {
    public:
        typedef ValueType value_type;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef ValueType *pointer;
        typedef const ValueType *const_pointer;
        typedef ValueType& reference;
        typedef const ValueType& const_reference;

        typedef ValueType* iterator;
        typedef const ValueType* const_iterator;

        unique_any_array_ptr() = default;
        unique_any_array_ptr(const unique_any_array_ptr&) = delete;
        unique_any_array_ptr& operator=(const unique_any_array_ptr&) = delete;

        unique_any_array_ptr(wistd::nullptr_t) WI_NOEXCEPT
        {
        }

        unique_any_array_ptr& operator=(wistd::nullptr_t) WI_NOEXCEPT
        {
            reset();
            return *this;
        }

        unique_any_array_ptr(pointer ptr, size_t size) WI_NOEXCEPT : m_ptr(ptr), m_size(size)
        {
        }

        unique_any_array_ptr(unique_any_array_ptr&& other) WI_NOEXCEPT : m_ptr(other.m_ptr), m_size(other.m_size)
        {
            other.m_ptr = nullptr;
            other.m_size = size_type{};
        }

        unique_any_array_ptr& operator=(unique_any_array_ptr&& other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                reset();
                swap(other);
            }
            return *this;
        }

        ~unique_any_array_ptr() WI_NOEXCEPT
        {
            reset();
        }

        void swap(unique_any_array_ptr& other) WI_NOEXCEPT
        {
            auto ptr = m_ptr;
            auto size = m_size;
            m_ptr = other.m_ptr;
            m_size = other.m_size;
            other.m_ptr = ptr;
            other.m_size = size;
        }

        iterator begin() WI_NOEXCEPT
        {
            return (iterator(m_ptr));
        }

        const_iterator begin() const WI_NOEXCEPT
        {
            return (const_iterator(m_ptr));
        }

        iterator end() WI_NOEXCEPT
        {
            return (iterator(m_ptr + m_size));
        }

        const_iterator end() const WI_NOEXCEPT
        {
            return (const_iterator(m_ptr + m_size));
        }

        const_iterator cbegin() const WI_NOEXCEPT
        {
            return (begin());
        }

        const_iterator cend() const WI_NOEXCEPT
        {
            return (end());
        }

        size_type size() const WI_NOEXCEPT
        {
            return (m_size);
        }

        bool empty() const WI_NOEXCEPT
        {
            return (size() == size_type{});
        }

        reference operator[](size_type position)
        {
            WI_ASSERT(position < m_size);
            _Analysis_assume_(position < m_size);
            return (m_ptr[position]);
        }

        const_reference operator[](size_type position) const
        {
            WI_ASSERT(position < m_size);
            _Analysis_assume_(position < m_size);
            return (m_ptr[position]);
        }

        reference front()
        {
            WI_ASSERT(!empty());
            return (m_ptr[0]);
        }

        const_reference front() const
        {
            WI_ASSERT(!empty());
            return (m_ptr[0]);
        }

        reference back()
        {
            WI_ASSERT(!empty());
            return (m_ptr[m_size - 1]);
        }

        const_reference back() const
        {
            WI_ASSERT(!empty());
            return (m_ptr[m_size - 1]);
        }

        ValueType* data() WI_NOEXCEPT
        {
            return (m_ptr);
        }

        const ValueType* data() const WI_NOEXCEPT
        {
            return (m_ptr);
        }

        pointer get() const WI_NOEXCEPT
        {
            return m_ptr;
        }

        explicit operator bool() const WI_NOEXCEPT
        {
            return (m_ptr != pointer());
        }

        pointer release() WI_NOEXCEPT
        {
            auto result = m_ptr;
            m_ptr = nullptr;
            m_size = size_type{};
            return result;
        }

        void reset() WI_NOEXCEPT
        {
            if (m_ptr)
            {
                reset_array(ElementDeleter());
                ArrayDeleter()(m_ptr);
                m_ptr = nullptr;
                m_size = size_type{};
            }
        }

        void reset(pointer ptr, size_t size) WI_NOEXCEPT
        {
            reset();
            m_ptr = ptr;
            m_size = size;
        }

        pointer* addressof() WI_NOEXCEPT
        {
            return &m_ptr;
        }

        pointer* operator&() WI_NOEXCEPT
        {
            reset();
            return addressof();
        }

        size_type* size_address() WI_NOEXCEPT
        {
            return &m_size;
        }

        template <typename TSize>
        struct size_address_ptr
        {
            unique_any_array_ptr& wrapper;
            TSize size{};
            bool replace = true;

            size_address_ptr(_Inout_ unique_any_array_ptr& output) :
                wrapper(output)
            {
            }

            size_address_ptr(size_address_ptr&& other) :
                wrapper(other.wrapper),
                size(other.size)
            {
                WI_ASSERT(other.replace);
                other.replace = false;
            }

            operator TSize*()
            {
                WI_ASSERT(replace);
                return &size;
            }

            ~size_address_ptr()
            {
                if (replace)
                {
                    *wrapper.size_address() = static_cast<size_type>(size);
                }
            }

            size_address_ptr(size_address_ptr const &other) = delete;
            size_address_ptr &operator=(size_address_ptr const &other) = delete;
        };

        template <typename T>
        size_address_ptr<T> size_address() WI_NOEXCEPT
        {
            return size_address_ptr<T>(*this);
        }

    private:
        pointer m_ptr = nullptr;
        size_type m_size{};

        void reset_array(const empty_deleter&)
        {
        }

        template <typename T>
        void reset_array(const T& deleter)
        {
            for (auto& element : make_range(m_ptr, m_size))
            {
                deleter(element);
            }
        }
    };

    // forward declaration
    template <typename T, typename err_policy>
    class com_ptr_t;

    /// @cond
    namespace details
    {
        template <typename UniqueAnyType>
        struct unique_any_array_deleter
        {
            template <typename T>
            void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T* p) const
            {
                UniqueAnyType::policy::close_reset(p);
            }
        };

        template <typename close_fn_t, close_fn_t close_fn>
        struct unique_struct_array_deleter
        {
            template <typename T>
            void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T& p) const
            {
                close_fn(&p);
            }
        };

        struct com_unknown_deleter
        {
            template <typename T>
            void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T* p) const
            {
                if (p)
                {
                    p->Release();
                }
            }
        };

        template <class T>
        struct element_traits
        {
            typedef empty_deleter deleter;
            typedef T type;
        };

        template <typename storage_t>
        struct element_traits<unique_any_t<storage_t>>
        {
            typedef unique_any_array_deleter<unique_any_t<storage_t>> deleter;
            typedef typename unique_any_t<storage_t>::pointer type;
        };

        template <typename T, typename err_policy>
        struct element_traits<com_ptr_t<T, err_policy>>
        {
            typedef com_unknown_deleter deleter;
            typedef T* type;
        };

        template <typename struct_t, typename close_fn_t, close_fn_t close_fn, typename init_fn_t, init_fn_t init_fn>
        struct element_traits<unique_struct<struct_t, close_fn_t, close_fn, init_fn_t, init_fn>>
        {
            typedef unique_struct_array_deleter<close_fn_t, close_fn> deleter;
            typedef struct_t type;
        };
    }
    /// @endcond

    template <typename T, typename ArrayDeleter>
    using unique_array_ptr = unique_any_array_ptr<typename details::element_traits<T>::type, ArrayDeleter, typename details::element_traits<T>::deleter>;

    /** Adapter for single-parameter 'free memory' for `wistd::unique_ptr`.
    This struct provides a standard wrapper for calling a platform function to deallocate memory held by a
    `wistd::unique_ptr`, making declaring them as easy as declaring wil::unique_any<>.

    Consider this adapter in preference to `wil::unique_any<>` when the returned type is really a pointer or an
    array of items; `wistd::unique_ptr<>` exposes `operator->()` and `operator[]` for array-typed things safely.
    ~~~~
    EXTERN_C VOID WINAPI MyDllFreeMemory(void* p);
    EXTERN_C HRESULT MyDllGetString(_Outptr_ PWSTR* pString);
    EXTERN_C HRESULT MyDllGetThing(_In_ PCWSTR pString, _Outptr_ PMYSTRUCT* ppThing);
    template<typename T>
    using unique_mydll_ptr = wistd::unique_ptr<T, wil::function_deleter<decltype(&MyDllFreeMemory), MyDllFreeMemory>>;
    HRESULT Test()
    {
    unique_mydll_ptr<WCHAR[]> dllString;
    unique_mydll_ptr<MYSTRUCT> thing;
    RETURN_IF_FAILED(MyDllGetString(wil::out_param(dllString)));
    RETURN_IF_FAILED(MyDllGetThing(dllString.get(), wil::out_param(thing)));
    if (thing->Member)
    {
    // ...
    }
    return S_OK;
    }
    ~~~~ */
    template<typename Q, Q TDeleter> struct function_deleter
    {
        template<typename T> void operator()(_Frees_ptr_opt_ T* toFree) const
        {
            TDeleter(toFree);
        }
    };

    /** Use unique_com_token to define an RAII type for a token-based resource that is managed by a COM interface.
    By comparison, unique_any_t has the requirement that the close function must be static. This works for functions
    such as CloseHandle(), but for any resource cleanup function that relies on a more complex interface,
    unique_com_token can be used.

    @tparam interface_t A COM interface pointer that will manage this resource type.
    @tparam token_t The token type that relates to the COM interface management functions.
    @tparam close_fn_t The type of the function that is called when the resource is destroyed.
    @tparam close_fn The function used to destroy the associated resource. This function should have the signature void(interface_t* source, token_t token).
    @tparam invalid_token Optional:An invalid token value. Defaults to default-constructed token_t().

    Example
    ~~~
    void __stdcall MyInterfaceCloseFunction(IMyInterface* source, DWORD token)
    {
    source->MyCloseFunction(token);
    }
    using unique_my_interface_token = wil::unique_com_token<IMyInterface, DWORD, decltype(MyInterfaceCloseFunction), MyInterfaceCloseFunction, 0xFFFFFFFF>;
    ~~~ */
    template <typename interface_t, typename token_t, typename close_fn_t, close_fn_t close_fn, token_t invalid_token = token_t()>
    class unique_com_token
    {
    public:
        unique_com_token() = default;

        unique_com_token(_In_opt_ interface_t* source, token_t token = invalid_token) WI_NOEXCEPT
        {
            reset(source, token);
        }

        unique_com_token(unique_com_token&& other) WI_NOEXCEPT : m_source(other.m_source), m_token(other.m_token)
        {
            other.m_source = nullptr;
            other.m_token = invalid_token;
        }

        unique_com_token& operator=(unique_com_token&& other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                reset();
                m_source = other.m_source;
                m_token = other.m_token;

                other.m_source = nullptr;
                other.m_token = invalid_token;
            }
            return *this;
        }

        ~unique_com_token() WI_NOEXCEPT
        {
            reset();
        }

        //! Determine if the underlying source and token are valid
        explicit operator bool() const WI_NOEXCEPT
        {
            return (m_token != invalid_token) && m_source;
        }

        //! Associates a new source and releases the existing token if valid
        void associate(_In_opt_ interface_t* source) WI_NOEXCEPT
        {
            reset(source, invalid_token);
        }

        //! Assigns a new source and token
        void reset(_In_opt_ interface_t* source, token_t token) WI_NOEXCEPT
        {
            WI_ASSERT(source || (token == invalid_token));

            // Determine if we need to call the close function on our previous token.
            if (m_token != invalid_token)
            {
                if ((m_source != source) || (m_token != token))
                {
                    close_fn(m_source, m_token);
                }
            }

            m_token = token;

            // Assign our new source and manage the reference counts
            if (m_source != source)
            {
                auto oldSource = m_source;
                m_source = source;

                if (m_source)
                {
                    m_source->AddRef();
                }

                if (oldSource)
                {
                    oldSource->Release();
                }
            }
        }

        //! Assigns a new token without modifying the source; associate must be called first
        void reset(token_t token) WI_NOEXCEPT
        {
            reset(m_source, token);
        }

        //! Closes the token and the releases the reference to the source
        void reset() WI_NOEXCEPT
        {
            reset(nullptr, invalid_token);
        }

        //! Exchanges values with another managed token
        void swap(unique_com_token& other) WI_NOEXCEPT
        {
            wistd::swap_wil(m_source, other.m_source);
            wistd::swap_wil(m_token, other.m_token);
        }

        //! Releases the held token to the caller without closing it and releases the reference to the source.
        //! Requires that the associated COM interface be kept alive externally or the released token may be invalidated
        token_t release() WI_NOEXCEPT
        {
            auto token = m_token;
            m_token = invalid_token;
            reset();
            return token;
        }

        //! Returns address of the managed token; associate must be called first
        token_t* addressof() WI_NOEXCEPT
        {
            WI_ASSERT(m_source);
            return &m_token;
        }

        //! Releases the held token and allows attaching a new token; associate must be called first
        token_t* operator&() WI_NOEXCEPT
        {
            reset(invalid_token);
            return addressof();
        }

        //! Retrieves the token
        token_t get() const WI_NOEXCEPT
        {
            return m_token;
        }

        unique_com_token(const unique_com_token&) = delete;
        unique_com_token& operator=(const unique_com_token&) = delete;

    private:
        interface_t* m_source = nullptr;
        token_t m_token = invalid_token;
    };

#ifndef WIL_HIDE_DEPRECATED_1611
    WIL_WARN_DEPRECATED_1611_PRAGMA(unique_com_token_any)

        // DO NOT USE; to be removed
        template <typename interface_t, typename token_t, typename close_fn_t, close_fn_t close_fn, token_t invalid_token = token_t()>
    using unique_com_token_any = unique_com_token<interface_t, token_t, close_fn_t, close_fn, invalid_token>;
#endif

    /** Use unique_com_call to define an RAII type that demands a particular parameter-less method be called on a COM interface.
    This allows implementing an RAII type that can call a Close() method (think IClosable) or a SetSite(nullptr)
    method (think IObjectWithSite) or some other method when a basic interface call is required as part of the RAII contract.
    see wil::com_set_site in wil\com.h for the IObjectWithSite support.

    @tparam interface_t A COM interface pointer that provides context to make the call.
    @tparam close_fn_t The type of the function that is called to invoke the method.
    @tparam close_fn The function used to invoke the interface method.  This function should have the signature void(interface_t* source).

    Example
    ~~~
    void __stdcall CloseIClosable(IClosable* source)
    {
    source->Close();
    }
    using unique_closable_call = wil::unique_com_call<IClosable, decltype(CloseIClosable), CloseIClosable>;
    ~~~ */
    template <typename interface_t, typename close_fn_t, close_fn_t close_fn>
    class unique_com_call
    {
    public:
        unique_com_call() = default;

        explicit unique_com_call(_In_opt_ interface_t* ptr) WI_NOEXCEPT
        {
            reset(ptr);
        }

        unique_com_call(unique_com_call&& other) WI_NOEXCEPT
        {
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }

        unique_com_call& operator=(unique_com_call&& other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                reset();
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        ~unique_com_call() WI_NOEXCEPT
        {
            reset();
        }

        //! Assigns an interface to make a given call on
        void reset(_In_opt_ interface_t* ptr = nullptr) WI_NOEXCEPT
        {
            if (ptr != m_ptr)
            {
                auto oldSource = m_ptr;
                m_ptr = ptr;
                if (m_ptr)
                {
                    m_ptr->AddRef();
                }
                if (oldSource)
                {
                    close_fn(oldSource);
                    oldSource->Release();
                }
            }
        }

        //! Exchanges values with another class
        void swap(unique_com_call& other) WI_NOEXCEPT
        {
            wistd::swap_wil(m_ptr, other.m_ptr);
        }

        //! Cancel the interface call that this class was expected to make
        void release() WI_NOEXCEPT
        {
            auto ptr = m_ptr;
            m_ptr = nullptr;
            if (ptr)
            {
                ptr->Release();
            }
        }

        //! Returns true if the call this class was expected to make is still outstanding
        explicit operator bool() const WI_NOEXCEPT
        {
            return (m_ptr != nullptr);
        }

        //! Returns address of the internal interface
        interface_t** addressof() WI_NOEXCEPT
        {
            return &m_ptr;
        }

        //! Releases the held interface (first performing the interface call if required)
        //! and allows attaching a new interface
        interface_t** operator&() WI_NOEXCEPT
        {
            reset();
            return addressof();
        }

        unique_com_call(const unique_com_call&) = delete;
        unique_com_call& operator=(const unique_com_call&) = delete;

    private:
        interface_t* m_ptr = nullptr;
    };


    /** Use unique_call to define an RAII type that demands a particular parameter-less global function be called.
    This allows implementing a RAII types that can call methods like CoUninitialize.

    @tparam close_fn_t The type of the function that is called to invoke the call.
    @tparam close_fn The function used to invoke the call.  This function should have the signature void().

    Example
    ~~~
    void __stdcall CoUninitializeFunction()
    {
    ::CoUninitialize();
    }
    using unique_couninitialize_call = wil::unique_call<decltype(CoUninitializeFunction), CoUninitializeFunction>;
    ~~~ */
    template <typename close_fn_t, close_fn_t close_fn>
    class unique_call
    {
    public:
        unique_call() = default;

        explicit unique_call(bool call) WI_NOEXCEPT : m_call(call)
        {
        }

        unique_call(unique_call&& other) WI_NOEXCEPT
        {
            m_call = other.m_call;
            other.m_call = false;
        }

        unique_call& operator=(unique_call&& other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                reset();
                m_call = other.m_call;
                other.m_call = false;
            }
            return *this;
        }

        ~unique_call() WI_NOEXCEPT
        {
            reset();
        }

        //! Assigns a new ptr and token
        void reset() WI_NOEXCEPT
        {
            auto call = m_call;
            m_call = false;
            if (call)
            {
                close_fn();
            }
        }

        //! Exchanges values with raii class
        void swap(unique_call& other) WI_NOEXCEPT
        {
            wistd::swap_wil(m_call, other.m_call);
        }

        //! Do not make the interface call that was expected of this class
        void release() WI_NOEXCEPT
        {
            m_call = false;
        }

        //! Returns true if the call that was expected is still outstanding
        explicit operator bool() const WI_NOEXCEPT
        {
            return m_call;
        }

        unique_call(const unique_call&) = delete;
        unique_call& operator=(const unique_call&) = delete;

    private:
        bool m_call = true;
    };

} // namespace wil
#endif // __WIL_RESOURCE


  // Hash deferral function for unique_any_t
#if (defined(_UNORDERED_SET_) || defined(_UNORDERED_MAP_)) && !defined(__WIL_RESOURCE_UNIQUE_HASH)
#define __WIL_RESOURCE_UNIQUE_HASH
namespace std
{
    template <typename storage_t>
    struct hash<wil::unique_any_t<storage_t>>
    {
        size_t operator()(wil::unique_any_t<storage_t> const &val) const
        {
            return (hash<wil::unique_any_t<storage_t>::pointer>()(val.get()));
        }
    };
}
#endif

// shared_any and weak_any implementation using <memory> STL header
#if defined(_MEMORY_) && defined(WIL_ENABLE_EXCEPTIONS) && !defined(WIL_RESOURCE_STL) && !defined(RESOURCE_SUPPRESS_STL)
#define WIL_RESOURCE_STL
namespace wil {

    template <typename storage_t>
    class weak_any;

    /// @cond
    namespace details
    {
        // This class provides the pointer storage behind the implementation of shared_any_t utilizing the given
        // resource_policy.  It is separate from shared_any_t to allow a type-specific specialization class to plug
        // into the inheritance chain between shared_any_t and shared_storage.  This allows classes like shared_event
        // to be a shared_any formed class, but also expose methods like SetEvent directly.

        template <typename unique_t>
        class shared_storage
        {
        protected:
            typedef typename unique_t unique_t;
            typedef typename unique_t::policy policy;
            typedef typename policy::pointer_storage pointer_storage;
            typedef typename policy::pointer pointer;
            typedef typename shared_storage<unique_t> base_storage;

            shared_storage() = default;

            explicit shared_storage(pointer_storage ptr)
            {
                if (policy::is_valid(ptr))
                {
                    m_ptr = std::make_shared<unique_t>(unique_t(ptr));      // unique_t on the stack to prevent leak on throw
                }
            }

            shared_storage(unique_t &&other)
            {
                if (other)
                {
                    m_ptr = std::make_shared<unique_t>(wistd::move(other));
                }
            }

            shared_storage(const shared_storage &other) WI_NOEXCEPT :
            m_ptr(other.m_ptr)
            {
            }

            shared_storage& operator=(const shared_storage &other) WI_NOEXCEPT
            {
                m_ptr = other.m_ptr;
                return *this;
            }

            shared_storage(shared_storage &&other) WI_NOEXCEPT :
            m_ptr(wistd::move(other.m_ptr))
            {
            }

            shared_storage(std::shared_ptr<unique_t> const &ptr) :
                m_ptr(ptr)
            {
            }

            void replace(shared_storage &&other) WI_NOEXCEPT
            {
                m_ptr = wistd::move(other.m_ptr);
            }

            bool is_valid() const WI_NOEXCEPT
            {
                return (m_ptr && static_cast<bool>(*m_ptr));
            }

        public:
            void reset(pointer_storage ptr = policy::invalid_value())
            {
                if (policy::is_valid(ptr))
                {
                    m_ptr = std::make_shared<unique_t>(unique_t(ptr));      // unique_t on the stack to prevent leak on throw
                }
                else
                {
                    m_ptr = nullptr;
                }
            }

            void reset(unique_t &&other)
            {
                m_ptr = std::make_shared<unique_t>(wistd::move(other));
            }

            void reset(wistd::nullptr_t) WI_NOEXCEPT
            {
                static_assert(wistd::is_same<policy::pointer_invalid, wistd::nullptr_t>::value, "reset(nullptr): valid only for handle types using nullptr as the invalid value");
                reset();
            }

            template <typename allow_t = typename policy::pointer_access, typename wistd::enable_if<!wistd::is_same<allow_t, details::pointer_access_none>::value, int>::type = 0>
            pointer get() const WI_NOEXCEPT
            {
                return (m_ptr ? m_ptr->get() : policy::invalid_value());
            }

            template <typename allow_t = typename policy::pointer_access, typename wistd::enable_if<wistd::is_same<allow_t, details::pointer_access_all>::value, int>::type = 0>
            pointer_storage *addressof()
            {
                if (!m_ptr)
                {
                    m_ptr = std::make_shared<unique_t>();
                }
                return m_ptr->addressof();
            }

            long int use_count() const WI_NOEXCEPT
            {
                return m_ptr.use_count();
            }

            bool unique() const WI_NOEXCEPT
            {
                return m_ptr.unique();
            }

        private:
            template <typename storage_t>
            friend class weak_any;

            std::shared_ptr<unique_t> m_ptr;
        };
    }
    /// @endcond

    // This class when paired with shared_storage and an optional type-specific specialization class implements
    // the same interface as STL's shared_ptr<> for resource handle types.  It is both copyable and movable, supporting
    // weak references and automatic closure of the handle upon release of the last shared_any.

    template <typename storage_t>
    class shared_any_t : public storage_t
    {
    public:
        typedef typename storage_t::policy policy;
        typedef typename policy::pointer_storage pointer_storage;
        typedef typename policy::pointer pointer;
        typedef typename storage_t::unique_t unique_t;

        // default and forwarding constructor: forwards default, all 'explicit' and multi-arg constructors to the base class
        template <typename... args_t>
        explicit shared_any_t(args_t&&... args) :  // should not be WI_NOEXCEPT (may forward to a throwing constructor)
            storage_t(wistd::forward<args_t>(args)...)
        {
        }

        shared_any_t(wistd::nullptr_t) WI_NOEXCEPT
        {
            static_assert(wistd::is_same<policy::pointer_invalid, wistd::nullptr_t>::value, "nullptr constructor: valid only for handle types using nullptr as the invalid value");
        }

        shared_any_t(shared_any_t &&other) WI_NOEXCEPT :
        storage_t(wistd::move(other))
        {
        }

        shared_any_t(const shared_any_t &other) WI_NOEXCEPT :
            storage_t(other)
        {
        }

        shared_any_t& operator=(shared_any_t &&other) WI_NOEXCEPT
        {
            if (this != wistd::addressof(other))
            {
                replace(wistd::move(static_cast<base_storage &>(other)));
            }
            return (*this);
        }

        shared_any_t& operator=(const shared_any_t& other) WI_NOEXCEPT
        {
            storage_t::operator=(other);
            return (*this);
        }

        shared_any_t(unique_t &&other) :
            storage_t(wistd::move(other))
        {
        }

        shared_any_t& operator=(unique_t &&other)
        {
            reset(wistd::move(other));
            return (*this);
        }

        shared_any_t& operator=(wistd::nullptr_t) WI_NOEXCEPT
        {
            static_assert(wistd::is_same<policy::pointer_invalid, wistd::nullptr_t>::value, "nullptr assignment: valid only for handle types using nullptr as the invalid value");
            reset();
            return (*this);
        }

        void swap(shared_any_t &other) WI_NOEXCEPT
        {
            shared_any_t self(wistd::move(*this));
            operator=(wistd::move(other));
            other = wistd::move(self);
        }

        explicit operator bool() const WI_NOEXCEPT
        {
            return is_valid();
        }

        pointer_storage *operator&()
        {
            static_assert(wistd::is_same<policy::pointer_access, details::pointer_access_all>::value, "operator & is not available for this handle");
            reset();
            return addressof();
        }

        pointer get() const WI_NOEXCEPT
        {
            static_assert(!wistd::is_same<policy::pointer_access, details::pointer_access_none>::value, "get(): the raw handle value is not available for this resource class");
            return storage_t::get();
        }

        // The following functions are publicly exposed by their inclusion in the base class

        // void reset(pointer_storage ptr = policy::invalid_value()) WI_NOEXCEPT
        // void reset(wistd::nullptr_t) WI_NOEXCEPT
        // pointer_storage *addressof() WI_NOEXCEPT                                     // (note: not exposed for opaque resource types)
    };

    template <typename unique_t>
    void swap(shared_any_t<unique_t>& left, shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        left.swap(right);
    }

    template <typename unique_t>
    bool operator==(const shared_any_t<unique_t>& left, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        return (left.get() == right.get());
    }

    template <typename unique_t>
    bool operator==(const shared_any_t<unique_t>& left, wistd::nullptr_t) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<shared_any_t<unique_t>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !left;
    }

    template <typename unique_t>
    bool operator==(wistd::nullptr_t, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<shared_any_t<unique_t>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !right;
    }

    template <typename unique_t>
    bool operator!=(const shared_any_t<unique_t>& left, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        return (!(left.get() == right.get()));
    }

    template <typename unique_t>
    bool operator!=(const shared_any_t<unique_t>& left, wistd::nullptr_t) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<shared_any_t<unique_t>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !!left;
    }

    template <typename unique_t>
    bool operator!=(wistd::nullptr_t, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        static_assert(wistd::is_same<shared_any_t<unique_t>::policy::pointer_invalid, wistd::nullptr_t>::value, "the resource class does not use nullptr as an invalid value");
        return !!right;
    }

    template <typename unique_t>
    bool operator<(const shared_any_t<unique_t>& left, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        return (left.get() < right.get());
    }

    template <typename unique_t>
    bool operator>=(const shared_any_t<unique_t>& left, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        return (!(left < right));
    }

    template <typename unique_t>
    bool operator>(const shared_any_t<unique_t>& left, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        return (right < left);
    }

    template <typename unique_t>
    bool operator<=(const shared_any_t<unique_t>& left, const shared_any_t<unique_t>& right) WI_NOEXCEPT
    {
        return (!(right < left));
    }


    // This class provides weak_ptr<> support for shared_any<>, bringing the same weak reference counting and lock() acquire semantics
    // to shared_any.

    template <typename shared_t>
    class weak_any
    {
    public:
        typedef shared_t shared_t;

        weak_any() WI_NOEXCEPT
        {
        }

        weak_any(const shared_t &other) WI_NOEXCEPT :
        m_weakPtr(other.m_ptr)
        {
        }

        weak_any(const weak_any &other) WI_NOEXCEPT :
            m_weakPtr(other.m_weakPtr)
        {
        }

        weak_any& operator=(const weak_any &right) WI_NOEXCEPT
        {
            m_weakPtr = right.m_weakPtr;
            return (*this);
        }

        weak_any& operator=(const shared_t &right) WI_NOEXCEPT
        {
            m_weakPtr = right.m_ptr;
            return (*this);
        }

        void reset() WI_NOEXCEPT
        {
            m_weakPtr.reset();
        }

        void swap(weak_any &other) WI_NOEXCEPT
        {
            m_weakPtr.swap(other.m_weakPtr);
        }

        bool expired() const WI_NOEXCEPT
        {
            return m_weakPtr.expired();
        }

        shared_t lock() const WI_NOEXCEPT
        {
            return shared_t(m_weakPtr.lock());
        }

    private:
        std::weak_ptr<typename shared_t::unique_t> m_weakPtr;
    };

    template <typename shared_t>
    void swap(weak_any<shared_t>& left, weak_any<shared_t>& right) WI_NOEXCEPT
    {
        left.swap(right);
    }

    template <typename unique_t>
    using shared_any = shared_any_t<details::shared_storage<unique_t>>;

} // namespace wil
#endif


#if defined(WIL_RESOURCE_STL) && (defined(_UNORDERED_SET_) || defined(_UNORDERED_MAP_)) && !defined(__WIL_RESOURCE_SHARED_HASH)
#define __WIL_RESOURCE_SHARED_HASH
namespace std
{
    template <typename storage_t>
    struct hash<wil::shared_any_t<storage_t>>
    {
        size_t operator()(wil::shared_any_t<storage_t> const &val) const
        {
            return (hash<wil::shared_any_t<storage_t>::pointer>()(val.get()));
        }
    };
}
#endif


namespace wil
{
#if defined(__NOTHROW_T_DEFINED) && !defined(__WIL__NOTHROW_T_DEFINED)
#define __WIL__NOTHROW_T_DEFINED
    /** Provides `std::make_unique()` semantics for resources allocated in a context that may not throw upon allocation failure.
    `wil::make_unique_nothrow()` is identical to `std::make_unique()` except for the following:
    * It returns `wistd::unique_ptr`, rather than `std::unique_ptr`
    * It returns an empty (null) `wistd::unique_ptr` upon allocation failure, rather than throwing an exception

    Note that `wil::make_unique_nothrow()` is not marked WI_NOEXCEPT as it may be used to create an exception-based class that may throw in its constructor.
    ~~~
    auto foo = wil::make_unique_nothrow<Foo>(fooConstructorParam1, fooConstructorParam2);
    if (foo)
    {
    foo->Bar();
    }
    ~~~
    */
    template <class _Ty, class... _Types>
    inline typename wistd::enable_if<!wistd::is_array<_Ty>::value, wistd::unique_ptr<_Ty> >::type make_unique_nothrow(_Types&&... _Args)
    {
        return (wistd::unique_ptr<_Ty>(new(std::nothrow) _Ty(wistd::forward<_Types>(_Args)...)));
    }

    /** Provides `std::make_unique()` semantics for array resources allocated in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_nothrow<Foo[]>(size); // the default constructor will be called on each Foo object
    if (foos)
    {
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    elem.Bar();
    }
    }
    ~~~
    */
    template <class _Ty>
    inline typename wistd::enable_if<wistd::is_array<_Ty>::value && wistd::extent<_Ty>::value == 0, wistd::unique_ptr<_Ty> >::type make_unique_nothrow(size_t _Size)
    {
        typedef typename wistd::remove_extent<_Ty>::type _Elem;
        return (wistd::unique_ptr<_Ty>(new(std::nothrow) _Elem[_Size]()));
    }

    template <class _Ty, class... _Types>
    typename wistd::enable_if<wistd::extent<_Ty>::value != 0, void>::type make_unique_nothrow(_Types&&...) = delete;

    /** Provides `std::make_unique()` semantics for resources allocated in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_failfast<Foo>(fooConstructorParam1, fooConstructorParam2);
    foo->Bar();
    ~~~
    */
    template <class _Ty, class... _Types>
    inline typename wistd::enable_if<!wistd::is_array<_Ty>::value, wistd::unique_ptr<_Ty> >::type make_unique_failfast(_Types&&... _Args)
    {
#pragma warning(suppress: 28193)    // temporary must be inspected (it is within the called function)
        return (wistd::unique_ptr<_Ty>(FAIL_FAST_IF_NULL_ALLOC(new(std::nothrow) _Ty(wistd::forward<_Types>(_Args)...))));
    }

    /** Provides `std::make_unique()` semantics for array resources allocated in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_nothrow<Foo[]>(size); // the default constructor will be called on each Foo object
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    elem.Bar();
    }
    ~~~
    */
    template <class _Ty>
    inline typename wistd::enable_if<wistd::is_array<_Ty>::value && wistd::extent<_Ty>::value == 0, wistd::unique_ptr<_Ty> >::type make_unique_failfast(size_t _Size)
    {
        typedef typename wistd::remove_extent<_Ty>::type _Elem;
#pragma warning(suppress: 28193)    // temporary must be inspected (it is within the called function)
        return (wistd::unique_ptr<_Ty>(FAIL_FAST_IF_NULL_ALLOC(new(std::nothrow) _Elem[_Size]())));
    }

    template <class _Ty, class... _Types>
    typename wistd::enable_if<wistd::extent<_Ty>::value != 0, void>::type make_unique_failfast(_Types&&...) = delete;
#endif // __WIL__NOTHROW_T_DEFINED

#if defined(_WINBASE_) && !defined(__WIL_WINBASE_)
#define __WIL_WINBASE_
    /// @cond
    namespace details
    {
        inline void __stdcall SetEvent(HANDLE h) WI_NOEXCEPT
        {
            __FAIL_FAST_ASSERT_WIN32_BOOL_FALSE__(::SetEvent(h));
        }

        inline void __stdcall ResetEvent(HANDLE h) WI_NOEXCEPT
        {
            __FAIL_FAST_ASSERT_WIN32_BOOL_FALSE__(::ResetEvent(h));
        }

        inline void __stdcall CloseHandle(HANDLE h) WI_NOEXCEPT
        {
            __FAIL_FAST_ASSERT_WIN32_BOOL_FALSE__(::CloseHandle(h));
        }

        inline void __stdcall ReleaseSemaphore(_In_ HANDLE h) WI_NOEXCEPT
        {
            __FAIL_FAST_ASSERT_WIN32_BOOL_FALSE__(::ReleaseSemaphore(h, 1, nullptr));
        }

        inline void __stdcall ReleaseMutex(_In_ HANDLE h) WI_NOEXCEPT
        {
            __FAIL_FAST_ASSERT_WIN32_BOOL_FALSE__(::ReleaseMutex(h));
        }

        inline void __stdcall CloseTokenLinkedToken(_In_ TOKEN_LINKED_TOKEN* linkedToken) WI_NOEXCEPT
        {
            if (linkedToken->LinkedToken && (linkedToken->LinkedToken != INVALID_HANDLE_VALUE))
            {
                __FAIL_FAST_ASSERT_WIN32_BOOL_FALSE__(::CloseHandle(linkedToken->LinkedToken));
            }
        }

        enum class PendingCallbackCancellationBehavior
        {
            Cancel,
            Wait,
            NoWait,
        };

        template <PendingCallbackCancellationBehavior cancellationBehavior>
        struct DestroyThreadPoolWait
        {
            static void Destroy(_In_ PTP_WAIT threadPoolWait) WI_NOEXCEPT
            {
                ::SetThreadpoolWait(threadPoolWait, nullptr, nullptr);
                ::WaitForThreadpoolWaitCallbacks(threadPoolWait, (cancellationBehavior == PendingCallbackCancellationBehavior::Cancel));
                ::CloseThreadpoolWait(threadPoolWait);
            }
        };

        template <>
        struct DestroyThreadPoolWait<PendingCallbackCancellationBehavior::NoWait>
        {
            static void Destroy(_In_ PTP_WAIT threadPoolWait) WI_NOEXCEPT
            {
                ::CloseThreadpoolWait(threadPoolWait);
            }
        };

        template <PendingCallbackCancellationBehavior cancellationBehavior>
        struct DestroyThreadPoolWork
        {
            static void Destroy(_In_ PTP_WORK threadpoolWork) WI_NOEXCEPT
            {
                ::WaitForThreadpoolWorkCallbacks(threadpoolWork, (cancellationBehavior == PendingCallbackCancellationBehavior::Cancel));
                ::CloseThreadpoolWork(threadpoolWork);
            }
        };

        template <>
        struct DestroyThreadPoolWork<PendingCallbackCancellationBehavior::NoWait>
        {
            static void Destroy(_In_ PTP_WORK threadpoolWork) WI_NOEXCEPT
            {
                ::CloseThreadpoolWork(threadpoolWork);
            }
        };

        // SetThreadpoolTimer(timer, nullptr, 0, 0) will cancel any pending callbacks,
        // then CloseThreadpoolTimer will asynchronusly close the timer if a callback is running.
        template <PendingCallbackCancellationBehavior cancellationBehavior>
        struct DestroyThreadPoolTimer
        {
            static void Destroy(_In_ PTP_TIMER threadpoolTimer) WI_NOEXCEPT
            {
                ::SetThreadpoolTimer(threadpoolTimer, nullptr, 0, 0);
#pragma warning(suppress:4127) // conditional expression is constant
                if (cancellationBehavior != PendingCallbackCancellationBehavior::NoWait)
                {
                    ::WaitForThreadpoolTimerCallbacks(threadpoolTimer, (cancellationBehavior == PendingCallbackCancellationBehavior::Cancel));
                }
                ::CloseThreadpoolTimer(threadpoolTimer);
            }
        };

        // PendingCallbackCancellationBehavior::NoWait explicitly does not block waiting for
        // callbacks when destructing.
        template <>
        struct DestroyThreadPoolTimer<PendingCallbackCancellationBehavior::NoWait>
        {
            static void Destroy(_In_ PTP_TIMER threadpoolTimer) WI_NOEXCEPT
            {
                ::CloseThreadpoolTimer(threadpoolTimer);
            }
        };

        template <PendingCallbackCancellationBehavior cancellationBehavior>
        struct DestroyThreadPoolIo
        {
            static void Destroy(_In_ PTP_IO threadpoolIo) WI_NOEXCEPT
            {
                ::WaitForThreadpoolIoCallbacks(threadpoolIo, (cancellationBehavior == PendingCallbackCancellationBehavior::Cancel));
                ::CloseThreadpoolIo(threadpoolIo);
            }
        };

        template <>
        struct DestroyThreadPoolIo<PendingCallbackCancellationBehavior::NoWait>
        {
            static void Destroy(_In_ PTP_IO threadpoolIo) WI_NOEXCEPT
            {
                ::CloseThreadpoolIo(threadpoolIo);
            }
        };

        template <typename close_fn_t, close_fn_t close_fn>
        struct handle_invalid_resource_policy : resource_policy<HANDLE, close_fn_t, close_fn, details::pointer_access_all, HANDLE, INVALID_HANDLE_VALUE, HANDLE>
        {
            __forceinline static bool is_valid(HANDLE ptr) WI_NOEXCEPT { return ((ptr != INVALID_HANDLE_VALUE) && (ptr != nullptr)); }
        };

        template <typename close_fn_t, close_fn_t close_fn>
        struct handle_null_resource_policy : resource_policy<HANDLE, close_fn_t, close_fn>
        {
            __forceinline static bool is_valid(HANDLE ptr) WI_NOEXCEPT { return ((ptr != nullptr) && (ptr != INVALID_HANDLE_VALUE)); }
        };

        typedef resource_policy<HANDLE, decltype(&details::CloseHandle), details::CloseHandle, details::pointer_access_all> handle_resource_policy;
    }
    /// @endcond

    template <typename close_fn_t, close_fn_t close_fn>
    using unique_any_handle_invalid = unique_any_t<details::unique_storage<details::handle_invalid_resource_policy<close_fn_t, close_fn>>>;

    template <typename close_fn_t, close_fn_t close_fn>
    using unique_any_handle_null = unique_any_t<details::unique_storage<details::handle_null_resource_policy<close_fn_t, close_fn>>>;

    typedef unique_any_handle_invalid<decltype(&::CloseHandle), ::CloseHandle> unique_hfile;
    typedef unique_any_handle_null<decltype(&::CloseHandle), ::CloseHandle> unique_handle;
    typedef unique_any_handle_invalid<decltype(&::FindClose), ::FindClose> unique_hfind;
    typedef unique_any<HMODULE, decltype(&::FreeLibrary), ::FreeLibrary> unique_hmodule;

    typedef unique_struct<TOKEN_LINKED_TOKEN, decltype(&details::CloseTokenLinkedToken), details::CloseTokenLinkedToken> unique_token_linked_token;

    using unique_tool_help_snapshot = unique_hfile;

    typedef unique_any<PTP_WAIT, void(*)(PTP_WAIT), details::DestroyThreadPoolWait<details::PendingCallbackCancellationBehavior::Cancel>::Destroy> unique_threadpool_wait;
    typedef unique_any<PTP_WAIT, void(*)(PTP_WAIT), details::DestroyThreadPoolWait<details::PendingCallbackCancellationBehavior::Wait>::Destroy> unique_threadpool_wait_nocancel;
    typedef unique_any<PTP_WAIT, void(*)(PTP_WAIT), details::DestroyThreadPoolWait<details::PendingCallbackCancellationBehavior::NoWait>::Destroy> unique_threadpool_wait_nowait;
    typedef unique_any<PTP_WORK, void(*)(PTP_WORK), details::DestroyThreadPoolWork<details::PendingCallbackCancellationBehavior::Cancel>::Destroy> unique_threadpool_work;
    typedef unique_any<PTP_WORK, void(*)(PTP_WORK), details::DestroyThreadPoolWork<details::PendingCallbackCancellationBehavior::Wait>::Destroy> unique_threadpool_work_nocancel;
    typedef unique_any<PTP_WORK, void(*)(PTP_WORK), details::DestroyThreadPoolWork<details::PendingCallbackCancellationBehavior::NoWait>::Destroy> unique_threadpool_work_nowait;
    typedef unique_any<PTP_TIMER, void(*)(PTP_TIMER), details::DestroyThreadPoolTimer<details::PendingCallbackCancellationBehavior::Cancel>::Destroy> unique_threadpool_timer;
    typedef unique_any<PTP_TIMER, void(*)(PTP_TIMER), details::DestroyThreadPoolTimer<details::PendingCallbackCancellationBehavior::Wait>::Destroy> unique_threadpool_timer_nocancel;
    typedef unique_any<PTP_TIMER, void(*)(PTP_TIMER), details::DestroyThreadPoolTimer<details::PendingCallbackCancellationBehavior::NoWait>::Destroy> unique_threadpool_timer_nowait;
    typedef unique_any<PTP_IO, void(*)(PTP_IO), details::DestroyThreadPoolIo<details::PendingCallbackCancellationBehavior::Cancel>::Destroy> unique_threadpool_io;
    typedef unique_any<PTP_IO, void(*)(PTP_IO), details::DestroyThreadPoolIo<details::PendingCallbackCancellationBehavior::Wait>::Destroy> unique_threadpool_io_nocancel;
    typedef unique_any<PTP_IO, void(*)(PTP_IO), details::DestroyThreadPoolIo<details::PendingCallbackCancellationBehavior::NoWait>::Destroy> unique_threadpool_io_nowait;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    typedef unique_any_handle_invalid<decltype(&::FindCloseChangeNotification), ::FindCloseChangeNotification> unique_hfind_change;
#endif

    typedef unique_any<HANDLE, decltype(&details::SetEvent), details::SetEvent, details::pointer_access_noaddress> event_set_scope_exit;
    typedef unique_any<HANDLE, decltype(&details::ResetEvent), details::ResetEvent, details::pointer_access_noaddress> event_reset_scope_exit;

    // Guarantees a SetEvent on the given event handle when the returned object goes out of scope
    // Note: call SetEvent early with the reset() method on the returned object or abort the call with the release() method
    inline _Check_return_ event_set_scope_exit SetEvent_scope_exit(HANDLE hEvent) WI_NOEXCEPT
    {
        __FAIL_FAST_ASSERT__(hEvent != nullptr);
        return event_set_scope_exit(hEvent);
    }

    // Guarantees a ResetEvent on the given event handle when the returned object goes out of scope
    // Note: call ResetEvent early with the reset() method on the returned object or abort the call with the release() method
    inline _Check_return_ event_reset_scope_exit ResetEvent_scope_exit(HANDLE hEvent) WI_NOEXCEPT
    {
        __FAIL_FAST_ASSERT__(hEvent != nullptr);
        return event_reset_scope_exit(hEvent);
    }

    // Checks to see if the given *manual reset* event is currently signaled.  The event must not be an auto-reset event.
    // Use when the event will only be set once (cancellation-style) or will only be reset by the polling thread
    inline bool event_is_signaled(HANDLE hEvent) WI_NOEXCEPT
    {
        auto status = ::WaitForSingleObjectEx(hEvent, 0, FALSE);
        // Fast fail will trip for wait failures, auto-reset events, or when the event is being both Set and Reset
        // from a thread other than the polling thread (use event_wait directly for those cases).
        __FAIL_FAST_ASSERT__((status == WAIT_TIMEOUT) || ((status == WAIT_OBJECT_0) && (WAIT_OBJECT_0 == ::WaitForSingleObjectEx(hEvent, 0, FALSE))));
        return (status == WAIT_OBJECT_0);
    }

    // Waits on the given handle for the specified duration
    inline bool handle_wait(HANDLE hEvent, DWORD dwMilliseconds = INFINITE) WI_NOEXCEPT
    {
        DWORD status = ::WaitForSingleObjectEx(hEvent, dwMilliseconds, FALSE);
        __FAIL_FAST_ASSERT__((status == WAIT_TIMEOUT) || (status == WAIT_OBJECT_0));
        return (status == WAIT_OBJECT_0);
    }

    enum class EventOptions
    {
        None = 0x0,
        ManualReset = 0x1,
        Signaled = 0x2
    };
    DEFINE_ENUM_FLAG_OPERATORS(EventOptions);

    template <typename storage_t, typename err_policy = err_exception_policy>
    class event_t : public storage_t
    {
    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit event_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // HRESULT or void error handling...
        typedef typename err_policy::result result;

        // Exception-based constructor to create an unnamed event
        event_t(EventOptions options)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions or fail fast; use the create method");
            create(options);
        }

        void ResetEvent() const WI_NOEXCEPT
        {
            details::ResetEvent(storage_t::get());
        }

        void SetEvent() const WI_NOEXCEPT
        {
            details::SetEvent(storage_t::get());
        }

        // Guarantees a SetEvent on the given event handle when the returned object goes out of scope
        // Note: call SetEvent early with the reset() method on the returned object or abort the call with the release() method
        _Check_return_ event_set_scope_exit SetEvent_scope_exit() const WI_NOEXCEPT
        {
            return wil::SetEvent_scope_exit(storage_t::get());
        }

        // Guarantees a ResetEvent on the given event handle when the returned object goes out of scope
        // Note: call ResetEvent early with the reset() method on the returned object or abort the call with the release() method
        _Check_return_ event_reset_scope_exit ResetEvent_scope_exit() const WI_NOEXCEPT
        {
            return wil::ResetEvent_scope_exit(storage_t::get());
        }

        // Checks if a *manual reset* event is currently signaled.  The event must not be an auto-reset event.
        // Use when the event will only be set once (cancellation-style) or will only be reset by the polling thread
        bool is_signaled() const WI_NOEXCEPT
        {
            return wil::event_is_signaled(storage_t::get());
        }

        // Basic WaitForSingleObject on the event handle with the given timeout
        bool wait(DWORD dwMilliseconds = INFINITE) const WI_NOEXCEPT
        {
            return wil::handle_wait(storage_t::get(), dwMilliseconds);
        }

        // Tries to create a named event -- returns false if unable to do so (gle may still be inspected with return=false)
        bool try_create(EventOptions options, PCWSTR name, _In_opt_ LPSECURITY_ATTRIBUTES pSecurity = nullptr, _Out_opt_ bool *pAlreadyExists = nullptr)
        {
            auto handle = ::CreateEventExW(pSecurity, name, (WI_IsFlagSet(options, EventOptions::ManualReset) ? CREATE_EVENT_MANUAL_RESET : 0) | (WI_IsFlagSet(options, EventOptions::Signaled) ? CREATE_EVENT_INITIAL_SET : 0), EVENT_ALL_ACCESS);
            if (!handle)
            {
                assign_to_opt_param(pAlreadyExists, false);
                return false;
            }
            assign_to_opt_param(pAlreadyExists, (::GetLastError() == ERROR_ALREADY_EXISTS));
            storage_t::reset(handle);
            return true;
        }

        // Returns HRESULT for unique_event_nothrow, void with exceptions for shared_event and unique_event
        result create(EventOptions options = EventOptions::None, PCWSTR name = nullptr, _In_opt_ LPSECURITY_ATTRIBUTES pSecurity = nullptr, _Out_opt_ bool *pAlreadyExists = nullptr)
        {
            return err_policy::LastErrorIfFalse(try_create(options, name, pSecurity, pAlreadyExists));
        }

        // Tries to open the named event -- returns false if unable to do so (gle may still be inspected with return=false)
        bool try_open(_In_ PCWSTR name, DWORD desiredAccess = SYNCHRONIZE | EVENT_MODIFY_STATE, bool inheritHandle = false)
        {
            auto handle = ::OpenEventW(desiredAccess, inheritHandle, name);
            if (handle == nullptr)
            {
                return false;
            }
            storage_t::reset(handle);
            return true;
        }

        // Returns HRESULT for unique_event_nothrow, void with exceptions for shared_event and unique_event
        result open(_In_ PCWSTR name, DWORD desiredAccess = SYNCHRONIZE | EVENT_MODIFY_STATE, bool inheritHandle = false)
        {
            return err_policy::LastErrorIfFalse(try_open(name, desiredAccess, inheritHandle));
        }
    };

    typedef unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_returncode_policy>>     unique_event_nothrow;
    typedef unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_failfast_policy>>       unique_event_failfast;
#ifdef WIL_ENABLE_EXCEPTIONS
    typedef unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_exception_policy>>      unique_event;
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    enum class SlimEventType
    {
        AutoReset,
        ManualReset,
    };

    /** A lean and mean event class.
    This class provides a very similar API to `wil::unique_event` but doesn't require a kernel object.

    The two variants of this class are:
    - `wil::slim_event_auto_reset`
    - `wil::slim_event_manual_reset`

    In addition, `wil::slim_event_auto_reset` has the alias `wil::slim_event`.

    Some key differences to `wil::unique_event` include:
    - There is no 'create()' function, as initialization occurs in the constructor and can't fail.
    - The move functions have been deleted.
    - For auto-reset events, the `is_signaled()` function doesn't reset the event. (Use `ResetEvent()` instead.)
    - The `ResetEvent()` function returns the previous state of the event.
    - To create a manual reset event, use `wil::slim_event_manual_reset'.
    ~~~~
    wil::slim_event finished;
    std::thread doStuff([&finished] () {
        Sleep(10);
        finished.SetEvent();
    });
    finished.wait();

    std::shared_ptr<wil::slim_event> CreateSharedEvent(bool startSignaled)
    {
        return std::make_shared<wil::slim_event>(startSignaled);
    }
    ~~~~ */
    template <SlimEventType Type>
    class slim_event_t
    {
    public:
        slim_event_t()
        {
        }

        slim_event_t(bool isSignaled) :
            m_isSignaled(isSignaled ? TRUE : FALSE)
        {
        }

        // Cannot change memory location.
        slim_event_t(const slim_event_t&) = delete;
        slim_event_t(slim_event_t&&) = delete;
        slim_event_t& operator=(const slim_event_t&) = delete;
        slim_event_t& operator=(slim_event_t&&) = delete;

        // Returns the previous state of the event.
        bool ResetEvent() WI_NOEXCEPT
        {
            return !!InterlockedExchange(&m_isSignaled, FALSE);
        }

        // Returns the previous state of the event.
        void SetEvent() WI_NOEXCEPT
        {
            // FYI: 'WakeByAddress*' invokes a full memory barrier.
            WriteRelease(&m_isSignaled, TRUE);

            #pragma warning(suppress:4127) // conditional expression is constant
            if (Type == SlimEventType::AutoReset)
            {
                WakeByAddressSingle(&m_isSignaled);
            }
            else
            {
                WakeByAddressAll(&m_isSignaled);
            }
        }

        // Checks if the event is currently signaled.
        // Note: Unlike Win32 auto-reset event objects, this will not reset the event.
        bool is_signaled() const WI_NOEXCEPT
        {
            return !!ReadAcquire(&m_isSignaled);
        }

        bool wait(DWORD timeoutMiliseconds) WI_NOEXCEPT
        {
            if (timeoutMiliseconds == 0)
            {
                return TryAcquireEvent();
            }
            else if (timeoutMiliseconds == INFINITE)
            {
                return wait();
            }

            UINT64 startTime;
            QueryUnbiasedInterruptTime(&startTime);

            UINT64 elapsedTimeMilliseconds = 0;

            while (!TryAcquireEvent())
            {
                if (elapsedTimeMilliseconds >= timeoutMiliseconds)
                {
                    return false;
                }

                DWORD newTimeout = static_cast<DWORD>(timeoutMiliseconds - elapsedTimeMilliseconds);

                if (!WaitForSignal(newTimeout))
                {
                    return false;
                }

                UINT64 currTime;
                QueryUnbiasedInterruptTime(&currTime);

                elapsedTimeMilliseconds = (currTime - startTime) / (10 * 1000);
            }

            return true;
        }

        bool wait() WI_NOEXCEPT
        {
            while (!TryAcquireEvent())
            {
                if (!WaitForSignal(INFINITE))
                {
                    return false;
                }
            }

            return true;
        }

    private:
        bool TryAcquireEvent() WI_NOEXCEPT
        {
            #pragma warning(suppress:4127) // conditional expression is constant
            if (Type == SlimEventType::AutoReset)
            {
                return ResetEvent();
            }
            else
            {
                return is_signaled();
            }
        }

        bool WaitForSignal(DWORD timeoutMiliseconds) WI_NOEXCEPT
        {
            LONG falseValue = FALSE;
            BOOL waitResult = WaitOnAddress(&m_isSignaled, &falseValue, sizeof(m_isSignaled), timeoutMiliseconds);
            __FAIL_FAST_ASSERT__(waitResult || ::GetLastError() == ERROR_TIMEOUT);
            return !!waitResult;
        }

        LONG m_isSignaled = FALSE;
    };

    /** An event object that will atomically revert to an unsignaled state anytime a `wait()` call succeeds (i.e. returns true). */
    using slim_event_auto_reset = slim_event_t<SlimEventType::AutoReset>;

    /** An event object that once signaled remains that way forever, unless `ResetEvent()` is called. */
    using slim_event_manual_reset = slim_event_t<SlimEventType::ManualReset>;

    /** An alias for `wil::slim_event_auto_reset`. */
    using slim_event = slim_event_auto_reset;

#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

    typedef unique_any<HANDLE, decltype(&details::ReleaseMutex), details::ReleaseMutex, details::pointer_access_none> mutex_release_scope_exit;

    inline _Check_return_ mutex_release_scope_exit ReleaseMutex_scope_exit(_In_ HANDLE hMutex) WI_NOEXCEPT
    {
        __FAIL_FAST_ASSERT__(hMutex != nullptr);
        return mutex_release_scope_exit(hMutex);
    }

    // For efficiency, avoid using mutexes when an srwlock or condition variable will do.
    template <typename storage_t, typename err_policy = err_exception_policy>
    class mutex_t : public storage_t
    {
    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit mutex_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // HRESULT or void error handling...
        typedef typename err_policy::result result;

        // Exception-based constructor to create a mutex (prefer unnamed (nullptr) for the name)
        mutex_t(_In_opt_ PCWSTR name)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions or fail fast; use the create method");
            create(name);
        }

        void ReleaseMutex() const WI_NOEXCEPT
        {
            details::ReleaseMutex(storage_t::get());
        }

        _Check_return_ mutex_release_scope_exit ReleaseMutex_scope_exit() const WI_NOEXCEPT
        {
            return wil::ReleaseMutex_scope_exit(storage_t::get());
        }

        _Check_return_ mutex_release_scope_exit acquire(_Out_opt_ DWORD *pStatus = nullptr, DWORD dwMilliseconds = INFINITE, BOOL bAlertable = FALSE)  const WI_NOEXCEPT
        {
            auto handle = storage_t::get();
            DWORD status = ::WaitForSingleObjectEx(handle, dwMilliseconds, bAlertable);
            assign_to_opt_param(pStatus, status);
            __FAIL_FAST_ASSERT__((status == WAIT_TIMEOUT) || (status == WAIT_OBJECT_0) || (status == WAIT_ABANDONED));
            return mutex_release_scope_exit(((status == WAIT_OBJECT_0) || (status == WAIT_ABANDONED)) ? handle : nullptr);
        }

        // Tries to create a named mutex -- returns false if unable to do so (gle may still be inspected with return=false)
        bool try_create(_In_opt_ PCWSTR name, DWORD dwFlags = 0, DWORD desiredAccess = MUTEX_ALL_ACCESS, _In_opt_ PSECURITY_ATTRIBUTES pMutexAttributes = nullptr)
        {
            auto handle = ::CreateMutexExW(pMutexAttributes, name, dwFlags, desiredAccess);
            if (handle == nullptr)
            {
                return false;
            }
            storage_t::reset(handle);
            return true;
        }

        // Returns HRESULT for unique_mutex_nothrow, void with exceptions for shared_mutex and unique_mutex
        result create(_In_opt_ PCWSTR name = nullptr, DWORD dwFlags = 0, DWORD desiredAccess = MUTEX_ALL_ACCESS, _In_opt_ PSECURITY_ATTRIBUTES pMutexAttributes = nullptr)
        {
            return err_policy::LastErrorIfFalse(try_create(name, dwFlags, desiredAccess, pMutexAttributes));
        }

        // Tries to open a named mutex -- returns false if unable to do so (gle may still be inspected with return=false)
        bool try_open(_In_ PCWSTR name, DWORD desiredAccess = SYNCHRONIZE | MUTEX_MODIFY_STATE, bool inheritHandle = false)
        {
            auto handle = ::OpenMutexW(desiredAccess, inheritHandle, name);
            if (handle == nullptr)
            {
                return false;
            }
            storage_t::reset(handle);
            return true;
        }

        // Returns HRESULT for unique_mutex_nothrow, void with exceptions for shared_mutex and unique_mutex
        result open(_In_ PCWSTR name, DWORD desiredAccess = SYNCHRONIZE | MUTEX_MODIFY_STATE, bool inheritHandle = false)
        {
            return err_policy::LastErrorIfFalse(try_open(name, desiredAccess, inheritHandle));
        }
    };

    typedef unique_any_t<mutex_t<details::unique_storage<details::handle_resource_policy>, err_returncode_policy>>     unique_mutex_nothrow;
    typedef unique_any_t<mutex_t<details::unique_storage<details::handle_resource_policy>, err_failfast_policy>>       unique_mutex_failfast;
#ifdef WIL_ENABLE_EXCEPTIONS
    typedef unique_any_t<mutex_t<details::unique_storage<details::handle_resource_policy>, err_exception_policy>>      unique_mutex;
#endif

    typedef unique_any<HANDLE, decltype(&details::ReleaseSemaphore), details::ReleaseSemaphore, details::pointer_access_none> semaphore_release_scope_exit;

    inline _Check_return_ semaphore_release_scope_exit ReleaseSemaphore_scope_exit(_In_ HANDLE hSemaphore) WI_NOEXCEPT
    {
        __FAIL_FAST_ASSERT__(hSemaphore != nullptr);
        return semaphore_release_scope_exit(hSemaphore);
    }

    template <typename storage_t, typename err_policy = err_exception_policy>
    class semaphore_t : public storage_t
    {
    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit semaphore_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // HRESULT or void error handling...
        typedef typename err_policy::result result;

        // Note that for custom-constructors the type given the constructor has to match exactly as not all implicit conversions will make it through the
        // forwarding constructor.  This constructor, for example, uses 'int' instead of 'LONG' as the count to ease that particular issue (const numbers are int by default).
        explicit semaphore_t(int initialCount, int maximumCount, _In_opt_ PCWSTR name = nullptr, DWORD desiredAccess = SEMAPHORE_ALL_ACCESS, _In_opt_ PSECURITY_ATTRIBUTES pSemaphoreAttributes = nullptr)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions or fail fast; use the create method");
            create(initialCount, maximumCount, name, desiredAccess, pSemaphoreAttributes);
        }

        void ReleaseSemaphore(long nReleaseCount = 1, _In_opt_ long *pnPreviousCount = nullptr) WI_NOEXCEPT
        {
            long nPreviousCount = 0;
            __FAIL_FAST_ASSERT__(::ReleaseSemaphore(get(), nReleaseCount, &nPreviousCount));
            assign_to_opt_param(pnPreviousCount, nPreviousCount);
        }

        _Check_return_ semaphore_release_scope_exit ReleaseSemaphore_scope_exit() WI_NOEXCEPT
        {
            return wil::ReleaseSemaphore_scope_exit(get());
        }

        _Check_return_ semaphore_release_scope_exit acquire(_Out_opt_ DWORD *pStatus = nullptr, DWORD dwMilliseconds = INFINITE, BOOL bAlertable = FALSE) WI_NOEXCEPT
        {
            auto handle = get();
            DWORD status = ::WaitForSingleObjectEx(handle, dwMilliseconds, bAlertable);
            assign_to_opt_param(pStatus, status);
            __FAIL_FAST_ASSERT__((status == WAIT_TIMEOUT) || (status == WAIT_OBJECT_0));
            return semaphore_release_scope_exit((status == WAIT_OBJECT_0) ? handle : nullptr);
        }

        // Tries to create a named event -- returns false if unable to do so (gle may still be inspected with return=false)
        bool try_create(LONG lInitialCount, LONG lMaximumCount, _In_opt_ PCWSTR name, DWORD desiredAccess = SEMAPHORE_ALL_ACCESS, _In_opt_ PSECURITY_ATTRIBUTES pSemaphoreAttributes = nullptr)
        {
            auto handle = ::CreateSemaphoreExW(pSemaphoreAttributes, lInitialCount, lMaximumCount, name, 0, desiredAccess);
            if (handle == nullptr)
            {
                return false;
            }
            storage_t::reset(handle);
            return true;
        }

        // Returns HRESULT for unique_semaphore_nothrow, void with exceptions for shared_event and unique_event
        result create(LONG lInitialCount, LONG lMaximumCount, _In_opt_ PCWSTR name = nullptr, DWORD desiredAccess = SEMAPHORE_ALL_ACCESS, _In_opt_ PSECURITY_ATTRIBUTES pSemaphoreAttributes = nullptr)
        {
            return err_policy::LastErrorIfFalse(try_create(lInitialCount, lMaximumCount, name, desiredAccess, pSemaphoreAttributes));
        }

        // Tries to open the named semaphore -- returns false if unable to do so (gle may still be inspected with return=false)
        bool try_open(_In_ PCWSTR name, DWORD desiredAccess = SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, bool inheritHandle = false)
        {
            auto handle = ::OpenSemaphoreW(desiredAccess, inheritHandle, name);
            if (handle == nullptr)
            {
                return false;
            }
            storage_t::reset(handle);
            return true;
        }

        // Returns HRESULT for unique_semaphore_nothrow, void with exceptions for shared_semaphore and unique_semaphore
        result open(_In_ PCWSTR name, DWORD desiredAccess = SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, bool inheritHandle = false)
        {
            return err_policy::LastErrorIfFalse(try_open(name, desiredAccess, inheritHandle));
        }
    };

    typedef unique_any_t<semaphore_t<details::unique_storage<details::handle_resource_policy>, err_returncode_policy>>  unique_semaphore_nothrow;
    typedef unique_any_t<semaphore_t<details::unique_storage<details::handle_resource_policy>, err_failfast_policy>>    unique_semaphore_failfast;
#ifdef WIL_ENABLE_EXCEPTIONS
    typedef unique_any_t<semaphore_t<details::unique_storage<details::handle_resource_policy>, err_exception_policy>>   unique_semaphore;
#endif

    typedef unique_any<SRWLOCK *, decltype(&::ReleaseSRWLockExclusive), ::ReleaseSRWLockExclusive, details::pointer_access_none> rwlock_release_exclusive_scope_exit;
    typedef unique_any<SRWLOCK *, decltype(&::ReleaseSRWLockShared), ::ReleaseSRWLockShared, details::pointer_access_none> rwlock_release_shared_scope_exit;

    inline _Check_return_ rwlock_release_exclusive_scope_exit AcquireSRWLockExclusive(_Inout_ SRWLOCK *plock) WI_NOEXCEPT
    {
        ::AcquireSRWLockExclusive(plock);
        return rwlock_release_exclusive_scope_exit(plock);
    }

    inline _Check_return_ rwlock_release_shared_scope_exit AcquireSRWLockShared(_Inout_ SRWLOCK *plock) WI_NOEXCEPT
    {
        ::AcquireSRWLockShared(plock);
        return rwlock_release_shared_scope_exit(plock);
    }

    inline _Check_return_ rwlock_release_exclusive_scope_exit TryAcquireSRWLockExclusive(_Inout_ SRWLOCK *plock) WI_NOEXCEPT
    {
        return rwlock_release_exclusive_scope_exit(::TryAcquireSRWLockExclusive(plock) ? plock : nullptr);
    }

    inline _Check_return_ rwlock_release_shared_scope_exit TryAcquireSRWLockShared(_Inout_ SRWLOCK *plock) WI_NOEXCEPT
    {
        return rwlock_release_shared_scope_exit(::TryAcquireSRWLockShared(plock) ? plock : nullptr);
    }

    class srwlock
    {
    public:
        srwlock(const srwlock&) = delete;
        srwlock& operator=(const srwlock&) = delete;

        srwlock() WI_NOEXCEPT :
        m_lock(SRWLOCK_INIT)
        {
        }

        _Check_return_ rwlock_release_exclusive_scope_exit lock_exclusive() WI_NOEXCEPT
        {
            return wil::AcquireSRWLockExclusive(&m_lock);
        }

        _Check_return_ rwlock_release_exclusive_scope_exit try_lock_exclusive() WI_NOEXCEPT
        {
            return wil::TryAcquireSRWLockExclusive(&m_lock);
        }

        _Check_return_ rwlock_release_shared_scope_exit lock_shared() WI_NOEXCEPT
        {
            return wil::AcquireSRWLockShared(&m_lock);
        }

        _Check_return_ rwlock_release_shared_scope_exit try_lock_shared() WI_NOEXCEPT
        {
            return wil::TryAcquireSRWLockShared(&m_lock);
        }

    private:
        SRWLOCK m_lock;
    };


    typedef unique_any<CRITICAL_SECTION *, decltype(&::LeaveCriticalSection), ::LeaveCriticalSection, details::pointer_access_none> cs_leave_scope_exit;

    inline _Check_return_ cs_leave_scope_exit EnterCriticalSection(_Inout_ CRITICAL_SECTION *pcs) WI_NOEXCEPT
    {
        ::EnterCriticalSection(pcs);
        return cs_leave_scope_exit(pcs);
    }

    inline _Check_return_ cs_leave_scope_exit TryEnterCriticalSection(_Inout_ CRITICAL_SECTION *pcs) WI_NOEXCEPT
    {
        return cs_leave_scope_exit(::TryEnterCriticalSection(pcs) ? pcs : nullptr);
    }

#if defined(_NTURTL_) && !defined(__WIL_NTURTL_CRITSEC_)
#define __WIL_NTURTL_CRITSEC_

    typedef unique_any<RTL_CRITICAL_SECTION*, decltype(&::RtlLeaveCriticalSection), RtlLeaveCriticalSection, details::pointer_access_none> rtlcs_leave_scope_exit;

    inline _Check_return_ cs_leave_scope_exit RtlEnterCriticalSection(_Inout_ RTL_CRITICAL_SECTION *pcs) WI_NOEXCEPT
    {
        ::RtlEnterCriticalSection(pcs);
        return cs_leave_scope_exit(pcs);
    }

    inline _Check_return_ cs_leave_scope_exit RtlTryEnterCriticalSection(_Inout_ RTL_CRITICAL_SECTION *pcs) WI_NOEXCEPT
    {
        return cs_leave_scope_exit(::RtlTryEnterCriticalSection(pcs) ? pcs : nullptr);
    }

#endif

    // Critical sections are worse than srwlocks in performance and memory usage (their only unique attribute
    // being recursive acquisition). Prefer srwlocks over critical sections when you don't need recursive acquisition.
    class critical_section
    {
    public:
        critical_section(const critical_section&) = delete;
        critical_section& operator=(const critical_section&) = delete;

        critical_section(ULONG spincount = 0) WI_NOEXCEPT
        {
            // Initialization will not fail without invalid params...
            ::InitializeCriticalSectionEx(&m_cs, spincount, 0);
        }

        ~critical_section() WI_NOEXCEPT
        {
            ::DeleteCriticalSection(&m_cs);
        }

        _Check_return_ cs_leave_scope_exit lock() WI_NOEXCEPT
        {
            return wil::EnterCriticalSection(&m_cs);
        }

        _Check_return_ cs_leave_scope_exit try_lock() WI_NOEXCEPT
        {
            return wil::TryEnterCriticalSection(&m_cs);
        }

    private:
        CRITICAL_SECTION m_cs;
    };

    /// @cond
    namespace details
    {
        template<typename string_class> struct string_allocator
        {
            static void* allocate(size_t /*size*/) WI_NOEXCEPT
            {
                static_assert(false, "This type did not provide a string_allocator, add a specialization of string_allocator to support your type.");
                return nullptr;
            }
        };
    }
    /// @endcond

    // This string helper does not support the ansi wil string helpers
    template<typename string_type>
    PCWSTR string_get_not_null(const string_type& string)
    {
        return string ? string.get() : L"";
    }

#ifndef MAKE_UNIQUE_STRING_MAX_CCH
#define MAKE_UNIQUE_STRING_MAX_CCH     2147483647  // max buffer size, in characters, that we support (same as INT_MAX)
#endif

    /** Copies a string (up to the given length) into memory allocated with a specified allocator returning null on failure.
    Use `wil::make_unique_string_nothrow()` for string resources returned from APIs that must satisfy a memory allocation contract
    that requires use of a specific allocator and free function (CoTaskMemAlloc/CoTaskMemFree, LocalAlloc/LocalFree, GlobalAlloc/GlobalFree, etc.).
    ~~~
    auto str = wil::make_unique_string_nothrow<wil::unique_cotaskmem_string>(L"a string of words", 8);
    RETURN_IF_NULL_ALLOC(str);
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"

    auto str = wil::make_unique_string_nothrow<unique_hlocal_string>(L"a string");
    RETURN_IF_NULL_ALLOC(str);
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"

    NOTE: If source is not null terminated, then length MUST be equal to or less than the size
          of the buffer pointed to by source.
    ~~~
    */
    template<typename string_type> string_type make_unique_string_nothrow(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        // When the source string exists, calculate the number of characters to copy up to either 
        // 1) the length that is given
        // 2) the length of the source string. When the source does not exist, use the given length
        //    for calculating both the size of allocated buffer and the number of characters to copy. 
        size_t lengthToCopy = length;
        if (source)
        {
            size_t maxLength = length < MAKE_UNIQUE_STRING_MAX_CCH ? length : MAKE_UNIQUE_STRING_MAX_CCH;
            PCWSTR endOfSource = source;
            while (maxLength && (*endOfSource != L'\0'))
            {
                endOfSource++;
                maxLength--;
            }
            lengthToCopy = endOfSource - source;
        }

        if (length == static_cast<size_t>(-1))
        {
            length = lengthToCopy;
        }
        const size_t allocatedBytes = (length + 1) * sizeof(*source);
        auto result = static_cast<PWSTR>(details::string_allocator<string_type>::allocate(allocatedBytes));

        if (result)
        {
            if (source)
            {
                const size_t bytesToCopy = lengthToCopy * sizeof(*source);
                memcpy_s(result, allocatedBytes, source, bytesToCopy);
                result[lengthToCopy] = L'\0'; // ensure the copied string is zero terminated
            }
            else
            {
                *result = L'\0'; // ensure null terminated in the "reserve space" use case.
            }
            result[length] = L'\0'; // ensure the final char of the buffer is zero terminated
        }
        return string_type(result);
    }
#ifndef WIL_NO_ASNI_STRINGS
    template<typename string_type> string_type make_unique_ansistring_nothrow(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        if (length == static_cast<size_t>(-1))
        {
            length = strlen(source);
        }
        const size_t cb = (length + 1) * sizeof(*source);
        auto result = static_cast<PSTR>(details::string_allocator<string_type>::allocate(cb));
        if (result)
        {
            if (source)
            {
                memcpy_s(result, cb, source, cb - sizeof(*source));
            }
            else
            {
                *result = '\0'; // ensure null terminated in the "reserve space" use case.
            }
            result[length] = '\0'; // ensure zero terminated
        }
        return string_type(result);
    }
#endif // WIL_NO_ASNI_STRINGS

    /** Copies a given string into memory allocated with a specified allocator that will fail fast on failure.
    The use of variadic templates parameters supports the 2 forms of make_unique_string, see those for more details.
    */
    template<typename string_type> string_type make_unique_string_failfast(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        auto result(make_unique_string_nothrow<string_type>(source, length));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifndef WIL_NO_ASNI_STRINGS
    template<typename string_type> string_type make_unique_ansistring_failfast(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        auto result(make_unique_ansistring_nothrow<string_type>(source, length));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_NO_ASNI_STRINGS

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Copies a given string into memory allocated with a specified allocator that will throw on failure.
    The use of variadic templates parameters supports the 2 forms of make_unique_string, see those for more details.
    */
    template<typename string_type> string_type make_unique_string(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1))
    {
        auto result(make_unique_string_nothrow<string_type>(source, length));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#ifndef WIL_NO_ASNI_STRINGS
    template<typename string_type> string_type make_unique_ansistring(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCSTR source, size_t length = static_cast<size_t>(-1))
    {
        auto result(make_unique_ansistring_nothrow<string_type>(source, length));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_NO_ASNI_STRINGS
#endif // WIL_ENABLE_EXCEPTIONS

    /// @cond
    namespace details
    {
        // string_maker abstracts creating a string for common string types. This form supports the
        // wil::unique_xxx_string types. Specializations of other types like HSTRING and std::wstring
        // are found in wil\winrt.h and wil\stl.h.
        // This design supports creating the string in a single step or using two phase construction.

        template<typename string_type> struct string_maker
        {
            HRESULT make(_In_reads_opt_(length) PCWSTR source, size_t length)
            {
                m_value = make_unique_string_nothrow<string_type>(source, length);
                return m_value ? S_OK : E_OUTOFMEMORY;
            }

            wchar_t* buffer() { WI_ASSERT(m_value.get());  return m_value.get(); }

            string_type release() { return wistd::move(m_value); }

            // Utility to abstract access to the null terminated m_value of all string types.
            static PCWSTR get(const string_type& value) { return value.get(); }

        private:
            string_type m_value; // a wil::unique_xxx_string type.
        };

        struct SecureZeroData
        {
            void *pointer;
            size_t sizeBytes;
            SecureZeroData(void *pointer_, size_t sizeBytes_ = 0) WI_NOEXCEPT { pointer = pointer_; sizeBytes = sizeBytes_; }
            operator void *() const WI_NOEXCEPT { return pointer; }
            static void Close(SecureZeroData data) WI_NOEXCEPT { ::SecureZeroMemory(data.pointer, data.sizeBytes); }
        };
    }
    /// @endcond

    typedef unique_any<void*, decltype(&details::SecureZeroData::Close), details::SecureZeroData::Close, details::pointer_access_all, details::SecureZeroData> secure_zero_memory_scope_exit;

    inline _Check_return_ secure_zero_memory_scope_exit SecureZeroMemory_scope_exit(_In_reads_bytes_(sizeBytes) void* pSource, size_t sizeBytes)
    {
        return secure_zero_memory_scope_exit(details::SecureZeroData(pSource, sizeBytes));
    }

    inline _Check_return_ secure_zero_memory_scope_exit SecureZeroMemory_scope_exit(_In_ PWSTR initializedString)
    {
        return SecureZeroMemory_scope_exit(static_cast<void*>(initializedString), wcslen(initializedString) * sizeof(initializedString[0]));
    }

    /// @cond
    namespace details
    {
        inline void __stdcall FreeProcessHeap(_Pre_opt_valid_ _Frees_ptr_opt_ void* p)
        {
            ::HeapFree(::GetProcessHeap(), 0, p);
        }
    }
    /// @endcond

    struct process_heap_deleter
    {
        template <typename T>
        void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T* p) const
        {
            details::FreeProcessHeap(p);
        }
    };

    struct virtualalloc_deleter
    {
        template<typename T>
        void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T* p) const
        {
            ::VirtualFree(p, 0, MEM_RELEASE);
        }
    };

#ifndef WIL_HIDE_DEPRECATED_1611
    WIL_WARN_DEPRECATED_1611_PRAGMA(unique_hheap_ptr)

        // deprecated, use unique_process_heap_ptr instead as that has the correct name.
        template <typename T>
    using unique_hheap_ptr = wistd::unique_ptr<T, process_heap_deleter>;
#endif

    template <typename T>
    using unique_process_heap_ptr = wistd::unique_ptr<T, process_heap_deleter>;

    typedef unique_any<PWSTR, decltype(&details::FreeProcessHeap), details::FreeProcessHeap> unique_process_heap_string;

    /// @cond
    namespace details
    {
        template<> struct string_allocator<unique_process_heap_string>
        {
            static void* allocate(size_t size) WI_NOEXCEPT
            {
                return ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            }
        };
    }
    /// @endcond

    /** Manages a typed pointer allocated with VirtualAlloc
    A specialization of wistd::unique_ptr<> that frees via VirtualFree(p, 0, MEM_RELEASE).
    */
    template<typename T>
    using unique_virtualalloc_ptr = wistd::unique_ptr<T, virtualalloc_deleter>;

#endif // __WIL_WINBASE_

// do not add new uses of UNIQUE_STRING_VALUE_EXPERIEMENT, see http://osgvsowi/11252630 for details
#if defined(_WINBASE_) && defined(UNIQUE_STRING_VALUE_EXPERIMENT) && !defined(__WIL_UNIQUE_STRING_VALUE_)
#define __WIL_UNIQUE_STRING_VALUE_
    /** Helper to make use of wil::unique_cotaskmem_string, wil::unique_hlocal_string, wil::unique_process_heap_string
    easier to use. Defaults to using unique_cotaskmem_string.

    wil::unique_string_value<> m_value;

    wil::unique_string_value<wil::unique_hlocal_string> m_localAllocValue;
    */
    template<typename string_type = unique_cotaskmem_string>
    class unique_string_value : public string_type
    {
    public:
        typedef string_type string_type;

        unique_string_value() = default;
        unique_string_value(PWSTR value) : string_type(value)
        {
        }

        unique_string_value(string_type&& other) : string_type(wistd::move(other))
        {
        }

        PCWSTR get_not_null() const
        {
            return get() ? get() : L"";
        }

        HRESULT set_nothrow(_In_opt_ PCWSTR value) WI_NOEXCEPT
        {
            if (value)
            {
                reset(wil::make_unique_string_nothrow<string_type>(value).release());
                return get() ? S_OK : E_OUTOFMEMORY; // log errors at the caller of this method.
            }
            else
            {
                reset();
                return S_OK;
            }
        }

        HRESULT copy_to_nothrow(_Outptr_opt_result_maybenull_ PWSTR* result) WI_NOEXCEPT
        {
            unique_string_value<string_type> temp;
            RETURN_IF_FAILED(temp.set_nothrow(get()));
            *result = temp.release();
            return S_OK;
        }

#ifdef WIL_ENABLE_EXCEPTIONS
        void set(_In_opt_ PCWSTR value)
        {
            THROW_IF_FAILED(set_nothrow(value));
        }
#endif

        unique_string_value<string_type>& operator=(string_type&& other)
        {
            string_type::operator=(wistd::move(other));
            return *this;
        }

        /** returns a pointer to and the size of the buffer. Useful for updating
        the value when the updated value is guaranteed to fit based on the size. */
        PWSTR get_dangerous_writeable_buffer(_Out_ size_t* size)
        {
            if (get())
            {
                *size = wcslen(get()) + 1; // include null
                return const_cast<PWSTR>(get());
            }
            else
            {
                *size = 0;
                return L""; // no space for writing, can only read the null value.
            }
        }
    };

    /// @cond
    namespace details
    {
        template<typename string_type> struct string_allocator<unique_string_value<string_type>>
        {
            static void* allocate(size_t size) WI_NOEXCEPT
            {
                return string_allocator<string_type>::allocate(size);
            }
        };
    }
    /// @endcond

#endif


#if defined(__WIL_WINBASE_) && defined(__NOTHROW_T_DEFINED) && !defined(__WIL_WINBASE_NOTHROW_T_DEFINED)
#define __WIL_WINBASE_NOTHROW_T_DEFINED
    // unique_event_watcher, unique_event_watcher_nothrow, unique_event_watcher_failfast
    //
    // Clients must include <new> or <new.h> to enable use of this class as it uses new(std::nothrow).
    // This is to avoid the dependency on those headers that some clients can't tolerate.
    //
    // These classes makes it easy to execute a provided function when an event
    // is signaled. It will create the event handle for you, take ownership of one
    // or duplicate a handle provided. It supports the ability to signal the
    // event using SetEvent() and SetEvent_scope_exit();
    //
    // This can be used to support producer-consumer pattern
    // where a producer updates some state then signals the event when done.
    // The consumer will consume that state in the callback provided to unique_event_watcher.
    //
    // Note, multiple signals may coalesce into a single callback.
    //
    // Example use of throwing version:
    // auto globalStateWatcher = wil::make_event_watcher([]
    //     {
    //         currentState = GetGlobalState();
    //     });
    //
    // UpdateGlobalState(value);
    // globalStateWatcher.SetEvent(); // signal observers so they can update
    //
    // Example use of non-throwing version:
    // auto globalStateWatcher = wil::make_event_watcher_nothrow([]
    //     {
    //         currentState = GetGlobalState();
    //     });
    // RETURN_HR_IF_FALSE(E_OUTOFMEMORY, globalStateWatcher)
    //
    // UpdateGlobalState(value);
    // globalStateWatcher.SetEvent(); // signal observers so they can update

    /// @cond
    namespace details
    {
        struct event_watcher_state
        {
            event_watcher_state(unique_event_nothrow &&eventHandle, wistd::function<void()> &&callback)
                : m_callback(wistd::move(callback)), m_event(wistd::move(eventHandle))
            {
            }
            wistd::function<void()> m_callback;
            unique_event_nothrow m_event;
            // The thread pool must be last to ensure that the other members are valid
            // when it is destructed as it will reference them.
            // See http://osgvsowi/2224623
            unique_threadpool_wait m_threadPoolWait;
        };

        inline void delete_event_watcher_state(_In_opt_ event_watcher_state *watcherStorage) { delete watcherStorage; }

        typedef resource_policy<event_watcher_state *, decltype(&delete_event_watcher_state),
            delete_event_watcher_state, details::pointer_access_none> event_watcher_state_resource_policy;
    }
    /// @endcond

    template <typename storage_t, typename err_policy = err_exception_policy>
    class event_watcher_t : public storage_t
    {
    public:
        // forward all base class constructors...
        template <typename... args_t>
        explicit event_watcher_t(args_t&&... args) WI_NOEXCEPT : storage_t(wistd::forward<args_t>(args)...) {}

        // HRESULT or void error handling...
        typedef typename err_policy::result result;

        // Exception-based constructors
        template <typename err_policy>
        event_watcher_t(unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_policy>> &&eventHandle, wistd::function<void()> &&callback)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions or fail fast; use the create method");
            create(wistd::move(eventHandle), wistd::move(callback));
        }

        event_watcher_t(_In_ HANDLE eventHandle, wistd::function<void()> &&callback)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions or fail fast; use the create method");
            create(eventHandle, wistd::move(callback));
        }

        event_watcher_t(wistd::function<void()> &&callback)
        {
            static_assert(wistd::is_same<void, result>::value, "this constructor requires exceptions or fail fast; use the create method");
            create(wistd::move(callback));
        }

        template <typename event_err_policy>
        result create(unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, event_err_policy>> &&eventHandle,
            wistd::function<void()> &&callback)
        {
            return err_policy::HResult(create_take_hevent_ownership(eventHandle.release(), wistd::move(callback)));
        }

        // Creates the event that you will be watching.
        result create(wistd::function<void()> &&callback)
        {
            unique_event_nothrow eventHandle;
            HRESULT hr = eventHandle.create(EventOptions::ManualReset); // auto-reset is supported too.
            if (FAILED(hr))
            {
                return err_policy::HResult(hr);
            }
            return err_policy::HResult(create_take_hevent_ownership(eventHandle.release(), wistd::move(callback)));
        }

        // Input is an event handler that is duplicated into this class.
        result create(_In_ HANDLE eventHandle, wistd::function<void()> &&callback)
        {
            unique_event_nothrow ownedHandle;
            if (!DuplicateHandle(GetCurrentProcess(), eventHandle, GetCurrentProcess(), &ownedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
            {
                return err_policy::LastError();
            }
            return err_policy::HResult(create_take_hevent_ownership(ownedHandle.release(), wistd::move(callback)));
        }

        // Provide access to the inner event and the very common SetEvent() method on it.
        unique_event_nothrow const& get_event() const WI_NOEXCEPT { return get()->m_event; }
        void SetEvent() const WI_NOEXCEPT { get()->m_event.SetEvent(); }

    private:
        // To avoid template expansion (if unique_event/unique_event_nothrow forms were used) this base
        // create function takes a raw handle and assumes its ownership, even on failure.
        HRESULT create_take_hevent_ownership(_In_ HANDLE rawHandleOwnershipTaken, wistd::function<void()> &&callback)
        {
            __FAIL_FAST_ASSERT__(rawHandleOwnershipTaken != nullptr); // invalid parameter
            unique_event_nothrow eventHandle(rawHandleOwnershipTaken);
            wistd::unique_ptr<details::event_watcher_state> watcherState(new(std::nothrow) details::event_watcher_state(wistd::move(eventHandle), wistd::move(callback)));
            RETURN_IF_NULL_ALLOC(watcherState);

            watcherState->m_threadPoolWait.reset(CreateThreadpoolWait([](PTP_CALLBACK_INSTANCE, void *context, TP_WAIT *pThreadPoolWait, TP_WAIT_RESULT)
            {
                auto pThis = static_cast<details::event_watcher_state *>(context);
                // Manual events must be re-set to avoid missing the last notification.
                pThis->m_event.ResetEvent();
                // Call the client before re-arming to ensure that multiple callbacks don't
                // run concurrently.
                pThis->m_callback();
                SetThreadpoolWait(pThreadPoolWait, pThis->m_event.get(), nullptr); // valid params ensure success
            }, watcherState.get(), nullptr));
            RETURN_LAST_ERROR_IF(!watcherState->m_threadPoolWait);
            storage_t::reset(watcherState.release()); // no more failures after this, pass ownership
            SetThreadpoolWait(storage_t::get()->m_threadPoolWait.get(), storage_t::get()->m_event.get(), nullptr);
            return S_OK;
        }
    };

    typedef unique_any_t<event_watcher_t<details::unique_storage<details::event_watcher_state_resource_policy>, err_returncode_policy>> unique_event_watcher_nothrow;
    typedef unique_any_t<event_watcher_t<details::unique_storage<details::event_watcher_state_resource_policy>, err_failfast_policy>> unique_event_watcher_failfast;

    template <typename err_policy>
    unique_event_watcher_nothrow make_event_watcher_nothrow(unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_policy>> &&eventHandle, wistd::function<void()> &&callback) WI_NOEXCEPT
    {
        unique_event_watcher_nothrow watcher;
        watcher.create(wistd::move(eventHandle), wistd::move(callback));
        return watcher; // caller must test for success using if (watcher)
    }

    inline unique_event_watcher_nothrow make_event_watcher_nothrow(_In_ HANDLE eventHandle, wistd::function<void()> &&callback) WI_NOEXCEPT
    {
        unique_event_watcher_nothrow watcher;
        watcher.create(eventHandle, wistd::move(callback));
        return watcher; // caller must test for success using if (watcher)
    }

    inline unique_event_watcher_nothrow make_event_watcher_nothrow(wistd::function<void()> &&callback) WI_NOEXCEPT
    {
        unique_event_watcher_nothrow watcher;
        watcher.create(wistd::move(callback));
        return watcher; // caller must test for success using if (watcher)
    }

    template <typename err_policy>
    unique_event_watcher_failfast make_event_watcher_failfast(unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_policy>> &&eventHandle, wistd::function<void()> &&callback)
    {
        return unique_event_watcher_failfast(wistd::move(eventHandle), wistd::move(callback));
    }

    inline unique_event_watcher_failfast make_event_watcher_failfast(_In_ HANDLE eventHandle, wistd::function<void()> &&callback)
    {
        return unique_event_watcher_failfast(eventHandle, wistd::move(callback));
    }

    inline unique_event_watcher_failfast make_event_watcher_failfast(wistd::function<void()> &&callback)
    {
        return unique_event_watcher_failfast(wistd::move(callback));
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    typedef unique_any_t<event_watcher_t<details::unique_storage<details::event_watcher_state_resource_policy>, err_exception_policy>> unique_event_watcher;

    template <typename err_policy>
    unique_event_watcher make_event_watcher(unique_any_t<event_t<details::unique_storage<details::handle_resource_policy>, err_policy>> &&eventHandle, wistd::function<void()> &&callback)
    {
        return unique_event_watcher(wistd::move(eventHandle), wistd::move(callback));
    }

    inline unique_event_watcher make_event_watcher(_In_ HANDLE eventHandle, wistd::function<void()> &&callback)
    {
        return unique_event_watcher(eventHandle, wistd::move(callback));
    }

    inline unique_event_watcher make_event_watcher(wistd::function<void()> &&callback)
    {
        return unique_event_watcher(wistd::move(callback));
    }
#endif // WIL_ENABLE_EXCEPTIONS

#endif // __WIL_WINBASE_NOTHROW_T_DEFINED

#if defined(__WIL_WINBASE_) && !defined(__WIL_WINBASE_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINBASE_STL
    typedef shared_any_t<event_t<details::shared_storage<unique_event>>> shared_event;
    typedef shared_any_t<mutex_t<details::shared_storage<unique_mutex>>> shared_mutex;
    typedef shared_any_t<semaphore_t<details::shared_storage<unique_semaphore>>> shared_semaphore;
    typedef shared_any<unique_hfile> shared_hfile;
    typedef shared_any<unique_handle> shared_handle;
    typedef shared_any<unique_hfind> shared_hfind;
    typedef shared_any<unique_hmodule> shared_hmodule;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    typedef shared_any<unique_threadpool_wait> shared_threadpool_wait;
    typedef shared_any<unique_threadpool_wait_nocancel> shared_threadpool_wait_nocancel;
    typedef shared_any<unique_threadpool_work> shared_threadpool_work;
    typedef shared_any<unique_threadpool_work_nocancel> shared_threadpool_work_nocancel;

    typedef shared_any<unique_hfind_change> shared_hfind_change;
#endif

    typedef weak_any<shared_event> weak_event;
    typedef weak_any<shared_mutex> weak_mutex;
    typedef weak_any<shared_semaphore> weak_semaphore;
    typedef weak_any<shared_hfile> weak_hfile;
    typedef weak_any<shared_handle> weak_handle;
    typedef weak_any<shared_hfind> weak_hfind;
    typedef weak_any<shared_hmodule> weak_hmodule;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    typedef weak_any<shared_threadpool_wait> weak_threadpool_wait;
    typedef weak_any<shared_threadpool_wait_nocancel> weak_threadpool_wait_nocancel;
    typedef weak_any<shared_threadpool_work> weak_threadpool_work;
    typedef weak_any<shared_threadpool_work_nocancel> weak_threadpool_work_nocancel;

    typedef weak_any<shared_hfind_change> weak_hfind_change;
#endif

#endif // __WIL_WINBASE_STL

#if defined(__WIL_WINBASE_) && defined(__NOTHROW_T_DEFINED) && !defined(__WIL_WINBASE_NOTHROW_T_DEFINED_STL) && defined(WIL_RESOURCE_STL) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WINBASE_NOTHROW_T_DEFINED_STL
    typedef shared_any_t<event_watcher_t<details::shared_storage<unique_event_watcher>>> shared_event_watcher;
    typedef weak_any<shared_event_watcher> weak_event_watcher;
#endif // __WIL_WINBASE_NOTHROW_T_DEFINED_STL

#if defined(_WINBASE_) && !defined(__WIL_WINBASE_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WINBASE_DESKTOP
    /// @cond
    namespace details
    {
        inline void __stdcall DestroyPrivateObjectSecurity(_Pre_opt_valid_ _Frees_ptr_opt_ PSECURITY_DESCRIPTOR pObjectDescriptor) WI_NOEXCEPT
        {
            ::DestroyPrivateObjectSecurity(&pObjectDescriptor);
        }
    }
    /// @endcond

    using hlocal_deleter = function_deleter<decltype(&::LocalFree), LocalFree>;

    template <typename T>
    using unique_hlocal_ptr = wistd::unique_ptr<T, hlocal_deleter>;

    /** Provides `std::make_unique()` semantics for resources allocated with `LocalAlloc()` in a context that may not throw upon allocation failure.
    Use `wil::make_unique_hlocal_nothrow()` for resources returned from APIs that must satisfy a memory allocation contract that requires the use of `LocalAlloc()` / `LocalFree()`.
    Use `wil::make_unique_nothrow()` when `LocalAlloc()` is not required.

    Allocations are initialized with placement new and will call constructors (if present), but this does not guarantee initialization.

    Note that `wil::make_unique_hlocal_nothrow()` is not marked WI_NOEXCEPT as it may be used to create an exception-based class that may throw in its constructor.
    ~~~
    auto foo = wil::make_unique_hlocal_nothrow<Foo>();
    if (foo)
    {
    // initialize allocated Foo object as appropriate
    }
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_hlocal_ptr<T>>::type make_unique_hlocal_nothrow(Args&&... args)
    {
        static_assert(wistd::is_trivially_destructible<T>::value, "T has a destructor that won't be run when used with this function; use make_unique instead");
        unique_hlocal_ptr<T> sp(static_cast<T*>(::LocalAlloc(LMEM_FIXED, sizeof(T))));
        if (sp)
        {
            // use placement new to initialize memory from the previous allocation
            new (sp.get()) T(wistd::forward<Args>(args)...);
        }
        return sp;
    }

    /** Provides `std::make_unique()` semantics for array resources allocated with `LocalAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_hlocal_nothrow<Foo[]>(size);
    if (foos)
    {
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_hlocal_ptr<T>>::type make_unique_hlocal_nothrow(size_t size)
    {
        typedef typename wistd::remove_extent<T>::type E;
        static_assert(wistd::is_trivially_destructible<E>::value, "E has a destructor that won't be run when used with this function; use make_unique instead");
        FAIL_FAST_IF((SIZE_MAX / sizeof(E)) < size);
        size_t allocSize = sizeof(E) * size;
        unique_hlocal_ptr<T> sp(static_cast<E*>(::LocalAlloc(LMEM_FIXED, allocSize)));
        if (sp)
        {
            // use placement new to initialize memory from the previous allocation;
            // note that array placement new cannot be used as the standard allows for operator new[]
            // to consume overhead in the allocation for internal bookkeeping
            for (auto& elem : make_range(static_cast<E*>(sp.get()), size))
            {
                new (&elem) E();
            }
        }
        return sp;
    }

    /** Provides `std::make_unique()` semantics for resources allocated with `LocalAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_hlocal_failfast<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_hlocal_ptr<T>>::type make_unique_hlocal_failfast(Args&&... args)
    {
        unique_hlocal_ptr<T> result(make_unique_hlocal_nothrow<T>(wistd::forward<Args>(args)...));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for array resources allocated with `LocalAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_hlocal_failfast<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_hlocal_ptr<T>>::type make_unique_hlocal_failfast(size_t size)
    {
        unique_hlocal_ptr<T> result(make_unique_hlocal_nothrow<T>(size));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Provides `std::make_unique()` semantics for resources allocated with `LocalAlloc()`.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_hlocal<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_hlocal_ptr<T>>::type make_unique_hlocal(Args&&... args)
    {
        unique_hlocal_ptr<T> result(make_unique_hlocal_nothrow<T>(wistd::forward<Args>(args)...));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for array resources allocated with `LocalAlloc()`.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_hlocal<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_hlocal_ptr<T>>::type make_unique_hlocal(size_t size)
    {
        unique_hlocal_ptr<T> result(make_unique_hlocal_nothrow<T>(size));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_ENABLE_EXCEPTIONS

    typedef unique_any<HLOCAL, decltype(&::LocalFree), ::LocalFree> unique_hlocal;
    typedef unique_any<PWSTR, decltype(&::LocalFree), ::LocalFree> unique_hlocal_string;
#ifndef WIL_NO_ASNI_STRINGS
    typedef unique_any<PSTR, decltype(&::LocalFree), ::LocalFree> unique_hlocal_ansistring;
#endif // WIL_NO_ASNI_STRINGS

    /// @cond
    namespace details
    {
        template<> struct string_allocator<unique_hlocal_string>
        {
            static void* allocate(size_t size) WI_NOEXCEPT
            {
                return ::LocalAlloc(LMEM_FIXED, size);
            }
        };
#ifndef WIL_NO_ASNI_STRINGS
        template<> struct string_allocator<unique_hlocal_ansistring>
        {
            static void* allocate(size_t size) WI_NOEXCEPT
            {
                return ::LocalAlloc(LMEM_FIXED, size);
            }
        };
#endif // WIL_NO_ASNI_STRINGS
    }
    /// @endcond

    inline auto make_hlocal_string_nothrow(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_string_nothrow<unique_hlocal_string>(source, length);
    }

    inline auto make_hlocal_string_failfast(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_string_failfast<unique_hlocal_string>(source, length);
    }

#ifndef WIL_NO_ASNI_STRINGS
    inline auto make_hlocal_ansistring_nothrow(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_ansistring_nothrow<unique_hlocal_ansistring>(source, length);
    }

    inline auto make_hlocal_ansistring_failfast(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_ansistring_failfast<unique_hlocal_ansistring>(source, length);
    }
#endif

#ifdef WIL_ENABLE_EXCEPTIONS
    inline auto make_hlocal_string(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1))
    {
        return make_unique_string<unique_hlocal_string>(source, length);
    }

#ifndef WIL_NO_ASNI_STRINGS
    inline auto make_hlocal_ansistring(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCSTR source, size_t length = static_cast<size_t>(-1))
    {
        return make_unique_ansistring<unique_hlocal_ansistring>(source, length);
    }
#endif // WIL_NO_ANSI_STRINGS
#endif // WIL_ENABLE_EXCEPTIONS

    struct hlocal_secure_deleter
    {
        template <typename T>
        void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T* p) const
        {
            if (p)
            {
#pragma warning(suppress: 26006 26007) // LocalSize() ensures proper buffer length
                ::SecureZeroMemory(p, ::LocalSize(p)); // this is safe since LocalSize() returns 0 on failure
                ::LocalFree(p);
            }
        }
    };

    template <typename T>
    using unique_hlocal_secure_ptr = wistd::unique_ptr<T, hlocal_secure_deleter>;

    /** Provides `std::make_unique()` semantics for secure resources allocated with `LocalAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_hlocal_secure_nothrow<Foo>();
    if (foo)
    {
    // initialize allocated Foo object as appropriate
    }
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_hlocal_secure_ptr<T>>::type make_unique_hlocal_secure_nothrow(Args&&... args)
    {
        return unique_hlocal_secure_ptr<T>(make_unique_hlocal_nothrow<T>(wistd::forward<Args>(args)...).release());
    }

    /** Provides `std::make_unique()` semantics for secure array resources allocated with `LocalAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_hlocal_secure_nothrow<Foo[]>(size);
    if (foos)
    {
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_hlocal_secure_ptr<T>>::type make_unique_hlocal_secure_nothrow(size_t size)
    {
        return unique_hlocal_secure_ptr<T>(make_unique_hlocal_nothrow<T>(size).release());
    }

    /** Provides `std::make_unique()` semantics for secure resources allocated with `LocalAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_hlocal_secure_failfast<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_hlocal_secure_ptr<T>>::type make_unique_hlocal_secure_failfast(Args&&... args)
    {
        unique_hlocal_secure_ptr<T> result(make_unique_hlocal_secure_nothrow<T>(wistd::forward<Args>(args)...));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for secure array resources allocated with `LocalAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_hlocal_secure_failfast<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_hlocal_secure_ptr<T>>::type make_unique_hlocal_secure_failfast(size_t size)
    {
        unique_hlocal_secure_ptr<T> result(make_unique_hlocal_secure_nothrow<T>(size));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Provides `std::make_unique()` semantics for secure resources allocated with `LocalAlloc()`.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_hlocal_secure<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_hlocal_secure_ptr<T>>::type make_unique_hlocal_secure(Args&&... args)
    {
        unique_hlocal_secure_ptr<T> result(make_unique_hlocal_secure_nothrow<T>(wistd::forward<Args>(args)...));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for secure array resources allocated with `LocalAlloc()`.
    See the overload of `wil::make_unique_hlocal_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_hlocal_secure<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_hlocal_secure_ptr<T>>::type make_unique_hlocal_secure(size_t size)
    {
        unique_hlocal_secure_ptr<T> result(make_unique_hlocal_secure_nothrow<T>(size));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_ENABLE_EXCEPTIONS

    typedef unique_hlocal_secure_ptr<wchar_t[]> unique_hlocal_string_secure;

    /** Copies a given string into secure memory allocated with `LocalAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_hlocal_string_nothrow()` with supplied length for more details.
    ~~~
    auto str = wil::make_hlocal_string_secure_nothrow(L"a string");
    RETURN_IF_NULL_ALLOC(str);
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"
    ~~~
    */
    inline auto make_hlocal_string_secure_nothrow(_In_ PCWSTR source) WI_NOEXCEPT
    {
        return unique_hlocal_string_secure(make_hlocal_string_nothrow(source).release());
    }

    /** Copies a given string into secure memory allocated with `LocalAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_hlocal_string_nothrow()` with supplied length for more details.
    ~~~
    auto str = wil::make_hlocal_string_secure_failfast(L"a string");
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"
    ~~~
    */
    inline auto make_hlocal_string_secure_failfast(_In_ PCWSTR source) WI_NOEXCEPT
    {
        unique_hlocal_string_secure result(make_hlocal_string_secure_nothrow(source));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Copies a given string into secure memory allocated with `LocalAlloc()`.
    See the overload of `wil::make_hlocal_string_nothrow()` with supplied length for more details.
    ~~~
    auto str = wil::make_hlocal_string_secure(L"a string");
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"
    ~~~
    */
    inline auto make_hlocal_string_secure(_In_ PCWSTR source)
    {
        unique_hlocal_string_secure result(make_hlocal_string_secure_nothrow(source));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif

    using hglobal_deleter = function_deleter<decltype(&::GlobalFree), ::GlobalFree>;

    template <typename T>
    using unique_hglobal_ptr = wistd::unique_ptr<T, hglobal_deleter>;

    typedef unique_any<HGLOBAL, decltype(&::GlobalFree), ::GlobalFree> unique_hglobal;
    typedef unique_any<PWSTR, decltype(&::GlobalFree), ::GlobalFree> unique_hglobal_string;
#ifndef WIL_NO_ASNI_STRINGS
    typedef unique_any<PSTR, decltype(&::GlobalFree), ::GlobalFree> unique_hglobal_ansistring;
#endif // WIL_NO_ASNI_STRINGS

    /// @cond
    namespace details
    {
        template<> struct string_allocator<unique_hglobal_string>
        {
            static void* allocate(size_t size) WI_NOEXCEPT
            {
                return ::GlobalAlloc(GPTR, size);
            }
        };
    }
    /// @endcond

    inline auto make_process_heap_string_nothrow(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_string_nothrow<unique_process_heap_string>(source, length);
    }

    inline auto make_process_heap_string_failfast(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_string_failfast<unique_process_heap_string>(source, length);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    inline auto make_process_heap_string(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1))
    {
        return make_unique_string<unique_process_heap_string>(source, length);
    }
#endif // WIL_ENABLE_EXCEPTIONS

    typedef unique_any_handle_null<decltype(&::HeapDestroy), ::HeapDestroy> unique_hheap;
    typedef unique_any<DWORD, decltype(&::TlsFree), ::TlsFree> unique_tls;
    typedef unique_any<PSECURITY_DESCRIPTOR, decltype(&::LocalFree), ::LocalFree> unique_hlocal_security_descriptor;
    typedef unique_any<PSECURITY_DESCRIPTOR, decltype(&details::DestroyPrivateObjectSecurity), details::DestroyPrivateObjectSecurity> unique_private_security_descriptor;
    typedef unique_any<HACCEL, decltype(&::DestroyAcceleratorTable), ::DestroyAcceleratorTable> unique_haccel;
    typedef unique_any<HCURSOR, decltype(&::DestroyCursor), ::DestroyCursor> unique_hcursor;
#if !defined(NOGDI)
    typedef unique_any<HDESK, decltype(&::CloseDesktop), ::CloseDesktop> unique_hdesk;
    typedef unique_any<HWINSTA, decltype(&::CloseWindowStation), ::CloseWindowStation> unique_hwinsta;
#endif // !defined(NOGDI)
    typedef unique_any<HWND, decltype(&::DestroyWindow), ::DestroyWindow> unique_hwnd;

#ifndef WIL_HIDE_DEPRECATED_1611
    WIL_WARN_DEPRECATED_1611_PRAGMA(unique_security_descriptor)

        // WARNING: 'unique_security_descriptor' has been deprecated and is pending deletion!
        // Use:
        // 1. 'unique_hlocal_security_descriptor' if you need to free using 'LocalFree'.
        // 2. 'unique_private_security_descriptor' if you need to free using 'DestroyPrivateObjectSecurity'.

        // DEPRECATED: use unique_private_security_descriptor
        using unique_security_descriptor = unique_private_security_descriptor;
#endif

#endif
#if defined(__WIL_WINBASE_DESKTOP) && !defined(__WIL_WINBASE_DESKTOP_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINBASE_DESKTOP_STL
    typedef shared_any<unique_hheap> shared_hheap;
    typedef shared_any<unique_hlocal> shared_hlocal;
    typedef shared_any<unique_tls> shared_tls;
    typedef shared_any<unique_hlocal_security_descriptor> shared_hlocal_security_descriptor;
    typedef shared_any<unique_private_security_descriptor> shared_private_security_descriptor;
    typedef shared_any<unique_haccel> shared_haccel;
    typedef shared_any<unique_hcursor> shared_hcursor;
#if !defined(NOGDI)
    typedef shared_any<unique_hdesk> shared_hdesk;
    typedef shared_any<unique_hwinsta> shared_hwinsta;
#endif // !defined(NOGDI)
    typedef shared_any<unique_hwnd> shared_hwnd;

    typedef weak_any<shared_hheap> weak_hheap;
    typedef weak_any<shared_hlocal> weak_hlocal;
    typedef weak_any<shared_tls> weak_tls;
    typedef weak_any<shared_hlocal_security_descriptor> weak_hlocal_security_descriptor;
    typedef weak_any<shared_private_security_descriptor> weak_private_security_descriptor;
    typedef weak_any<shared_haccel> weak_haccel;
    typedef weak_any<shared_hcursor> weak_hcursor;
#if !defined(NOGDI)
    typedef weak_any<shared_hdesk> weak_hdesk;
    typedef weak_any<shared_hwinsta> weak_hwinsta;
#endif // !defined(NOGDI)
    typedef weak_any<shared_hwnd> weak_hwnd;
#endif // __WIL_WINBASE_DESKTOP_STL

#if defined(_COMBASEAPI_H_) && !defined(__WIL__COMBASEAPI_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM) && (NTDDI_VERSION >= NTDDI_WIN8)
#define __WIL__COMBASEAPI_H_
    typedef unique_any<CO_MTA_USAGE_COOKIE, decltype(&::CoDecrementMTAUsage), ::CoDecrementMTAUsage> unique_mta_usage_cookie;

    /// @cond
    namespace details
    {
        inline void __stdcall MultiQiCleanup(_In_ MULTI_QI* multiQi)
        {
            if (multiQi->pItf)
            {
                multiQi->pItf->Release();
                multiQi->pItf = nullptr;
            }
        }
    }
    /// @endcond

    //! A type that calls CoRevertToSelf on destruction (or reset()).
    using unique_coreverttoself_call = unique_call<decltype(&::CoRevertToSelf), ::CoRevertToSelf>;

    //! Calls CoImpersonateClient and fail-fasts if it fails; returns an RAII object that reverts
    _Check_return_ inline unique_coreverttoself_call CoImpersonateClient_failfast()
    {
        FAIL_FAST_IF_FAILED(::CoImpersonateClient());
        return unique_coreverttoself_call();
    }

    typedef unique_struct<MULTI_QI, decltype(&details::MultiQiCleanup), details::MultiQiCleanup> unique_multi_qi;
#endif // __WIL__COMBASEAPI_H_
#if defined(__WIL__COMBASEAPI_H_) && defined(WIL_ENABLE_EXCEPTIONS) && !defined(__WIL__COMBASEAPI_H_EXCEPTIONAL)
#define __WIL__COMBASEAPI_H_EXCEPTIONAL
    _Check_return_ inline unique_coreverttoself_call CoImpersonateClient()
    {
        THROW_IF_FAILED(::CoImpersonateClient());
        return unique_coreverttoself_call();
    }
#endif
#if defined(__WIL__COMBASEAPI_H_) && !defined(__WIL__COMBASEAPI_H__STL) && defined(WIL_RESOURCE_STL) && (NTDDI_VERSION >= NTDDI_WIN8)
#define __WIL__COMBASEAPI_H__STL
    typedef shared_any<unique_mta_usage_cookie> shared_mta_usage_cookie;
    typedef weak_any<shared_mta_usage_cookie> weak_mta_usage_cookie;
#endif // __WIL__COMBASEAPI_H__STL

#if defined(_COMBASEAPI_H_) && !defined(__WIL__COMBASEAPI_H_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) && (NTDDI_VERSION >= NTDDI_WIN8)
#define __WIL__COMBASEAPI_H_APP
    //! A type that calls CoUninitialize on destruction (or reset()).
    using unique_couninitialize_call = unique_call<decltype(&::CoUninitialize), ::CoUninitialize>;

    //! Calls CoInitializeEx and fail-fasts if it fails; returns an RAII object that reverts
    _Check_return_ inline unique_couninitialize_call CoInitializeEx_failfast(DWORD coinitFlags = 0 /*COINIT_MULTITHREADED*/)
    {
        FAIL_FAST_IF_FAILED(::CoInitializeEx(nullptr, coinitFlags));
        return unique_couninitialize_call();
    }
#endif // __WIL__COMBASEAPI_H_APP
#if defined(__WIL__COMBASEAPI_H_APP) && defined(WIL_ENABLE_EXCEPTIONS) && !defined(__WIL__COMBASEAPI_H_APPEXCEPTIONAL)
#define __WIL__COMBASEAPI_H_APPEXCEPTIONAL
    _Check_return_ inline unique_couninitialize_call CoInitializeEx(DWORD coinitFlags = 0 /*COINIT_MULTITHREADED*/)
    {
        THROW_IF_FAILED(::CoInitializeEx(nullptr, coinitFlags));
        return unique_couninitialize_call();
    }
#endif

#if defined(__ROAPI_H_) && !defined(__WIL__ROAPI_H_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) && (NTDDI_VERSION >= NTDDI_WIN8)
#define __WIL__ROAPI_H_APP
    //! A type that calls RoUninitialize on destruction (or reset()).
    //! Use as a replacement for Windows::Foundation::Uninitialize.
    using unique_rouninitialize_call = unique_call<decltype(&::RoUninitialize), ::RoUninitialize>;

    //! Calls RoInitialize and fail-fasts if it fails; returns an RAII object that reverts
    //! Use as a replacement for Windows::Foundation::Initialize
    _Check_return_ inline unique_rouninitialize_call RoInitialize_failfast(RO_INIT_TYPE initType = RO_INIT_MULTITHREADED)
    {
        FAIL_FAST_IF_FAILED(::RoInitialize(initType));
        return unique_rouninitialize_call();
    }
#endif // __WIL__ROAPI_H_APP
#if defined(__WIL__ROAPI_H_APP) && defined(WIL_ENABLE_EXCEPTIONS) && !defined(__WIL__ROAPI_H_APPEXCEPTIONAL)
#define __WIL__ROAPI_H_APPEXCEPTIONAL
    //! Calls RoInitialize and throws an exception if it fails; returns an RAII object that reverts
    //! Use as a replacement for Windows::Foundation::Initialize
    _Check_return_ inline unique_rouninitialize_call RoInitialize(RO_INIT_TYPE initType = RO_INIT_MULTITHREADED)
    {
        THROW_IF_FAILED(::RoInitialize(initType));
        return unique_rouninitialize_call();
    }
#endif

#if defined(__WINSTRING_H_) && !defined(__WIL__WINSTRING_H_)
#define __WIL__WINSTRING_H_
    typedef unique_any<HSTRING, decltype(&::WindowsDeleteString), ::WindowsDeleteString> unique_hstring;

    template<> inline unique_hstring make_unique_string_nothrow<unique_hstring>(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length) WI_NOEXCEPT
    {
        WI_ASSERT(source != nullptr); // the HSTRING version of this function does not suport this case
        if (length == static_cast<size_t>(-1))
        {
            length = wcslen(source);
        }

        unique_hstring result;
        ::WindowsCreateString(source, static_cast<UINT32>(length), &result);
        return result;
    }

    /// @cond
    namespace details
    {
        template<> struct string_maker<unique_hstring>
        {
            ~string_maker()
            {
                WindowsDeleteStringBuffer(m_bufferHandle); // ok to call with null
            }

            HRESULT make(_In_reads_opt_(length) PCWSTR source, size_t length)
            {
                if (source)
                {
                    RETURN_IF_FAILED(WindowsCreateString(source, static_cast<UINT32>(length), &value));
                }
                else
                {
                    // Need to set it to the empty string to support the empty string case.
                    value.reset(static_cast<HSTRING>(nullptr));
                    RETURN_IF_FAILED(WindowsPreallocateStringBuffer(static_cast<UINT32>(length), &m_charBuffer, &m_bufferHandle));
                }
                return S_OK;
            }
            wchar_t* buffer() { WI_ASSERT(m_charBuffer != nullptr);  return m_charBuffer; }

            unique_hstring release()
            {
                if (m_bufferHandle)
                {
                    const auto hr = WindowsPromoteStringBuffer(m_bufferHandle, &value);
                    FAIL_FAST_IF_FAILED(hr);  // Failure here is only due to invalid input, null terminator overwritten, a bug in the usage.
                    m_bufferHandle = nullptr; // after promotion must not delete
                }
                m_charBuffer = nullptr;
                return wistd::move(value);
            }

            static PCWSTR get(const wil::unique_hstring& value) { return WindowsGetStringRawBuffer(value.get(), nullptr); }

        private:
            unique_hstring value;
            HSTRING_BUFFER m_bufferHandle = nullptr;
            wchar_t* m_charBuffer = nullptr;
        };
    }
    /// @endcond
    typedef unique_any<HSTRING_BUFFER, decltype(&::WindowsDeleteStringBuffer), ::WindowsDeleteStringBuffer> unique_hstring_buffer;

    /** Promotes an hstring_buffer to an HSTRING.
    When an HSTRING_BUFFER object is promoted to a real string it must not be passed to WindowsDeleteString. The caller owns the
    HSTRING afterwards.
    ~~~
    HRESULT Type::MakePath(_Out_ HSTRING* path)
    {
    wchar_t* bufferStorage = nullptr;
    wil::unique_hstring_buffer theBuffer;
    RETURN_IF_FAILED(::WindowsPreallocateStringBuffer(65, &bufferStorage, &theBuffer));
    RETURN_IF_FAILED(::PathCchCombine(bufferStorage, 65, m_foo, m_bar));
    RETURN_IF_FAILED(wil::make_hstring_from_buffer_nothrow(wistd::move(theBuffer), path)));
    return S_OK;
    }
    ~~~
    */
    inline HRESULT make_hstring_from_buffer_nothrow(unique_hstring_buffer&& source, _Out_ HSTRING* promoted)
    {
        HRESULT hr = ::WindowsPromoteStringBuffer(source.get(), promoted);
        if (SUCCEEDED(hr))
        {
            source.release();
        }
        return hr;
    }

    //! A fail-fast variant of `make_hstring_from_buffer_nothrow`
    inline unique_hstring make_hstring_from_buffer_failfast(unique_hstring_buffer&& source)
    {
        unique_hstring result;
        FAIL_FAST_IF_FAILED(make_hstring_from_buffer_nothrow(wistd::move(source), &result));
        return result;
    }

#if defined WIL_ENABLE_EXCEPTIONS
    /** Promotes an hstring_buffer to an HSTRING.
    When an HSTRING_BUFFER object is promoted to a real string it must not be passed to WindowsDeleteString. The caller owns the
    HSTRING afterwards.
    ~~~
    wil::unique_hstring Type::Make()
    {
    wchar_t* bufferStorage = nullptr;
    wil::unique_hstring_buffer theBuffer;
    THROW_IF_FAILED(::WindowsPreallocateStringBuffer(65, &bufferStorage, &theBuffer));
    THROW_IF_FAILED(::PathCchCombine(bufferStorage, 65, m_foo, m_bar));
    return wil::make_hstring_from_buffer(wistd::move(theBuffer));
    }
    ~~~
    */
    inline unique_hstring make_hstring_from_buffer(unique_hstring_buffer&& source)
    {
        unique_hstring result;
        THROW_IF_FAILED(make_hstring_from_buffer_nothrow(wistd::move(source), &result));
        return result;
    }
#endif


#endif // __WIL__WINSTRING_H_
#if defined(__WIL__WINSTRING_H_) && !defined(__WIL__WINSTRING_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL__WINSTRING_H_STL
    typedef shared_any<unique_hstring> shared_hstring;
    typedef shared_any<unique_hstring_buffer> shared_hstring_buffer;
    typedef weak_any<shared_hstring> weak_hstring;
    typedef weak_any<shared_hstring_buffer> weak_hstring_buffer;
#endif // __WIL__WINSTRING_H_STL


#if defined(_WINREG_) && !defined(__WIL_WINREG_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WINREG_
    typedef unique_any<HKEY, decltype(&::RegCloseKey), ::RegCloseKey> unique_hkey;
#endif // __WIL_WINREG_
#if defined(__WIL_WINREG_) && !defined(__WIL_WINREG_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINREG_STL
    typedef shared_any<unique_hkey> shared_hkey;
    typedef weak_any<shared_hkey> weak_hkey;
#endif // __WIL_WINREG_STL


#if defined(__MSGQUEUE_H__) && !defined(__WIL__MSGQUEUE_H__)
#define __WIL__MSGQUEUE_H__
    typedef unique_any<details::handle_invalid_resource_policy<decltype(&::CloseMsgQueue), ::CloseMsgQueue> unique_msg_queue;
#endif // __WIL__MSGQUEUE_H__
#if defined(__WIL__MSGQUEUE_H__) && !defined(__WIL__MSGQUEUE_H__STL) && defined(WIL_RESOURCE_STL)
#define __WIL__MSGQUEUE_H__STL
    typedef shared_any<unique_msg_queue> shared_msg_queue;
    typedef weak_any<shared_msg_queue> weak_msg_queue;
#endif // __WIL__MSGQUEUE_H__STL


#if defined(_OLEAUTO_H_) && !defined(__WIL_OLEAUTO_H_)
#define __WIL_OLEAUTO_H_
    typedef unique_any<BSTR, decltype(&::SysFreeString), ::SysFreeString> unique_bstr;

    inline wil::unique_bstr make_bstr_nothrow(PCWSTR source) WI_NOEXCEPT
    {
        return wil::unique_bstr(::SysAllocString(source));
    }

    inline wil::unique_bstr make_bstr_failfast(PCWSTR source) WI_NOEXCEPT
    {
        return wil::unique_bstr(FAIL_FAST_IF_NULL_ALLOC(::SysAllocString(source)));
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    inline wil::unique_bstr make_bstr(PCWSTR source)
    {
        wil::unique_bstr result(make_bstr_nothrow(source));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_ENABLE_EXCEPTIONS

#endif // __WIL_OLEAUTO_H_
#if defined(__WIL_OLEAUTO_H_) && !defined(__WIL_OLEAUTO_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_OLEAUTO_H_STL
    typedef shared_any<unique_bstr> shared_bstr;
    typedef weak_any<shared_bstr> weak_bstr;
#endif // __WIL_OLEAUTO_H_STL


#if (defined(_WININET_) || defined(_DUBINET_)) && !defined(__WIL_WININET_)
#define __WIL_WININET_
    typedef unique_any<HINTERNET, decltype(&::InternetCloseHandle), ::InternetCloseHandle> unique_hinternet;
#endif // __WIL_WININET_
#if defined(__WIL_WININET_) && !defined(__WIL_WININET_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WININET_STL
    typedef shared_any<unique_hinternet> shared_hinternet;
    typedef weak_any<shared_hinternet> weak_hinternet;
#endif // __WIL_WININET_STL


#if defined(_WINHTTPX_) && !defined(__WIL_WINHTTP_)
#define __WIL_WINHTTP_
    typedef unique_any<HINTERNET, decltype(&::WinHttpCloseHandle), ::WinHttpCloseHandle> unique_winhttp_hinternet;
#endif // __WIL_WINHTTP_
#if defined(__WIL_WINHTTP_) && !defined(__WIL_WINHTTP_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINHTTP_STL
    typedef shared_any<unique_winhttp_hinternet> shared_winhttp_hinternet;
    typedef weak_any<shared_winhttp_hinternet> weak_winhttp_hinternet;
#endif // __WIL_WINHTTP_STL


#if defined(_WINSOCKAPI_) && !defined(__WIL_WINSOCKAPI_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WINSOCKAPI_
    typedef unique_any<SOCKET, int (WINAPI*)(SOCKET), ::closesocket, details::pointer_access_all, SOCKET, INVALID_SOCKET, SOCKET> unique_socket;
#endif // __WIL_WINSOCKAPI_
#if defined(__WIL_WINSOCKAPI_) && !defined(__WIL_WINSOCKAPI_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINSOCKAPI_STL
    typedef shared_any<unique_socket> shared_socket;
    typedef weak_any<shared_socket> weak_socket;
#endif // __WIL_WINSOCKAPI_STL


#if defined(_WINGDI_) && !defined(__WIL_WINGDI_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && !defined(NOGDI) && !defined(WIL_KERNEL_MODE)
#define __WIL_WINGDI_
    struct WindowDC
    {
        HDC dc;
        HWND hwnd;
        WindowDC(HDC dc_, HWND hwnd_ = nullptr) WI_NOEXCEPT { dc = dc_; hwnd = hwnd_; }
        operator HDC() const WI_NOEXCEPT { return dc; }
        static void Close(WindowDC wdc) WI_NOEXCEPT { ::ReleaseDC(wdc.hwnd, wdc.dc); }
    };
    typedef unique_any<HDC, decltype(&WindowDC::Close), WindowDC::Close, details::pointer_access_all, WindowDC> unique_hdc_window;

    struct PaintDC
    {
        HWND hwnd;
        PAINTSTRUCT ps;
        PaintDC(HDC hdc = nullptr) { ::ZeroMemory(this, sizeof(*this)); ps.hdc = hdc; }
        operator HDC() const WI_NOEXCEPT { return ps.hdc; }
        static void Close(PaintDC pdc) WI_NOEXCEPT { ::EndPaint(pdc.hwnd, &pdc.ps); }
    };
    typedef unique_any<HDC, decltype(&PaintDC::Close), PaintDC::Close, details::pointer_access_all, PaintDC> unique_hdc_paint;

    struct SelectResult
    {
        HGDIOBJ hgdi;
        HDC hdc;
        SelectResult(HGDIOBJ hgdi_, HDC hdc_ = nullptr) WI_NOEXCEPT { hgdi = hgdi_; hdc = hdc_; }
        operator HGDIOBJ() const WI_NOEXCEPT { return hgdi; }
        static void Close(SelectResult sr) WI_NOEXCEPT { ::SelectObject(sr.hdc, sr.hgdi); }
    };
    typedef unique_any<HGDIOBJ, decltype(&SelectResult::Close), SelectResult::Close, details::pointer_access_all, SelectResult> unique_select_object;

    inline unique_hdc_window GetDC(HWND hwnd) WI_NOEXCEPT
    {
        return unique_hdc_window(WindowDC(::GetDC(hwnd), hwnd));
    }

    inline unique_hdc_window GetWindowDC(HWND hwnd) WI_NOEXCEPT
    {
        return unique_hdc_window(WindowDC(::GetWindowDC(hwnd), hwnd));
    }

    inline unique_hdc_paint BeginPaint(HWND hwnd, _Out_opt_ PPAINTSTRUCT pPaintStruct = nullptr) WI_NOEXCEPT
    {
        PaintDC pdc;
        HDC hdc = ::BeginPaint(hwnd, &pdc.ps);
        assign_to_opt_param(pPaintStruct, pdc.ps);
        return (hdc == nullptr) ? unique_hdc_paint() : unique_hdc_paint(pdc);
    }

    inline unique_select_object SelectObject(HDC hdc, HGDIOBJ gdiobj) WI_NOEXCEPT
    {
        return unique_select_object(SelectResult(::SelectObject(hdc, gdiobj), hdc));
    }

    typedef unique_any<HGDIOBJ, decltype(&::DeleteObject), ::DeleteObject> unique_hgdiobj;
    typedef unique_any<HPEN, decltype(&::DeleteObject), ::DeleteObject> unique_hpen;
    typedef unique_any<HBRUSH, decltype(&::DeleteObject), ::DeleteObject> unique_hbrush;
    typedef unique_any<HFONT, decltype(&::DeleteObject), ::DeleteObject> unique_hfont;
    typedef unique_any<HBITMAP, decltype(&::DeleteObject), ::DeleteObject> unique_hbitmap;
    typedef unique_any<HRGN, decltype(&::DeleteObject), ::DeleteObject> unique_hrgn;
    typedef unique_any<HPALETTE, decltype(&::DeleteObject), ::DeleteObject> unique_hpalette;
    typedef unique_any<HDC, decltype(&::DeleteDC), ::DeleteDC> unique_hdc;
    typedef unique_any<HICON, decltype(&::DestroyIcon), ::DestroyIcon> unique_hicon;
    typedef unique_any<HMENU, decltype(&::DestroyMenu), ::DestroyMenu> unique_hmenu;
#endif // __WIL_WINGDI_
#if defined(__WIL_WINGDI_) && !defined(__WIL_WINGDI_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINGDI_STL
    typedef shared_any<unique_hgdiobj> shared_hgdiobj;
    typedef shared_any<unique_hpen> shared_hpen;
    typedef shared_any<unique_hbrush> shared_hbrush;
    typedef shared_any<unique_hfont> shared_hfont;
    typedef shared_any<unique_hbitmap> shared_hbitmap;
    typedef shared_any<unique_hrgn> shared_hrgn;
    typedef shared_any<unique_hpalette> shared_hpalette;
    typedef shared_any<unique_hdc> shared_hdc;
    typedef shared_any<unique_hicon> shared_hicon;
    typedef shared_any<unique_hmenu> shared_hmenu;

    typedef weak_any<shared_hgdiobj> weak_hgdiobj;
    typedef weak_any<shared_hpen> weak_hpen;
    typedef weak_any<shared_hbrush> weak_hbrush;
    typedef weak_any<shared_hfont> weak_hfont;
    typedef weak_any<shared_hbitmap> weak_hbitmap;
    typedef weak_any<shared_hrgn> weak_hrgn;
    typedef weak_any<shared_hpalette> weak_hpalette;
    typedef weak_any<shared_hdc> weak_hdc;
    typedef weak_any<shared_hicon> weak_hicon;
    typedef weak_any<shared_hmenu> weak_hmenu;
#endif // __WIL_WINGDI_STL

#if defined(_PACKAGEIDENTITY_H_) && !defined(__WIL_PACKAGEIDENTITY_H_)
#define __WIL_PACKAGEIDENTITY_H_
    template<typename T>
    using unique_appxmem_ptr = wistd::unique_ptr<T, function_deleter<decltype(&AppXFreeMemory), AppXFreeMemory>>;
#endif // __WIL_PACKAGEIDENTITY_H_

#if defined(_WTSAPI32APIEXT_H_) && !defined(__WIL_WTSAPI32APIEXT_H_)
#define __WIL_WTSAPI32APIEXT_H_
    template<typename T>
    using unique_wtsmem_ptr = wistd::unique_ptr<T, function_deleter<decltype(&WTSFreeMemory), WTSFreeMemory>>;
#endif // __WIL_WTSAPI32APIEXT_H_

#if defined(_WINSCARD_H_) && !defined(__WIL_WINSCARD_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WINSCARD_H_
    typedef unique_any<SCARDCONTEXT, decltype(&::SCardReleaseContext), ::SCardReleaseContext> unique_scardctx;
#endif // __WIL_WINSCARD_H_
#if defined(__WIL_WINSCARD_H_) && !defined(__WIL_WINSCARD_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WINSCARD_H_STL
    typedef shared_any<unique_scardctx> shared_scardctx;
    typedef weak_any<shared_scardctx> weak_scardctx;
#endif // __WIL_WINSCARD_H_STL


#if defined(__WINCRYPT_H__) && !defined(__WIL__WINCRYPT_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL__WINCRYPT_H__
    /// @cond
    namespace details
    {
        inline void __stdcall CertCloseStoreNoParam(_Pre_opt_valid_ _Frees_ptr_opt_ HCERTSTORE hCertStore) WI_NOEXCEPT
        {
            ::CertCloseStore(hCertStore, 0);
        }

        inline void __stdcall CryptReleaseContextNoParam(_Pre_opt_valid_ _Frees_ptr_opt_ HCRYPTPROV hCryptCtx) WI_NOEXCEPT
        {
            ::CryptReleaseContext(hCryptCtx, 0);
        }
    }
    /// @endcond

    typedef unique_any<PCCERT_CONTEXT, decltype(&::CertFreeCertificateContext), ::CertFreeCertificateContext> unique_cert_context;
    typedef unique_any<PCCERT_CHAIN_CONTEXT, decltype(&::CertFreeCertificateChain), ::CertFreeCertificateChain> unique_cert_chain_context;
    typedef unique_any<HCERTSTORE, decltype(&details::CertCloseStoreNoParam), details::CertCloseStoreNoParam> unique_hcertstore;
    typedef unique_any<HCRYPTPROV, decltype(&details::CryptReleaseContextNoParam), details::CryptReleaseContextNoParam> unique_hcryptprov;
    typedef unique_any<HCRYPTKEY, decltype(&::CryptDestroyKey), ::CryptDestroyKey> unique_hcryptkey;
    typedef unique_any<HCRYPTHASH, decltype(&::CryptDestroyHash), ::CryptDestroyHash> unique_hcrypthash;
#endif // __WIL__WINCRYPT_H__
#if defined(__WIL__WINCRYPT_H__) && !defined(__WIL__WINCRYPT_H__STL) && defined(WIL_RESOURCE_STL)
#define __WIL__WINCRYPT_H__STL
    typedef shared_any<unique_cert_context> shared_cert_context;
    typedef shared_any<unique_cert_chain_context> shared_cert_chain_context;
    typedef shared_any<unique_hcertstore> shared_hcertstore;
    typedef shared_any<unique_hcryptprov> shared_hcryptprov;
    typedef shared_any<unique_hcryptkey> shared_hcryptkey;
    typedef shared_any<unique_hcrypthash> shared_hcrypthash;

    typedef weak_any<shared_cert_context> weak_cert_context;
    typedef weak_any<shared_cert_chain_context> weak_cert_chain_context;
    typedef weak_any<shared_hcertstore> weak_hcertstore;
    typedef weak_any<shared_hcryptprov> weak_hcryptprov;
    typedef weak_any<shared_hcryptkey> weak_hcryptkey;
    typedef weak_any<shared_hcrypthash> weak_hcrypthash;
#endif // __WIL__WINCRYPT_H__STL


#if defined(__NCRYPT_H__) && !defined(__WIL_NCRYPT_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_NCRYPT_H__
    using ncrypt_deleter = function_deleter<decltype(&::NCryptFreeBuffer), NCryptFreeBuffer>;

    template <typename T>
    using unique_ncrypt_ptr = wistd::unique_ptr<T, ncrypt_deleter>;

    typedef unique_any<NCRYPT_PROV_HANDLE, decltype(&::NCryptFreeObject), ::NCryptFreeObject> unique_ncrypt_prov;
    typedef unique_any<NCRYPT_KEY_HANDLE, decltype(&::NCryptFreeObject), ::NCryptFreeObject> unique_ncrypt_key;
    typedef unique_any<NCRYPT_SECRET_HANDLE, decltype(&::NCryptFreeObject), ::NCryptFreeObject> unique_ncrypt_secret;
#endif // __WIL_NCRYPT_H__
#if defined(__WIL_NCRYPT_H__) && !defined(__WIL_NCRYPT_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_NCRYPT_H_STL
    typedef shared_any<unique_ncrypt_prov> shared_ncrypt_prov;
    typedef shared_any<unique_ncrypt_key> shared_ncrypt_key;
    typedef shared_any<unique_ncrypt_secret> shared_ncrypt_secret;

    typedef weak_any<shared_ncrypt_prov> weak_ncrypt_prov;
    typedef weak_any<shared_ncrypt_key> weak_ncrypt_key;
    typedef weak_any<shared_ncrypt_secret> weak_ncrypt_secret;
#endif // __WIL_NCRYPT_H_STL


#if defined(_DSREGAPI_) && !defined(__WIL_DSREGAPI__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_DSREGAPI__
    typedef unique_any<PDSREG_JOIN_INFO, decltype(&::DsrFreeJoinInfo), ::DsrFreeJoinInfo> unique_join_info;
    typedef unique_any<PCXH_SCENARIO_INFO, decltype(&::DsrFreeCxhScenarioInfo), ::DsrFreeCxhScenarioInfo> unique_scenario_info;
#endif // __WIL_DSREGAPI__
#if defined(__WIL_DSREGAPI__) && !defined(__WIL_DSREGAPI_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_DSREGAPI_STL
    typedef shared_any<unique_join_info> shared_join_info;
    typedef shared_any<unique_scenario_info> shared_scenario_info;

    typedef weak_any<shared_join_info> weak_join_info;
    typedef weak_any<shared_scenario_info> weak_scenario_info;
#endif // __WIL_DSREGAPI_STL

#if defined(__BCRYPT_H__) && !defined(__WIL_BCRYPT_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_BCRYPT_H__
    /// @cond
    namespace details
    {
        inline void __stdcall BCryptCloseAlgorithmProviderNoFlags(_Pre_opt_valid_ _Frees_ptr_opt_ BCRYPT_ALG_HANDLE hAlgorithm) WI_NOEXCEPT
        {
            ::BCryptCloseAlgorithmProvider(hAlgorithm, 0);
        }
    }
    /// @endcond

    using bcrypt_deleter = function_deleter<decltype(&::BCryptFreeBuffer), BCryptFreeBuffer>;

    template <typename T>
    using unique_bcrypt_ptr = wistd::unique_ptr<T, bcrypt_deleter>;

    typedef unique_any<BCRYPT_ALG_HANDLE, decltype(&details::BCryptCloseAlgorithmProviderNoFlags), details::BCryptCloseAlgorithmProviderNoFlags> unique_bcrypt_algorithm;
    typedef unique_any<BCRYPT_HASH_HANDLE, decltype(&::BCryptDestroyHash), ::BCryptDestroyHash> unique_bcrypt_hash;
    typedef unique_any<BCRYPT_KEY_HANDLE, decltype(&::BCryptDestroyKey), ::BCryptDestroyKey> unique_bcrypt_key;
    typedef unique_any<BCRYPT_SECRET_HANDLE, decltype(&::BCryptDestroySecret), ::BCryptDestroySecret> unique_bcrypt_secret;
#endif // __WIL_BCRYPT_H__
#if defined(__WIL_BCRYPT_H__) && !defined(__WIL_BCRYPT_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_BCRYPT_H_STL
    typedef shared_any<unique_bcrypt_algorithm> shared_bcrypt_algorithm;
    typedef shared_any<unique_bcrypt_hash> shared_bcrypt_hash;
    typedef shared_any<unique_bcrypt_key> shared_bcrypt_key;
    typedef shared_any<unique_bcrypt_secret> shared_bcrypt_secret;

    typedef weak_any<shared_bcrypt_algorithm> weak_bcrypt_algorithm;
    typedef weak_any<shared_bcrypt_hash> weak_bcrypt_hash;
    typedef weak_any<unique_bcrypt_key> weak_bcrypt_key;
    typedef weak_any<shared_bcrypt_secret> weak_bcrypt_secret;
#endif // __WIL_BCRYPT_H_STL


#if defined(_OBJBASE_H_) && !defined(__WIL_OBJBASE_H_)
#define __WIL_OBJBASE_H_
    using cotaskmem_deleter = function_deleter<decltype(&::CoTaskMemFree), ::CoTaskMemFree>;

    template <typename T>
    using unique_cotaskmem_ptr = wistd::unique_ptr<T, cotaskmem_deleter>;

    template <typename T>
    using unique_cotaskmem_array_ptr = unique_array_ptr<T, cotaskmem_deleter>;

    /** Provides `std::make_unique()` semantics for resources allocated with `CoTaskMemAlloc()` in a context that may not throw upon allocation failure.
    Use `wil::make_unique_cotaskmem_nothrow()` for resources returned from APIs that must satisfy a memory allocation contract that requires the use of `CoTaskMemAlloc()` / `CoTaskMemFree()`.
    Use `wil::make_unique_nothrow()` when `CoTaskMemAlloc()` is not required.

    Allocations are initialized with placement new and will call constructors (if present), but this does not guarantee initialization.

    Note that `wil::make_unique_cotaskmem_nothrow()` is not marked WI_NOEXCEPT as it may be used to create an exception-based class that may throw in its constructor.
    ~~~
    auto foo = wil::make_unique_cotaskmem_nothrow<Foo>();
    if (foo)
    {
    // initialize allocated Foo object as appropriate
    }
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_cotaskmem_ptr<T>>::type make_unique_cotaskmem_nothrow(Args&&... args)
    {
        static_assert(wistd::is_trivially_destructible<T>::value, "T has a destructor that won't be run when used with this function; use make_unique instead");
        unique_cotaskmem_ptr<T> sp(static_cast<T*>(::CoTaskMemAlloc(sizeof(T))));
        if (sp)
        {
            // use placement new to initialize memory from the previous allocation
            new (sp.get()) T(wistd::forward<Args>(args)...);
        }
        return sp;
    }

    /** Provides `std::make_unique()` semantics for array resources allocated with `CoTaskMemAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_cotaskmem_nothrow<Foo[]>(size);
    if (foos)
    {
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_cotaskmem_ptr<T>>::type make_unique_cotaskmem_nothrow(size_t size)
    {
        typedef typename wistd::remove_extent<T>::type E;
        static_assert(wistd::is_trivially_destructible<E>::value, "E has a destructor that won't be run when used with this function; use make_unique instead");
        FAIL_FAST_IF((SIZE_MAX / sizeof(E)) < size);
        size_t allocSize = sizeof(E) * size;
        unique_cotaskmem_ptr<T> sp(static_cast<E*>(::CoTaskMemAlloc(allocSize)));
        if (sp)
        {
            // use placement new to initialize memory from the previous allocation;
            // note that array placement new cannot be used as the standard allows for operator new[]
            // to consume overhead in the allocation for internal bookkeeping
            for (auto& elem : make_range(static_cast<E*>(sp.get()), size))
            {
                new (&elem) E();
            }
        }
        return sp;
    }

    /** Provides `std::make_unique()` semantics for resources allocated with `CoTaskMemAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_cotaskmem_failfast<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_cotaskmem_ptr<T>>::type make_unique_cotaskmem_failfast(Args&&... args)
    {
        unique_cotaskmem_ptr<T> result(make_unique_cotaskmem_nothrow<T>(wistd::forward<Args>(args)...));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for array resources allocated with `CoTaskMemAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_cotaskmem_failfast<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_cotaskmem_ptr<T>>::type make_unique_cotaskmem_failfast(size_t size)
    {
        unique_cotaskmem_ptr<T> result(make_unique_cotaskmem_nothrow<T>(size));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Provides `std::make_unique()` semantics for resources allocated with `CoTaskMemAlloc()`.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_cotaskmem<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_cotaskmem_ptr<T>>::type make_unique_cotaskmem(Args&&... args)
    {
        unique_cotaskmem_ptr<T> result(make_unique_cotaskmem_nothrow<T>(wistd::forward<Args>(args)...));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for array resources allocated with `CoTaskMemAlloc()`.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_cotaskmem<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_cotaskmem_ptr<T>>::type make_unique_cotaskmem(size_t size)
    {
        unique_cotaskmem_ptr<T> result(make_unique_cotaskmem_nothrow<T>(size));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_ENABLE_EXCEPTIONS

    typedef unique_any<void*, decltype(&::CoTaskMemFree), ::CoTaskMemFree> unique_cotaskmem;
    typedef unique_any<PWSTR, decltype(&::CoTaskMemFree), ::CoTaskMemFree> unique_cotaskmem_string;
#ifndef WIL_NO_ASNI_STRINGS
    typedef unique_any<PSTR, decltype(&::CoTaskMemFree), ::CoTaskMemFree> unique_cotaskmem_ansistring;
#endif // WIL_NO_ASNI_STRINGS

    /// @cond
    namespace details
    {
        template<> struct string_allocator<unique_cotaskmem_string>
        {
            static void* allocate(size_t size) WI_NOEXCEPT
            {
                return ::CoTaskMemAlloc(size);
            }
        };
    }
    /// @endcond

    inline auto make_cotaskmem_string_nothrow(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_string_nothrow<unique_cotaskmem_string>(source, length);
    }

    inline auto make_cotaskmem_string_failfast(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1)) WI_NOEXCEPT
    {
        return make_unique_string_failfast<unique_cotaskmem_string>(source, length);
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    inline auto make_cotaskmem_string(
        _When_(length != static_cast<size_t>(-1), _In_reads_opt_(length))
        _When_(length == static_cast<size_t>(-1), _In_)
        PCWSTR source, size_t length = static_cast<size_t>(-1))
    {
        return make_unique_string<unique_cotaskmem_string>(source, length);
    }

#endif // WIL_ENABLE_EXCEPTIONS
#endif // __WIL_OBJBASE_H_
#if defined(__WIL_OBJBASE_H_) && !defined(__WIL_OBJBASE_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_OBJBASE_H_STL
    typedef shared_any<unique_cotaskmem> shared_cotaskmem;
    typedef weak_any<shared_cotaskmem> weak_cotaskmem;
    typedef shared_any<unique_cotaskmem_string> shared_cotaskmem_string;
    typedef weak_any<shared_cotaskmem_string> weak_cotaskmem_string;
#endif // __WIL_OBJBASE_H_STL

#if defined(__WIL_OBJBASE_H_) && defined(__WIL_WINBASE_) && !defined(__WIL_OBJBASE_AND_WINBASE_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_OBJBASE_AND_WINBASE_H_

    struct cotaskmem_secure_deleter
    {
        template <typename T>
        void operator()(_Pre_opt_valid_ _Frees_ptr_opt_ T* p) const
        {
            IMalloc* malloc;
            if (SUCCEEDED(::CoGetMalloc(1, &malloc)))
            {
                size_t const size = malloc->GetSize(p);
                if (size != static_cast<size_t>(-1))
                {
                    ::SecureZeroMemory(p, size);
                }
                malloc->Release();
            }
            ::CoTaskMemFree(p);
        }
    };

    template <typename T>
    using unique_cotaskmem_secure_ptr = wistd::unique_ptr<T, cotaskmem_secure_deleter>;

    /** Provides `std::make_unique()` semantics for secure resources allocated with `CoTaskMemAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_cotaskmem_secure_nothrow<Foo>();
    if (foo)
    {
    // initialize allocated Foo object as appropriate
    }
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_cotaskmem_secure_ptr<T>>::type make_unique_cotaskmem_secure_nothrow(Args&&... args)
    {
        return unique_cotaskmem_secure_ptr<T>(make_unique_cotaskmem_nothrow<T>(wistd::forward<Args>(args)...).release());
    }

    /** Provides `std::make_unique()` semantics for secure array resources allocated with `CoTaskMemAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_cotaskmem_secure_nothrow<Foo[]>(size);
    if (foos)
    {
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_cotaskmem_secure_ptr<T>>::type make_unique_cotaskmem_secure_nothrow(size_t size)
    {
        return unique_cotaskmem_secure_ptr<T>(make_unique_cotaskmem_nothrow<T>(size).release());
    }

    /** Provides `std::make_unique()` semantics for secure resources allocated with `CoTaskMemAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_cotaskmem_secure_failfast<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_cotaskmem_secure_ptr<T>>::type make_unique_cotaskmem_secure_failfast(Args&&... args)
    {
        unique_cotaskmem_secure_ptr<T> result(make_unique_cotaskmem_secure_nothrow<T>(wistd::forward<Args>(args)...));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for secure array resources allocated with `CoTaskMemAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_cotaskmem_secure_failfast<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_cotaskmem_secure_ptr<T>>::type make_unique_cotaskmem_secure_failfast(size_t size)
    {
        unique_cotaskmem_secure_ptr<T> result(make_unique_cotaskmem_secure_nothrow<T>(size));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Provides `std::make_unique()` semantics for secure resources allocated with `CoTaskMemAlloc()`.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    auto foo = wil::make_unique_cotaskmem_secure<Foo>();
    // initialize allocated Foo object as appropriate
    ~~~
    */
    template <typename T, typename... Args>
    inline typename wistd::enable_if<!wistd::is_array<T>::value, unique_cotaskmem_secure_ptr<T>>::type make_unique_cotaskmem_secure(Args&&... args)
    {
        unique_cotaskmem_secure_ptr<T> result(make_unique_cotaskmem_secure_nothrow<T>(wistd::forward<Args>(args)...));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }

    /** Provides `std::make_unique()` semantics for secure array resources allocated with `CoTaskMemAlloc()`.
    See the overload of `wil::make_unique_cotaskmem_nothrow()` for non-array types for more details.
    ~~~
    const size_t size = 42;
    auto foos = wil::make_unique_cotaskmem_secure<Foo[]>(size);
    for (auto& elem : wil::make_range(foos.get(), size))
    {
    // initialize allocated Foo objects as appropriate
    }
    ~~~
    */
    template <typename T>
    inline typename wistd::enable_if<wistd::is_array<T>::value && wistd::extent<T>::value == 0, unique_cotaskmem_secure_ptr<T>>::type make_unique_cotaskmem_secure(size_t size)
    {
        unique_cotaskmem_secure_ptr<T> result(make_unique_cotaskmem_secure_nothrow<T>(size));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif // WIL_ENABLE_EXCEPTIONS

    typedef unique_cotaskmem_secure_ptr<wchar_t[]> unique_cotaskmem_string_secure;

    /** Copies a given string into secure memory allocated with `CoTaskMemAlloc()` in a context that may not throw upon allocation failure.
    See the overload of `wil::make_cotaskmem_string_nothrow()` with supplied length for more details.
    ~~~
    auto str = wil::make_cotaskmem_string_secure_nothrow(L"a string");
    if (str)
    {
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"
    }
    ~~~
    */
    inline unique_cotaskmem_string_secure make_cotaskmem_string_secure_nothrow(_In_ PCWSTR source) WI_NOEXCEPT
    {
        return unique_cotaskmem_string_secure(make_cotaskmem_string_nothrow(source).release());
    }

    /** Copies a given string into secure memory allocated with `CoTaskMemAlloc()` in a context that must fail fast upon allocation failure.
    See the overload of `wil::make_cotaskmem_string_nothrow()` with supplied length for more details.
    ~~~
    auto str = wil::make_cotaskmem_string_secure_failfast(L"a string");
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"
    ~~~
    */
    inline unique_cotaskmem_string_secure make_cotaskmem_string_secure_failfast(_In_ PCWSTR source) WI_NOEXCEPT
    {
        unique_cotaskmem_string_secure result(make_cotaskmem_string_secure_nothrow(source));
        FAIL_FAST_IF_NULL_ALLOC(result);
        return result;
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    /** Copies a given string into secure memory allocated with `CoTaskMemAlloc()`.
    See the overload of `wil::make_cotaskmem_string_nothrow()` with supplied length for more details.
    ~~~
    auto str = wil::make_cotaskmem_string_secure(L"a string");
    std::wcout << L"This is " << str.get() << std::endl; // prints "This is a string"
    ~~~
    */
    inline unique_cotaskmem_string_secure make_cotaskmem_string_secure(_In_ PCWSTR source)
    {
        unique_cotaskmem_string_secure result(make_cotaskmem_string_secure_nothrow(source));
        THROW_IF_NULL_ALLOC(result);
        return result;
    }
#endif
#endif // __WIL_OBJBASE_AND_WINBASE_H_

#if defined(_OLE2_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && !defined(__WIL_OLE2_H_)
#define __WIL_OLE2_H_
    typedef unique_struct<STGMEDIUM, decltype(&::ReleaseStgMedium), ::ReleaseStgMedium> unique_stg_medium;
    struct unique_hglobal_locked : public unique_any<void*, decltype(&::GlobalUnlock), ::GlobalUnlock>
    {
        unique_hglobal_locked() = delete;

        unique_hglobal_locked(_In_ STGMEDIUM& medium) : unique_any<void*, decltype(&::GlobalUnlock), ::GlobalUnlock>(medium.hGlobal)
        {
            // GlobalLock returns a pointer to the associated global memory block and that's what callers care about.
            m_globalMemory = GlobalLock(medium.hGlobal);
        }

        // In the future, we could easily add additional constructor overloads such as unique_hglobal_locked(HGLOBAL) to make consumption easier.

        pointer get() const
        {
            return m_globalMemory;
        }

    private:
        pointer m_globalMemory;
    };
#endif // __WIL_OLE2_H_

#if (defined(__SECURITY_RUNTIME_H__) || defined(__ACCESSHLPR_H__)) && !defined(__WIL__SECURITY_RUNTIME_H__)
#define __WIL__SECURITY_RUNTIME_H__
    typedef unique_any<PSECURITY_DESCRIPTOR, decltype(&::FreeTransientObjectSecurityDescriptor), ::FreeTransientObjectSecurityDescriptor> unique_transient_security_descriptor;
#endif // __WIL__SECURITY_RUNTIME_H__
#if defined(__WIL__SECURITY_RUNTIME_H__) && !defined(__WIL__SECURITY_RUNTIME_H__STL) && defined(WIL_RESOURCE_STL)
#define __WIL__SECURITY_RUNTIME_H__STL
    typedef shared_any<unique_transient_security_descriptor> shared_transient_security_descriptor;
    typedef weak_any<shared_transient_security_descriptor> weak_transient_security_descriptor;
#endif // __WIL__SECURITY_RUNTIME_H__STL

#if defined(_VAULTCLIEXT_H_) && !defined(__WIL_VAULTCLEXT_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_VAULTCLEXT_H__
    /// @cond
    namespace details
    {
        inline void __stdcall VaultCloseVault(_Pre_opt_valid_ _Frees_ptr_opt_ VAULT_HANDLE hVault) WI_NOEXCEPT
        {
#pragma prefast(suppress:6031, "annotation issue: VaultCloseVault is annotated __checkreturn, but a failure is not actionable to us");
            ::VaultCloseVault(&hVault);
        }
    }
    /// @endcond

    typedef unique_any<VAULT_HANDLE, decltype(&details::VaultCloseVault), details::VaultCloseVault> unique_vaulthandle;

    typedef unique_any<PVAULT_ITEM, decltype(&::VaultFree), ::VaultFree> unique_vaultitem;
#endif // __WIL_VAULTCLEXT_H__
#if defined(__WIL_VAULTCLEXT_H__) && !defined(__WIL_VAULTCLEXT_H__STL) && defined(WIL_RESOURCE_STL)
#define __WIL_VAULTCLEXT_H__
    typedef shared_any<unique_vaulthandle> shared_vaulthandle;
    typedef weak_any<unique_vaulthandle> weak_vaulthandle;

    typedef shared_any<unique_vaultitem> shared_vaultitem;
    typedef weak_any<unique_vaultitem> weak_vaultitem;
#endif // __WIL_VAULTCLEXT_H__

#if defined(_INC_COMMCTRL) && !defined(__WIL_INC_COMMCTRL)
#define __WIL_INC_COMMCTRL
    typedef unique_any<HIMAGELIST, decltype(&::ImageList_Destroy), ::ImageList_Destroy> unique_himagelist;
#endif // __WIL_INC_COMMCTRL
#if defined(__WIL_INC_COMMCTRL) && !defined(__WIL_INC_COMMCTRL_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_INC_COMMCTRL_STL
    typedef shared_any<unique_himagelist> shared_himagelist;
    typedef weak_any<shared_himagelist> weak_himagelist;
#endif // __WIL_INC_COMMCTRL_STL


#if defined(_WINSVC_) && !defined(__WIL_WINSVC) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_HANDLE_H_WINSVC
    typedef unique_any<SC_HANDLE, decltype(&::CloseServiceHandle), ::CloseServiceHandle> unique_schandle;
#endif // __WIL_HANDLE_H_WINSVC
#if defined(__WIL_HANDLE_H_WINSVC) && !defined(__WIL_HANDLE_H_WINSVC_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_HANDLE_H_WINSVC_STL
    typedef shared_any<unique_schandle> shared_schandle;
    typedef weak_any<shared_schandle> weak_schandle;
#endif // __WIL_HANDLE_H_WINSVC_STL

#if defined(_INC_DEVOBJ) && !defined(__WIL_INC_DEVOBJ_) && defined(__WIL_WINBASE_)
#define __WIL_INC_DEVOBJ_
    typedef unique_any_handle_invalid<decltype(&::DevObjDestroyDeviceInfoList), ::DevObjDestroyDeviceInfoList> unique_hdolist;
#endif // __WIL_INC_DEVOBJ_
#if defined(__WIL_INC_DEVOBJ_) && !defined(__WIL_INC_DEVOBJ_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_INC_DEVOBJ_STL
    typedef shared_any<unique_hdolist> shared_hdolist;
    typedef weak_any<shared_hdolist> weak_hdolist;
#endif // __WIL_INC_DEVOBJ_STL

#if defined(_INC_STDIO) && !defined(__WIL_INC_STDIO) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_INC_STDIO
    typedef unique_any<FILE*, decltype(&::_pclose), ::_pclose> unique_pipe;
#endif // __WIL_INC_STDIO
#if defined(__WIL_INC_STDIO) && !defined(__WIL__INC_STDIO_STL) && defined(WIL_RESOURCE_STL)
#define __WIL__INC_STDIO_STL
    typedef shared_any<unique_pipe> shared_pipe;
    typedef weak_any<shared_pipe> weak_pipe;
#endif // __WIL__INC_STDIO_STL

#if defined(_NTLSA_) && !defined(__WIL_NTLSA_)
#define __WIL_NTLSA_
    typedef unique_any<LSA_HANDLE, decltype(&::LsaClose), ::LsaClose> unique_hlsa;

    using lsa_freemem_deleter = function_deleter<decltype(&::LsaFreeMemory), LsaFreeMemory>;

    template <typename T>
    using unique_lsamem_ptr = wistd::unique_ptr<T, lsa_freemem_deleter>;
#endif // _NTLSA_
#if defined(_NTLSA_) && !defined(__WIL_NTLSA_SHARED) && defined(__WIL_SHARED)
#define __WIL_NTLSA_SHARED;
    typedef shared_any<unique_hlsa> shared_hlsa;
    typedef weak_any<shared_hlsa> weak_hlsa;

    typedef shared_any<unique_lsamem_ptr> shared_lsamem_ptr;
    typedef weak_any<shared_lsamem_ptr> weak_lsamem_ptr;
#endif // _NTLSA_

#if defined(_NTLSA_IFS_) && !defined(__WIL_HANDLE_H_NTLSA_IFS_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_HANDLE_H_NTLSA_IFS_
    using lsa_deleter = function_deleter<decltype(&::LsaFreeReturnBuffer), LsaFreeReturnBuffer>;

    template <typename T>
    using unique_lsa_ptr = wistd::unique_ptr<T, lsa_deleter>;
#endif // __WIL_HANDLE_H_NTLSA_IFS_

#if defined(_NTURTL_) && defined(__NOTHROW_T_DEFINED) && !defined(__WIL_NTURTL_WNF_)
#define __WIL_NTURTL_WNF_
    /** Windows Notification Facility (WNF) helpers that make it easy to produce and consume wnf state blocks.
    Learn more about WNF here: http://windowsarchive/sites/windows8docs/Win8%20Feature%20Docs/Windows%20Core%20(CORE)/Kernel%20Platform%20Group%20(KPG)/Kernel%20Core%20Software%20Services/Communications/Windows%20Notification%20Facility%20(WNF)/WNF%20-%20Functional%20Specification%20BETA.docm

    Clients must include <new> or <new.h> to enable use of these helpers as they use new(std::nothrow).
    This is to avoid the dependency on those headers that some clients can't tolerate.
    Clients must also include <nt.h>, <ntrtl.h>, and <nturtl.h> before any other windows-related headers such as <windows.h>.

    NOTE: The subscription wrappers will wait for outstanding callbacks on destruction

    Example use of throwing version:
    ~~~
    auto stateChangeSubscription = wil::make_wnf_subscription<TYPE>([](const TYPE& type)
    {
    // Do stuff with type
    });
    ~~~
    Example use of non-throwing version:
    ~~~
    auto stateChangeSubscription = wil::make_wnf_subscription_nothrow<TYPE>([](const TYPE& type)
    {
    // Do stuff with type
    });
    RETURN_HR_IF_FALSE(E_OUTOFMEMORY, stateChangeSubscription);
    ~~~
    */

    //! WNF_CHANGE_STAMP is an alias for ULONG that causes callers that don't specify changeStamp to select the zero size query with change stamp.
    //! To avoid this use WNF_CHANGE_STAMP_STRUCT instead of WNF_CHANGE_STAMP for this parameter.
    struct WNF_CHANGE_STAMP_STRUCT
    {
        WNF_CHANGE_STAMP value = 0;
        operator WNF_CHANGE_STAMP /* ULONG */ () const { return value; }
        static void AssignToOptParam(_In_opt_ WNF_CHANGE_STAMP_STRUCT* changeStampStruct, WNF_CHANGE_STAMP value)
        {
            if (changeStampStruct)
            {
                changeStampStruct->value = value;
            }
        }
    };

    //! Checks the published state and optionally the change stamp for state blocks of any size.
    inline HRESULT wnf_is_published_nothrow(WNF_STATE_NAME const& stateName, _Out_ bool* isPublished, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr) WI_NOEXCEPT
    {
        *isPublished = false;
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, static_cast<WNF_CHANGE_STAMP>(0));
        ULONG stateDataSize = 0;
        WNF_CHANGE_STAMP tempChangeStamp;
        HRESULT queryResult = HRESULT_FROM_NT(NtQueryWnfStateData(&stateName, nullptr, nullptr, &tempChangeStamp, nullptr, &stateDataSize));
        RETURN_HR_IF(queryResult, FAILED(queryResult) && (queryResult != HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL)));
        *isPublished = (tempChangeStamp != 0);
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, tempChangeStamp);
        return S_OK;
    }

    //! Query the published state and optioanlly change stamp for zero sized state blocks. This will fail fast for not-zero sized state blocks.
    inline HRESULT wnf_query_nothrow(WNF_STATE_NAME const& stateName, _Out_ bool* isPublished, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr) WI_NOEXCEPT
    {
        *isPublished = false;
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, static_cast<WNF_CHANGE_STAMP>(0));
        ULONG stateDataSize = 0;
        WNF_CHANGE_STAMP tempChangeStamp;
        HRESULT queryResult = HRESULT_FROM_NT(NtQueryWnfStateData(&stateName, nullptr, nullptr, &tempChangeStamp, nullptr, &stateDataSize));
        RETURN_HR_IF(queryResult, FAILED(queryResult) && (queryResult != HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL)));
        __FAIL_FAST_ASSERT__((tempChangeStamp == 0) || (stateDataSize == 0));
        LOG_HR_IF_MSG(E_UNEXPECTED, (tempChangeStamp != 0) && (stateDataSize != 0), "Inconsistent state data size in wnf_query");
        *isPublished = (tempChangeStamp != 0) && (stateDataSize == 0);  // if the size is non zero, consider it unpublished
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, tempChangeStamp);
        return S_OK;
    }

    //! Query the published state, state data and optionally the change stamp for fixed size state blocks.
    template <typename state_data_t>
    HRESULT wnf_query_nothrow(WNF_STATE_NAME const& stateName, _Out_ bool* isPublished,
        _Pre_ _Notnull_ _Pre_ _Writable_elements_(1) _Post_ _When_((*isPublished == true), _Valid_) state_data_t* stateData, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr) WI_NOEXCEPT
    {
        *isPublished = false;
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, static_cast<WNF_CHANGE_STAMP>(0));
        ULONG stateDataSize = sizeof(*stateData);
        WNF_CHANGE_STAMP tempChangeStamp;
        HRESULT queryResult = HRESULT_FROM_NT(NtQueryWnfStateData(&stateName, nullptr, nullptr, &tempChangeStamp, stateData, &stateDataSize));
        RETURN_HR_IF(queryResult, FAILED(queryResult) && (queryResult != HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL)));
        __FAIL_FAST_ASSERT__((tempChangeStamp == 0) || (stateDataSize == sizeof(*stateData)));
        LOG_HR_IF_MSG(E_UNEXPECTED, (tempChangeStamp != 0) && (stateDataSize != sizeof(*stateData)), "Inconsistent state data size in wnf_query");
        *isPublished = (tempChangeStamp != 0) && (stateDataSize == sizeof(*stateData));  // if the size is mismatched, consider it unpublished
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, tempChangeStamp);
        return S_OK;
    }

    //! Query the published state, state data and optionally the change stamp for variable size state blocks.
    inline HRESULT wnf_query_nothrow(WNF_STATE_NAME const& stateName, _Out_ bool* isPublished,
        _Out_writes_bytes_(stateDataByteCount) void* stateData, size_t stateDataByteCount,
        _Out_ size_t* stateDataWrittenByteCount, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr) WI_NOEXCEPT
    {
        *isPublished = false;
        *stateDataWrittenByteCount = 0;
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, static_cast<WNF_CHANGE_STAMP>(0));

        WNF_CHANGE_STAMP tempChangeStamp;
        ULONG size = static_cast<ULONG>(stateDataByteCount);
        RETURN_IF_NTSTATUS_FAILED(NtQueryWnfStateData(&stateName, nullptr, nullptr, &tempChangeStamp, stateData, &size));
        *isPublished = (tempChangeStamp != 0);
        *stateDataWrittenByteCount = static_cast<size_t>(size);
        WNF_CHANGE_STAMP_STRUCT::AssignToOptParam(changeStamp, tempChangeStamp);
        return S_OK;
    }

    //! Returns true if the value is published, false if not (or malformed or on error).
    inline _Success_(return) bool try_wnf_query(WNF_STATE_NAME const& stateName, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr) WI_NOEXCEPT
    {
        bool isPublished;
        return SUCCEEDED(wnf_query_nothrow(stateName, &isPublished, changeStamp)) && isPublished;
    }

    //! Returns true if the value is published, false if not (or malformed or on error)
    //! and returns the fixed size state data.
    template <typename state_data_t>
    _Success_(return) bool try_wnf_query(WNF_STATE_NAME const& stateName, _Out_ state_data_t* stateData, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr) WI_NOEXCEPT
    {
        bool isPublished;
        return SUCCEEDED(wnf_query_nothrow(stateName, &isPublished, stateData, changeStamp)) && isPublished;
    }

    //! Publish a zero sized state block.
    inline HRESULT wnf_publish_nothrow(WNF_STATE_NAME const& stateName) WI_NOEXCEPT
    {
        return HRESULT_FROM_NT(RtlPublishWnfStateData(stateName, nullptr, nullptr, 0, nullptr));
    }

    //! Publish a variable sized state block.
    inline HRESULT wnf_publish_nothrow(WNF_STATE_NAME const& stateName, _In_reads_bytes_(size) const void* stateData, const size_t size) WI_NOEXCEPT
    {
        return HRESULT_FROM_NT(RtlPublishWnfStateData(stateName, nullptr, stateData, static_cast<const ULONG>(size), nullptr));
    }

    //! Publish a fixed sized state block.
    template <typename state_data_t>
    HRESULT wnf_publish_nothrow(WNF_STATE_NAME const& stateName, state_data_t const& stateData) WI_NOEXCEPT
    {
        return HRESULT_FROM_NT(RtlPublishWnfStateData(stateName, nullptr, &stateData, sizeof(stateData), nullptr));
    }

#ifdef WIL_ENABLE_EXCEPTIONS
    //! Checks the published state and optionally the change stamp for state blocks of any size.
    inline _Success_(return) bool wnf_is_published(WNF_STATE_NAME const& stateName, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr)
    {
        bool isPublished;
        THROW_IF_FAILED(wnf_is_published_nothrow(stateName, &isPublished, changeStamp));
        return isPublished;
    }

    //! Query the published state, state data and optionally the change stamp for fixed size state blocks.
    template <typename state_data_t>
    _Success_(return) bool wnf_query(WNF_STATE_NAME const& stateName, _Out_ state_data_t* stateData, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr)
    {
        bool isPublished;
        THROW_IF_FAILED(wnf_query_nothrow(stateName, &isPublished, stateData, changeStamp));
        return isPublished;
    }

    //! Query the published state and optioanlly change stamp for zero sized state blocks. This will fail fast for not-zero sized state blocks.
    inline _Success_(return) bool wnf_query(WNF_STATE_NAME const& stateName, _Out_opt_ WNF_CHANGE_STAMP_STRUCT* changeStamp = nullptr)
    {
        bool isPublished;
        THROW_IF_FAILED(wnf_query_nothrow(stateName, &isPublished, changeStamp));
        return isPublished;
    }

    //! Publish a zero-size state block.
    inline void wnf_publish(WNF_STATE_NAME const& stateName)
    {
        THROW_IF_FAILED(wnf_publish_nothrow(stateName));
    }

    //! Publish a fixed size state block.
    template <typename state_data_t>
    void wnf_publish(WNF_STATE_NAME const& stateName, state_data_t const& stateData)
    {
        THROW_IF_FAILED(wnf_publish_nothrow(stateName, stateData));
    }

#endif // WIL_ENABLE_EXCEPTIONS

    //! Manage the lifetime of a user subscription.
    typedef unique_any<PWNF_USER_SUBSCRIPTION, decltype(&::RtlUnsubscribeWnfNotificationWaitForCompletion), ::RtlUnsubscribeWnfNotificationWaitForCompletion, details::pointer_access_all> unique_pwnf_user_subscription;
    WNF_CHANGE_STAMP const WnfChangeStampLatest = MAXULONG;

    /// @cond
    namespace details
    {
        struct empty_wnf_state
        {
        };

        template <typename state_data_t>
        struct wnf_array_callback_type
        {
            typedef typename wistd::function<void(state_data_t const*, size_t)> type;
        };

        template <typename state_data_t>
        struct wnf_callback_type
        {
            typedef typename wistd::function<void(state_data_t const&)> type;
        };

        template <>
        struct wnf_callback_type<empty_wnf_state>
        {
            typedef wistd::function<void()> type;
        };

        struct wnf_subscription_state_base
        {
            virtual ~wnf_subscription_state_base() {};
            unique_pwnf_user_subscription m_subscription;
        };

        template <typename state_data_t = empty_wnf_state>
        struct wnf_subscription_state : public wnf_subscription_state_base
        {
            typedef typename details::wnf_callback_type<state_data_t>::type callback_type;

            wnf_subscription_state(callback_type&& callback) : m_callback(wistd::move(callback)) {}

            ~wnf_subscription_state() override
            {
                m_subscription.reset();     // subscription must be released prior to the callback (ordering)
            }

            template <typename state_data_t>
            void InternalCallback(state_data_t const* stateData, ULONG stateDataSize)
            {
                // Must check the size here vs. in the subscription lambda in make_wnf_subscription_state since the default
                // case uses empty_wnf_state which has a size > 0
                __FAIL_FAST_ASSERT__(stateDataSize == sizeof(*stateData));
                if (stateDataSize != sizeof(*stateData))
                {
                    LOG_HR_MSG(E_UNEXPECTED, "Inconsistent state data size in WNF callback");
                    return;
                }
                m_callback(*stateData);
            }

            template <>
            void InternalCallback<empty_wnf_state>(empty_wnf_state const* /*stateData*/, ULONG /*stateDataSize*/)
            {
                m_callback();
            }

            callback_type m_callback;
        };

        template <typename state_data_t = empty_wnf_state>
        HRESULT make_wnf_subscription_state(WNF_STATE_NAME const& stateName, typename details::wnf_callback_type<state_data_t>::type&& callback, _In_ WNF_CHANGE_STAMP subscribeFrom, _Outptr_ wnf_subscription_state<state_data_t>** subscriptionState) WI_NOEXCEPT
        {
            *subscriptionState = nullptr;
            wistd::unique_ptr<wnf_subscription_state<state_data_t>> subscriptionStateT(new(std::nothrow) wnf_subscription_state<state_data_t>(wistd::move(callback)));
            RETURN_IF_NULL_ALLOC(subscriptionStateT.get());

            if (subscribeFrom == WnfChangeStampLatest)
            {
                // Retrieve the latest changestamp and use that as the basis for change notifications
                ULONG bufferSize = 0;
                HRESULT queryResult = HRESULT_FROM_NT(NtQueryWnfStateData(&stateName, 0, nullptr, &subscribeFrom, nullptr, &bufferSize));
                RETURN_HR_IF(queryResult, FAILED(queryResult) && (queryResult != HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL)));
            }

            const auto result = RtlSubscribeWnfStateChangeNotification(wil::out_param(subscriptionStateT->m_subscription), stateName, subscribeFrom,
                [](WNF_STATE_NAME, WNF_CHANGE_STAMP changeStamp, WNF_TYPE_ID*, void* callbackContext, const void* buffer, ULONG length) -> NTSTATUS
            {
                // Delete notifications always have a change stamp of 0.  Since this class doesn't
                // provide for a way to handle them differently, it drops them to avoid a fast-fail
                // due to the payload size of a delete being potentially different than an update.
                if (changeStamp != 0)
                {
                    static_cast<details::wnf_subscription_state<state_data_t>*>(callbackContext)->InternalCallback(static_cast<state_data_t const*>(buffer), length);
                }
                return STATUS_SUCCESS;
            }, subscriptionStateT.get(), nullptr, 0, 0);
            RETURN_IF_NTSTATUS_FAILED(result);

            *subscriptionState = subscriptionStateT.release();
            return S_OK;
        }

        template <typename state_data_t>
        struct wnf_array_subscription_state : public wnf_subscription_state_base
        {
            typedef typename details::wnf_array_callback_type<state_data_t>::type callback_type;

            wnf_array_subscription_state(callback_type&& callback) : m_callback(wistd::move(callback))
            {
            }

            ~wnf_array_subscription_state() override
            {
                m_subscription.reset();     // subscription must be released prior to the callback (ordering)
            }

            template <typename state_data_t>
            void InternalCallback(state_data_t const* stateData, ULONG stateDataSize)
            {
                if ((stateDataSize % sizeof(state_data_t)) != 0)
                {
                    LOG_HR_MSG(E_UNEXPECTED, "Inconsistent state data size in WNF callback");
                    return;
                }

                // for empty payload, return zero sized null array.
                size_t itemSize = static_cast<size_t>(stateDataSize / sizeof(state_data_t));
                m_callback(stateData, itemSize);
            }

            callback_type m_callback;
        };

        template <typename state_data_t>
        HRESULT make_wnf_array_subscription_state(
            WNF_STATE_NAME const& stateName,
            typename details::wnf_array_callback_type<state_data_t>::type&& callback,
            _In_ WNF_CHANGE_STAMP subscribeFrom,
            _Outptr_ wnf_array_subscription_state<state_data_t>** subscriptionState) WI_NOEXCEPT
        {
            *subscriptionState = nullptr;
            wistd::unique_ptr<wnf_array_subscription_state<state_data_t>> subscriptionStateT(new(std::nothrow) wnf_array_subscription_state<state_data_t>(wistd::move(callback)));
            RETURN_IF_NULL_ALLOC(subscriptionStateT.get());

            if (subscribeFrom == WnfChangeStampLatest)
            {
                // Retrieve the latest changestamp and use that as the basis for change notifications
                ULONG bufferSize = 0;
                const HRESULT queryResult = HRESULT_FROM_NT(NtQueryWnfStateData(&stateName, 0, nullptr, &subscribeFrom, nullptr, &bufferSize));
                RETURN_HR_IF(queryResult, FAILED(queryResult) && (queryResult != HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL)));
            }

            RETURN_IF_FAILED(HRESULT_FROM_NT(RtlSubscribeWnfStateChangeNotification(wil::out_param(subscriptionStateT->m_subscription), stateName, subscribeFrom,
                [](WNF_STATE_NAME /*stateName*/, WNF_CHANGE_STAMP /*changeStamp*/,
                    WNF_TYPE_ID* /*typeID*/, void* callbackContext, const void* buffer, ULONG length) -> NTSTATUS
            {
                static_cast<details::wnf_array_subscription_state<state_data_t>*>(callbackContext)->InternalCallback(static_cast<state_data_t const*>(buffer), length);
                return STATUS_SUCCESS;
            }, subscriptionStateT.get(), nullptr, 0, 0)));

            *subscriptionState = subscriptionStateT.release();
            return S_OK;
        }

        inline void delete_wnf_subscription_state(_In_opt_ wnf_subscription_state_base* subscriptionState) { delete subscriptionState; }
    }
    /// @endcond

    //! Manage the lifetime of a subscription.
    typedef unique_any<details::wnf_subscription_state_base*, decltype(&details::delete_wnf_subscription_state), details::delete_wnf_subscription_state, details::pointer_access_none> unique_wnf_subscription;

    //! Subscribe to changes for a zero or fixed size wnf state block.
    //! the callback will only be called if the payload size matches the size of the state block.
    template<typename state_data_t = details::empty_wnf_state>
    unique_wnf_subscription make_wnf_subscription_nothrow(WNF_STATE_NAME const& stateName, typename details::wnf_callback_type<state_data_t>::type&& callback, _In_ WNF_CHANGE_STAMP subscribeFrom = 0) WI_NOEXCEPT
    {
        details::wnf_subscription_state<state_data_t>* subscriptionState;
        if (SUCCEEDED(details::make_wnf_subscription_state<state_data_t>(stateName, wistd::move(callback), subscribeFrom, &subscriptionState)))
        {
            return unique_wnf_subscription(subscriptionState);
        }
        return nullptr;
    }

    //! Subscribe to changes for a dynamic size wnf state block that is an array of elements each of size state_data_t
    //! the callback will only be called with the pointer to the first item of the array and the number of items in the array.
    //!
    //! auto varArraySubscription = wil::make_wnf_array_subscription_nothrow<wchar_t>(
    //!     WNF_FDBK_QUESTION_NOTIFICATION,
    //!     [&](wchar_t const* array, size_t length)
    //!     {
    //!     });
    template<typename state_data_t>
    unique_wnf_subscription make_wnf_array_subscription_nothrow(
        WNF_STATE_NAME const& stateName,
        typename details::wnf_array_callback_type<state_data_t>::type&& callback,
        _In_ WNF_CHANGE_STAMP subscribeFrom = 0) WI_NOEXCEPT
    {
        details::wnf_array_subscription_state<state_data_t>* subscriptionState;
        if (SUCCEEDED(details::make_wnf_array_subscription_state<state_data_t>(stateName, wistd::move(callback), subscribeFrom, &subscriptionState)))
        {
            return unique_wnf_subscription(subscriptionState);
        }
        return nullptr;
    }
#ifdef WIL_ENABLE_EXCEPTIONS

    //! Subscribe to changes for a zero or fixed size wnf state block.
    //! the callback will only be called if the payload size matches the size of the state block.
    template<typename state_data_t = details::empty_wnf_state>
    unique_wnf_subscription make_wnf_subscription(WNF_STATE_NAME const& stateName, typename details::wnf_callback_type<state_data_t>::type&& callback, _In_ WNF_CHANGE_STAMP subscribeFrom = 0)
    {
        details::wnf_subscription_state<state_data_t>* subscriptionState;
        THROW_IF_FAILED(details::make_wnf_subscription_state<state_data_t>(stateName, wistd::move(callback), subscribeFrom, &subscriptionState));
        return unique_wnf_subscription(subscriptionState);
    }

    //! Subscribe to changes for a dynamic size wnf state block that is an array of elements each of size state_data_t
    //! the callback will only be called if the payload size has at least one item in the array.
    //!
    //! auto varArraySubscription = wil::make_wnf_array_subscription<wchar_t>(
    //!     WNF_FDBK_QUESTION_NOTIFICATION,
    //!     [&](wchar_t const* array, size_t length)
    //!     {
    //!     });
    template<typename state_data_t>
    unique_wnf_subscription make_wnf_array_subscription(
        WNF_STATE_NAME const& stateName,
        typename details::wnf_array_callback_type<state_data_t>::type&& callback,
        _In_ WNF_CHANGE_STAMP subscribeFrom = 0)
    {
        details::wnf_array_subscription_state<state_data_t>* subscriptionState;
        THROW_IF_FAILED(details::make_wnf_array_subscription_state<state_data_t>(stateName, wistd::move(callback), subscribeFrom, &subscriptionState));
        return unique_wnf_subscription(subscriptionState);
    }

#endif // WIL_ENABLE_EXCEPTIONS

#endif // __WIL_NTURTL_WNF_

#if defined(__WERAPI_H__) && !defined(__WIL_WERAPI_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WERAPI_H__
    typedef unique_any<HREPORT, decltype(&WerReportCloseHandle), WerReportCloseHandle> unique_wer_report;
#endif

#if defined(__MIDLES_H__) && !defined(__WIL_MIDLES_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_MIDLES_H__
    typedef unique_any<handle_t, decltype(&::MesHandleFree), ::MesHandleFree> unique_rpc_pickle;
#endif
#if defined(__WIL_MIDLES_H__) && !defined(__WIL_MIDLES_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_MIDLES_H_STL
    typedef shared_any<unique_rpc_pickle> shared_rpc_pickle;
    typedef weak_any<shared_rpc_pickle> weak_rpc_pickle;
#endif

#if defined(__RPCDCE_H__) && !defined(__WIL_RPCDCE_H__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_RPCDCE_H__
    /// @cond
    namespace details
    {
        inline void __stdcall WpRpcBindingFree(_Pre_opt_valid_ _Frees_ptr_opt_ RPC_BINDING_HANDLE binding)
        {
            ::RpcBindingFree(&binding);
        }

        inline void __stdcall WpRpcBindingVectorFree(_Pre_opt_valid_ _Frees_ptr_opt_ RPC_BINDING_VECTOR* bindingVector)
        {
            ::RpcBindingVectorFree(&bindingVector);
        }

        inline void __stdcall WpRpcStringFree(_Pre_opt_valid_ _Frees_ptr_opt_ RPC_WSTR wstr)
        {
            ::RpcStringFreeW(&wstr);
        }
    }
    /// @endcond

    typedef unique_any<RPC_BINDING_HANDLE, decltype(&details::WpRpcBindingFree), details::WpRpcBindingFree> unique_rpc_binding;
    typedef unique_any<RPC_BINDING_VECTOR*, decltype(&details::WpRpcBindingVectorFree), details::WpRpcBindingVectorFree> unique_rpc_binding_vector;
    typedef unique_any<RPC_WSTR, decltype(&details::WpRpcStringFree), details::WpRpcStringFree> unique_rpc_wstr;
#endif
#if defined(__WIL_RPCDCE_H__) && !defined(__WIL_RPCDCE_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_RPCDCE_H_STL
    typedef shared_any<unique_rpc_binding> shared_rpc_binding;
    typedef weak_any<shared_rpc_binding> weak_rpc_binding;
    typedef shared_any<unique_rpc_binding_vector> shared_rpc_binding_vector;
    typedef weak_any<shared_rpc_binding_vector> weak_rpc_binding_vector;
    typedef shared_any<unique_rpc_wstr> shared_rpc_wstr;
    typedef weak_any<unique_rpc_wstr> weak_rpc_wstr;
#endif

#if defined(__SRUMAPI_H__) && !defined(__WIL_SRUMAPI_H__)
#define __WIL_SRUMAPI_H__
    typedef unique_any<PSRU_STATS_RECORD_SET, decltype(&::SruFreeRecordSet), ::SruFreeRecordSet> unique_srum_recordset;
#endif
#if defined(__WIL_SRUMAPI_H__) && !defined(__WIL_SRUMAPI_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_SRUMAPI_H_STL
    typedef shared_any<unique_srum_recordset> shared_srum_recordset;
    typedef weak_any<shared_srum_recordset> weak_srum_recordset;
#endif

#if defined(_DUSMAPI_H) && !defined(__WIL_DUSMAPI_H_)
#define __WIL_DUSMAPI_H_
    using dusm_deleter = function_deleter<decltype(&::DusmFree), DusmFree>;

    template<typename T>
    using unique_dusm_ptr = wistd::unique_ptr<T, dusm_deleter>;
#endif

#if defined(_WCMAPI_H) && !defined(__WIL_WCMAPI_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WCMAPI_H_
    using wcm_deleter = function_deleter<decltype(&::WcmFreeMemory), WcmFreeMemory>;

    template<typename T>
    using unique_wcm_ptr = wistd::unique_ptr<T, wcm_deleter>;
#endif

#if defined(_NETIOAPI_H_) && defined(_WS2IPDEF_) && defined(MIB_INVALID_TEREDO_PORT_NUMBER) && !defined(__WIL_NETIOAPI_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_NETIOAPI_H_
    typedef unique_any<PMIB_IF_TABLE2, decltype(&::FreeMibTable), ::FreeMibTable> unique_mib_iftable;
#endif
#if defined(__WIL_NETIOAPI_H_) && !defined(__WIL_NETIOAPI_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_NETIOAPI_H_STL
    typedef shared_any<unique_mib_iftable> shared_mib_iftable;
    typedef weak_any<shared_mib_iftable> weak_mib_iftable;
#endif

#if defined(__WWAN_API_DECL__) && !defined(__WIL_WWAN_API_DECL__) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WWAN_API_DECL__
    using wwan_deleter = function_deleter<decltype(&::WwanFreeMemory), WwanFreeMemory>;

    template<typename T>
    using unique_wwan_ptr = wistd::unique_ptr<T, wwan_deleter>;

    /// @cond
    namespace details
    {
        inline void __stdcall CloseWwanHandle(_Frees_ptr_ HANDLE hClientHandle)
        {
            ::WwanCloseHandle(hClientHandle, nullptr);
        }
    }
    /// @endcond

    typedef unique_any<HANDLE, decltype(&details::CloseWwanHandle), details::CloseWwanHandle, details::pointer_access_all, HANDLE, INVALID_HANDLE_VALUE> unique_wwan_handle;
#endif
#if defined(__WIL_WWAN_API_DECL__) && !defined(__WIL_WWAN_API_DECL_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WWAN_API_DECL_STL
    typedef shared_any<unique_wwan_handle> shared_wwan_handle;
    typedef weak_any<shared_wwan_handle> weak_wwan_handle;
#endif

#if defined(_WLAN_WLANAPI_H) && !defined(__WIL_WLAN_WLANAPI_H) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_WLAN_WLANAPI_H
    using wlan_deleter = function_deleter<decltype(&::WlanFreeMemory), ::WlanFreeMemory>;

    template<typename T>
    using unique_wlan_ptr = wistd::unique_ptr < T, wlan_deleter >;

    /// @cond
    namespace details
    {
        inline void __stdcall CloseWlanHandle(_Frees_ptr_ HANDLE hClientHandle)
        {
            ::WlanCloseHandle(hClientHandle, nullptr);
        }
    }
    /// @endcond

    typedef unique_any<HANDLE, decltype(&details::CloseWlanHandle), details::CloseWlanHandle, details::pointer_access_all, HANDLE, INVALID_HANDLE_VALUE> unique_wlan_handle;
#endif
#if defined(__WIL_WLAN_WLANAPI_H) && !defined(__WIL_WLAN_WLANAPI_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_WLAN_WLANAPI_H_STL
    typedef shared_any<unique_wlan_handle> shared_wlan_handle;
    typedef weak_any<shared_wlan_handle> weak_wlan_handle;
#endif

#if defined(_NTEXAPI_) && !defined(__WIL_NTEXAPI_)
#define __WIL_NTEXAPI_
    typedef wil::unique_struct<WNF_STATE_NAME, decltype(&::NtDeleteWnfStateName), ::NtDeleteWnfStateName> unique_wnf_state_name;
#endif

#if defined(STATELOCK_FLAG_TRANSIENT) && !defined(__WIL_STATELOCK_FLAG_TRANSIENT)
#define __WIL_STATELOCK_FLAG_TRANSIENT
    typedef unique_any<HSTATE, decltype(&::CloseState), ::CloseState, details::pointer_access_all, HSTATE, INVALID_STATE_HANDLE> unique_hstate;
    typedef unique_any<HSTATE_NOTIFICATION, decltype(&::CloseStateChangeNotification), ::CloseStateChangeNotification, details::pointer_access_all, HSTATE_NOTIFICATION, nullptr> unique_hstate_notification;
#endif

#if defined(_EFSWRTEXT_H_) && !defined(__WIL_EFSWRTEXT_H_)
#define __WIL_EFSWRTEXT_H_
    typedef unique_any<PENCRYPTION_PROTECTOR_LIST, decltype(&::FreeIdentityProtectorList), ::FreeIdentityProtectorList> unique_pencryption_protector_list;
#endif

#if defined(_EDPUTIL_H_) && !defined(__WIL_EDPUTIL_H_)
#define __WIL_EDPUTIL_H_
    typedef unique_any<EDP_CONTEXT*, decltype(&::EdpFreeContext), ::EdpFreeContext> unique_edp_context;
#endif

#if defined(_HPOWERNOTIFY_DEF_) && !defined(__WIL_HPOWERNOTIFY_DEF_H_)
#define __WIL_HPOWERNOTIFY_DEF_H_
    typedef unique_any<HPOWERNOTIFY, decltype(&::UnregisterPowerSettingNotification), ::UnregisterPowerSettingNotification> unique_hpowernotify;
#endif

#if defined(__WIL_WINBASE_DESKTOP) && defined(SID_DEFINED) && !defined(__WIL_PSID_DEF_H_)
#define __WIL_PSID_DEF_H_
    typedef unique_any<PSID, decltype(&::LocalFree), ::LocalFree> unique_any_psid;
#if defined(_OBJBASE_H_)
    typedef unique_any<PSID, decltype(&::CoTaskMemFree), ::CoTaskMemFree> unique_cotaskmem_psid;
#endif
#endif

#if defined(__WINCRYPT_H_) && !defined(__WIL_WINCRYPT_DEF_H_)
#define __WIL_WINCRYPT_DEF_H_
    typedef unique_any<PCCERT_CONTEXT, decltype(&::CertFreeCertificateContext), ::CertFreeCertificateContext> unique_certContext;
#endif

#if defined(_PROCESSTHREADSAPI_H_) && !defined(__WIL_PROCESSTHREADSAPI_H_DESK_SYS) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
#define __WIL_PROCESSTHREADSAPI_H_DESK_SYS
    /// @cond
    namespace details
    {
        inline void __stdcall CloseProcessInformation(_In_ PROCESS_INFORMATION* p)
        {
            if (p->hProcess)
            {
                CloseHandle(p->hProcess);
            }

            if (p->hThread)
            {
                CloseHandle(p->hThread);
            }
        }
    }
    /// @endcond

    /** Manages the outbound parameter containing handles returned by `CreateProcess()` and related methods.
    ~~~
    unique_process_information process;
    CreateProcessW(..., CREATE_SUSPENDED, ..., &process);
    THROW_IF_WIN32_BOOL_FALSE(ResumeThread(process.hThread));
    THROW_LAST_ERROR_IF_FALSE(WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0);
    ~~~
    */
    using unique_process_information = unique_struct<PROCESS_INFORMATION, decltype(&details::CloseProcessInformation), details::CloseProcessInformation>;
#endif

#if defined(POLICYMANAGER_H) && !defined(__WIL_POLICYSTRING_DEF_H_)
#define __WIL_POLICYSTRING_DEF_H_
    typedef unique_any<PWSTR, decltype(&::PolicyManager_FreeStringValue), ::PolicyManager_FreeStringValue> unique_policy_string;
#endif

#if defined(COREMESSAGING_API) && !defined(__WIL_UNIQUE_HENDPOINT_)
#define __WIL_UNIQUE_HENDPOINT_

    namespace details
    {
        inline void __stdcall IMessageSessionCloseEndpointFunction(IMessageSession* source, HENDPOINT token)
        {
            source->CloseEndpoint(token);
        }
    }

    using unique_hendpoint = unique_com_token<IMessageSession, HENDPOINT, decltype(details::IMessageSessionCloseEndpointFunction), details::IMessageSessionCloseEndpointFunction, 0>;
#endif

#if defined(_APPMODEL_H_) && !defined(__WIL_APPMODEL_H_) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define __WIL_APPMODEL_H_
    typedef unique_any<PACKAGE_INFO_REFERENCE, decltype(&::ClosePackageInfo), ::ClosePackageInfo> unique_package_info_reference;
#endif // __WIL_APPMODEL_H_
#if defined(__WIL_APPMODEL_H_) && !defined(__WIL_APPMODEL_H_STL) && defined(WIL_RESOURCE_STL)
#define __WIL_APPMODEL_H_STL
    typedef shared_any<unique_package_info_reference> shared_package_info_reference;
    typedef weak_any<shared_package_info_reference> weak_package_info_reference;
#endif // __WIL_APPMODEL_H_STL

#if defined(WDFAPI) && !defined(__WIL_WDFAPI)
#define __WIL_WDFAPI
    template<typename TWDFOBJECT>
    using unique_wdf_any = unique_any<TWDFOBJECT, decltype(&::WdfObjectDelete), &::WdfObjectDelete>;

    using unique_wdf_object          = unique_wdf_any<WDFOBJECT>;

    using unique_wdf_timer           = unique_wdf_any<WDFTIMER>;
    using unique_wdf_work_item       = unique_wdf_any<WDFWORKITEM>;

    using unique_wdf_wait_lock       = unique_wdf_any<WDFWAITLOCK>;
    using unique_wdf_spin_lock       = unique_wdf_any<WDFSPINLOCK>;

    using unique_wdf_memory          = unique_wdf_any<WDFMEMORY>;

    using unique_wdf_dma_enabler     = unique_wdf_any<WDFDMAENABLER>;
    using unique_wdf_dma_transaction = unique_wdf_any<WDFDMATRANSACTION>;
    using unique_wdf_common_buffer   = unique_wdf_any<WDFCOMMONBUFFER>;

    using unique_wdf_key             = unique_wdf_any<WDFKEY>;
    using unique_wdf_string          = unique_wdf_any<WDFSTRING>;
    using unique_wdf_collection      = unique_wdf_any<WDFCOLLECTION>;

    using wdf_wait_lock_release_scope_exit =
        unique_any<
            WDFWAITLOCK,
            decltype(&::WdfWaitLockRelease),
            ::WdfWaitLockRelease,
            details::pointer_access_none>;

    inline
    _Check_return_
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Acquires_lock_(lock)
    wdf_wait_lock_release_scope_exit
    acquire_wdf_wait_lock(WDFWAITLOCK lock) WI_NOEXCEPT
    {
        ::WdfWaitLockAcquire(lock, nullptr);
        return wdf_wait_lock_release_scope_exit(lock);
    }

    inline
    _Check_return_
    _IRQL_requires_max_(APC_LEVEL)
    _When_(return, _Acquires_lock_(lock))
    wdf_wait_lock_release_scope_exit
    try_acquire_wdf_wait_lock(WDFWAITLOCK lock) WI_NOEXCEPT
    {
        LONGLONG timeout = 0;
        NTSTATUS status = ::WdfWaitLockAcquire(lock, &timeout);
        if (status == STATUS_SUCCESS)
        {
            return wdf_wait_lock_release_scope_exit(lock);
        }
        else
        {
            return wdf_wait_lock_release_scope_exit();
        }
    }

    using wdf_spin_lock_release_scope_exit =
        unique_any<
            WDFSPINLOCK,
            decltype(&::WdfSpinLockRelease),
            ::WdfSpinLockRelease,
            details::pointer_access_none>;

    inline
    _Check_return_
    _IRQL_requires_max_(DISPATCH_LEVEL)
    _IRQL_raises_(DISPATCH_LEVEL)
    _Acquires_lock_(lock)
    wdf_spin_lock_release_scope_exit
    acquire_wdf_spin_lock(WDFSPINLOCK lock) WI_NOEXCEPT
    {
        ::WdfSpinLockAcquire(lock);
        return wdf_spin_lock_release_scope_exit(lock);
    }

#endif

} // namespace wil

#pragma warning(pop)

