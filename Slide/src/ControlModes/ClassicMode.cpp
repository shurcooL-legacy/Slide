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

#include "../Wm5/Wm5Math.h"
#include "../Wm5/Wm5Vector3.h"
#include "../Wm5/Wm5Matrix4.h"
#include "../Wm5/Wm5Quaternion.h"
#include "../Wm5/Wm5IntrTriangle3Triangle3.h"
#include "../Wm5/Wm5IntrLine3Plane3.h"

#include <Opcode.h>

#include "../Arcball/trackball_bell.h"

#include "../Models/ModelLoader.h"
#include "../Models/ColladaModel.h"
#include "../Models/SupermarketModel.h"
#include "../Models/SketchUpModel.h"
#include "../Scene.h"
#include "../SceneObject.h"

#include "ControlMode.h"
#include "ClassicMode.h"

Wm5::Vector3d ZoomSelectedModel(double dDistance);
Wm5::Vector3d ZoomSelectedModel(double dDistance, Wm5::Vector3d oDirection);

ClassicMode::ClassicMode()
	: nDesktopCursorX(0),
	  nDesktopCursorY(0),
	  bCursorArmed(false),
	  bCursorJustHidden(false),
	  nMouseButtonsDown(0),
	  bSelectPerformed(false),
	  bModelMovedByUser(false),
	  bTESTMode0Performed(false),
	  bTESTMode1Performed(false),
	  nTESTView(0),
	  nTESTMode2(0)
{
}

ClassicMode::~ClassicMode()
{
}

void ClassicMode::SlideResolution()
{
	if (0 != MyScene.GetSelectedObjectId() && bModelMovedByUser)
	{
		bool bCollisionExists = CheckForCollision();

		// Snap back
		if (!bCollisionExists)
		{
			SnapPersp(Back);
			bCollisionExists = CheckForCollision();
		}

		// Snap front
		if (bCollisionExists)
		{
			int nIterations = 3;

			do SnapPersp(Front);
			while ((bCollisionExists = CheckForCollision()) && nIterations-- > 0);
		}

		if (!bCollisionExists)
			bModelMovedByUser = false;
	}
}

void ClassicMode::ProcessMouseButton(int MouseButton, int Action)
{//printf("  mb\n");
	static int nPrevMouseButtonsDown = 0;

	if (GLFW_PRESS == Action)
	{
		nMouseButtonsDown |= (1 << MouseButton);

		if (!nPrevMouseButtonsDown && nMouseButtonsDown)
		{
			glfwGetMousePos(&nDesktopCursorX, &nDesktopCursorY);
			bCursorArmed = true;
		}
	}
	else
	{
		nMouseButtonsDown &= ~(1 << MouseButton);

		if (nPrevMouseButtonsDown && !nMouseButtonsDown)
		{
			if (bCursorArmed)
			{
				bCursorArmed = false;
				bSelectPerformed = true;
			}
			else
			{
				glfwEnable(GLFW_MOUSE_CURSOR);// printf("mouse visible\n");
				glfwSetMousePos(nDesktopCursorX, nDesktopCursorY);
			}
		}
	}

	nPrevMouseButtonsDown = nMouseButtonsDown;
}

void ClassicMode::ProcessMousePos(int MousePosX, int MousePosY)
{//if (nMouseButtonsDown == 0) printf("mm 0 - %d,%d\n", MousePosX, MousePosY); else printf("     mm 1! - %d,%d\n", MousePosX, MousePosY);
	static int nPrevMousePosX, nPrevMousePosY;

	if (bCursorArmed)
	{
		if (Wm5::Mathd::FAbs(MousePosX - nDesktopCursorX) >= 5 || Wm5::Mathd::FAbs(MousePosY - nDesktopCursorY) >= 5)
		{
			bCursorArmed = false;
			glfwDisable(GLFW_MOUSE_CURSOR); //printf("mouse hidden\n");
			nPrevMousePosX = -(MousePosX - nDesktopCursorX);
			nPrevMousePosY = -(MousePosY - nDesktopCursorY);
			bCursorJustHidden = true;
		}
		return;
	}

	if (bCursorJustHidden)
	{
		bCursorJustHidden = false;
		nPrevMousePosX += MousePosX;
		nPrevMousePosY += MousePosY;
	}

	if (nMouseButtonsDown)
	{
		int nMouseMovedX = MousePosX - nPrevMousePosX;
		int nMouseMovedY = MousePosY - nPrevMousePosY;
		nPrevMousePosX = MousePosX;
		nPrevMousePosY = MousePosY;

		bool bCtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
		bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));

		//printf("mouse moved by %d, %d (%d down)\n", nMouseMovedX, nMouseMovedY, nMouseButtonsDown);
		if (!bCtrlPressed && !bShiftPressed)
		{
			if (nMouseButtonsDown == 1) {
				camera.x += -0.012 * nMouseMovedY * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.y += -0.012 * nMouseMovedY * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.rh += 0.15 * nMouseMovedX;
			} else if (nMouseButtonsDown == 3) {
				camera.x += 0.012 * nMouseMovedX * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.y += -0.012 * nMouseMovedX * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.z += -0.012 * nMouseMovedY;
			} else if (nMouseButtonsDown == 2) {
				camera.rh += 0.15 * nMouseMovedX;
				camera.rv += -0.15 * nMouseMovedY;
			}
			while (camera.rh < 0) camera.rh += 360;
			while (camera.rh >= 360) camera.rh -= 360;
			if (camera.rv > 90) camera.rv = 90;
			if (camera.rv < -90) camera.rv = -90;
			//printf("Cam rot h = %f, v = %f\n", camera.rh, camera.rv);
		}
		else
		{
			if (0 != MyScene.GetSelectedObjectId()) {
				Wm5::Vector3d oDisplacement(Wm5::Vector3d::ZERO);

				/*if (1 == nMouseButtonsDown) {
					oDisplacement.Y() += 0.012 * nMouseMovedX;
				} else if (2 == nMouseButtonsDown) {
					oDisplacement.X() += 0.012 * nMouseMovedX;
				} else if (3 == nMouseButtonsDown) {
					oDisplacement.Z() += -0.012 * nMouseMovedY;
				}*/
				if (1 == nMouseButtonsDown) {
					double dScaleFactor = 0.010;
					oDisplacement.X() += dScaleFactor * nMouseMovedX * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += -dScaleFactor * nMouseMovedX * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.X() += dScaleFactor * nMouseMovedY * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += dScaleFactor * nMouseMovedY * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Z() += -dScaleFactor * nMouseMovedY * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);
				} else if (2 == nMouseButtonsDown) {
					oDisplacement.X() += 0.012 * nMouseMovedX * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += -0.012 * nMouseMovedX * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.X() += -0.012 * nMouseMovedY * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += -0.012 * nMouseMovedY * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				} else if (3 == nMouseButtonsDown) {
					oDisplacement.Z() += -0.012 * nMouseMovedY;
				}

				MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oDisplacement);
				if (bShiftPressed) {
					camera.x += oDisplacement.X();
					camera.y += oDisplacement.Y();
					camera.z += oDisplacement.Z();
				}

				if (1 == nMouseButtonsDown &&
					oDisplacement != Wm5::Vector3d::ZERO)
					bModelMovedByUser = true;
			}
		}
	}
}

void ClassicMode::ProcessMouseWheel(int MouseWheel)
{
	static int nPrevMouseWheel = MouseWheel;

	int nMouseWheelMoved = MouseWheel - nPrevMouseWheel;
	nPrevMouseWheel = MouseWheel;

	if (0 != MyScene.GetSelectedObjectId())
	{
		bool bShiftPressed = (glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS || glfwGetKey(GLFW_KEY_RSHIFT) == GLFW_PRESS);

		// Find the new mouse ray
		Wm5::Vector3d oMouseDirection;
		{
			Wm5::Vector3d oNearPoint, oFarPoint;

			float fDepth;
			int MousePosX, MousePosY; glfwGetMousePos(&MousePosX, &MousePosY); MousePosY = nViewportHeight - 1 - MousePosY;
			glReadPixels(MousePosX, MousePosY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &fDepth);
			//GLfloat winX, winY, winZ;
			GLdouble posX, posY, posZ;
			GLdouble modelMatrix[16], projMatrix[16];
			GLint viewport[4];
			glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
			glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
			glGetIntegerv(GL_VIEWPORT, viewport);
			fDepth = 0;
			gluUnProject(MousePosX, MousePosY, fDepth, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			oNearPoint.X() = posX;
			oNearPoint.Y() = posY;
			oNearPoint.Z() = posZ;
			fDepth = 1;
			gluUnProject(MousePosX, MousePosY, fDepth, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			oFarPoint.X() = posX;
			oFarPoint.Y() = posY;
			oFarPoint.Z() = posZ;
			oMouseDirection = oFarPoint - oNearPoint;
			oMouseDirection.Normalize();
		}
		//Wm5::Line3d oMouseRay(oCamera, oMouseDirection);

		Wm5::Vector3d oDisplacement;
		if (0 == nTESTMode2) oDisplacement = ZoomSelectedModel(nMouseWheelMoved);
		else				 oDisplacement = ZoomSelectedModel(nMouseWheelMoved, oMouseDirection);
		MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oDisplacement);
		if (bShiftPressed) {
			camera.x += oDisplacement.X();
			camera.y += oDisplacement.Y();
			camera.z += oDisplacement.Z();
		}
	}
}

void ClassicMode::ProcessKey(int Key, int Action)
{
	if (GLFW_PRESS == Action)
	{
		switch (Key) {
		case 'P':
			printf("--->>>>>>>>>>---\n");
			printf("std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();\n");
			for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin() + 1; it0 != MyScene.m_Objects.end(); ++it0) {
				printf("++it0; (*it0)->ModifyPosition().X() = %f; (*it0)->ModifyPosition().Y() = %f; (*it0)->ModifyPosition().Z() = %f;\n",
					(*it0)->GetPosition().X(), (*it0)->GetPosition().Y(), (*it0)->GetPosition().Z());
			}
			printf("\n");
			printf("camera.x = %f; camera.y = %f; camera.z = %f; camera.rh = %f; camera.rv = %f;\n", camera.x, camera.y, camera.z, camera.rh, camera.rv);
			printf("---<<<<<<<<<<---\n");
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '7':
		case '8':
			Scene::InitializeScene(Key - '1' + 1);
			break;
		case 'B':
			bTESTMode0Performed = true;
			break;
		case 'F':
			bTESTMode1Performed = true;
			break;
		case 'V':
			nTESTView = 1 - nTESTView;
			break;
		case 'Z':
			nTESTMode2 = 1 - nTESTMode2;
			if (0 == nTESTMode2) printf("Using Zoom(double)\n");
			else				 printf("Using Zoom(double, Vector3)\n");
			break;
		case 'T':
			break;
		default:
			break;
		}
	}
}

/*void SnapFrontPersp()
{
	glPushMatrix();

	// Set a smaller viewport with same aspect ratio
	uint32 nCustomViewportWidth = std::min<uint32>(400, nViewportWidth);
	//uint32 nCustomViewportWidth = std::min<uint32>(nViewportWidth, nViewportWidth);
	uint32 nCustomViewportHeight = nCustomViewportWidth * nViewportHeight / nViewportWidth;
	glViewport(0, 0, nCustomViewportWidth, nCustomViewportHeight);

	// Render only the colliding the static scene
	//MyScene.Render(Scene::StaticCollidingSceneGeometry);
	MyScene.Render(Scene::StaticSceneGeometry);

double tt = 0;
double t0 = glfwGetTime();
	uint8 * pPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pPixels));
	float * pDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths));
tt += glfwGetTime() - t0;

	// Render the selected model
	MyScene.Render(Scene::SelectedObjectBackFace);

t0 = glfwGetTime();
	uint8 * pSPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels));
	float * pSDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths));
tt += glfwGetTime() - t0;
//printf("SnapFrontPersp() read pixels in %f secs.\n", tt);

	GLfloat winX, winY;
	GLdouble posX, posY, posZ;
	GLdouble modelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	GLdouble projMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);

	double dMinBackDistanceSqr = 0;
	Wm5::Vector3d oMoveFrontVector(Wm5::Vector3d::ZERO);

	//Model::nSlidingModel = 0;
	uint16 SlidingModel = 0;
	uint16 SlidingTriangle = 0;
	uint16 SelectedTriangle = 0;
	uint32 ClosestPixel = 0;

t0 = glfwGetTime();
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
			winX = (GLfloat)(nPixel % nCustomViewportWidth);
			winY = (GLfloat)(nPixel / nCustomViewportWidth);
			gluUnProject(winX, winY, d, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			Wm5::Vector3d oD(posX, posY, posZ);
			gluUnProject(winX, winY, sd, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			Wm5::Vector3d oSD(posX, posY, posZ);
			//printf ("World coords at z= %f (Depth) are (%f, %f, %f)\n", winZ, posX, posY, posZ);

			double dDepthDiffSqr = Wm5::Mathd::Sign(d - sd) * (oD - oSD).SquaredLength();

			if (dDepthDiffSqr < dMinBackDistanceSqr)
			{
				dMinBackDistanceSqr = dDepthDiffSqr;
				oMoveFrontVector = (oD - oSD);

				//Model::nSlidingModel = r;
				//Model::nSlidingTriangle = static_cast<uint32>(g) << 8 | b;
				SlidingModel = static_cast<uint16>(g) << 8 | r;
				SlidingTriangle = static_cast<uint16>(a) << 8 | b;
				SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
				ClosestPixel = nPixel;
			}
		}
	}
//printf("SnapFrontPersp() processed pixels in %f secs.\n", glfwGetTime() - t0);
	delete[] pPixels;
	delete[] pDepths;
	delete[] pSPixels;
	delete[] pSDepths;
//printf("SlidingModel = %d\n SlidingTriangle = %d\n SelectedTriangle = %d\n ClosestPixel = %d\n", SlidingModel, SlidingTriangle, SelectedTriangle, ClosestPixel);

	// Geometry distance correction
	if (dMinBackDistanceSqr != 0)
	{
		winX = (GLfloat)(ClosestPixel % nCustomViewportWidth);
		winY = (GLfloat)(ClosestPixel / nCustomViewportWidth);
		gluUnProject(winX, winY, 0, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
		Wm5::Vector3d oNearPoint(posX, posY, posZ);
		gluUnProject(winX, winY, 1, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
		Wm5::Vector3d oFarPoint(posX, posY, posZ);
		Wm5::Vector3d Direction = oFarPoint - oNearPoint;
		Direction.Normalize();

		Wm5::Line3d Line(oNearPoint, Direction);

		float * pSlidingVertexData = &MyScene.m_Objects.at(SlidingModel - 1)->m_pVertexData[3 * 3 * SlidingTriangle];
		Wm5::Plane3d SlidingPlane(Wm5::Vector3d(pSlidingVertexData[0], pSlidingVertexData[1], pSlidingVertexData[2]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition(),
								  Wm5::Vector3d(pSlidingVertexData[3], pSlidingVertexData[4], pSlidingVertexData[5]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition(),
								  Wm5::Vector3d(pSlidingVertexData[6], pSlidingVertexData[7], pSlidingVertexData[8]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition());

		float * pSelectedVertexData = &MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->m_pVertexData[3 * 3 * SelectedTriangle];
		Wm5::Plane3d SelectedPlane(Wm5::Vector3d(pSelectedVertexData[0], pSelectedVertexData[1], pSelectedVertexData[2]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition(),
								   Wm5::Vector3d(pSelectedVertexData[3], pSelectedVertexData[4], pSelectedVertexData[5]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition(),
								   Wm5::Vector3d(pSelectedVertexData[6], pSelectedVertexData[7], pSelectedVertexData[8]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition());

		Wm5::Vector3d SlidingIntersection, SelectedIntersection;
		{
			Wm5::IntrLine3Plane3d Intersection(Line, SlidingPlane);
			if (Intersection.Find())
				SlidingIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
			else
				printf("Error: No sliding intersection!\n");
		}
		{
			Wm5::IntrLine3Plane3d Intersection(Line, SelectedPlane);
			if (Intersection.Find())
				SelectedIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
			else
				printf("Error: No selected intersection!\n");
		}

		oMoveFrontVector = SlidingIntersection - SelectedIntersection;
		oMoveFrontVector += ZoomSelectedModel(-0.001, oMoveFrontVector);
	}

	if (dMinBackDistanceSqr != 0)
		//MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oMoveFrontVector + ZoomSelectedModel(-0.01, oMoveBackVector));
		MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oMoveFrontVector);

	glViewport(0, 0, nViewportWidth, nViewportHeight);

	glPopMatrix();
}

void SnapBackPersp()
{
	glPushMatrix();

	// Set a smaller viewport with same aspect ratio
	uint32 nCustomViewportWidth = std::min<uint32>(400, nViewportWidth);
	//uint32 nCustomViewportWidth = std::min<uint32>(nViewportWidth, nViewportWidth);
	uint32 nCustomViewportHeight = nCustomViewportWidth * nViewportHeight / nViewportWidth;
	glViewport(0, 0, nCustomViewportWidth, nCustomViewportHeight);

	// Render the static scene
	/*if (0 != Model::nSlidingModel)
	{
		// Just the model we're sliding on, if any
		MyScene.m_Objects.at(Model::nSlidingModel - 1)->Render(2, false);
	}
	else* /
	{
		MyScene.Render(Scene::StaticSceneGeometry);
	}

double tt = 0;
double t0 = glfwGetTime();
	uint8 * pPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pPixels));
	float * pDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths));
tt += glfwGetTime() - t0;

	// Render the selected model
	MyScene.Render(Scene::SelectedObjectBackFace);

t0 = glfwGetTime();
	uint8 * pSPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels));
	float * pSDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths));
tt += glfwGetTime() - t0;
//printf("SnapBackPersp() read pixels in %f secs.\n", tt);

	GLfloat winX, winY;
	GLdouble posX, posY, posZ;
	GLdouble modelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	GLdouble projMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);

	float fMinPositiveDistanceSqr = -1;		// Positive distance means away from the camera (i.e. to snap back to background); valid range is (0, +oo)
	Wm5::Vector3d oMoveBackVector(Wm5::Vector3d::ZERO);

	//Model::nSlidingModel = 0;
	uint16 SlidingModel = 0;
	uint16 SlidingTriangle = 0;
	uint16 SelectedTriangle = 0;
	uint32 ClosestPixel = 0;

t0 = glfwGetTime();
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
			winX = (GLfloat)(nPixel % nCustomViewportWidth);
			winY = (GLfloat)(nPixel / nCustomViewportWidth);
			gluUnProject(winX, winY, d, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			Wm5::Vector3d oD(posX, posY, posZ);
			gluUnProject(winX, winY, sd, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			Wm5::Vector3d oSD(posX, posY, posZ);
			//printf ("World coords at z= %f (Depth) are (%f, %f, %f)\n", winZ, posX, posY, posZ);

			double dDepthDiffSqr = Wm5::Mathd::Sign(d - sd) * (oD - oSD).SquaredLength();

			if (dDepthDiffSqr >= 0.0 && (dDepthDiffSqr < fMinPositiveDistanceSqr || fMinPositiveDistanceSqr == -1))
			{
				fMinPositiveDistanceSqr = static_cast<float>(dDepthDiffSqr);
				oMoveBackVector = (oD - oSD);

				//Model::nSlidingModel = r;
				//Model::nSlidingTriangle = static_cast<uint32>(g) << 8 | b;
				SlidingModel = static_cast<uint16>(g) << 8 | r;
				SlidingTriangle = static_cast<uint16>(a) << 8 | b;
				SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
				ClosestPixel = nPixel;
			}
		}
	}
//printf("SnapBackPersp() processed pixels in %f secs.\n", glfwGetTime() - t0);
	delete[] pPixels;
	delete[] pDepths;
	delete[] pSPixels;
	delete[] pSDepths;
//printf("SlidingModel = %d\n SlidingTriangle = %d\n SelectedTriangle = %d\n ClosestPixel = %d\n", SlidingModel, SlidingTriangle, SelectedTriangle, ClosestPixel);

	// Geometry distance correction
	if (fMinPositiveDistanceSqr != -1)
	{
		winX = (GLfloat)(ClosestPixel % nCustomViewportWidth);
		winY = (GLfloat)(ClosestPixel / nCustomViewportWidth);
		gluUnProject(winX, winY, 0, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
		Wm5::Vector3d oNearPoint(posX, posY, posZ);
		gluUnProject(winX, winY, 1, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
		Wm5::Vector3d oFarPoint(posX, posY, posZ);
		Wm5::Vector3d Direction = oFarPoint - oNearPoint;
		Direction.Normalize();

		Wm5::Line3d Line(oNearPoint, Direction);

		float * pSlidingVertexData = &MyScene.m_Objects.at(SlidingModel - 1)->m_pVertexData[3 * 3 * SlidingTriangle];
		Wm5::Plane3d SlidingPlane(Wm5::Vector3d(pSlidingVertexData[0], pSlidingVertexData[1], pSlidingVertexData[2]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition(),
								  Wm5::Vector3d(pSlidingVertexData[3], pSlidingVertexData[4], pSlidingVertexData[5]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition(),
								  Wm5::Vector3d(pSlidingVertexData[6], pSlidingVertexData[7], pSlidingVertexData[8]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition());

		float * pSelectedVertexData = &MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->m_pVertexData[3 * 3 * SelectedTriangle];
		Wm5::Plane3d SelectedPlane(Wm5::Vector3d(pSelectedVertexData[0], pSelectedVertexData[1], pSelectedVertexData[2]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition(),
								   Wm5::Vector3d(pSelectedVertexData[3], pSelectedVertexData[4], pSelectedVertexData[5]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition(),
								   Wm5::Vector3d(pSelectedVertexData[6], pSelectedVertexData[7], pSelectedVertexData[8]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition());

		Wm5::Vector3d SlidingIntersection, SelectedIntersection;
		{
			Wm5::IntrLine3Plane3d Intersection(Line, SlidingPlane);
			if (Intersection.Find())
				SlidingIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
			else
				printf("Error: No sliding intersection!\n");
		}
		{
			Wm5::IntrLine3Plane3d Intersection(Line, SelectedPlane);
			if (Intersection.Find())
				SelectedIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
			else
				printf("Error: No selected intersection!\n");
		}

		oMoveBackVector = SlidingIntersection - SelectedIntersection;
		oMoveBackVector += ZoomSelectedModel(-0.001, oMoveBackVector);
	}

	if (fMinPositiveDistanceSqr != -1)
		//MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oMoveBackVector + ZoomSelectedModel(-0.01, oMoveBackVector));
		MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oMoveBackVector);

	glViewport(0, 0, nViewportWidth, nViewportHeight);

	glPopMatrix();
}*/
void ClassicMode::SnapPersp(SnapDirection SnapDirection)
{
	glPushMatrix();

	// Set a smaller viewport with same aspect ratio
	uint32 nCustomViewportWidth = std::min<uint32>(400, nViewportWidth);
	//uint32 nCustomViewportWidth = std::min<uint32>(nViewportWidth, nViewportWidth);
	uint32 nCustomViewportHeight = nCustomViewportWidth * nViewportHeight / nViewportWidth;
	glViewport(0, 0, nCustomViewportWidth, nCustomViewportHeight);

	if (Front == SnapDirection)
	{
		// Render only the colliding static scene
		MyScene.Render(Scene::StaticCollidingSceneGeometry);
		//MyScene.Render(Scene::StaticSceneGeometry);
	}
	else if (Back == SnapDirection)
	{
		// Render the static scene
		/*if (0 != Model::nSlidingModel)
		{
			// Just the model we're sliding on, if any
			MyScene.m_Objects.at(Model::nSlidingModel - 1)->Render(2, false);
		}
		else
		{
			MyScene.Render(Scene::StaticSceneGeometry);
		}*/
		MyScene.Render(Scene::StaticSceneGeometry);
	}

double tt = 0;
double t0 = glfwGetTime();
	uint8 * pPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pPixels));
	float * pDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths));
tt += glfwGetTime() - t0;

	// Render the selected model
	MyScene.Render(Scene::SelectedObjectBackFace);

t0 = glfwGetTime();
	uint8 * pSPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels));
	float * pSDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths));
tt += glfwGetTime() - t0;
//printf("SnapPersp() read pixels in %f secs.\n", tt);

	GLfloat winX, winY;
	GLdouble posX, posY, posZ;
	GLdouble modelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	GLdouble projMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);

	double dMinBackDistanceSqr = 0;
	float fMinPositiveDistanceSqr = -1;		// Positive distance means away from the camera (i.e. to snap back to background); valid range is (0, +oo)
	Wm5::Vector3d oMoveVector(Wm5::Vector3d::ZERO);

	//Model::nSlidingModel = 0;
	uint16 SlidingModel = 0;
	uint16 SlidingTriangle = 0;
	uint16 SelectedTriangle = 0;
	uint32 ClosestPixel = 0;

	bool IsObjectVisible = false;

t0 = glfwGetTime();
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
			winX = (GLfloat)(nPixel % nCustomViewportWidth);
			winY = (GLfloat)(nPixel / nCustomViewportWidth);
			gluUnProject(winX, winY, d, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			Wm5::Vector3d oD(posX, posY, posZ);
			gluUnProject(winX, winY, sd, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
			Wm5::Vector3d oSD(posX, posY, posZ);
			//printf ("World coords at z= %f (Depth) are (%f, %f, %f)\n", winZ, posX, posY, posZ);

			double dDepthDiffSqr = Wm5::Mathd::Sign(d - sd) * (oD - oSD).SquaredLength();

			if (Front == SnapDirection)
			{
				if (dDepthDiffSqr < dMinBackDistanceSqr)
				{
					dMinBackDistanceSqr = dDepthDiffSqr;
					oMoveVector = (oD - oSD);

					//Model::nSlidingModel = r;
					//Model::nSlidingTriangle = static_cast<uint32>(g) << 8 | b;
					SlidingModel = static_cast<uint16>(g) << 8 | r;
					SlidingTriangle = static_cast<uint16>(a) << 8 | b;
					SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
					ClosestPixel = nPixel;
				}
			}
			else if (Back == SnapDirection)
			{
				if (dDepthDiffSqr >= 0.0 && (dDepthDiffSqr < fMinPositiveDistanceSqr || fMinPositiveDistanceSqr == -1))
				{
					fMinPositiveDistanceSqr = static_cast<float>(dDepthDiffSqr);
					oMoveVector = (oD - oSD);

					//Model::nSlidingModel = r;
					//Model::nSlidingTriangle = static_cast<uint32>(g) << 8 | b;
					SlidingModel = static_cast<uint16>(g) << 8 | r;
					SlidingTriangle = static_cast<uint16>(a) << 8 | b;
					SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
					ClosestPixel = nPixel;
				}
			}

			if (!IsObjectVisible && dDepthDiffSqr >= 0) IsObjectVisible = true;
		}
		else if (!IsObjectVisible && (sr+sg) != 0 && (r+g) == 0)
		{
			IsObjectVisible = true;
		}
	}

	// TODO: Get rid of code duplication and double calculation
	// If object isn't visible, do a Front snap
	if (Back == SnapDirection && !IsObjectVisible)
	{
		SnapDirection = Front;

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
				winX = (GLfloat)(nPixel % nCustomViewportWidth);
				winY = (GLfloat)(nPixel / nCustomViewportWidth);
				gluUnProject(winX, winY, d, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
				Wm5::Vector3d oD(posX, posY, posZ);
				gluUnProject(winX, winY, sd, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
				Wm5::Vector3d oSD(posX, posY, posZ);
				//printf ("World coords at z= %f (Depth) are (%f, %f, %f)\n", winZ, posX, posY, posZ);

				double dDepthDiffSqr = Wm5::Mathd::Sign(d - sd) * (oD - oSD).SquaredLength();

				if (dDepthDiffSqr < dMinBackDistanceSqr)
				{
					dMinBackDistanceSqr = dDepthDiffSqr;
					oMoveVector = (oD - oSD);

					//Model::nSlidingModel = r;
					//Model::nSlidingTriangle = static_cast<uint32>(g) << 8 | b;
					SlidingModel = static_cast<uint16>(g) << 8 | r;
					SlidingTriangle = static_cast<uint16>(a) << 8 | b;
					SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
					ClosestPixel = nPixel;
				}
			}
		}
	}
//printf("SnapPersp() processed pixels in %f secs.\n", glfwGetTime() - t0);
	delete[] pPixels;
	delete[] pDepths;
	delete[] pSPixels;
	delete[] pSDepths;
//printf("SlidingModel = %d\n SlidingTriangle = %d\n SelectedTriangle = %d\n ClosestPixel = %d\n", SlidingModel, SlidingTriangle, SelectedTriangle, ClosestPixel);

	// Geometry distance correction
	if (dMinBackDistanceSqr != 0 || fMinPositiveDistanceSqr != -1)
	{
		winX = (GLfloat)(ClosestPixel % nCustomViewportWidth);
		winY = (GLfloat)(ClosestPixel / nCustomViewportWidth);
		gluUnProject(winX, winY, 0, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
		Wm5::Vector3d oNearPoint(posX, posY, posZ);
		gluUnProject(winX, winY, 1, modelMatrix, projMatrix, viewport, &posX, &posY, &posZ);
		Wm5::Vector3d oFarPoint(posX, posY, posZ);
		Wm5::Vector3d Direction = oFarPoint - oNearPoint;
		Direction.Normalize();

		Wm5::Line3d Line(oNearPoint, Direction);

		/*float * pSlidingVertexData = &MyScene.m_Objects.at(SlidingModel - 1)->m_pVertexData[3 * 3 * SlidingTriangle];
		Wm5::Plane3d SlidingPlane(Wm5::Vector3d(pSlidingVertexData[0], pSlidingVertexData[1], pSlidingVertexData[2]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition(),
								  Wm5::Vector3d(pSlidingVertexData[3], pSlidingVertexData[4], pSlidingVertexData[5]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition(),
								  Wm5::Vector3d(pSlidingVertexData[6], pSlidingVertexData[7], pSlidingVertexData[8]) + MyScene.m_Objects.at(SlidingModel - 1)->GetPosition());

		float * pSelectedVertexData = &MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->m_pVertexData[3 * 3 * SelectedTriangle];
		Wm5::Plane3d SelectedPlane(Wm5::Vector3d(pSelectedVertexData[0], pSelectedVertexData[1], pSelectedVertexData[2]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition(),
								   Wm5::Vector3d(pSelectedVertexData[3], pSelectedVertexData[4], pSelectedVertexData[5]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition(),
								   Wm5::Vector3d(pSelectedVertexData[6], pSelectedVertexData[7], pSelectedVertexData[8]) + MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->GetPosition());*/
		Wm5::Triangle3d SlidingTriangle3 = MyScene.GetObject(SlidingModel).GetTriangle(SlidingTriangle);
		Wm5::Plane3d SlidingPlane(SlidingTriangle3.V[0], SlidingTriangle3.V[1], SlidingTriangle3.V[2]);
		Wm5::Triangle3d SelectedTriangle3 = MyScene.GetSelectedObject().GetTriangle(SelectedTriangle);
		Wm5::Plane3d SelectedPlane(SelectedTriangle3.V[0], SelectedTriangle3.V[1], SelectedTriangle3.V[2]);

		Wm5::Vector3d SlidingIntersection, SelectedIntersection;
		{
			Wm5::IntrLine3Plane3d Intersection(Line, SlidingPlane);
			if (Intersection.Find())
				SlidingIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
			else
				printf("Error: No sliding intersection!\n");
		}
		{
			Wm5::IntrLine3Plane3d Intersection(Line, SelectedPlane);
			if (Intersection.Find())
				SelectedIntersection = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
			else
				printf("Error: No selected intersection!\n");
		}

		oMoveVector = SlidingIntersection - SelectedIntersection;
		oMoveVector += ZoomSelectedModel(-0.001, oMoveVector);
	}

	MyScene.m_Objects.at(MyScene.GetSelectedObjectId() - 1)->MoveBy(oMoveVector);

	glViewport(0, 0, nViewportWidth, nViewportHeight);

	glPopMatrix();
}
