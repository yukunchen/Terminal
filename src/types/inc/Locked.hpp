/*++
Copyright (c) Microsoft Corporation

Module Name:
- Locked.hpp

Abstract:
- Defines a wrapper to return a pointer to an object with exclusive use during
  its lifetime (locked.)
- It will auto-free the underlying mutex when it goes out of scope.

Author:
- Michael Niksa <miniksa> 19-Mar-2018

--*/

#pragma once
#include <mutex>

template<typename T>
class Locked 
{
public:
    Locked(T* const item, std::recursive_mutex& lock);

    Locked(T* const item, std::unique_lock<std::recursive_mutex>&& lock);

    ~Locked() = default;
    Locked(Locked&) = delete;
    Locked(Locked&&) = default;
    Locked& operator=(const Locked&) & = delete;
    Locked& operator=(Locked &&) & = default;

    [[nodiscard]]
    T* const operator->() const noexcept;

    [[nodiscard]]
    T* const get() const noexcept;

private:
    T* const _item;
    std::unique_lock<std::recursive_mutex> _lock;
};

#include "LockedImpl.hpp"
