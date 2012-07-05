#include "../Globals.h"

#include <cstdio>
#include <set>
#include <vector>

// FCollada
#define NO_LIBXML
// disable deprecated warning
//#pragma warning( disable : 4996 )
#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FUtils/FUObject.h"

#include "../Models/ModelLoader.h"
#include "../Models/ColladaModel.h"
#include "../Models/SupermarketModel.h"
#include "../Models/SketchUpModel.h"
#include "../Scene.h"
#include "../SceneObject.h"

#include "ControlMode.h"
#include "InputTestMode.h"

extern Wm5::Vector3d DebugVector[10];

void PlaySound();

// Went back to 025 thanks to additional Wm5 collision detection filtering
// Started using 100 instead of 025, because scene 2 had problems
// Right now went back to using 025, because 010 sometimes takes too many SnapGeometry(Front) calls to resolve collisions.
//const double InputTestMode::m_kBufferDistance = 0.010000;
//const double InputTestMode::m_kBufferDistance = 0.001000;
//const double InputTestMode::m_kBufferDistance = 0.000100;
  const double InputTestMode::m_kBufferDistance = 0.000025;
//const double InputTestMode::m_kBufferDistance = 0.000010;

InputTestMode::InputTestMode()
	: m_CursorX(0),
	  m_CursorY(0),
	  m_CursorRayDirection(),
	  //m_DragPerformed(false),
	  m_UnderCursorPosition(),
	  nMouseButtonsDown(0),
	  bModelMovedByUser(false),
	  ModelRotatedByUser(false),
	  bTESTMode0Performed(false),
	  bTESTMode1Performed(false),
	  nTESTView(0),
	  nTESTMode2(2),
	  m_DebugSwitches(),
	  m_MouseCursorVisible(true),
	  m_MouseCursorIgnorePositionAlways(true),
	  m_MouseCursorIgnorePositionOnce(false),
	  /*m_UnrealCameraModule(),
	  m_UnderCursorTranslationModule(),
	  m_TwoAxisValuatorModule(),*/
	  m_ControlModuleMapping(),
	  m_SlidingConstraint(Wm5::Vector3d::ZERO, 0),
	  m_PickingFboId(),
	  m_PickingColorTexId(),
	  m_PickingDepthTexId(),
	  m_QueryId(),
	  m_DepthPeelFboId(),
	  m_DepthPeelColorTexId(),
	  m_DepthPeelDepthTexId(),
	  m_DepthPeelImageWidth(0),
	  m_DepthPeelImageHeight(0),
	  m_DepthPeelShader()
{
	{
		std::vector<uint32> ButtonMappings; ButtonMappings.push_back(1000 + GLFW_MOUSE_BUTTON_LEFT); ButtonMappings.push_back(0 + GLFW_KEY_LALT);
		std::vector<uint32> SliderMappings; SliderMappings.push_back(1000 + 2);
		std::vector<uint32> AxisMappings; AxisMappings.push_back(1000 + 0); AxisMappings.push_back(1000 + 1);
		std::vector<uint32> PositiveConstraints;
		std::vector<uint32> NegativeConstraints; NegativeConstraints.push_back(GLFW_KEY_LCTRL);// NegativeConstraints.push_back(1000 + GLFW_MOUSE_BUTTON_RIGHT);
		m_ControlModuleMapping.AddMapping(new UnderCursorTranslationModule(), 0, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}

	{
		std::vector<uint32> ButtonMappings; ButtonMappings.push_back(1000 + GLFW_MOUSE_BUTTON_RIGHT);
		std::vector<uint32> SliderMappings; SliderMappings.push_back(1000 + 0); SliderMappings.push_back(1000 + 1); SliderMappings.push_back(1000 + 2);
		std::vector<uint32> AxisMappings;
		std::vector<uint32> PositiveConstraints;
		std::vector<uint32> NegativeConstraints; NegativeConstraints.push_back(GLFW_KEY_LCTRL);// NegativeConstraints.push_back(1000 + GLFW_MOUSE_BUTTON_LEFT);
		m_ControlModuleMapping.AddMapping(new TwoAxisValuatorModule(), 0, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}

#ifdef SLIDE_USE_BAMBOO
	{
		std::vector<uint32> ButtonMappings; ButtonMappings.push_back(1000 + GLFW_MOUSE_BUTTON_LEFT); ButtonMappings.push_back(1000 + GLFW_MOUSE_BUTTON_RIGHT);
		std::vector<uint32> SliderMappings; SliderMappings.push_back(1000 + 0); SliderMappings.push_back(1000 + 1);
		std::vector<uint32> AxisMappings;
		std::vector<uint32> PositiveConstraints; PositiveConstraints.push_back(GLFW_KEY_LCTRL);
		std::vector<uint32> NegativeConstraints;
		m_ControlModuleMapping.AddMapping(new UnrealCameraModule(), 1, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}
#else
	{
		std::vector<uint32> ButtonMappings; ButtonMappings.push_back(2000 + 0); ButtonMappings.push_back(2000 + 1);
		std::vector<uint32> SliderMappings; SliderMappings.push_back(2000 + 0); SliderMappings.push_back(2000 + 1);
		std::vector<uint32> AxisMappings;
		std::vector<uint32> PositiveConstraints;
		std::vector<uint32> NegativeConstraints;
		m_ControlModuleMapping.AddMapping(new UnrealCameraModule(), 1, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}
#endif
}

InputTestMode::~InputTestMode()
{
	// Delete FBOs (Render Targets)
	if (m_PickingFboId) glDeleteFramebuffers(1, &m_PickingFboId);
	if (m_PickingColorTexId) glDeleteTextures(1, &m_PickingColorTexId);
	if (m_PickingDepthTexId) glDeleteTextures(1, &m_PickingDepthTexId);
	if (m_QueryId) glDeleteQueries(1, &m_QueryId);
	if (m_DepthPeelFboId[0]) glDeleteFramebuffers(2, m_DepthPeelFboId);
	if (m_DepthPeelColorTexId[0]) glDeleteTextures(2, m_DepthPeelColorTexId);
	if (m_DepthPeelDepthTexId[0]) glDeleteTextures(2, m_DepthPeelDepthTexId);

	DestroyShaders();
}

void InputTestMode::BuildShaders()
{
	m_DepthPeelShader.attachVertexShader("src/Shaders/depth_peel_vertex.glsl");
	m_DepthPeelShader.attachFragmentShader("src/Shaders/depth_peel_fragment.glsl");
	m_DepthPeelShader.link();

	CHECK_GL_ERRORS
}

void InputTestMode::DestroyShaders()
{
	m_DepthPeelShader.destroy();

	CHECK_GL_ERRORS
}

void InputTestMode::Activate()
{
	glGenQueries(1, &m_QueryId);

	// Initialize FBOs (Render Targets)
	{
		glGenFramebuffers(1, &m_PickingFboId);
		glGenTextures(1, &m_PickingColorTexId);
		glGenTextures(1, &m_PickingDepthTexId);

		{
			glBindTexture(GL_TEXTURE_2D, m_PickingColorTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			glBindTexture(GL_TEXTURE_2D, m_PickingDepthTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			glBindFramebuffer(GL_FRAMEBUFFER, m_PickingFboId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PickingColorTexId, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_PickingDepthTexId, 0);
		}
	}

	{
		//m_DepthPeelImageWidth = nViewportWidth;
		//m_DepthPeelImageHeight = nViewportHeight;
		m_DepthPeelImageWidth = std::min<uint32>(400, nViewportWidth);
		m_DepthPeelImageHeight = m_DepthPeelImageWidth * nViewportHeight / nViewportWidth;

		glGenFramebuffers(2, m_DepthPeelFboId);
		glGenTextures(2, m_DepthPeelColorTexId);
		glGenTextures(2, m_DepthPeelDepthTexId);

		for (int Texture = 0; Texture < 2; ++Texture)
		{
			glBindTexture(GL_TEXTURE_RECTANGLE, m_DepthPeelColorTexId[Texture]);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, m_DepthPeelImageWidth, m_DepthPeelImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			glBindTexture(GL_TEXTURE_RECTANGLE, m_DepthPeelDepthTexId[Texture]);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT32F, m_DepthPeelImageWidth, m_DepthPeelImageHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			glBindFramebuffer(GL_FRAMEBUFFER, m_DepthPeelFboId[Texture]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_DepthPeelColorTexId[Texture], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, m_DepthPeelDepthTexId[Texture], 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		CHECK_GL_ERRORS
	}

	BuildShaders();

	m_MouseCursorIgnorePositionOnce = true;
	m_MouseCursorIgnorePositionAlways = false;
}

bool InputTestMode::IsModeActive(InputMode InputMode)
{
	switch (InputMode)
	{
	case StickToPlane:
		//return (GLFW_PRESS == glfwGetKey('X') && m_StickToPlaneModePossible);
#ifdef SLIDE_TRANSLATIONAL_SNAPPING_ENABLED
		return m_StickToPlaneModePossible;
#else
		return true;
#endif
		break;
	default:
		return false;
		break;
	}
}

void InputTestMode::SlideResolution()
{
	if (0 != MyScene.GetSelectedObjectId())
	{
		if (bModelMovedByUser)
		{
#if 1
			// DEBUG: This needs to be redone properly
			{
				GLuint SampleCount; glGetQueryObjectuiv(m_QueryId, GL_QUERY_RESULT, &SampleCount);
				bool SelectedObjectVisible = (0 != SampleCount);
				//printf("VISIBLE > %d\n", SelectedObjectVisible);

				if (!SelectedObjectVisible) {
					printf("Object became invisible, bringing it to front...\n");
					SnapOrtho(SnapMode::Front, StartingPosition::Invisible);
					printf("Brought it to front.\n");
					bModelMovedByUser = false;
					return;
				}
			}
#endif

			bool CollisionExists = CheckForCollision();

#if 1
			if (IsModeActive(StickToPlane) && 0 != MyScene.m_SlidingObject && Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CollisionExists)
			{
				/*if (CollisionExists)
				{
					PlaySound();
					ResolveCollisionAlongPlane();
					CollisionExists = CheckForCollision();
				}
				else
				{
					// If no collision is found when moving object back against second constraint, unsnap it
					if (Wm5::Vector3d::ZERO != m_SecondConstraint.Normal && !CheckForCollisionTemporary(m_SecondConstraint.Normal * -0.002)) {
						m_SecondConstraint = m_ThirdConstraint;
						m_ThirdConstraint.Normal = Wm5::Vector3d::ZERO;
						m_StickToPlaneModePossible = false;
					}

					// If no collision is found when moving object back against third constraint, unsnap it
					if (Wm5::Vector3d::ZERO != m_ThirdConstraint.Normal && !CheckForCollisionTemporary(m_ThirdConstraint.Normal * -0.002)) {
						m_ThirdConstraint.Normal = Wm5::Vector3d::ZERO;
						m_StickToPlaneModePossible = false;
					}

					// If no collision is found when moving object back against primary constraint, snap it back to background
					if (!CheckForCollisionTemporary(MyScene.GetSlidingPlane().Normal * -0.002)) {
						m_SecondConstraint.Normal = Wm5::Vector3d::ZERO;
						m_ThirdConstraint.Normal = Wm5::Vector3d::ZERO;
						m_StickToPlaneModePossible = false;
					}
				}*/

				// If no collision is found when moving object back against primary constraint, unsnap it
				if (Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CheckForCollisionTemporary(m_SlidingConstraint.Normal * -2 * m_kBufferDistance)) {
					m_SlidingConstraint.Normal = Wm5::Vector3d::ZERO;
					//printf("Dropped sliding constraint\n");
				}
			}
			if (!(IsModeActive(StickToPlane) && 0 != MyScene.m_SlidingObject && Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CollisionExists))
			{
#if 1
				if (!CollisionExists)
					// Snap back
					SnapOrtho(SnapMode::Back, StartingPosition::NonColliding);
				else
					// Snap front
					SnapOrtho(SnapMode::Front, StartingPosition::Colliding);
#else
				// Snap back
				if (!CollisionExists)
				{
#if 0
					MoveSelectedObject(SnapOrtho(Back));
					//MoveSelectedObject(ZoomSelectedModel(m_kBufferDistance, m_CursorRayDirection));		// Push it a little further back to force a collision, which will cause geometry to resolve
					CollisionExists = CheckForCollision();
#else
					SnapOrtho(Back);
					CollisionExists = false;
#endif
				}

				// Snap front
				// TODO: Eventually, may need to do this in a better way for all cases (i.e. use framebuffer for rough Snap Front estimate). Imagine a highly-tesselated sphere, 5 iterations will not be nearly enough.
				int Iterations = 5;		// Multiple iterations are needed because we resolve only colliding part of selected object, not the entire thing
				while (CollisionExists && Iterations-- > 0) {
					MoveSelectedObject(SnapGeometry(Front));
					CollisionExists = CheckForCollision();
				}
				//printf("iterations for geo front: %d\n", 50-Iterations);

				//m_StickToPlaneModePossible = true;
#endif
			}

			//if (!CollisionExists)
#endif
				bModelMovedByUser = false;
		}
		else if (ModelRotatedByUser)
		{
#if 1
			{
				bool CollisionExists = CheckForCollision();

				//Wm5::Vector3d Normal(0, 0, 1);
				Wm5::Vector3d Normal(m_SlidingConstraint.Normal);

				if (!CollisionExists)
				{
					Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(Normal));

					Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);
					Wm5::Vector3d SnapDirection(-Normal);

					SnapOrtho(Back, SnapOrigin, SnapDirection, StartingPosition::NonColliding, true);
				}
				else
				{
					Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(-Normal));

					Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);
					Wm5::Vector3d SnapDirection(Normal);

					SnapOrtho(Front, SnapOrigin, SnapDirection, StartingPosition::Colliding, true);
				}
			}
#elif 0
			//if (0 != MyScene.m_SlidingObject)
			//if (0 != MyScene.GetSelectedObject().m_ContainedByObject)
			{
				//Wm5::Vector3d Normal(MyScene.GetObject(MyScene.GetSelectedObject().m_ContainedByObject).GetTriangleNormal(MyScene.m_SlidingTriangle));
				//Wm5::Vector3d Normal(MyScene.GetSlidingPlane());
				Wm5::Vector3d Normal(0, 0, 1);
				Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(Normal));

				Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);
				Wm5::Vector3d SnapDirection(-Normal);

				bool CollisionExists = CheckForCollision();

				// Snap back
				if (!CollisionExists)
				{
					printf("SnapOrtho(Back, SnapOrigin, SnapDirection);\n");
					MoveSelectedObject(SnapOrtho(Back, SnapOrigin, SnapDirection, true));
					CollisionExists = CheckForCollision();
				}

				// Snap front
				int Iterations = 5;		// Multiple iterations are needed because we resolve only colliding part of selected object, not the entire thing
				while (CollisionExists && Iterations-- > 0) {
					printf("SnapOrtho(Front, SnapOrigin, SnapDirection);\n");
					MoveSelectedObject(SnapOrtho(Front, SnapOrigin, SnapDirection, true));
					CollisionExists = CheckForCollision();
				}
			}
#endif

			ModelRotatedByUser = false;
		}
	}
}

void InputTestMode::PickModel()
{
//if (MyScene.GetSelectedObjectId()) MyScene.GetSelectedObject().m_oIntersectingTris.clear();

	glViewport(-m_CursorX, -m_CursorY, nViewportWidth, nViewportHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, m_PickingFboId);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);

	MyScene.Render(Scene::ObjectPicking);

	uint8 cPixel[4]; glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, cPixel);
	uint16 NewSelectedObject = cPixel[1] << 8 | cPixel[0];

	// Return to normal framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glDrawBuffer(GL_BACK);
	glViewport(0, 0, nViewportWidth, nViewportHeight);

	if (1 == NewSelectedObject) NewSelectedObject = 0;

	// If we selected a different object, clear the sliding object
	if (NewSelectedObject != MyScene.GetSelectedObjectId() && 0 != NewSelectedObject) {
		MyScene.m_SlidingObject = MyScene.GetObject(NewSelectedObject).m_ContainedByObject;
		if (0 != MyScene.m_SlidingObject) exit(0);
		MyScene.m_SlidingTriangle = 0;		// DEBUG: This needs to be either set to correct value if 0 != SlidingObject, or else ignored
		MyScene.m_SelectedTriangle = 0;		// DEBUG: This needs to be either set to correct value if 0 != SlidingObject, or else ignored

// DEBUG: This is a temporary hack, I should think about where to reset the sliding constraint (or perhaps keep it on a per-object basis)
m_SlidingConstraint.Normal = Wm5::Vector3d::ZERO;
	}

	MyScene.SetSelectedObjectId(NewSelectedObject);

	uint32 Triangle = cPixel[3] << 8 | cPixel[2];
//if (MyScene.GetSelectedObjectId()) MyScene.GetSelectedObject().m_oIntersectingTris.insert(Triangle);

	if (MyScene.GetSelectedObjectId()) {
		//m_OriginalSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();

		printf("  selected model #%d at %d, %d, triangle = %d.\n", MyScene.GetSelectedObjectId(), m_CursorX, m_CursorY, Triangle);

		bModelMovedByUser = true;
	}
}

void InputTestMode::ComputeUnderCursorPosition()
{
	glViewport(-m_CursorX, -m_CursorY, nViewportWidth, nViewportHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, m_PickingFboId);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);

	GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
	GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);

	float Depth; glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(&Depth));
	gluUnProject(0, 0, Depth, ModelMatrix, ProjectionMatrix, Viewport, &m_UnderCursorPosition.X(), &m_UnderCursorPosition.Y(), &m_UnderCursorPosition.Z());

	// Return to normal framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glDrawBuffer(GL_BACK);
	glViewport(0, 0, nViewportWidth, nViewportHeight);
}

void InputTestMode::MoveSelectedObject(Wm5::Vector3d MoveVector)
{
	//MyScene.GetSelectedObject().MoveBy(MoveVector);
	std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
	for (auto it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
		if ((*it0)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
			(*it0)->MoveBy(MoveVector);
	m_UnderCursorPosition += MoveVector;
}

void InputTestMode::SetMouseCursorVisibility(bool Visible)
{
	static int MouseCursorDesktopPositionX, MouseCursorDesktopPositionY;

	// Hide Mouse Cursor
	if (!Visible && m_MouseCursorVisible)
	{
		glfwGetMousePos(&MouseCursorDesktopPositionX, &MouseCursorDesktopPositionY);
		m_MouseCursorIgnorePositionAlways = true;
		glfwDisable(GLFW_MOUSE_CURSOR);
		m_MouseCursorIgnorePositionOnce = true;
		m_MouseCursorIgnorePositionAlways = false;

		m_MouseCursorVisible = false;
	}
	// Show Mouse Cursor
	else if (Visible && !m_MouseCursorVisible)
	{
		m_MouseCursorIgnorePositionAlways = true;
		glfwEnable(GLFW_MOUSE_CURSOR);
		glfwSetMousePos(MouseCursorDesktopPositionX, MouseCursorDesktopPositionY);
		m_MouseCursorIgnorePositionOnce = true;
		m_MouseCursorIgnorePositionAlways = false;

		m_MouseCursorVisible = true;

		// Create a mouse position event
		ProcessMousePos(m_CursorX, m_CursorY);
	}
}

bool InputTestMode::GetMouseCursorVisibility()
{
	return m_MouseCursorVisible;
}

void InputTestMode::ProcessMouseButton(int MouseButton, int Action)
{//if (Action) printf("  mb +%d\n", MouseButton); else printf("  mb -%d\n", MouseButton);
	static int nPrevMouseButtonsDown = 0;

	bool bCtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
	bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
	bool bAltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

	m_ControlModuleMapping.ProcessButton(1000 + MouseButton, (GLFW_PRESS == Action));

#if 0
	if (0 == MouseButton || 1 == MouseButton)
		m_UnrealCameraModule.ProcessButton(MouseButton, (GLFW_PRESS == Action));
#endif

	if (GLFW_PRESS == Action)
	{
		nMouseButtonsDown |= (1 << MouseButton);

#if 0
		if (0 == MouseButton && !glfwGetMouseButton(GLFW_MOUSE_BUTTON_2) && !bCtrlPressed/* && !bShiftPressed && !bAltPressed*/)
			m_UnderCursorTranslationModule.ProcessButton(0, true);
#endif

#if 0
		if (1 == MouseButton && !glfwGetMouseButton(GLFW_MOUSE_BUTTON_1) && !bCtrlPressed/* && !bShiftPressed && !bAltPressed*/)
			m_TwoAxisValuatorModule.ProcessButton(0, true);
#endif

		if (!nPrevMouseButtonsDown && nMouseButtonsDown)
		{
			//if (bCtrlPressed || (8 == nMouseButtonsDown || 16 == nMouseButtonsDown || (8+16) == nMouseButtonsDown)) bCursorArmed = true;
			if (bCtrlPressed)
			{
				//SetMouseCursorVisibility(false);
			}

			if (GLFW_MOUSE_BUTTON_LEFT == MouseButton && !bCtrlPressed)
			{
			}
			else if (GLFW_MOUSE_BUTTON_RIGHT == MouseButton && !bCtrlPressed)
			{
			}
		}
	}
	else if (GLFW_RELEASE == Action)
	{
		nMouseButtonsDown &= ~(1 << MouseButton);

#if 0
		if (0 == MouseButton)
			m_UnderCursorTranslationModule.ProcessButton(0, false);
#endif

#if 0
		if (1 == MouseButton)
			m_TwoAxisValuatorModule.ProcessButton(0, false);
#endif

		/*if (GLFW_MOUSE_BUTTON_LEFT == MouseButton) {
			m_SecondConstraint.Normal = Wm5::Vector3d::ZERO;
			m_ThirdConstraint.Normal = Wm5::Vector3d::ZERO;
			m_StickToPlaneModePossible = false;
		}*/

		if (nPrevMouseButtonsDown && !nMouseButtonsDown)
		{
#if 0
			if (0 != MyScene.GetSelectedObjectId() && 1 == nPrevMouseButtonsDown)
			{
				printf("Stopped sliding on object %d\n", MyScene.m_SlidingObject);
				if (MyScene.GetSelectedObjectId() == MyScene.m_SlidingObject) printf("Error: MyScene.GetSelectedObjectId() == MyScene.m_SlidingObject\n");

				// Clear previous group association, if it exists
				if (0 != MyScene.GetSelectedObject().m_ContainedByObject) {
					MyScene.GetObject(MyScene.GetSelectedObject().m_ContainedByObject).m_ContainedObjects.erase(MyScene.GetSelectedObjectId());
				}

				// Set Contained-By Object
				MyScene.GetSelectedObject().m_ContainedByObject = MyScene.m_SlidingObject;

				// Insert new group association
				if (0 != MyScene.GetSelectedObject().m_ContainedByObject) {
					MyScene.GetObject(MyScene.GetSelectedObject().m_ContainedByObject).m_ContainedObjects.insert(MyScene.GetSelectedObjectId());
				}
			}
#endif

			//SetMouseCursorVisibility(true);
		}
	}

	nPrevMouseButtonsDown = nMouseButtonsDown;
}

void InputTestMode::ProcessMousePos(int MousePosX, int MousePosY)
{//if (nMouseButtonsDown == 0) printf("mm 0 - %d,%d\n", MousePosX, MousePosY); else printf("     mm 1! - %d,%d\n", MousePosX, MousePosY);
	static int nPrevMousePosX, nPrevMousePosY;

	MousePosY = nViewportHeight - 1 - MousePosY;

	bool bCtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
	bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
	bool bAltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

	int nMouseMovedX = MousePosX - nPrevMousePosX;
	int nMouseMovedY = MousePosY - nPrevMousePosY;
	nPrevMousePosX = MousePosX;
	nPrevMousePosY = MousePosY;

	if (m_MouseCursorIgnorePositionAlways)
	{
		//if (nMouseButtonsDown == 0) printf("mm 0 - %d,%d (toggling)\n", nMouseMovedX, nMouseMovedY); else printf("     mm 1! - %d,%d (toggling)\n", nMouseMovedX, nMouseMovedY);
		return;
	}

	if (m_MouseCursorIgnorePositionOnce)
	{
		m_MouseCursorIgnorePositionOnce = false;
		//if (nMouseButtonsDown == 0) printf("mm 0 - %d,%d (just toggled)\n", nMouseMovedX, nMouseMovedY); else printf("     mm 1! - %d,%d (just toggled)\n", nMouseMovedX, nMouseMovedY);
		return;
	}

//if (nMouseButtonsDown == 0) printf("mm 0 - %d,%d\n", nMouseMovedX, nMouseMovedY); else printf("     mm 1! - %d,%d\n", nMouseMovedX, nMouseMovedY);

	m_ControlModuleMapping.ProcessSlider(1000 + 0, nMouseMovedX);
	m_ControlModuleMapping.ProcessSlider(1000 + 1, nMouseMovedY);
	if (GetMouseCursorVisibility())
	{
		{
			m_CursorX = MousePosX;
			m_CursorY = MousePosY;

			// Find the new cursor ray
			{
				Wm5::Vector3d Camera(camera.x, camera.y, camera.z), FarPoint;
				GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
				GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
				GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);

				gluUnProject(m_CursorX, m_CursorY, 1.0f, ModelMatrix, ProjectionMatrix, Viewport, &FarPoint.X(), &FarPoint.Y(), &FarPoint.Z());

				m_CursorRayDirection = FarPoint - Camera;
				m_CursorRayDirection.Normalize();
			}
		}

		m_ControlModuleMapping.Process2Axes(1000 + 0, MousePosX, MousePosY);
	}

#if 1
	// STUDY: Orbit Camera around the table
	if (GLFW_PRESS == glfwGetMouseButton(GLFW_MOUSE_BUTTON_4) || GLFW_PRESS == glfwGetMouseButton(GLFW_MOUSE_BUTTON_5))
	{
		camera.rh += 0.20 * nMouseMovedX;
		camera.rv += 0.20 * nMouseMovedY;
		while (camera.rh < 0) camera.rh += 360;
		while (camera.rh >= 360) camera.rh -= 360;
		if (camera.rv > 12) camera.rv = 12; if (camera.rv < -90) camera.rv = -90;

		Wm5::Vector3d LookAtPoint(-5.880347, 4.9107215, 1.1);
		double Distance = 6;

		Wm5::Vector3d ViewDirection(Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD));
		Wm5::Vector3d Camera(camera.x, camera.y, camera.z);

		Camera = ViewDirection * -5 + LookAtPoint;
		camera.x = Camera.X(); camera.y = Camera.Y(); camera.z = Camera.Z();
	}
#endif
}

void InputTestMode::ProcessMouseWheel(int MouseWheel)
{
	static int PrevMouseWheel = MouseWheel;

	int MouseWheelMoved = MouseWheel - PrevMouseWheel;
	PrevMouseWheel = MouseWheel;

	m_ControlModuleMapping.ProcessSlider(1000 + 2, MouseWheelMoved);
	m_ControlModuleMapping.ProcessAxis(1000 + 2, MouseWheel);

	if (0 != MyScene.GetSelectedObjectId())
	{
		bool CtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
		bool ShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
		bool AltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

		if (!CtrlPressed && !ShiftPressed && !AltPressed)
		{
#if 0
			m_UnderCursorTranslationModule.ProcessSlider(0, MouseWheelMoved);

			m_TwoAxisValuatorModule.ProcessSlider(2, MouseWheelMoved);
#endif
		}
	}
}

void InputTestMode::ProcessKey(int Key, int Action)
{
	bool CtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
	bool ShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
	bool AltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

	m_ControlModuleMapping.ProcessButton(0 + Key, (GLFW_PRESS == Action));

	if (GLFW_PRESS == Action)
	{
		switch (Key) {
		case GLFW_KEY_ESC:
			if (!nMouseButtonsDown && !MyScene.GetSelectedObjectId())
			{
				KeepRunning = false;
			}
			else if (nMouseButtonsDown)
			{
				nMouseButtonsDown = 0;
				/*if (MyScene.GetSelectedObjectId()) {
					m_UnderCursorPosition += m_OriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition();
					MyScene.GetSelectedObject().ModifyPosition() = m_OriginalSelectedObjectPosition;
				}*/
			}
			else if (MyScene.GetSelectedObjectId())
			{
				MyScene.SetSelectedObjectId(0);
			}
			break;
		case 'P':
			printf("--->>>>>>>>>>---\n");
			printf("std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();\n");
			for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin() + 1; it0 != MyScene.m_Objects.end(); ++it0) {
				printf("++it0; (*it0)->ModifyPosition().X() = %f; (*it0)->ModifyPosition().Y() = %f; (*it0)->ModifyPosition().Z() = %f; (*it0)->ModifyRotation().W() = %f; (*it0)->ModifyRotation().X() = %f; (*it0)->ModifyRotation().Y() = %f; (*it0)->ModifyRotation().Z() = %f;\n",
					(*it0)->GetPosition().X(), (*it0)->GetPosition().Y(), (*it0)->GetPosition().Z(), (*it0)->ModifyRotation().W(), (*it0)->ModifyRotation().X(), (*it0)->ModifyRotation().Y(), (*it0)->ModifyRotation().Z());
			}
			printf("\n");
			printf("camera.x = %f; camera.y = %f; camera.z = %f; camera.rh = %f; camera.rv = %f;\n", camera.x, camera.y, camera.z, camera.rh, camera.rv);
			printf("---<<<<<<<<<<---\n");
			break;
		case 'O':
			printf("--->>>>>>>>>>---\n");
			for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0) {
				printf("Object %d:\n", (*it0)->GetId());
				printf("  m_ContainedObjects: { ");
					for (std::set<uint16>::iterator it1 = (*it0)->m_ContainedObjects.begin(); it1 != (*it0)->m_ContainedObjects.end(); ++it1) {
						printf("%d, ", *it1);
					}
					printf("}\n");
				printf("  m_ContainedByObject: %d\n", (*it0)->m_ContainedByObject);
			}
			printf("---<<<<<<<<<<---\n");
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
			Scene::InitializeScene(Key - '1' + 1);
			break;
		case GLFW_KEY_F1: case GLFW_KEY_F2: case GLFW_KEY_F3: case GLFW_KEY_F4: case GLFW_KEY_F5: case GLFW_KEY_F6: case GLFW_KEY_F7: case GLFW_KEY_F8: case GLFW_KEY_F9: case GLFW_KEY_F10: case GLFW_KEY_F11: case GLFW_KEY_F12:
			m_DebugSwitches[Key - GLFW_KEY_F1] = !m_DebugSwitches[Key - GLFW_KEY_F1];
			break;
		case 'B':
			bTESTMode0Performed = true;
			break;
		case 'F':
			bTESTMode1Performed = true;
			break;
		case 'C':
			printf("CollisionExists = %d\n", CheckForCollision());
			if (MyScene.GetSelectedObjectId())
				printf("SelObj.Z() = %.20f (%f, %f)\n", MyScene.GetSelectedObject().GetPosition().Z(), MyScene.GetSelectedObject().GetPosition().X(), MyScene.GetSelectedObject().GetPosition().Y());
			break;
		case 'V':
			nTESTView = 1 - nTESTView;
			break;
		case 'Z':
			nTESTMode2 = 1 - nTESTMode2;
			printf("nTESTMode2 = %d\n", nTESTMode2);
			break;
		case GLFW_KEY_KP_ADD:
			++nTESTMode2;
			printf("NumberDepthLayersToPeel = %d\n", nTESTMode2);
			break;
		case GLFW_KEY_KP_SUBTRACT:
			if (nTESTMode2 <= 0) break;
			--nTESTMode2;
			printf("NumberDepthLayersToPeel = %d\n", nTESTMode2);
			break;
		case 'R':
			DestroyShaders();
			BuildShaders();
			break;
		case 'X':
			break;
		//case 'R':
		case 'T':
		case 'Y':
			if (0 != MyScene.GetSelectedObjectId()) {
				bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
				//Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Wm5::Vector3d(0, 0, 1), 45 * Wm5::Mathd::DEG_TO_RAD);
				Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Wm5::Vector3d(('R' == Key) * (!bShiftPressed ? 1 : -1), ('T' == Key) * (!bShiftPressed ? 1 : -1), ('Y' == Key) * (!bShiftPressed ? 1 : -1)), 45 * Wm5::Mathd::DEG_TO_RAD);
				Wm5::Vector3d V1(1, 0, 0);
				Wm5::Vector3d V2(1, 0.1, 0); V2.Normalize();
				//RotateBy.Align(V1, V2);
				MyScene.GetSelectedObject().ModifyRotation() = MyScene.GetSelectedObject().ModifyRotation() * RotateBy;		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
				MyScene.GetSelectedObject().ModifyRotation().Normalize();
				//printf("Quaternion.Length() == %.30f\n", MyScene.GetSelectedObject().ModifyRotation().Length());
				ModelRotatedByUser = true;

				Wm5::Vector3d axis;
				double angle;
				MyScene.GetSelectedObject().ModifyRotation().ToAxisAngle(axis, angle);
				printf("angle around some-axis: %f\n", angle * Wm5::Mathd::RAD_TO_DEG * axis.Z());
			}
			break;
		default:
			break;
		}
	}
	else if (GLFW_RELEASE == Action)
	{
		/*switch (Key)
		{
		default:
			break;
		}*/
	}
}

void InputTestMode::ProcessTouchButton(int TouchId, int Action)
{
	m_ControlModuleMapping.ProcessButton(2000 + TouchId, 0 != Action);
}

void InputTestMode::ProcessTouchPos(int TouchId, double TouchPosX, double TouchPosY)
{
	m_ControlModuleMapping.Process2Axes(2000 + 2 * TouchId, TouchPosX, TouchPosY);
}

void InputTestMode::ProcessTouchMove(int TouchId, double TouchMoveX, double TouchMoveY)
{
	TouchMoveY *= -1;		// TODO: Remove this hack

	m_ControlModuleMapping.ProcessSlider(2000 + 2 * TouchId + 0, TouchMoveX);
	m_ControlModuleMapping.ProcessSlider(2000 + 2 * TouchId + 1, TouchMoveY);
}

// Input: Two triangles Tri0 and Tri1 (such that they are intersecting or touching), a unit vector Direction
// Output: The minimum non-negative value k, such that after moving triangle Tri0 by k*Direction results in the two triangles no longer intersecting (within some small value)
double InputTestMode::ResolveTriTriCollisionAlongVector(Wm5::Triangle3d Tri0, Wm5::Triangle3d Tri1, Wm5::Vector3d Direction)
{
	// TODO: Remove magic consts and calculate them based on triangle size/bounding box
	const double InitialBacktrack = 10;

	// Move Tri0 back before the collision could have occurred
	for (int Vertex = 0; Vertex < 3; ++Vertex)
		Tri0.V[Vertex] += Direction * InitialBacktrack;

	Wm5::IntrTriangle3Triangle3d Intersection(Tri0, Tri1);

	// Now test how much it can move forward (towards its original position) before there is a collision
	bool Result = Intersection.Test(InitialBacktrack, -Direction, Wm5::Vector3d::ZERO);
	if (Result) {
if ((InitialBacktrack - Intersection.GetContactTime()) < 0) printf(" ++++++++++++++ POST-CONDITION FAILED in ResolveTriTriCollisionAlongVector: returning negative k\n");
		// Return the difference between collision time and Tri0's starting position, this is the value of k
		return (InitialBacktrack - Intersection.GetContactTime());
	}

	// The two triangles were not intersecting at all
	return 0;
}

// Returns true if selected object is visible from camera POV, using perspective projection
bool InputTestMode::CheckForVisibility()
{
	// Set a smaller viewport with same aspect ratio
	GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);
	uint32 nCustomViewportWidth = m_DepthPeelImageWidth, nCustomViewportHeight = m_DepthPeelImageHeight;
	glViewport(0, 0, nCustomViewportWidth, nCustomViewportHeight);

	// Perspective projection
	//SetOpenGLProjectionMatrix(Perspective);
	{
		glPushMatrix();		// Push Modelview Matrix
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();		// Push Projection Matrix
		glLoadIdentity();
		gluPerspective(45, static_cast<double>(nViewportWidth) / nViewportHeight, 0.1, 100);
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();
		glRotated(camera.rv + 90, -1, 0, 0);
		glRotated(camera.rh, 0, 0, 1);
		glTranslated(-camera.x, -camera.y, -camera.z);
	}

	// Check that object is visible, using perspective projection
	bool IsObjectVisible;
	{
		MyScene.Render(Scene::SelectedObjectVisibilityQuery);

		GLuint SampleCount; glGetQueryObjectuiv(m_QueryId, GL_QUERY_RESULT, &SampleCount);
		IsObjectVisible = (0 != SampleCount);
	}
	//printf("IsObjectVisible = %d\n", IsObjectVisible);

	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();		// Restore Projection Matrix
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();		// Restore Modelview Matrix
	}
	glViewport(Viewport[0], Viewport[1], Viewport[2], Viewport[3]);		// Restore Viewport

	return IsObjectVisible;
}

Wm5::Vector3d InputTestMode::SnapOrtho(SnapMode SnapMode, StartingPosition StartingPosition, int8 TargetDepthLayer)
{
	Wm5::Vector3d SnapDirection;
	if (Back == SnapMode)
		SnapDirection = m_CursorRayDirection;
	else if (Front == SnapMode)
		SnapDirection = -m_CursorRayDirection;

	Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(-SnapDirection));
	Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);

	return SnapOrtho(SnapMode, SnapOrigin, SnapDirection, StartingPosition, false, TargetDepthLayer);
}
Wm5::Vector3d InputTestMode::SnapOrtho(SnapMode SnapMode, Wm5::Vector3d SnapOrigin, Wm5::Vector3d SnapDirection, StartingPosition StartingPosition, bool SkipVisibilityCheck, int8 TargetDepthLayer)
{
//if (Back == SnapMode) printf("SnapOrtho(Back):  ");
//if (Front == SnapMode) printf("SnapOrtho(Front): ");

	SnapDirection.Normalize();

	const Wm5::Vector3d kOriginalSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();
	Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);

#if 1
	// Try a quick collision resolve in a special simple case
	if (Colliding == StartingPosition && (Front == SnapMode && 1 == TargetDepthLayer))
	{
		Wm5::Vector3d OriginalSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();		// TODO: Make this function operate using temporary object positions, instead of physically moving selected object inside; after that, this variable is no longer needed

		MoveSelectedObject(SnapGeometry(-SnapDirection));

		if (!CheckForCollision() && (SkipVisibilityCheck ? true : CheckForVisibility()))
			//return MoveVector;			// Because I'm actually moving the object within this function...
			return Wm5::Vector3d::ZERO;		// TODO: Clean this up
		else
			MoveSelectedObject(OriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition());		// Reset Object Position
	}
#endif

	// Set a smaller viewport with same aspect ratio
	/*uint32 nCustomViewportWidth = std::min<uint32>(400, nViewportWidth);
	//uint32 nCustomViewportWidth = nViewportWidth;
	uint32 nCustomViewportHeight = nCustomViewportWidth * nViewportHeight / nViewportWidth;*/
	uint32 nCustomViewportWidth = m_DepthPeelImageWidth, nCustomViewportHeight = m_DepthPeelImageHeight;
	glViewport(0, 0, nCustomViewportWidth, nCustomViewportHeight);

	// Ortho projection
	{
		//SetOpenGLProjectionMatrix(Ortho);
		glPushMatrix();		// Push Modelview Matrix
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();		// Push Projection Matrix
		glLoadIdentity();
		glOrtho(-100, 100, -100, 100, 0, 100);
		glMatrixMode(GL_MODELVIEW);

		// Look at the selected model
		Wm5::Vector3d CameraOrigin(SnapOrigin);
		Wm5::Vector3d CameraTarget(SnapOrigin + SnapDirection);
		glLoadIdentity();
		if (CameraOrigin.X() != CameraTarget.X() || CameraOrigin.Y() != CameraTarget.Y())
			gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 0, 0, 1);
		else
			gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 1, 0, 0);

		Wm5::Tuple<4, double> ProjectedBoundingBox = MyScene.GetSelectedObject().GetProjectedBoundingBox();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(ProjectedBoundingBox[0] - 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]), ProjectedBoundingBox[2] + 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]),
				ProjectedBoundingBox[1] - 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), ProjectedBoundingBox[3] + 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), 0, 100);
		glMatrixMode(GL_MODELVIEW);
	}

	{
		uint8 * pSPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight];
		float * pSDepths = new float[nCustomViewportWidth * nCustomViewportHeight];
		uint8 * pSPixels2 = nullptr;
		float * pSDepths2 = nullptr;
		if (Front == SnapMode) {
			pSPixels2 = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight];
			pSDepths2 = new float[nCustomViewportWidth * nCustomViewportHeight];
		}
		uint8 * pPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight];
		float * pDepths = new float[nCustomViewportWidth * nCustomViewportHeight];

		// Peel Depth Layers away
		Wm5::Vector3d OriginalSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();		// TODO: Make this function operate using temporary object positions, instead of physically moving selected object inside; after that, this variable is no longer needed
		Wm5::Vector3d PreviousSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();
		uint8 DepthLayersAchieved = 0;
		uint8 MaxLayersToPeel = 16;
		for (uint8 DepthLayer = 0; DepthLayersAchieved < TargetDepthLayer && DepthLayer <= MaxLayersToPeel; ++DepthLayer)
		{
			// Reset Object Position
			MoveSelectedObject(OriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition());

			// Set target FBO
			glBindFramebuffer(GL_FRAMEBUFFER, m_DepthPeelFboId[DepthLayer % 2]);
			//glDrawBuffer(GL_COLOR_ATTACHMENT0);

			if (0 == DepthLayer)
			{
				if (Front == SnapMode) {
					MyScene.Render(Scene::SelectedObjectBackFace);

					glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels2));
					glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths2));
				}

				// Render the selected model
				if (Back == SnapMode)
					MyScene.Render(Scene::SelectedObjectBackFace);
				else if (Front == SnapMode)
					MyScene.Render(Scene::SelectedObjectFrontFace);

				glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels));
				glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths));
			}
			else
			{
				m_DepthPeelShader.bind();
				m_DepthPeelShader.bindTextureRECT("DepthTex", m_DepthPeelDepthTexId[(DepthLayer - 1) % 2], 0);
				if (Back == SnapMode)
					MyScene.Render(Scene::StaticSceneGeometry);
				else if (Front == SnapMode)
					MyScene.Render(Scene::StaticSceneGeometryBackFace);
				m_DepthPeelShader.unbind();

				glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pPixels));
				glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths));

				if (Back == SnapMode)
				{
					double dMinPositiveDistance = 2;		// Positive distance means away from the camera (i.e. to snap back to background); valid range is (0, +1)
					//uint32 ClosestPixel = 0;
					for (uint32 nPixel = 0; nPixel < nCustomViewportWidth * nCustomViewportHeight; ++nPixel)
					{
						uint8 r = pPixels[4 * nPixel + 0];
						uint8 g = pPixels[4 * nPixel + 1];
						uint8 b = pPixels[4 * nPixel + 2];
						uint8 a = pPixels[4 * nPixel + 3];
						float d = pDepths[nPixel];

						uint8 sr = pSPixels[4 * nPixel + 0];
						uint8 sg = pSPixels[4 * nPixel + 1];
						uint8 sb = pSPixels[4 * nPixel + 2];
						uint8 sa = pSPixels[4 * nPixel + 3];
						float sd = pSDepths[nPixel];

						if ((sr+sg) != 0 && (r+g) != 0)
						{
							double dDepthDiff = static_cast<double>(d) - sd;

							if (dDepthDiff >= 0.0 && dDepthDiff < dMinPositiveDistance)
							{
								dMinPositiveDistance = dDepthDiff;
								MoveVector = 100 * dDepthDiff * SnapDirection;

								//ClosestPixel = nPixel;
							}
						}
					}

					// Make sure there is something visible, and quit early if not (in that case, it doesn't make sense to go further)
					// This creates for "either get to the target level, or go back to original position" behaviour
					// TODO: Perhaps this needs to be changed to "do as best as you can"
					if (2 == dMinPositiveDistance && !(Invisible == StartingPosition)) {
						break;
					}
				}
				else if (Front == SnapMode)
				{
					MultiRange<float> mr;		// Positive distance means away from the camera (i.e. to snap back to background); valid range is (0, +1)
					for (uint32 nPixel = 0; nPixel < nCustomViewportWidth * nCustomViewportHeight; ++nPixel)
					{
						uint8 r = pPixels[4 * nPixel + 0];
						uint8 g = pPixels[4 * nPixel + 1];
						uint8 b = pPixels[4 * nPixel + 2];
						uint8 a = pPixels[4 * nPixel + 3];
						float d = pDepths[nPixel];

						// Front Faces of selected object
						uint8 sr = pSPixels[4 * nPixel + 0];
						uint8 sg = pSPixels[4 * nPixel + 1];
						uint8 sb = pSPixels[4 * nPixel + 2];
						uint8 sa = pSPixels[4 * nPixel + 3];
						float sd = pSDepths[nPixel];

						// Back Faces of selected object
						uint8 sr2 = pSPixels2[4 * nPixel + 0];
						uint8 sg2 = pSPixels2[4 * nPixel + 1];
						uint8 sb2 = pSPixels2[4 * nPixel + 2];
						uint8 sa2 = pSPixels2[4 * nPixel + 3];
						float sd2 = pSDepths2[nPixel];

						if ((sr+sg) != 0 && (sr2+sg2) != 0 && (r+g) != 0)
						{
							float dDepthDiff = d - sd;
							///assert(sd < sd2);
							float SelectedObjectThickness = sd2 - sd;		// Should be a non-negative number >= 0

							if (dDepthDiff >= 0.0)
								mr.insert(std::pair<float, float>(dDepthDiff - SelectedObjectThickness, dDepthDiff));
						}
					}

					if (!mr.empty())
						MoveVector = 100 * static_cast<double>(mr.front().second) * SnapDirection;

					// Make sure there is something visible, and quit early if not (in that case, it doesn't make sense to go further)
					// This creates for "either get to the target level, or go back to original position" behaviour
					// TODO: Perhaps this needs to be changed to "do as best as you can"
					if (mr.empty() && !(Invisible == StartingPosition)) {
						break;
					}
				}

				// Find out if we've successfully made a pop to the next layer, or if the pop was unsuccessful (either unable to resolve a collision
				{
					if (Back == SnapMode)
						MoveVector += ZoomSelectedModel(m_kBufferDistance, SnapDirection);		// Push it a little further *back* to force a collision, which will cause geometry to resolve
					else if (Front == SnapMode)
						MoveVector += ZoomSelectedModel(-m_kBufferDistance, SnapDirection);		// Pull it a little *forward* to force a collision, which will cause geometry to resolve
					MoveSelectedObject(MoveVector);

					bool CollisionExists = CheckForCollision();
					if (Front == SnapMode && !CollisionExists) {
						MoveSelectedObject(ZoomSelectedModel(2 * m_kBufferDistance, SnapDirection));		// Push it a little further *back* to force a collision, which will cause geometry to resolve
						CollisionExists = CheckForCollision();
					}
					printf("CollisionExists = %d at new place (layer %d)\n", CollisionExists, DepthLayer);

					//int Iterations = 5;
					int Iterations = 50;
					while (CollisionExists && Iterations-- > 0) {
						if (Back == SnapMode)
							MoveSelectedObject(SnapGeometry(SnapDirection));
						else if (Front == SnapMode)
							MoveSelectedObject(SnapGeometry(-SnapDirection));

						CollisionExists = CheckForCollision();
					}
					//printf("Took %d SnapGeometry() iterations.\n", 50 - Iterations);
					//printf("sli obj = %d, sli tri = %d | sel tri = %d\n", MyScene.m_SlidingObject, MyScene.m_SlidingTriangle, MyScene.m_SelectedTriangle);

					// Make sure the object doesn't end up in exactly the same position it started from, unless this is a snap *back* to the first layer
					bool EndedUpInPreviousPosition = (0.000001 * m_kBufferDistance * m_kBufferDistance) >= (PreviousSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition()).SquaredLength();
					bool WorkingOnFirstLayer = (0 == DepthLayersAchieved);
					if (!CollisionExists && (!EndedUpInPreviousPosition || (WorkingOnFirstLayer && Back == SnapMode)))
					{
#if 0
						PreviousSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();
						++DepthLayersAchieved;
#else
						// Set target FBO
						glBindFramebuffer(GL_FRAMEBUFFER, m_DepthPeelFboId[(DepthLayer + 1) % 2]);
						//glDrawBuffer(GL_COLOR_ATTACHMENT0);

						bool SelectedObjectVisible = SkipVisibilityCheck ? true : CheckForVisibility();

						if (SelectedObjectVisible)
						{
							PreviousSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();
							++DepthLayersAchieved;
						}
						else if (Back == SnapMode)
						{
							//MoveSelectedObject(PreviousSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition());
							MoveSelectedObject(OriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition());
							break;
						}
						else if (Front == SnapMode)
						{
							//MoveSelectedObject(PreviousSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition());
							//break;
							PreviousSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();
						}
#endif
					}
					else if (CollisionExists)
					{
						printf("Problem! Unhandled exception. Unable to Geo-resolve collision (took too many iterations, giving up).\n");
					}
				}

#if 0
				// Geometry distance correction
				do if (dMinBackDistance != 0 || dMinPositiveDistance != 2)		// Use <do> loop to make use of <break> keyword
				{
					Wm5::Vector3d Point;
					{
						GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
						GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
						GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);
						uint32 PixelX = ClosestPixel % nCustomViewportWidth;
						uint32 PixelY = ClosestPixel / nCustomViewportWidth;
						gluUnProject(PixelX, PixelY, 0, ModelMatrix, ProjectionMatrix, Viewport, &Point.X(), &Point.Y(), &Point.Z());

						//DebugVector[0] = Point;
						//DebugVector[1] = Point + (SnapDirection * 10);
					}
					Wm5::Line3d Line(Point, SnapDirection);

					Wm5::Triangle3d SlidingTriangle = MyScene.GetObject(MyScene.m_SlidingObject).GetTriangle(MyScene.m_SlidingTriangle);
					Wm5::Plane3d SlidingPlane(SlidingTriangle.V[0], SlidingTriangle.V[1], SlidingTriangle.V[2]);
					Wm5::Triangle3d SelectedTriangle = MyScene.GetSelectedObject().GetTriangle(MyScene.m_SelectedTriangle);
					Wm5::Plane3d SelectedPlane(SelectedTriangle.V[0], SelectedTriangle.V[1], SelectedTriangle.V[2]);

					if (SnapDirection.Dot(MyScene.GetObject(MyScene.m_SlidingObject).GetTriangleNormal(MyScene.m_SlidingTriangle)) >= 0) {
						printf("Warning (Geometry distance correction): Sliding Triangle facing wrong way!\n");
						break;
					}
					if (SnapDirection.Dot(MyScene.GetSelectedObject().GetTriangleNormal(MyScene.m_SelectedTriangle)) <= 0) {
						printf("Warning (Geometry distance correction): Selected Triangle facing wrong way!\n");
						break;
					}

					Wm5::Vector3d SlidingIntersection, SelectedIntersection;
					{
						Wm5::IntrLine3Plane3d Intersection(Line, SlidingPlane);
						if (Intersection.Find())
							SlidingIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
						else {
							printf("Warning (Geometry distance correction): No sliding intersection!\n");
							break;
						}
					}
					{
						Wm5::IntrLine3Plane3d Intersection(Line, SelectedPlane);
						if (Intersection.Find())
							SelectedIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
						else {
							printf("Warning (Geometry distance correction): No selected intersection!\n");
							break;
						}
					}

					//printf("Geometry distance correction: sli %d/%d | sel %d/%d\n", MyScene.m_SlidingObject, MyScene.m_SlidingTriangle, MyScene.GetSelectedObjectId(), MyScene.m_SelectedTriangle);
					MoveVector = SlidingIntersection - SelectedIntersection;
				} while (false);
#endif
			}
		}

		delete[] pPixels; delete[] pDepths; delete[] pSPixels; delete[] pSDepths;
	}

	//SetOpenGLProjectionMatrix(Perspective);
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();		// Restore Projection Matrix
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();		// Restore Modelview Matrix
	}

	// Return to normal framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glDrawBuffer(GL_BACK);
	glViewport(0, 0, nViewportWidth, nViewportHeight);

// Distance buffer is disabled in Ortho algorithm; we are letting the object collide faster on purpose so that geometry snap will fix it up
#if 0
	{
		double BufferDistance = m_kBufferDistance;
		if (0 != MyScene.m_SlidingObject) {
			Wm5::Vector3d SlidingNormal = MyScene.GetSlidingPlane(MoveVector).Normal;		// For GetSlidingPlane() to work properly, objects have to be in proper place
//SlidingNormal = MyScene.GetSlidingPlane().Normal;
			/// Target = m_kBufferDistance / cos(Theta)
			double FAbsCosTheta = Wm5::Mathd::FAbs(SnapDirection.Dot(SlidingNormal));
			if (FAbsCosTheta < 0.000001)
				FAbsCosTheta = 1;
			BufferDistance = m_kBufferDistance / FAbsCosTheta;
			//printf("%f\n", SlidingNormal.Length());
		}

		MoveVector += ZoomSelectedModel(-BufferDistance, SnapDirection);

		//MoveSelectedObject(MoveVector);
	}
#elif 0
	// Push it a little further back to force a collision, which will cause geometry to resolve
	MoveVector += ZoomSelectedModel(m_kBufferDistance, SnapDirection);
#endif

	//printf("SelObj.Z() = %.20f (%f, %f)\n", (MyScene.GetSelectedObject().GetPosition() + MoveVector).Z(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).X(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).Y());

	//return MoveVector;			// Because I'm actually moving the object within this function...
	//return Wm5::Vector3d::ZERO;		// TODO: Clean this up
	return kOriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition();		// TODO: Doing this temporary for depth-pop sounds to work
}

Wm5::Vector3d InputTestMode::SnapGeometry(SnapMode SnapMode)
{
	Wm5::Vector3d SnapDirection;
	if (Back == SnapMode)
		SnapDirection = -m_CursorRayDirection;
	else if (Front == SnapMode)
		SnapDirection = m_CursorRayDirection;

	return SnapGeometry(SnapDirection);
}
Wm5::Vector3d InputTestMode::SnapGeometry(Wm5::Vector3d SnapDirection)
{
//printf("SnapGeometry(): ");

	SnapDirection.Normalize();

	Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);

	// Geometry resolution
	{
		MyScene.m_SlidingObject = 0;		// Reset m_SlidingObject

		double MaxResolutionDistance = 0;
		for (auto it0 = m_IntersectingPairs.begin(); it0 != m_IntersectingPairs.end(); ++it0)
		{
			Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetObject(it0->first.first).GetTriangle(it0->first.second);		// Selected Object, to be moved
			Wm5::Triangle3d CollidingObjectTriangle = MyScene.GetObject(it0->second.first).GetTriangle(it0->second.second);		// Colliding Object, static

			double ResolutionDistance = ResolveTriTriCollisionAlongVector(SelectedObjectTriangle, CollidingObjectTriangle, -SnapDirection);

			// Find the maximum resolution distance, because we need to make sure all triangle collisions are resolved
			if (ResolutionDistance > MaxResolutionDistance) {
				MaxResolutionDistance = ResolutionDistance;
				MyScene.m_SlidingObject = it0->second.first;
				MyScene.m_SlidingTriangle = it0->second.second;
				MyScene.m_SelectedTriangle = it0->first.second;
			}
		}

		MoveVector = -SnapDirection * MaxResolutionDistance;
	}

//printf("preObj.Z() = %.20f (%f, %f)\nSnapGeome(Front): ", (MyScene.GetSelectedObject().GetPosition() + MoveVector).Z(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).X(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).Y());
	{
		double BufferDistance = m_kBufferDistance;
		if (0 != MyScene.m_SlidingObject) {
			Wm5::Vector3d SlidingNormal = MyScene.GetSlidingPlane(MoveVector).Normal;		// For GetSlidingPlane() to work properly, objects have to be in proper place
//SlidingNormal = MyScene.GetSlidingPlane().Normal;
			/// Target = m_kBufferDistance / cos(Theta)
			double FAbsCosTheta = Wm5::Mathd::FAbs(SnapDirection.Dot(SlidingNormal));
			if (FAbsCosTheta < 0.000001)
				FAbsCosTheta = 1;
			BufferDistance = m_kBufferDistance / FAbsCosTheta;
			//printf("%f\n", SlidingNormal.Length());

m_SlidingConstraint.Normal = SlidingNormal;		// DEBUG: Set Sliding Constraint... Find a proper place and way to do this
		}

		MoveVector += ZoomSelectedModel(-BufferDistance, SnapDirection);
	}

	//printf("SelObj.Z() = %.20f (%f, %f)\n", (MyScene.GetSelectedObject().GetPosition() + MoveVector).Z(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).X(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).Y());

//if (0 != MyScene.m_SlidingObject) printf("Acquired new Sliding Constraint (N = %f, %f, %f)\n", m_SlidingConstraint.Normal.X(), m_SlidingConstraint.Normal.Y(), m_SlidingConstraint.Normal.Z());

	return MoveVector;
}

Wm5::Vector3d InputTestMode::SnapFullGeometryTEST()
{
	Wm5::Vector3d SnapDirection = -m_CursorRayDirection;

	SnapDirection.Normalize();

	Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);

	// Geometry resolution
	{
		MyScene.m_SlidingObject = 0;		// Reset m_SlidingObject

		double MaxResolutionDistance = 0;
		for (uint16 SelectedObjectTriangleId = 0; SelectedObjectTriangleId < MyScene.GetSelectedObject().GetTriangleCount(); ++SelectedObjectTriangleId)
		{
			Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetSelectedObject().GetTriangle(SelectedObjectTriangleId);		// Selected Object, to be moved

			for (uint16 OtherObjectId = 1; OtherObjectId < MyScene.m_Objects.size(); ++OtherObjectId)
			{
				if (MyScene.GetSelectedObjectId() == OtherObjectId) continue;

				for (uint16 OtherObjectTriangleId = 0; OtherObjectTriangleId < MyScene.GetObject(OtherObjectId).GetTriangleCount(); ++OtherObjectTriangleId)
				{
					Wm5::Triangle3d OtherObjectTriangle = MyScene.GetObject(OtherObjectId).GetTriangle(OtherObjectTriangleId);		// Other Object, static

					double ResolutionDistance = ResolveTriTriCollisionAlongVector(SelectedObjectTriangle, OtherObjectTriangle, -SnapDirection);

					// Find the maximum resolution distance, because we need to make sure all triangle collisions are resolved
					if (ResolutionDistance > MaxResolutionDistance) {
						MaxResolutionDistance = ResolutionDistance;
						MyScene.m_SlidingObject = OtherObjectId;
						MyScene.m_SlidingTriangle = OtherObjectTriangleId;
						MyScene.m_SelectedTriangle = SelectedObjectTriangleId;
					}
				}
			}
		}

		MoveVector = -SnapDirection * MaxResolutionDistance;
	}

//printf("preObj.Z() = %.20f (%f, %f)\nSnapGeome(Front): ", (MyScene.GetSelectedObject().GetPosition() + MoveVector).Z(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).X(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).Y());
	{
		double BufferDistance = m_kBufferDistance;
		if (0 != MyScene.m_SlidingObject) {
			Wm5::Vector3d SlidingNormal = MyScene.GetSlidingPlane(MoveVector).Normal;		// For GetSlidingPlane() to work properly, objects have to be in proper place
//SlidingNormal = MyScene.GetSlidingPlane().Normal;
			/// Target = m_kBufferDistance / cos(Theta)
			double FAbsCosTheta = Wm5::Mathd::FAbs(SnapDirection.Dot(SlidingNormal));
			if (FAbsCosTheta < 0.000001)
				FAbsCosTheta = 1;
			BufferDistance = m_kBufferDistance / FAbsCosTheta;
			//printf("%f\n", SlidingNormal.Length());

m_SlidingConstraint.Normal = SlidingNormal;		// DEBUG: Set Sliding Constraint... Find a proper place and way to do this
		}

		MoveVector += ZoomSelectedModel(-BufferDistance, SnapDirection);
	}

	//printf("SelObj.Z() = %.20f (%f, %f)\n", (MyScene.GetSelectedObject().GetPosition() + MoveVector).Z(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).X(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).Y());

//if (0 != MyScene.m_SlidingObject) printf("Acquired new Sliding Constraint (N = %f, %f, %f)\n", m_SlidingConstraint.Normal.X(), m_SlidingConstraint.Normal.Y(), m_SlidingConstraint.Normal.Z());

	return MoveVector;
}
