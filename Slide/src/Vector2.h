#pragma once
#ifndef __Vector2_H__
#define __Vector2_H__

template <typename T> class Vector2
{
public:
	Vector2();
	Vector2(T x, T y);
	Vector2(const Vector2 & Other);
	~Vector2();

	T X() const;
    T & X ();
    T Y() const;
    T & Y();

	T operator [] (const uint8 & Index) const;

	Vector2 & operator = (const Vector2 & Other);

	Vector2 operator + (const Vector2 & Other) const;
	Vector2 operator - (const Vector2 & Other) const;
	Vector2 operator * (const T Value) const;
	Vector2 operator / (const T Value) const;

private:
	T m_Tuple[2];
};

#include "Vector2.hpp"

typedef Vector2<sint32> Vector2n;
typedef Vector2<double> Vector2d;

#endif // __Vector2_H__
