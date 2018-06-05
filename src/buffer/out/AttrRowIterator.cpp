/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "AttrRowIterator.hpp"
#include "AttrRow.hpp"

AttrRowIterator AttrRowIterator::CreateEndIterator(const ATTR_ROW& attrRow)
{
    AttrRowIterator it{ attrRow };
    it._setToEnd();
    return it;
}

AttrRowIterator::AttrRowIterator(const ATTR_ROW& attrRow) :
    _attrRow{ attrRow },
    _currentRunIndex{ 0 },
    _currentAttributeIndex{ 0 },
    _logicalIndex{ 0 },
    _alreadyAccessed{ false }
{
}

AttrRowIterator::operator bool() const noexcept
{
    if (_alreadyAccessed)
    {
        return (_currentRunIndex < _attrRow._list.size());
    }
    else
    {
        return (_logicalIndex < _attrRow._cchRowWidth);
    }
}

bool AttrRowIterator::operator==(const AttrRowIterator& it) const
{
    if (_alreadyAccessed && it._alreadyAccessed)
    {
        return (_attrRow == it._attrRow &&
                _currentRunIndex == it._currentRunIndex &&
                _currentAttributeIndex == it._currentAttributeIndex);
    }
    else if (!_alreadyAccessed && !it._alreadyAccessed)
    {
        return (_logicalIndex == it._logicalIndex);
    }
    // the two iterators are in different internal states. convert a copy of one to the "accessed" state so
    // they can be compared.
    else if (_alreadyAccessed && !it._alreadyAccessed)
    {
        auto copy = it;
        copy._convertToAccessed();
        return (*this == copy);
    }
    else
    {
        auto copy = *this;
        copy._convertToAccessed();
        return (it == copy);
    }
}

bool AttrRowIterator::operator!=(const AttrRowIterator& it) const
{
    return !(*this == it);
}

AttrRowIterator& AttrRowIterator::operator++()
{
    _increment(1);
    return *this;
}

AttrRowIterator AttrRowIterator::operator++(int)
{
    auto copy = *this;
    _increment(1);
    return copy;
}

AttrRowIterator& AttrRowIterator::operator--()
{
    _decrement(1);
    return *this;
}

AttrRowIterator AttrRowIterator::operator--(int)
{
    auto copy = *this;
    _decrement(1);
    return copy;
}

const TextAttribute* AttrRowIterator::operator->() const
{
    if (!_alreadyAccessed)
    {
        const_cast<AttrRowIterator&>(*this)._convertToAccessed();
    }
    return &_attrRow._list.at(_currentRunIndex).GetAttributes();
}

const TextAttribute& AttrRowIterator::operator*() const
{
    if (!_alreadyAccessed)
    {
        const_cast<AttrRowIterator&>(*this)._convertToAccessed();
    }
    return _attrRow._list.at(_currentRunIndex).GetAttributes();
}

// Routine Description:
// - increments the index the iterator points to
// Arguments:
// - count - the amount to increment by
void AttrRowIterator::_increment(size_t count)
{
    if (!_alreadyAccessed)
    {
        _logicalIndex += count;
    }
    else
    {
        while (count > 0)
        {
            const size_t runLength = _attrRow._list.at(_currentRunIndex).GetLength();
            if (count + _currentAttributeIndex < runLength)
            {
                _currentAttributeIndex += count;
                return;
            }
            else
            {
                count -= runLength - _currentAttributeIndex;
                ++_currentRunIndex;
                _currentAttributeIndex = 0;
            }
        }
    }
}

// Routine Description:
// - decrements the index the iterator points to
// Arguments:
// - count - the amount to decrement by
void AttrRowIterator::_decrement(size_t count)
{
    if (!_alreadyAccessed)
    {
        _logicalIndex -= count;
    }
    else
    {
        while (count > 0)
        {
            if (count < _currentAttributeIndex)
            {
                _currentAttributeIndex -= count;
                return;
            }
            else
            {
                count -= _currentAttributeIndex;
                --_currentRunIndex;
                _currentAttributeIndex = _attrRow._list.at(_currentRunIndex).GetLength() - 1;
            }
        }
    }
}

// Routine Description:
// - converts the iterator to an "accessed" iterator, switching over to the other set of indices
void AttrRowIterator::_convertToAccessed()
{
    // can only do this process once and there's no going back
    FAIL_FAST_IF(_alreadyAccessed);

    _alreadyAccessed = true;
    if (_logicalIndex >= _attrRow._cchRowWidth)
    {
        // this is an end iterator, which can go one past the end of a list.
        // FindAttrIndex doesn't like this, so handle it here instead
        _setToEnd();
    }
    else
    {
        size_t amountRemaining = 0;
        _currentRunIndex = _attrRow.FindAttrIndex(_logicalIndex, &amountRemaining);
        _currentAttributeIndex = _attrRow._list.at(_currentRunIndex).GetLength() - amountRemaining;
    }
}

// Routine Description:
// - sets fields on the iterator to describe the end() state of the ATTR_ROW
void AttrRowIterator::_setToEnd()
{
    _alreadyAccessed = true;
    _currentRunIndex = _attrRow._list.size();
    _currentAttributeIndex = 0;
}
