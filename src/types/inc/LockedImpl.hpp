/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

template<typename T>
Locked<T>::Locked(T* const item, std::recursive_mutex& lock) :
    _item(item),
    _lock(lock)
{
}

template<typename T>
Locked<T>::Locked(T* const item, std::unique_lock<std::recursive_mutex>&& lock) :
    _item(item),
    _lock(std::move(lock))
{
}

template<typename T>
[[nodiscard]]
T* const Locked<T>::operator->() const noexcept
{
    return this->_item;
}

template<typename T>
[[nodiscard]]
T* const Locked<T>::get() const noexcept
{
    return this->_item;
}
