/*++
Copyright (c) Microsoft Corporation

Module Name:
- AttrRowIterator.hpp

Abstract:
- iterator for ATTR_ROW to walk the TextAttributes of the run
- read only iterator

Author(s):
- Austin Diviness (AustDi) 04-Jun-2018
--*/


#pragma once

#include "TextAttribute.hpp"

class ATTR_ROW;

// for perf reasons, internally an AttrRowIterator has two different states: either it's previously accessed a
// TextAttribute from its associated ATTR_ROW or it hasn't. conversion from not accessed to accessed is a one
// way process which will slow down the incrementing/decrementing of the iterator but speed up the accessing
// of elements from the ATTR_ROW. this state is controlled by the _alreadyAccessed field. we do this so that
// we can quickly navigate the iterator to the beginning of the index we want to start reading from and then
// be able to quickly read consecutive elements.
//
// when in the "not accessed" state, the index of the element the iterator points to is stored in
// _logicalIndex. this is the public facing index of an element in the ATTR_ROW (the zero-indexed column of a
// row). when in the "accessed" state, the index of the element the iterator points to is stored in
// _currentRunIndex and _currentAttributeIndex. these are indices into the internal run-length-encoded storage
// of an ATTR_ROW. the fields that correspond to a state's index tracking are not guaranteed to be valid when
// not in that state so make sure to use the correct fields for the current state and any new functionality
// should work in both states.
class AttrRowIterator final
{
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = TextAttribute;
    using difference_type = std::ptrdiff_t;
    using pointer = TextAttribute*;
    using reference = TextAttribute&;

    static AttrRowIterator CreateEndIterator(const ATTR_ROW& attrRow);

    AttrRowIterator(const ATTR_ROW& attrRow);

    operator bool() const noexcept;

    bool operator==(const AttrRowIterator& it) const;
    bool operator!=(const AttrRowIterator& it) const;

    AttrRowIterator& operator++();
    AttrRowIterator operator++(int);

    AttrRowIterator& operator--();
    AttrRowIterator operator--(int);

    const TextAttribute* operator->() const;
    const TextAttribute& operator*() const;

private:
    const ATTR_ROW& _attrRow;
    size_t _currentRunIndex; // index of run in ATTR_ROW::_list
    size_t _currentAttributeIndex; // index of TextAttribute within the current TextAttributeRun
    size_t _logicalIndex; // "public" index of the attr row (ex. TextAttribute for the 3rd column)
    bool _alreadyAccessed; // true if we've already accessed the ATTR_ROW;

    void _increment(size_t count);
    void _decrement(size_t count);
    void _convertToAccessed();
    void _setToEnd();
};
