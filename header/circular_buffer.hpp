#ifndef CIRCULAR_BUFFER_R
#define CIRCULAR_BUFFER_R

#include <memory>               // std::allocator, std::allocator_traits, std::uninitialized_copy
#include <cstddef>              // std::ptrdiff_t, std::size_t
#include <stdexcept>            // std::out_of_range
#include <cstring>              // std::memset
#include <iterator>             // std::bidirectional_iterator_tag
#include <type_traits>          // std::remove_cv
#include <iostream>             // std::ostream
#include <tuple>                // std::tie

template <typename T, typename Allocator = std::allocator<T>>
class CircularBuffer
{
    const unsigned char INVALID = 0xEE;

    Allocator mAlloc;
    size_t mCapacity;
    T* mBuffer;
    std::size_t mFirst;
    std::size_t mLast;

public:
    template <typename T1, bool isReversed = false>
    class Iterator
    {
        T1 *mIter;
        friend class PreIncrementHelper;
        friend class PreDecrementHelper;

        template <typename U, bool reverse>
        struct PreIncrementHelper;

        template <typename U>
        struct PreIncrementHelper<U, false>
        {
            void operator()(Iterator& iter) const { ++iter.mIter; }
        };

        template <typename U>
        struct PreIncrementHelper<U, true>
        {
            void operator()(Iterator& iter) const { --iter.mIter; }
        };

        template <typename U, bool reverse>
        struct PreDecrementHelper;

        template <typename U>
        struct PreDecrementHelper<U, false>
        {
            void operator()(Iterator& iter) const { --iter.mIter; }
        };

        template <typename U>
        struct PreDecrementHelper<U, true>
        {
            void operator()(Iterator& iter) const { ++iter.mIter; }
        };

    public:
        using iterator_category =   std::bidirectional_iterator_tag;
        using difference_type =     std::ptrdiff_t;
        using value_type =          typename std::decay<T1>::type;
        using pointer =             typename std::remove_reference<T1>::type*;
        using reference =           typename std::remove_reference<T1>::type&;

        Iterator(T1 *_iter) : mIter{ _iter} { }
        reference operator*() noexcept                      { return *mIter; }
        reference operator->() noexcept                     { return *mIter; }
        bool operator==(const Iterator& rhs) const noexcept { return mIter == rhs.mIter; }
        bool operator!=(const Iterator& rhs) const noexcept { return !operator==(rhs); }
        Iterator operator++() const                         { PreIncrementHelper<T1, isReversed>{}(*const_cast<Iterator*>(this)); return *this; }
        Iterator operator++(int) const                      { Iterator retval{ *this }; ++(*this); return retval; }
        Iterator operator--() const                         { PreDecrementHelper<T1, isReversed>{}(*const_cast<Iterator*>(this)); return *this; }
        Iterator operator--(int) const                      { Iterator retval{ *this }; --(*this); return retval; }
        friend std::ostream& operator<<(std::ostream& os, const Iterator& rhs)  { return os << reinterpret_cast<void*>(rhs.mIter); }
    };

    using value_type =              typename std::remove_cv<T>::type;
    using allocator_type =          Allocator;
    using size_type =               std::size_t;
    using difference_type =         std::ptrdiff_t;
    using reference =               value_type&;
    using const_reference =         const value_type&;
    using pointer =                 typename std::allocator_traits<Allocator>::pointer;
    using const_pointer =           typename std::allocator_traits<Allocator>::const_pointer;
    using iterator =                Iterator<T>;
    using const_iterator =          Iterator<const T>;
    using reverse_iterator =        Iterator<T, true>;
    using const_reverse_iterator =  Iterator<const T, true>;

    CircularBuffer(size_t size = 1)
        : mAlloc{ }, mCapacity{ size }, mBuffer{ mAlloc.allocate(size) },
          mFirst{ 0 }, mLast{ 0 }
    {
        for (auto& elem : *this)
            memset(&elem, INVALID, sizeof(elem));
    }

    CircularBuffer(const CircularBuffer& rhs)
        : mAlloc{ rhs.mAlloc }, mCapacity{ rhs.mCapacity },
          mBuffer{ mAlloc.allocate(mCapacity) }, mFirst{ rhs.mFirst },
          mLast{ rhs.mLast }
    {
        T* end = rhs.mBuffer + mCapacity;
        std::uninitialized_copy(rhs.mBuffer, end, mBuffer);
    }

    CircularBuffer(CircularBuffer&& rhs)
        : mAlloc{ rhs.mAlloc }, mCapacity{ rhs.mCapacity },
          mBuffer{ rhs.mBuffer }, mFirst{ rhs.mFirst }, mLast{ rhs.mLast }
    {
        rhs.mBuffer = nullptr;
        rhs.mFirst = 0;
        rhs.mLast = 0;
        rhs.mCapacity = 0;
    }

    template <typename U>
    CircularBuffer(U _begin, U _end)
        : mAlloc{ }, mCapacity{ static_cast<unsigned long>(_end - _begin) }, 
          mBuffer{ mAlloc.allocate(mCapacity) }, mFirst{ 0 }, mLast{ mCapacity - 1 }
    {
        std::uninitialized_copy(_begin, _end, mBuffer);
    }

    ~CircularBuffer()
    {
        mAlloc.deallocate(mBuffer, mCapacity);
    }

    CircularBuffer& operator=(const CircularBuffer& rhs)
    {
        T* tmp = mAlloc.allocate(rhs.mCapacity);
        T* end = rhs.mBuffer + rhs.mCapacity;
        std::uninitialized_copy(rhs.mBuffer, end, tmp);
        mAlloc.deallocate(mBuffer, mCapacity);
        
        mAlloc = rhs.mAlloc;
        mCapacity = rhs.mCapacity;
        mBuffer = tmp;
        mFirst = rhs.mFirst;
        mLast = rhs.mLast;
        return *this;
    }

    CircularBuffer& operator=(CircularBuffer&& rhs)
    {
        std::swap(mCapacity, rhs.mCapacity);
        std::swap(mBuffer, rhs.mBuffer);
        std::swap(mFirst, rhs.mFirst);
        std::swap(mLast, rhs.mLast);
        std::swap(mAlloc, rhs.mAlloc);
        return *this;
    }

    size_type size() const noexcept
    {
        if (mFirst == mLast)
            return *reinterpret_cast<unsigned char*>(&mBuffer[mFirst]) != INVALID;
        if (mFirst < mLast)
            return mLast - mFirst + 1;
        return mCapacity - (mFirst - mLast - 1);
    }

    value_type operator[](unsigned index) const
    {
        if (index >= mCapacity)
            throw std::out_of_range{ "Accessing out of bounds!" };
        return mBuffer[index];
    }

    reference operator[](unsigned index)
    {
        if (index >= mCapacity)
            throw std::out_of_range{ "Accessing out of bounds!" };
        return mBuffer[index];
    }

    void resize(unsigned sz)
    {
        T* tmp = mAlloc.allocate(sz);
        size_type diff = mLast - mFirst;
        if (mCapacity <= sz)
            std::uninitialized_copy(mBuffer, mBuffer + mCapacity, tmp);
        else
        {
            std::uninitialized_copy(mBuffer, mBuffer + sz, tmp);
        }
        mAlloc.deallocate(mBuffer, mCapacity);
        mCapacity = sz;
        mBuffer = tmp;
    }

    void push(const T& value)
    {
        if (empty())
            new (mBuffer + mLast) T{ value };
        else if (size() != mCapacity)
        {
            mLast = (mLast + 1) % mCapacity;
            new (mBuffer + mLast) T{ value };
        }
        else
        {
            new (mBuffer + mFirst) T{ value };
            mLast = mFirst;
            mFirst = (mFirst + 1) % mCapacity;
        }
    }

    template <typename ... Args>
    void emplace(Args&& ... args)
    {
        if (empty())
            new (mBuffer + mLast) T{ std::forward<Args>(args)... };
        else if (size() != mCapacity)
        {
            mLast = (mLast + 1) % mCapacity;
            new (mBuffer + mLast) T{ std::forward<Args>(args)... };
        }
        else
        {
            new (mBuffer + mFirst) T{ std::forward<Args>(args)... };
            mLast = mFirst;
            mFirst = (mFirst + 1) % mCapacity;
        }
    }

    value_type pop()
    {
        value_type value = mBuffer[mFirst];
        pointer iter = mBuffer + mFirst;
        memset(iter, INVALID, sizeof(*iter));
        if (mFirst != mLast)
            mFirst = (mFirst + 1) % mCapacity;
        return value;
    }

    void clear()
    {
        std::size_t end = mLast + 1;
        for (std::size_t i = mFirst; i != end; ++i)
            mBuffer[i].~T();
        for (auto& elem : *this)
            memset(&elem, INVALID, sizeof(elem));
        mFirst = mLast = 0;
    }

    value_type at(unsigned index) const                         { return operator[](index); }
    reference at(unsigned index)                                { return operator[](index); }
    value_type back() const                                     { return mBuffer[mLast]; }
    reference back()                                            { return mBuffer[mLast]; }
    value_type first()                                          { return mBuffer[mFirst]; }
    reference first() const                                     { return mBuffer[mFirst]; }
    pointer data() const                                        { return mBuffer; }
    Allocator get_allocator() const                             { return mAlloc; }
    bool empty() const noexcept                                 { return mLast == mFirst && *reinterpret_cast<unsigned char*>(&mBuffer[mFirst]) == INVALID; }
    size_type capacity() const noexcept                         { return mCapacity; }
    iterator begin() const noexcept                             { return mBuffer; }
    iterator end() const noexcept                               { return mBuffer + mCapacity; }
    const_iterator cbegin() const noexcept                      { return mBuffer; }
    const_iterator cend() const noexcept                        { return mBuffer + mCapacity; }
    reverse_iterator rbegin() const noexcept                    { return mBuffer + mLast; }
    reverse_iterator rend() const noexcept                      { return mBuffer - 1; }
    const_reverse_iterator crbegin() const noexcept             { return mBuffer + mLast; }
    const_reverse_iterator crend() const noexcept               { return mBuffer - 1; }
    bool operator==(const CircularBuffer& rhs) const noexcept   { return std::tie(mBuffer, mFirst, mLast, mCapacity) == std::tie(rhs.mBuffer, rhs.mFirst, rhs.mLast, rhs.mCapacity); }
    bool operator!=(const CircularBuffer& rhs) const noexcept   { return !operator==(rhs); }
    void swap(CircularBuffer& rhs)                              { *this = std::move(rhs); }
};

#endif