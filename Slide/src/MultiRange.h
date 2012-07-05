#pragma once
#ifndef __MultiRange_H__
#define __MultiRange_H__

template <typename T> class MultiRange
{
public:
	MultiRange();
	~MultiRange();

	void insert(std::pair<T, T> & Range);

	void pop_front();

	std::pair<T, T> front();

	bool empty() const;
	uint32 size() const;

	void clear();

private:
	MultiRange(const MultiRange &);
	MultiRange & operator =(const MultiRange &);

	std::list<std::pair<T, T>> m_Ranges;
};

#include "MultiRange.cpp"

#endif // __MultiRange_H__
