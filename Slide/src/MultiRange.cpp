template <typename T> MultiRange<T>::MultiRange()
	: m_Ranges()
{
}

template <typename T> MultiRange<T>::~MultiRange()
{
}

template <typename T> void MultiRange<T>::insert(std::pair<T, T> & Range)
{
	///assert(Range.first <= Range.second);

	auto it0 = m_Ranges.begin();

	// Skip all the ranges that end before ours starts
	while (it0 != m_Ranges.end() && it0->second < Range.first) ++it0;

	if (it0 != m_Ranges.end()) Range.first = std::min<T>(Range.first, it0->first);		// Extend our range to the left

	// Delete all the ranges that start before ours ends
	while (it0 != m_Ranges.end() && it0->first <= Range.second) {
		Range.second = std::max<T>(Range.second, it0->second);		// Extend our range to the right

		it0 = m_Ranges.erase(it0);
	}

	m_Ranges.insert(it0, Range);		// Insert the new range
}

template <typename T> void MultiRange<T>::pop_front()
{
	m_Ranges.pop_front();
}

template <typename T> std::pair<T, T> MultiRange<T>::front()
{
	return m_Ranges.front();
}

template <typename T> bool MultiRange<T>::empty() const
{
	return m_Ranges.empty();
}

template <typename T> uint32 MultiRange<T>::size() const
{
	return m_Ranges.size();
}

template <typename T> void MultiRange<T>::clear()
{
	m_Ranges.clear();
}
