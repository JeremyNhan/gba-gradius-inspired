#ifndef SS_POOL_H
#define SS_POOL_H

#include "bn_assert.h"

namespace ss
{

/**
 * Fixed-capacity object pool with stable slot indices and deterministic iteration order.
 *
 * T must provide `bool active` and `void clear()` (clear releases hardware resources such as
 * sprites and sets active = false). No heap allocation ever happens: all T objects are
 * constructed once with the pool.
 *
 * When the pool is full, spawn() returns nullptr and increments dropped(); callers treat that
 * as "the object was not created", which is the defined overflow behaviour for the whole game.
 */
template<typename T, int Capacity>
class pool
{

public:
    static constexpr int capacity = Capacity;

    [[nodiscard]] T* spawn()
    {
        for(int index = 0; index < Capacity; ++index)
        {
            T& item = _items[index];

            if(! item.active)
            {
                item.active = true;
                ++_count;
                return &item;
            }
        }

        ++_dropped;
        return nullptr;
    }

    void release(T& item)
    {
        BN_ASSERT(item.active, "Releasing an inactive pool item");
        item.clear();
        --_count;
    }

    void clear()
    {
        for(T& item : _items)
        {
            if(item.active)
            {
                item.clear();
            }
        }

        _count = 0;
    }

    [[nodiscard]] int count() const
    {
        return _count;
    }

    [[nodiscard]] int dropped() const
    {
        return _dropped;
    }

    [[nodiscard]] T& operator[](int index)
    {
        return _items[index];
    }

    [[nodiscard]] int index_of(const T& item) const
    {
        return int(&item - _items);
    }

    T* begin()
    {
        return _items;
    }

    T* end()
    {
        return _items + Capacity;
    }

    const T* begin() const
    {
        return _items;
    }

    const T* end() const
    {
        return _items + Capacity;
    }

private:
    T _items[Capacity];
    int _count = 0;
    int _dropped = 0;
};

}

#endif
