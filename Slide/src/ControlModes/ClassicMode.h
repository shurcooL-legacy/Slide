#pragma once
#ifndef __ClassicMode_H__
#define __ClassicMode_H__

class ClassicMode : public ControlMode
{
public:
	ClassicMode();
	virtual ~ClassicMode();

	virtual void SlideResolution();

	virtual void ProcessMouseButton(int MouseButton, int Action);
	virtual void ProcessMousePos(int MousePosX, int MousePosY);
	virtual	void ProcessMouseWheel(int MouseWheel);
	virtual	void ProcessKey(int Key, int Action);

	int nDesktopCursorX, nDesktopCursorY;

	bool bCursorArmed;
	bool bCursorJustHidden;
	int nMouseButtonsDown;

	bool bSelectPerformed;
	bool bModelMovedByUser;

	bool bTESTMode0Performed;
	bool bTESTMode1Performed;
	int nTESTView;
	int nTESTMode2;

private:
	ClassicMode(const ClassicMode &);
	ClassicMode & operator =(const ClassicMode &);

	public:enum SnapDirection { Front, Back };private:
	public:void SnapPersp(SnapDirection SnapDirection);private:
};

#endif // __ClassicMode_H__
