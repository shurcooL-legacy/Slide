#pragma once
#ifndef __UnderCursorMode_H__
#define __UnderCursorMode_H__

class UnderCursorMode : public ControlMode
{
public:
	UnderCursorMode();
	virtual ~UnderCursorMode();

	void Activate() {}

	virtual void SlideResolution();

	void PickModel();
	void ComputeUnderCursorPosition();
	Wm5::Tuple<2, double> CalculateArcballCoordinates();

	virtual void ProcessMouseButton(int MouseButton, int Action);
	virtual void ProcessMousePos(int MousePosX, int MousePosY);
	virtual	void ProcessMouseWheel(int MouseWheel);
	virtual	void ProcessKey(int Key, int Action);

	int				m_CursorX;
	int				m_CursorY;
	Wm5::Vector3d	m_CursorRayDirection;

	bool			m_DragPerformed;
	Wm5::Vector3d	m_UnderCursorPosition;
	Wm5::Vector3d	m_OriginalSelectedObjectPosition;

	bool bCursorArmed;
	bool CursorWasArmed;
	bool bCursorJustHidden;
	int nMouseButtonsDown;

	bool bSelectPerformed;
	bool bModelMovedByUser;
	bool ModelRotatedByUser;

	bool bTESTMode0Performed;
	bool bTESTMode1Performed;
	int nTESTView;
	int nTESTMode2;

private:
	UnderCursorMode(const UnderCursorMode &);
	UnderCursorMode & operator =(const UnderCursorMode &);

	int nDesktopCursorX, nDesktopCursorY;

	public:enum SnapMode { Front, Back };private:
	public:void SnapOrtho(SnapMode SnapMode);private:
	void SnapOrtho(SnapMode SnapMode, Wm5::Vector3d SnapOrigin, Wm5::Vector3d SnapDirection);

	enum InputMode { StickToPlane };
	bool IsModeActive(InputMode InputMode);

	double ResolveTriTriCollisionAlongVector(Wm5::Triangle3d Tri0, Wm5::Triangle3d Tri1, Wm5::Vector3d Direction);
	void ResolveCollisionAlongPlane();

	Wm5::Plane3d	m_SecondConstraint;
	Wm5::Plane3d	m_ThirdConstraint;
	bool			m_StickToPlaneModePossible;

	static const int		m_kUnsnapDistance = 50;
};

#endif // __UnderCursorMode_H__
