#ifndef _CHECKED_H
#define _CHECKED_H


//#include <AK/Assertions.h>
#include "NumericLimits.h"
#include "StdLibExtras.h"

namespace AK {

/*template<typename Destination, typename Source, bool destination_is_wider = (sizeof(Destination) >= sizeof(Source)), bool destination_is_signed = NumericLimits<Destination>::is_signed(), bool source_is_signed = NumericLimits<Source>::is_signed()>
struct TypeBoundsChecker;

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, false, false, false> {
    static constexpr bool is_within_range(Source value)
    {
        return value <= NumericLimits<Destination>::max();
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, false, true, true> {
    static constexpr bool is_within_range(Source value)
    {
        return value <= NumericLimits<Destination>::max()
            && NumericLimits<Destination>::min() <= value;
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, false, false, true> {
    static constexpr bool is_within_range(Source value)
    {
        return static_cast<MakeUnsigned<Source>>(value) <= NumericLimits<Destination>::max();
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, false, true, false> {
    static constexpr bool is_within_range(Source value)
    {
        return value <= static_cast<Source>(NumericLimits<Destination>::max());
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, true, false, false> {
    static constexpr bool is_within_range(Source)
    {
        return true;
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, true, true, true> {
    static constexpr bool is_within_range(Source)
    {
        return true;
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, true, false, true> {
    static constexpr bool is_within_range(Source value)
    {
        return value >= 0;
    }
};

template<typename Destination, typename Source>
struct TypeBoundsChecker<Destination, Source, true, true, false> {
    static constexpr bool is_within_range(Source value)
    {
        if (sizeof(Destination) > sizeof(Source))
            return true;
        return value <= static_cast<Source>(NumericLimits<Destination>::max());
    }
};

template<typename Destination, typename Source>
[[nodiscard]] constexpr bool is_within_range(Source value)
{
    return TypeBoundsChecker<Destination, Source>::is_within_range(value);
}*/

template<typename T>
class Checked {
public:
    constexpr Checked() = default;

    constexpr Checked(T value)
        : m_value(value)
    {
    }

/*    template<typename U>
    constexpr Checked(U value)
    {
        m_overflow = !is_within_range<T>(value);
        m_value = value;
    }
*/
    constexpr Checked(const Checked&) = default;

    constexpr Checked(Checked&& other)
        : m_value(exchange(other.m_value, 0))
        , m_overflow(exchange(other.m_overflow, false))
    {
    }

    template<typename U>
    constexpr Checked& operator=(U value)
    {
        return *this = Checked(value);
    }

    constexpr Checked& operator=(const Checked& other) = default;

    constexpr Checked& operator=(Checked&& other)
    {
        m_value = exchange(other.m_value, 0);
        m_overflow = exchange(other.m_overflow, false);
        return *this;
    }

    [[nodiscard]] constexpr bool has_overflow() const
    {
        return m_overflow;
    }

    inline constexpr bool operator!() const
    {
        VERIFY(!m_overflow);
        return !m_value;
    }

    inline constexpr T value() const
    {
        VERIFY(!m_overflow);
        return m_value;
    }

    constexpr void add(T other)
    {
        m_overflow |= __builtin_add_overflow(m_value, other, &m_value);
    }

    constexpr void sub(T other)
    {
        m_overflow |= __builtin_sub_overflow(m_value, other, &m_value);
    }

    constexpr void mul(T other)
    {
        m_overflow |= __builtin_mul_overflow(m_value, other, &m_value);
    }

    constexpr void div(T other)
    {
        m_value /= other;
    }

    constexpr Checked& operator+=(const Checked& other)
    {
        m_overflow |= other.m_overflow;
        add(other.value());
        return *this;
    }

    constexpr Checked& operator+=(T other)
    {
        add(other);
        return *this;
    }

    constexpr Checked& operator-=(const Checked& other)
    {
        m_overflow |= other.m_overflow;
        sub(other.value());
        return *this;
    }

    constexpr Checked& operator-=(T other)
    {
        sub(other);
        return *this;
    }

    constexpr Checked& operator*=(const Checked& other)
    {
        m_overflow |= other.m_overflow;
        mul(other.value());
        return *this;
    }

    constexpr Checked& operator*=(T other)
    {
        mul(other);
        return *this;
    }

    constexpr Checked& operator/=(const Checked& other)
    {
        m_overflow |= other.m_overflow;
        div(other.value());
        return *this;
    }

    constexpr Checked& operator/=(T other)
    {
        div(other);
        return *this;
    }

    constexpr Checked& operator++()
    {
        add(1);
        return *this;
    }

    constexpr Checked operator++(int)
    {
        Checked old { *this };
        add(1);
        return old;
    }

    constexpr Checked& operator--()
    {
        sub(1);
        return *this;
    }

    constexpr Checked operator--(int)
    {
        Checked old { *this };
        sub(1);
        return old;
    }

    template<typename U, typename V>
    [[nodiscard]] static constexpr bool addition_would_overflow(U u, V v)
    {
#ifdef __clang__
        Checked checked;
        checked = u;
        checked += v;
        return checked.has_overflow();
#else
        return __builtin_add_overflow_p(u, v, (T)0);
#endif
    }

    template<typename U, typename V>
    [[nodiscard]] static constexpr bool multiplication_would_overflow(U u, V v)
    {
#ifdef __clang__
        Checked checked;
        checked = u;
        checked *= v;
        return checked.has_overflow();
#else
        return __builtin_mul_overflow_p(u, v, (T)0);
#endif
    }

    template<typename U, typename V, typename X>
    [[nodiscard]] static constexpr bool multiplication_would_overflow(U u, V v, X x)
    {
        Checked checked;
        checked = u;
        checked *= v;
        checked *= x;
        return checked.has_overflow();
    }

private:
    T m_value {};
    bool m_overflow { false };
};

template<typename T>
constexpr Checked<T> operator+(const Checked<T>& a, const Checked<T>& b)
{
    Checked<T> c { a };
    c.add(b.value());
    return c;
}

template<typename T>
constexpr Checked<T> operator-(const Checked<T>& a, const Checked<T>& b)
{
    Checked<T> c { a };
    c.sub(b.value());
    return c;
}

template<typename T>
constexpr Checked<T> operator*(const Checked<T>& a, const Checked<T>& b)
{
    Checked<T> c { a };
    c.mul(b.value());
    return c;
}

template<typename T>
constexpr Checked<T> operator/(const Checked<T>& a, const Checked<T>& b)
{
    Checked<T> c { a };
    c.div(b.value());
    return c;
}

template<typename T>
constexpr bool operator<(const Checked<T>& a, T b)
{
    return a.value() < b;
}

template<typename T>
constexpr bool operator>(const Checked<T>& a, T b)
{
    return a.value() > b;
}

template<typename T>
constexpr bool operator>=(const Checked<T>& a, T b)
{
    return a.value() >= b;
}

template<typename T>
constexpr bool operator<=(const Checked<T>& a, T b)
{
    return a.value() <= b;
}

template<typename T>
constexpr bool operator==(const Checked<T>& a, T b)
{
    return a.value() == b;
}

template<typename T>
constexpr bool operator!=(const Checked<T>& a, T b)
{
    return a.value() != b;
}

template<typename T>
constexpr bool operator<(T a, const Checked<T>& b)
{
    return a < b.value();
}

template<typename T>
constexpr bool operator>(T a, const Checked<T>& b)
{
    return a > b.value();
}

template<typename T>
constexpr bool operator>=(T a, const Checked<T>& b)
{
    return a >= b.value();
}

template<typename T>
constexpr bool operator<=(T a, const Checked<T>& b)
{
    return a <= b.value();
}

template<typename T>
constexpr bool operator==(T a, const Checked<T>& b)
{
    return a == b.value();
}

template<typename T>
constexpr bool operator!=(T a, const Checked<T>& b)
{
    return a != b.value();
}

template<typename T>
constexpr Checked<T> make_checked(T value)
{
    return Checked<T>(value);
}

}

using AK::Checked;
//using AK::make_checked;

#endif
