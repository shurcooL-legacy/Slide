#pragma once
#ifndef __InputListener_H__
#define __InputListener_H__

class InputListener
{
public:
	InputListener();
	virtual ~InputListener();

	// Raw low-level input
	virtual void ProcessButton(InputManager::InputId ButtonId, bool Pressed) {}
	virtual void ProcessSlider(InputManager::InputId SliderId, double MovedAmount) {}
	virtual void ProcessAxis(InputManager::InputId AxisId, InputManager::AxisState AxisState) {}
	virtual void Process2Axes(InputManager::InputId FirstAxisId, InputManager::AxisState AxisState[2]) {}

	// Virtual high-level input
	virtual void ProcessCharacter(int Character, bool Pressed) {}
	/*virtual bool ProcessMouseButton(int nMouseButton, bool bPressed) { return false; }
	virtual bool ProcessMousePosition(int nMousePositionX, int nMousePositionY) { return false; }*/

private:
	InputListener(const InputListener &);
	InputListener & operator =(const InputListener &);
};

#endif // __InputListener_H__
