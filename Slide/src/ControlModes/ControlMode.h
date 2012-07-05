#pragma once
#ifndef __ControlMode_H__
#define __ControlMode_H__

class ControlMode
{
public:
	ControlMode();
	virtual ~ControlMode();

	virtual void ProcessMouseButton(int MouseButton, int Action) = 0;
	virtual void ProcessMousePos(int MousePosX, int MousePosY) = 0;
	virtual	void ProcessMouseWheel(int MouseWheel) = 0;
	virtual	void ProcessKey(int Key, int Action) = 0;

private:
	ControlMode(const ControlMode &);
	ControlMode & operator =(const ControlMode &);
};

#endif // __ControlMode_H__
