/*++
Copyright (c) Microsoft Corporation

Module Name:
- RepeatingIterator.hpp

Abstract:
- iterator for cycling over a container without hitting an end. Use to provide a view of a container as if it
contained an arbitrary number of repeated elements

Author(s):
- Austin Diviness (AustDi) 15-May-2018
--*/

#pragma once

template <typename T, typename IteratorType>
class RepeatingIterator final
{
private:
    void _increment(size_t count)
    {
        const size_t totalDistance = _end - _start;
        const size_t currentDistance = _current - _start;
        const size_t rotations = (count + currentDistance) / totalDistance;
        const size_t newCurrentDistance = (count + currentDistance) % totalDistance;
        _loopCount += rotations;
        _current = _start + newCurrentDistance;
    }

public:

    using iterator_category = std::forward_iterator_tag;
    using value_type = typename IteratorType::value_type;
    using difference_type = typename IteratorType::difference_type;
    using pointer = typename IteratorType::pointer;
    using reference = typename IteratorType::reference;


    RepeatingIterator(IteratorType start, IteratorType end) :
        _start{ start },
        _end{ end },
        _current{ start },
        _loopCount{ 0 }
    {
        THROW_HR_IF(E_INVALIDARG, start > end);
    }

    RepeatingIterator(const RepeatingIterator<T, IteratorType>& a) = default;
    ~RepeatingIterator() = default;
    RepeatingIterator& operator=(const RepeatingIterator<T, IteratorType>& a) = default;

    RepeatingIterator& operator++()
    {
        _increment(1);
        return *this;
    }

    RepeatingIterator operator++(int)
    {
        auto copy = *this;
        _increment(1);
        return copy;
    }

    RepeatingIterator& operator+=(const size_t movement)
    {
        _increment(movement);
        return *this;
    }

    RepeatingIterator operator+(const size_t movement) const
    {
        auto copy = *this;
        copy._increment(movement);
        return copy;
    }

    const T& operator*() const
    {
        return *_current;
    }

    const T* operator->()
    {
        return &*_current;
    }

    bool operator==(const RepeatingIterator<T, IteratorType>& a)
    {
        return (_start == a._start &&
               _end == a._end &&
               _current == a._current &&
               _loopCount == a._loopCount);
    }

    bool operator!=(const RepeatingIterator<T, IteratorType>& a)
    {
        return !(*this == a);
    }

private:
    IteratorType _start;
    IteratorType _end;
    IteratorType _current;
    size_t _loopCount;
};
