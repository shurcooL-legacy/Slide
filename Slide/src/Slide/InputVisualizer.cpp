#include "InputVisualizer.h"

namespace Slide
{

	InputVisualizer::InputVisualizer()
		: m_nCounter(0)
	{
	}

	InputVisualizer::~InputVisualizer()
	{
	}

	void InputVisualizer::IncrementCounter()
	{
		++m_nCounter;
	}

}
