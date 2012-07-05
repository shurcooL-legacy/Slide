#pragma once
#ifndef __InputTestMode_H__
#define __InputTestMode_H__

Wm5::Vector3d ZoomSelectedModel(double dDistance);
Wm5::Vector3d ZoomSelectedModel(double dDistance, Wm5::Vector3d oDirection);

// Returns a vector that is the shadow of Vector on Plane
Wm5::Vector3d ProjectVectorOntoPlane(Wm5::Vector3d Vector, Wm5::Vector3d PlaneNormal);

// Finds the closest point to P that lies on Plane
// TODO: Rewrite this in a more optimal way, once I know a faster direct way... It currently intersects a Line with Direction == Plane.Normal with the Plane
Wm5::Vector3d ProjectPointOntoPlane(Wm5::Vector3d Point, Wm5::Plane3d Plane);

// Uses OpenGL to project a 3d point onto screen space
Wm5::Vector3d ProjectPointFrom3dToScreen(Wm5::Vector3d Point);

class InputTestMode : public ControlMode
{
public:
	InputTestMode();
	virtual ~InputTestMode();

	void Activate();

	virtual void SlideResolution();

	void PickModel();
	void ComputeUnderCursorPosition();

	void MoveSelectedObject(Wm5::Vector3d MoveVector);

	virtual void ProcessMouseButton(int MouseButton, int Action);
	virtual void ProcessMousePos(int MousePosX, int MousePosY);
	virtual	void ProcessMouseWheel(int MouseWheel);
	virtual	void ProcessKey(int Key, int Action);

	void ProcessTouchButton(int TouchId, int Action);
	void ProcessTouchPos(int TouchId, double TouchPosX, double TouchPosY);
	void ProcessTouchMove(int TouchId, double TouchMoveX, double TouchMoveY);

	int				m_CursorX;
	int				m_CursorY;
	Wm5::Vector3d	m_CursorRayDirection;

	//bool			m_DragPerformed;
	Wm5::Vector3d	m_UnderCursorPosition;

	int nMouseButtonsDown;

	bool bModelMovedByUser;
	bool ModelRotatedByUser;

	bool bTESTMode0Performed;
	bool bTESTMode1Performed;
	int nTESTView;
	int nTESTMode2;
	bool m_DebugSwitches[12];

private:
	InputTestMode(const InputTestMode &);
	InputTestMode & operator =(const InputTestMode &);

	bool		m_MouseCursorVisible;

	bool		m_MouseCursorIgnorePositionAlways;
	bool		m_MouseCursorIgnorePositionOnce;

	/*UnrealCameraModule				m_UnrealCameraModule;
	UnderCursorTranslationModule	m_UnderCursorTranslationModule;
	TwoAxisValuatorModule			m_TwoAxisValuatorModule;*/
	ControlModuleMapping		m_ControlModuleMapping;

	public:Wm5::Plane3d		m_SlidingConstraint;private:

	public:void SetMouseCursorVisibility(bool Visible);private:
	bool GetMouseCursorVisibility();

	public:enum SnapMode { Front, Back };
	enum StartingPosition { NonColliding, Colliding, Invisible };private:
	public:Wm5::Vector3d SnapOrtho(SnapMode SnapMode, StartingPosition StartingPosition, int8 TargetDepthLayer = 1);private:
	Wm5::Vector3d SnapOrtho(SnapMode SnapMode, Wm5::Vector3d SnapOrigin, Wm5::Vector3d SnapDirection, StartingPosition StartingPosition, bool SkipVisibilityCheck = false, int8 TargetDepthLayer = 1);
	public:Wm5::Vector3d SnapGeometry(SnapMode SnapMode);private:
	Wm5::Vector3d SnapGeometry(Wm5::Vector3d SnapDirection);
	public:Wm5::Vector3d SnapFullGeometryTEST();private:

	public:bool CheckForVisibility();private:

	enum InputMode { StickToPlane };
	bool IsModeActive(InputMode InputMode);

	static double ResolveTriTriCollisionAlongVector(Wm5::Triangle3d Tri0, Wm5::Triangle3d Tri1, Wm5::Vector3d Direction);

	GLuint		m_PickingFboId;
	GLuint		m_PickingColorTexId;
	GLuint		m_PickingDepthTexId;

	public:GLuint		m_QueryId;

	GLuint		m_DepthPeelFboId[2];
	GLuint		m_DepthPeelColorTexId[2];
	GLuint		m_DepthPeelDepthTexId[2];
	GLsizei		m_DepthPeelImageWidth;
	GLsizei		m_DepthPeelImageHeight;

	GLSLProgramObject	m_DepthPeelShader;private:

	void BuildShaders();
	void DestroyShaders();

	// Buffer distance added in Geometry Collision Resolution to safeguard from the collision being detected again (i.e. distance how much objects "float" on top of surfaces)
	public:static const double m_kBufferDistance;private:
};

#endif // __InputTestMode_H__
