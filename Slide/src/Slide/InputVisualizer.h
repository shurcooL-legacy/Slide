#pragma once
#ifndef __InputVisualizer_H__
#define __InputVisualizer_H__

namespace Slide
{

	class InputVisualizer
	{
	public:
		InputVisualizer();
		~InputVisualizer();

		void IncrementCounter();

	private:
		InputVisualizer(const InputVisualizer &);
		InputVisualizer & operator =(const InputVisualizer &);

		int m_nCounter;
	};

}

#endif // __InputVisualizer_H__
