#ifndef _STDLIBEXTRAS_H
#define _STDLIBEXTRAS_H

namespace jaat_std{

template<typename T>
constexpr T&& move(T& arg)
{
    return static_cast<T&&>(arg);
}


template<typename T>
constexpr T min(const T& a, const T& b)
{
    return b < a ? b : a;
}

template<typename T>
constexpr T max(const T& a, const T& b)
{
    return a < b ? b : a;
}

template<typename T>
constexpr T clamp(const T& value, const T& min, const T& max)
{
    assert(max >= min);
    if (value > max)
        return max;
    if (value < min)
        return min;
    return value;
}

template<typename T, typename U>
constexpr T ceil_div(T a, U b)
{
    static_assert(sizeof(T) == sizeof(U));
    T result = a / b;
    if ((a % b) != 0)
        ++result;
    return result;
}

template<typename T, typename U>
inline void swap(T& a, U& b)
{
    U tmp = move((U&)a);
    a = (T &&) move(b);
    b = move(tmp);
}

/*template<typename T, typename U = T>
constexpr T exchange(T& slot, U&& value)
{
    T old_value = move(slot);
    slot = forward<U>(value);
    return old_value;
}*/


inline void fast_u32_copy(unsigned int* dest, const unsigned int* src, int count)//size_t count)
{
    asm volatile(
        "rep movsl\n"
        : "+S"(src), "+D"(dest), "+c"(count)::"memory");
}

inline void fast_u32_fill(unsigned int* dest, unsigned int value, int count)//size_t count)
{
    asm volatile(
        "rep stosl\n"
        : "=D"(dest), "=c"(count)
        : "D"(dest), "c"(count), "a"(value)
        : "memory");
}

}



#endif
