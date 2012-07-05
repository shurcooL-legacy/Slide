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
#include "../Wm5/Wm5Vector4.h"
#include "../Wm5/Wm5Plane3.h"
#include "../Wm5/Wm5Matrix4.h"
#include "../Wm5/Wm5IntrTriangle3Triangle3.h"
#include "../Wm5/Wm5IntrLine3Plane3.h"
#include "../Wm5/Wm5Quaternion.h"

#include <Opcode.h>

#include "../Arcball/trackball_bell.h"

#include "../Models/ModelLoader.h"
#include "../Models/ColladaModel.h"
#include "../Models/SupermarketModel.h"
#include "../Models/SketchUpModel.h"
#include "../Scene.h"
#include "../SceneObject.h"

#include "ControlMode.h"
#include "UnderCursorMode.h"

extern Wm5::Vector3d DebugVector[10];

// STUDY
double StartTimeSTUDY = 0;
double EndTimeSTUDY = 0;
bool StartedSTUDY = true;

void PlaySound()
{
	static double lastPlayed = 0;
	double now = glfwGetTime();
	if (lastPlayed < now - 1) {
		printf("\a");
		//MessageBeep(0xFFFFFFFF);
		lastPlayed = now;
	}
}

Wm5::Vector3d ZoomSelectedModel(double dDistance);
Wm5::Vector3d ZoomSelectedModel(double dDistance, Wm5::Vector3d oDirection);

// Returns a vector that is the shadow of Vector on Plane
Wm5::Vector3d ProjectVectorOntoPlane(Wm5::Vector3d Vector, Wm5::Vector3d PlaneNormal)
{
	PlaneNormal.Normalize();
	Wm5::Vector3d PlaneSide = PlaneNormal.Cross(Vector); PlaneSide.Normalize();

	Wm5::Vector3d ProjectedVector = PlaneSide.Cross(PlaneNormal);
	ProjectedVector *= ProjectedVector.Dot(Vector);

	return ProjectedVector;
}

// Finds the closest point to P that lies on Plane
// TODO: Rewrite this in a more optimal way, once I know a faster direct way... It currently intersects a Line with Direction == Plane.Normal with the Plane
Wm5::Vector3d ProjectPointOntoPlane(Wm5::Vector3d Point, Wm5::Plane3d Plane)
{
	Wm5::Line3d Line(Point, Plane.Normal);
	Wm5::IntrLine3Plane3d Intersection(Line, Plane);
	if (Intersection.Find()) {
		Wm5::Vector3d ProjectedPoint = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
		return ProjectedPoint;
	} else {
		printf("Error in ProjectPointOntoPlane(): No intersection!\n");
		return Wm5::Vector3d::ZERO;
	}
}

// Uses OpenGL to project a 3d point onto screen space
Wm5::Vector3d ProjectPointFrom3dToScreen(Wm5::Vector3d Point)
{
	GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
	GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);
	Wm5::Vector3d ProjectedPoint;

	gluProject(Point.X(), Point.Y(), Point.Z(), ModelMatrix, ProjectionMatrix, Viewport, &ProjectedPoint.X(), &ProjectedPoint.Y(), &ProjectedPoint.Z());

	return ProjectedPoint;
}

UnderCursorMode::UnderCursorMode()
	: nDesktopCursorX(0),
	  nDesktopCursorY(0),
	  m_CursorX(0),
	  m_CursorY(0),
	  m_CursorRayDirection(),
	  m_DragPerformed(false),
	  m_UnderCursorPosition(),
	  m_OriginalSelectedObjectPosition(),
	  bCursorArmed(false),
	  CursorWasArmed(false),
	  bCursorJustHidden(false),
	  nMouseButtonsDown(0),
	  bSelectPerformed(false),
	  bModelMovedByUser(false),
	  ModelRotatedByUser(false),
	  bTESTMode0Performed(false),
	  bTESTMode1Performed(false),
	  nTESTView(0),
	  nTESTMode2(0),
	  m_SecondConstraint(Wm5::Vector3d::ZERO, 0),
	  m_ThirdConstraint(Wm5::Vector3d::ZERO, 0),
	  m_StickToPlaneModePossible(true)
{
	// Unit Test for ResolveTriTriCollisionAlongVector()
	{
		Wm5::Triangle3d Tri0(Wm5::Vector3d(-2, 0, 0), Wm5::Vector3d(2, 0, 0), Wm5::Vector3d(0, 5, 0));
		Wm5::Triangle3d Tri1(Wm5::Vector3d(0, 1, -1), Wm5::Vector3d(0, 1, 1), Wm5::Vector3d(0, -10, 0));
		Wm5::Vector3d Direction(0, 1, 0); Direction.Normalize();

		// Input: two triangles Tri0 and Tri1 (such that they are intersecting or touching), a unit vector Direction
		// Output: the minimum non-negative value k, such that after moving triangle Tri0 by k*Direction results in the two triangles no longer intersecting (within some small value)
		double k = UnderCursorMode::ResolveTriTriCollisionAlongVector(Tri0, Tri1, Direction);
		printf("ResolveTriTriCollisionAlongVector(Tri0, Tri1, Direction) Result: k = %f\n", k);		// Should be 1.0 for input above

		if (k != 1.0) {
			printf("Test failed: 1.0 != ResolveTriTriCollisionAlongVector()\n");
			exit(0);
		}
	}

	// Unit Test for ProjectVectorOntoPlane()
	{
		Wm5::Vector3d Vector(1, 0, 1);
		Wm5::Vector3d PlaneNormal(0, 0, 1);

		Wm5::Vector3d Result = ProjectVectorOntoPlane(Vector, PlaneNormal);
		printf("ProjectVectorOntoPlane(Vector, PlaneNormal) Result: (%f, %f, %f)\n", Result.X(), Result.Y(), Result.Z());
	}
}

UnderCursorMode::~UnderCursorMode()
{
}

bool UnderCursorMode::IsModeActive(InputMode InputMode)
{
	switch (InputMode)
	{
	case StickToPlane:
		//return (GLFW_PRESS == glfwGetKey('X') && m_StickToPlaneModePossible);
#ifdef SLIDE_TRANSLATIONAL_SNAPPING_ENABLED
		return m_StickToPlaneModePossible;
#else
		return false;
#endif
		break;
	default:
		return false;
		break;
	}
}

void UnderCursorMode::ResolveCollisionAlongPlane()
{
#define ResolveCollisionAlongPlane_METHOD 1
#if 1 == ResolveCollisionAlongPlane_METHOD
	if (0 == MyScene.m_SlidingObject) { printf("Exiting from UnderCursorMode::ResolveCollisionAlongPlane() because assert failed:\n 0 == MyScene.m_SlidingObject\n"); exit(2); }
	Wm5::Triangle3d SlidingTriangle = MyScene.GetObject(MyScene.m_SlidingObject).GetTriangle(MyScene.m_SlidingTriangle);
	Wm5::Plane3d SlidingPlane(SlidingTriangle.V[0], SlidingTriangle.V[1], SlidingTriangle.V[2]);

	double MinResolutionDistance = 1000000;
	Wm5::Vector3d ResolutionVector;

	for (auto it0 = m_IntersectingPairs.begin(); it0 != m_IntersectingPairs.end(); ++it0)
	{
		Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetObject(it0->first.first).GetTriangle(it0->first.second);		// Selected Object, to be moved
		Wm5::Triangle3d CollidingObjectTriangle = MyScene.GetObject(it0->second.first).GetTriangle(it0->second.second);		// Colliding Object, static

		Wm5::Vector3d CollidingObjectTriangleNormal = Wm5::Plane3d(CollidingObjectTriangle.V[0], CollidingObjectTriangle.V[1], CollidingObjectTriangle.V[2]).Normal;
		Wm5::Vector3d Direction = ProjectVectorOntoPlane(CollidingObjectTriangleNormal, SlidingPlane.Normal);

		double ResolutionDistance = ResolveTriTriCollisionAlongVector(SelectedObjectTriangle, CollidingObjectTriangle, Direction);

		ResolutionDistance += 0.001;

		// Find the minimum resolution distance, because we need to resolve in the direction that has least penetration
		if (ResolutionDistance < MinResolutionDistance && !CheckForCollisionTemporary(Direction * ResolutionDistance)) {
			MinResolutionDistance = ResolutionDistance;
			ResolutionVector = Direction * ResolutionDistance;
		}
	}

	printf("MinResolutionDistance %f\n", MinResolutionDistance);
	if (1000000 == MinResolutionDistance)
		ResolutionVector = Wm5::Vector3d::ZERO;
	else {
		Wm5::Vector3d ResolutionNormal = ResolutionVector; ResolutionNormal.Normalize();
		if (Wm5::Vector3d::ZERO == m_SecondConstraint.Normal)
			m_SecondConstraint = Wm5::Plane3d(ResolutionNormal, Wm5::Vector3d(ResolutionVector + m_UnderCursorPosition));
		else
			m_ThirdConstraint = Wm5::Plane3d(ResolutionNormal, Wm5::Vector3d(ResolutionVector + m_UnderCursorPosition));
		//printf("m_SecondConstraint Normal %f %f %f\n", m_SecondConstraint.Normal.X(), m_SecondConstraint.Normal.Y(), m_SecondConstraint.Normal.Z());
		//m_StickToPlaneModePossible = true;
	}

	m_UnderCursorPosition += ResolutionVector;
	MyScene.GetSelectedObject().ModifyPosition() += ResolutionVector;
#elif 2 == ResolveCollisionAlongPlane_METHOD
	if (0 == MyScene.m_SlidingObject) { printf("Exiting from UnderCursorMode::ResolveCollisionAlongPlane() because assert failed:\n 0 == MyScene.m_SlidingObject\n"); exit(2); }
	Wm5::Triangle3d SlidingTriangle = MyScene.GetObject(MyScene.m_SlidingObject).GetTriangle(MyScene.m_SlidingTriangle);
	Wm5::Plane3d SlidingPlane(SlidingTriangle.V[0], SlidingTriangle.V[1], SlidingTriangle.V[2]);

	int Iterations = 10;
	do {
		double MinResolutionDistance = 1000000;
		Wm5::Vector3d ResolutionVector;

		for (auto it0 = m_IntersectingPairs.begin(); it0 != m_IntersectingPairs.end(); ++it0)
		{
			Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetObject(it0->first.first).GetTriangle(it0->first.second);		// Selected Object, to be moved
			Wm5::Triangle3d CollidingObjectTriangle = MyScene.GetObject(it0->second.first).GetTriangle(it0->second.second);		// Colliding Object, static

			Wm5::Vector3d CollidingObjectTriangleNormal = Wm5::Plane3d(CollidingObjectTriangle.V[0], CollidingObjectTriangle.V[1], CollidingObjectTriangle.V[2]).Normal;
			Wm5::Vector3d Direction = ProjectVectorOntoPlane(CollidingObjectTriangleNormal, SlidingPlane.Normal);
			if (Direction.Length() < 0.001) continue;

			double ResolutionDistance = ResolveTriTriCollisionAlongVector(SelectedObjectTriangle, CollidingObjectTriangle, Direction);
			if (0 == ResolutionDistance) ResolutionDistance = 1000000;

			ResolutionDistance += 0.001;

			// Find the minimum resolution distance, because we need to resolve in the direction that has least penetration
			if (ResolutionDistance < MinResolutionDistance) {
				MinResolutionDistance = ResolutionDistance;
				ResolutionVector = Direction * ResolutionDistance;
			}
		}

		printf("MinResolutionDistance %f\n", MinResolutionDistance);
		if (1000000 == MinResolutionDistance)
			ResolutionVector = Wm5::Vector3d::ZERO;

		m_UnderCursorPosition += ResolutionVector;
		MyScene.GetSelectedObject().ModifyPosition() += ResolutionVector;
	}
	while (CheckForCollision() && Iterations-- > 0);
#else
	m_UnderCursorPosition += m_OriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition();
	MyScene.GetSelectedObject().ModifyPosition() = m_OriginalSelectedObjectPosition;
#endif
}

void UnderCursorMode::SlideResolution()
{
	if (0 != MyScene.GetSelectedObjectId())
	{
		if (bModelMovedByUser)
		{
			bool CollisionExists = CheckForCollision();

#if 1
			if (IsModeActive(StickToPlane) && 0 != MyScene.m_SlidingObject)
			{
				if (CollisionExists)
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
				}
			}
			else
			{
				// Snap back
				if (!CollisionExists)
				{
					SnapOrtho(Back);
					CollisionExists = CheckForCollision();
				}

				// Snap front
				{
					// Multiple iterations are needed because we resolve only colliding part of selected object, not the entire thing
					// TODO: Eventually, may need to do this in a better way for all cases (i.e. use framebuffer for rough Snap Front estimate). Imagine a highly-tesselated sphere, 5 iterations will not be nearly enough.
					int Iterations = 5;

					while (CollisionExists && Iterations-- > 0) {
						SnapOrtho(Front);
						CollisionExists = CheckForCollision();
					}
				}

				m_StickToPlaneModePossible = true;
			}

			//if (!CollisionExists)
#endif
				bModelMovedByUser = false;
		}
		else if (ModelRotatedByUser)
		{
#if 1
			//if (0 != MyScene.m_SlidingObject)
			//if (0 != MyScene.GetSelectedObject().m_ContainedByObject)
			{
				//Wm5::Vector3d Normal(MyScene.GetObject(MyScene.GetSelectedObject().m_ContainedByObject).GetTriangleNormal(MyScene.m_SlidingTriangle));
				//Wm5::Vector3d Normal(MyScene.GetSlidingPlane());
				Wm5::Vector3d Normal(0, 0, 1);
				Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(Normal));

				Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);
				Wm5::Vector3d SnapDirection(MyScene.GetSelectedObject().GetPosition() - SnapOrigin);

				bool CollisionExists = CheckForCollision();

				// Snap back
				if (!CollisionExists)
				{
					printf("SnapOrtho(Back, SnapOrigin, SnapDirection);\n");
					SnapOrtho(Back, SnapOrigin, SnapDirection);
					CollisionExists = CheckForCollision();
				}

				// Snap front
				if (CollisionExists)
				{
					// Multiple iterations are needed because we resolve only colliding part of selected object, not the entire thing
					int Iterations = 5;

					while (CollisionExists && Iterations-- > 0) {
						printf("SnapOrtho(Front, SnapOrigin, SnapDirection);\n");
						SnapOrtho(Front, SnapOrigin, SnapDirection);
						CollisionExists = CheckForCollision();
					}
				}
			}
#endif

			ModelRotatedByUser = false;
		}
	}
}

void UnderCursorMode::PickModel()
{
//if (MyScene.GetSelectedObjectId()) MyScene.GetSelectedObject().m_oIntersectingTris.clear();

	unsigned char cPixel[4]; glReadPixels(m_CursorX, m_CursorY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, cPixel);
	uint16 NewSelectedObject = cPixel[1] << 8 | cPixel[0];

	if (1 == NewSelectedObject) NewSelectedObject = 0;

	// If we selected a different object, clear the sliding object
	if (NewSelectedObject != MyScene.GetSelectedObjectId() && 0 != NewSelectedObject) {
		MyScene.m_SlidingObject = MyScene.GetObject(NewSelectedObject).m_ContainedByObject;
		if (0 != MyScene.m_SlidingObject) exit(0);
		MyScene.m_SlidingTriangle = 0;		// DEBUG: This needs to be either set to correct value if 0 != SlidingObject, or else ignored
		MyScene.m_SelectedTriangle = 0;		// DEBUG: This needs to be either set to correct value if 0 != SlidingObject, or else ignored
	}

	MyScene.SetSelectedObjectId(NewSelectedObject);

	uint32 Triangle = cPixel[3] << 8 | cPixel[2];
//if (MyScene.GetSelectedObjectId()) MyScene.GetSelectedObject().m_oIntersectingTris.insert(Triangle);

	if (MyScene.GetSelectedObjectId()) {
		m_OriginalSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();

		printf("  selected model #%d at %d, %d, triangle = %d.\n", MyScene.GetSelectedObjectId(), m_CursorX, m_CursorY, Triangle);

		bModelMovedByUser = true;
	}
}

void UnderCursorMode::ComputeUnderCursorPosition()
{
	GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
	GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);

	float Depth; glReadPixels(m_CursorX, m_CursorY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(&Depth));
	gluUnProject(m_CursorX, m_CursorY, Depth, ModelMatrix, ProjectionMatrix, Viewport, &m_UnderCursorPosition.X(), &m_UnderCursorPosition.Y(), &m_UnderCursorPosition.Z());
}

// Calculate normalized Arcball coordinates
Wm5::Tuple<2, double> UnderCursorMode::CalculateArcballCoordinates()
{
	Wm5::Tuple<2, double> ArcballCoords;
	if (0 != MyScene.GetSelectedObjectId())
	{
		GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
		GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
		GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);
		Wm5::Vector3d ObjectCenter, ObjectCorner;

		//Wm5::Vector3d ObjectPosition(MyScene.GetSelectedObject().GetPosition());
		Wm5::Vector3d ObjectPosition(m_OriginalSelectedObjectPosition);
		gluProject(ObjectPosition.X(), ObjectPosition.Y(), ObjectPosition.Z(), ModelMatrix, ProjectionMatrix, Viewport, &ObjectCenter.X(), &ObjectCenter.Y(), &ObjectCenter.Z());

		Wm5::Vector3d Horizontal(Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD), -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD), 0);
		Horizontal *= MyScene.GetSelectedObject().GetTB().radius();
		gluProject(ObjectPosition.X() + Horizontal.X(), ObjectPosition.Y() + Horizontal.Y(), ObjectPosition.Z() + Horizontal.Z(), ModelMatrix, ProjectionMatrix, Viewport, &ObjectCorner.X(), &ObjectCorner.Y(), &ObjectCorner.Z());

		double Radius = Wm5::Mathd::FAbs(ObjectCenter.X() - ObjectCorner.X());

		ArcballCoords[0] = (m_CursorX - ObjectCenter.X()) / Radius;
		ArcballCoords[1] = (m_CursorY - ObjectCenter.Y()) / Radius;
		//printf("Arcball X, Y = %f, %f\n", ArcballCoords[0], ArcballCoords[1]);
	}

	return ArcballCoords;
}

#undef PI
void QuaternionToEulerAngles(Wm5::Quaterniond q1, double & heading, double & attitude, double & bank)
{
	double test = q1.X()*q1.Y() + q1.Z()*q1.W();
	if (test > 0.499) { // singularity at north pole
		heading = 2 * atan2(q1.X(), q1.W());
		attitude = Wm5::Mathd::PI / 2;
		bank = 0;
		return;
	}
	if (test < -0.499) { // singularity at south pole
		heading = -2 * atan2(q1.X(), q1.W());
		attitude = - Wm5::Mathd::PI / 2;
		bank = 0;
		return;
	}
    double sqx = q1.X()*q1.X();
    double sqy = q1.Y()*q1.Y();
    double sqz = q1.Z()*q1.Z();

    heading = atan2(2*q1.Y()*q1.W()-2*q1.X()*q1.Z(), 1 - 2*sqy - 2*sqz);
	attitude = asin(2*test);
	bank = atan2(2*q1.X()*q1.W() - 2*q1.Y()*q1.Z(), 1 - 2*sqx - 2*sqz);
}

// Convert from Euler Angles
Wm5::Quaterniond QuaternionFromEulerAngles(double heading, double attitude, double bank)
{
	// Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll
	// and multiply those together.
	// the calculation below does the same, just shorter
 
	/*float p = pitch/* * Wm5::Mathd::DEG_TO_RAD* / / 2.0;
	float y = yaw/* * Wm5::Mathd::DEG_TO_RAD* / / 2.0;
	float r = roll/* * Wm5::Mathd::DEG_TO_RAD* / / 2.0;
 
	float sinp = sin(p);
	float siny = sin(y);
	float sinr = sin(r);
	float cosp = cos(p);
	float cosy = cos(y);
	float cosr = cos(r);
 
	Wm5::Quaterniond Quaternion(cosr * cosp * cosy + sinr * sinp * siny,
						sinr * cosp * cosy - cosr * sinp * siny,
						cosr * sinp * cosy + sinr * cosp * siny,
						cosr * cosp * siny - sinr * sinp * cosy);
	Quaternion.Normalize();
 
	return Quaternion;*/

	Wm5::Quaterniond RotateHeading = Wm5::Quaterniond(Wm5::Vector3d(0, 1, 0), heading);
	Wm5::Quaterniond RotateAttitude = Wm5::Quaterniond(Wm5::Vector3d(0, 0, 1), attitude);
	Wm5::Quaterniond RotateBank = Wm5::Quaterniond(Wm5::Vector3d(1, 0, 0), bank);

	return RotateHeading * RotateAttitude * RotateBank;
}

void UnderCursorMode::ProcessMouseButton(int MouseButton, int Action)
{//printf("  mb\n");
	static int nPrevMouseButtonsDown = 0;

	bool bCtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
	bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
	bool bAltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

	// Calculate normalized Arcball coordinates
	Wm5::Tuple<2, double> ArcballCoords = CalculateArcballCoordinates();

	if (GLFW_PRESS == Action)
	{
		// Start study timer
		if (!StartedSTUDY) {
			StartedSTUDY = true;
			StartTimeSTUDY = glfwGetTime();
			printf("| Study started at %f seconds.\n", StartTimeSTUDY);
		}

		nMouseButtonsDown |= (1 << MouseButton);

		if (!nPrevMouseButtonsDown && nMouseButtonsDown)
		{
			nDesktopCursorX = m_CursorX;
			nDesktopCursorY = m_CursorY;
			if (bCtrlPressed || (8 == nMouseButtonsDown || 16 == nMouseButtonsDown || (8+16) == nMouseButtonsDown)) bCursorArmed = true;

			if (GLFW_MOUSE_BUTTON_LEFT == MouseButton && !bCtrlPressed)
			{
				if (!bAltPressed && !bShiftPressed) bSelectPerformed = true;
				m_DragPerformed = true;
			}
			else if (GLFW_MOUSE_BUTTON_RIGHT == MouseButton && !bCtrlPressed)
			{
				m_DragPerformed = true;
			}

#ifdef SLIDE_ARCBALL_ROTATIONS
			if (0 != MyScene.GetSelectedObjectId() && 2 == nMouseButtonsDown && bAltPressed)
			{
				m_OriginalSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();

				ArcballCoords = CalculateArcballCoordinates();

				glPushMatrix();
					// Look at the selected model
					glLoadIdentity();
					if (camera.x != MyScene.GetSelectedObject().GetPosition().X() || camera.y != MyScene.GetSelectedObject().GetPosition().Y())
						gluLookAt(camera.x, camera.y, camera.z, MyScene.GetSelectedObject().GetPosition().X(),
																MyScene.GetSelectedObject().GetPosition().Y(),
																MyScene.GetSelectedObject().GetPosition().Z(), 0, 0, 1);
					else
						gluLookAt(camera.x, camera.y, camera.z, MyScene.GetSelectedObject().GetPosition().X(),
																MyScene.GetSelectedObject().GetPosition().Y(),
																MyScene.GetSelectedObject().GetPosition().Z(), 1, 0, 0);
					double globalModelviewMatrixForRot[16]; glGetDoublev(GL_MODELVIEW_MATRIX, globalModelviewMatrixForRot);
				glPopMatrix();
				Bell & TB = MyScene.GetSelectedObject().ModifyTB();
				TB.begin_drag(ArcballCoords[0], ArcballCoords[1], globalModelviewMatrixForRot);
			}
#endif // SLIDE_ARCBALL_ROTATIONS
		}
	}
	else /// if (GLFW_RELEASE == Action)
	{
		// Record the end study time
		if (StartedSTUDY) {
			EndTimeSTUDY = glfwGetTime();
			//printf("| Study potentially ended at %f seconds.\n", EndTimeSTUDY);
		}

		nMouseButtonsDown &= ~(1 << MouseButton);

		/*if (GLFW_MOUSE_BUTTON_LEFT == MouseButton) {
			m_SecondConstraint.Normal = Wm5::Vector3d::ZERO;
			m_ThirdConstraint.Normal = Wm5::Vector3d::ZERO;
			m_StickToPlaneModePossible = false;
		}*/

		if (2 == nPrevMouseButtonsDown)
		{
			// Snap to nearest rotation
			if (0 != MyScene.GetSelectedObjectId())
			{
#if 0
				Wm5::Quaterniond ClosestRotateBy;
				double ClosestAngle = 10;
				for (uint8 Direction = 0; Direction < 6; ++Direction) {
#define				SLIDE_TOTAL_ORIENTATIONS (4)
					for (uint8 Orientation = 0; Orientation < SLIDE_TOTAL_ORIENTATIONS; ++Orientation) {
						Wm5::Vector3d SnappedDirection;
						if (0 == Direction) { SnappedDirection.X() = 0; SnappedDirection.Y() = 1; SnappedDirection.Z() = 0; }			// Forward
						else if (1 == Direction) { SnappedDirection.X() = -1; SnappedDirection.Y() = 0; SnappedDirection.Z() = 0; }		// Left
						else if (2 == Direction) { SnappedDirection.X() = 1; SnappedDirection.Y() = 0; SnappedDirection.Z() = 0; }		// Right
						else if (3 == Direction) { SnappedDirection.X() = 0; SnappedDirection.Y() = -1; SnappedDirection.Z() = 0; }		// Back
						else if (4 == Direction) { SnappedDirection.X() = 0; SnappedDirection.Y() = 0; SnappedDirection.Z() = 1; }		// Up
						else if (5 == Direction) { SnappedDirection.X() = 0; SnappedDirection.Y() = 0; SnappedDirection.Z() = -1; }		// Down

						//Wm5::Quaterniond RotateBy;
						//RotateBy.Align(Wm5::Vector3d(Reference.X(), Reference.Y(), Reference.Z()), SnappedDirection);

						// First orient quaternion in one of 6 directions, then rotate along that direction to get into all possible orientations
						Wm5::Quaterniond RotateBy; RotateBy.Align(Wm5::Vector3d(0, 1, 0), SnappedDirection);
						RotateBy = Wm5::Quaterniond(Wm5::Vector3d(0, 1, 0), (360/SLIDE_TOTAL_ORIENTATIONS) * Orientation * Wm5::Mathd::DEG_TO_RAD) * RotateBy;		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
						Wm5::Quaterniond RotateBy2 = MyScene.GetSelectedObject().ModifyRotation().Inverse() * RotateBy;

						Wm5::Vector3d Axis; double Angle;
						RotateBy2.ToAxisAngle(Axis, Angle);
#						undef PI
						Angle += 2 * Wm5::Mathd::PI; while (Angle > Wm5::Mathd::PI) Angle -= 2 * Wm5::Mathd::PI; if (Angle < 0) Angle *= -1;		// Get angle in [0, +180) range
						//printf("Direction %d got Angle %f\n", Direction, Angle * 180 / 3.141592);

						if (Angle < ClosestAngle) {
							ClosestAngle = Angle;
							ClosestRotateBy = RotateBy;
						}
					}
				}

				MyScene.GetSelectedObject().ModifyRotation() = ClosestRotateBy;// * MyScene.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
#else
				/*Wm5::Vector3d Axis;
				double Angle;
				MyScene.GetSelectedObject().ModifyRotation().ToAxisAngle(Axis, Angle);

				printf("Axis.Length() == %f\n", Axis.Length());
				//Wm5::Vector3d ViewDirection(Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD));
				double AxisVAngle = Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ASin(Axis.Z());
				double RAxisVAngle = std::floor(AxisVAngle / 90 + 0.5) * 90;
				Axis.Z() = Wm5::Mathd::Sin(RAxisVAngle * Wm5::Mathd::DEG_TO_RAD);
				Axis.Normalize();

				Wm5::Quaterniond SnappedRotation;
				SnappedRotation.FromAxisAngle(Axis, Angle);
				MyScene.GetSelectedObject().ModifyRotation() = SnappedRotation;*/

				/*{
					double cz, sz, cx, sx, cy, sy;
					MyScene.GetSelectedObject().ModifyRotation().FactorZXY(cz, sz, cx, sx, cy, sy);
					printf("z, x, y: %f %f %f", Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ACos(cz), Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ACos(cx), Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ACos(cy));
				}*/

#define SLIDE_ROTATE_INCREMENT (90.0/3.0)
				{
					double heading, attitude, bank;
					QuaternionToEulerAngles(MyScene.GetSelectedObject().ModifyRotation(), heading, attitude, bank);
					heading  = floor(heading * Wm5::Mathd::RAD_TO_DEG / SLIDE_ROTATE_INCREMENT + 0.5) * SLIDE_ROTATE_INCREMENT * Wm5::Mathd::DEG_TO_RAD;
					attitude  = floor(attitude * Wm5::Mathd::RAD_TO_DEG / SLIDE_ROTATE_INCREMENT + 0.5) * SLIDE_ROTATE_INCREMENT * Wm5::Mathd::DEG_TO_RAD;
					bank  = floor(bank * Wm5::Mathd::RAD_TO_DEG / SLIDE_ROTATE_INCREMENT + 0.5) * SLIDE_ROTATE_INCREMENT * Wm5::Mathd::DEG_TO_RAD;

					printf("heading, attitude, bank = %f %f %f\n", heading * Wm5::Mathd::RAD_TO_DEG, attitude * Wm5::Mathd::RAD_TO_DEG, bank * Wm5::Mathd::RAD_TO_DEG);

					MyScene.GetSelectedObject().ModifyRotation() = QuaternionFromEulerAngles(heading, attitude, bank);
					//MyScene.GetSelectedObject().ModifyRotation() = QuaternionFromEulerAngles(0 * Wm5::Mathd::DEG_TO_RAD, 0 * Wm5::Mathd::DEG_TO_RAD, 30 * Wm5::Mathd::DEG_TO_RAD);
				}
#endif
				MyScene.GetSelectedObject().ModifyRotation().Normalize();
				ModelRotatedByUser = true;
			}
		}

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

#ifdef SLIDE_ARCBALL_ROTATIONS
			if (0 != MyScene.GetSelectedObjectId() && 2 == nPrevMouseButtonsDown && bAltPressed)
			{
				Bell & TB = MyScene.GetSelectedObject().ModifyTB();
				TB.end_drag(ArcballCoords[0], ArcballCoords[1]);
			}
#endif // SLIDE_ARCBALL_ROTATIONS

			if (bCursorArmed)
			{
				bCursorArmed = false;
				//bSelectPerformed = true;
			}
			else if (CursorWasArmed)
			{
				CursorWasArmed = false;
				glfwEnable(GLFW_MOUSE_CURSOR);// printf("mouse visible\n");
				glfwSetMousePos(nDesktopCursorX, nViewportHeight - 1 - nDesktopCursorY);
			}
		}
	}

	nPrevMouseButtonsDown = nMouseButtonsDown;
}

void UnderCursorMode::ProcessMousePos(int MousePosX, int MousePosY)
{//if (nMouseButtonsDown == 0) printf("mm 0 - %d,%d\n", MousePosX, MousePosY); else printf("     mm 1! - %d,%d\n", MousePosX, MousePosY);
	static int nPrevMousePosX, nPrevMousePosY;

	MousePosY = nViewportHeight - 1 - MousePosY;

	bool bCtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
	bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
	bool bAltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

	if (bCursorArmed)
	{
		if (Wm5::Mathd::FAbs(MousePosX - nDesktopCursorX) >= 5 || Wm5::Mathd::FAbs(MousePosY - nDesktopCursorY) >= 5)
		{
			bCursorArmed = false;
			CursorWasArmed = true;
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

	// Calculate normalized Arcball coordinates
	Wm5::Tuple<2, double> ArcballCoords = CalculateArcballCoordinates();

	int nMouseMovedX = MousePosX - nPrevMousePosX;
	int nMouseMovedY = MousePosY - nPrevMousePosY;
	nPrevMousePosX = MousePosX;
	nPrevMousePosY = MousePosY;

	if (nMouseButtonsDown && !bSelectPerformed)
	{
		if (0 != MyScene.GetSelectedObjectId() && 2 == nMouseButtonsDown && bAltPressed)
		{
#ifdef SLIDE_ARCBALL_ROTATIONS
			Bell & TB = MyScene.GetSelectedObject().ModifyTB();
			TB.drag(ArcballCoords[0], ArcballCoords[1]);
			ModelRotatedByUser = true;
#else // SLIDE_ARCBALL_ROTATIONS
			glPushMatrix();
				// Look at the selected model
				glLoadIdentity();
				if (camera.x != MyScene.GetSelectedObject().GetPosition().X() || camera.y != MyScene.GetSelectedObject().GetPosition().Y())
					gluLookAt(camera.x, camera.y, camera.z, MyScene.GetSelectedObject().GetPosition().X(),
															MyScene.GetSelectedObject().GetPosition().Y(),
															MyScene.GetSelectedObject().GetPosition().Z(), 0, 0, 1);
				else
					gluLookAt(camera.x, camera.y, camera.z, MyScene.GetSelectedObject().GetPosition().X(),
															MyScene.GetSelectedObject().GetPosition().Y(),
															MyScene.GetSelectedObject().GetPosition().Z(), 1, 0, 0);
				double globalModelviewMatrixForRot[16]; glGetDoublev(GL_MODELVIEW_MATRIX, globalModelviewMatrixForRot);
			glPopMatrix();
			Bell & TB = MyScene.GetSelectedObject().ModifyTB();
			TB.begin_drag(0, 0, globalModelviewMatrixForRot);
			const double Multiplier = 0.005;
			TB.drag(Multiplier * nMouseMovedX, Multiplier * nMouseMovedY);
			TB.get_current_rotation();
			ModelRotatedByUser = true;
#endif
		}

		//printf("mouse moved by %d, %d (%d down)\n", nMouseMovedX, nMouseMovedY, nMouseButtonsDown);
		if (bCtrlPressed && !bShiftPressed)
		{
			if (nMouseButtonsDown == 1) {
				camera.x += 0.012 * nMouseMovedY * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.y += 0.012 * nMouseMovedY * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.rh += 0.15 * nMouseMovedX;
			} else if (nMouseButtonsDown == 3) {
				camera.x += 0.012 * nMouseMovedX * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.y += -0.012 * nMouseMovedX * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				camera.z += 0.012 * nMouseMovedY;
			} else if (nMouseButtonsDown == 2) {
				camera.rh += 0.15 * nMouseMovedX;
				camera.rv += 0.15 * nMouseMovedY;
			}
			while (camera.rh < 0) camera.rh += 360;
			while (camera.rh >= 360) camera.rh -= 360;
			if (camera.rv > 90) camera.rv = 90; if (camera.rv < -90) camera.rv = -90;
			//printf("Cam rot h = %f, v = %f\n", camera.rh, camera.rv);
		}
		else if (!bShiftPressed)
		{
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
					/*double dScaleFactor = 0.010;
					oDisplacement.X() += dScaleFactor * nMouseMovedX * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += -dScaleFactor * nMouseMovedX * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.X() += dScaleFactor * nMouseMovedY * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += dScaleFactor * nMouseMovedY * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Z() += -dScaleFactor * nMouseMovedY * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);*/

					// Check if we should unsnap
					{
						// Project the original point under mouse cursor to screen
						Wm5::Vector3d ProjectedPoint = ProjectPointFrom3dToScreen(m_UnderCursorPosition);
						Wm5::Vector2d ProjectedCursor(ProjectedPoint.X(), ProjectedPoint.Y());
						Wm5::Vector2d RealCursor(m_CursorX, m_CursorY);

						if ((RealCursor - ProjectedCursor).Length() > m_kUnsnapDistance) {
							//if (IsModeActive(StickToPlane)) PlaySound();
							m_SecondConstraint.Normal = Wm5::Vector3d::ZERO;
							m_ThirdConstraint.Normal = Wm5::Vector3d::ZERO;
							m_StickToPlaneModePossible = false;
						}
					}

					{
						Wm5::Vector3d Camera(camera.x, camera.y, camera.z);
						Wm5::Line3d Line(Camera, m_CursorRayDirection);

						Wm5::Vector3d Normal;
						if (0 == MyScene.m_SlidingObject)
						{
							Wm5::Vector3d Horizontal(Wm5::Vector3d::ZERO), Vertical(Wm5::Vector3d::ZERO);

							Horizontal.X() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
							Horizontal.Y() += -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
							Vertical.X() += Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
							Vertical.Y() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
							Vertical.Z() += -Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);

							Normal = Horizontal.Cross(Vertical);
						}
						else
						{
							Normal = MyScene.GetSlidingPlane().Normal;
							//printf("Used %d/%d as MyScene.m_SlidingObject/Triangle\n", MyScene.m_SlidingObject, MyScene.m_SlidingTriangle);
						}
						Wm5::Plane3d MovementPlane(Normal, m_UnderCursorPosition);

						Wm5::Vector3d IntersectionPoint;
						{
							Wm5::IntrLine3Plane3d Intersection(Line, MovementPlane);
							if (Intersection.Find())
								IntersectionPoint = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
							else
								printf("Error: No intersection!\n");
						}

#define SLIDE_USE_SECOND_CONSTRAINT 1
#if SLIDE_USE_SECOND_CONSTRAINT
						if (Wm5::Vector3d::ZERO != m_SecondConstraint.Normal)
						{
							IntersectionPoint = ProjectPointOntoPlane(IntersectionPoint, m_SecondConstraint);
						}
						
						if (Wm5::Vector3d::ZERO != m_ThirdConstraint.Normal)
						{
							IntersectionPoint = ProjectPointOntoPlane(IntersectionPoint, m_ThirdConstraint);
						}
#endif

						oDisplacement = IntersectionPoint - m_UnderCursorPosition;
						//m_UnderCursorPosition = Intersection;
					}
				}/* else if (2 == nMouseButtonsDown) {
					oDisplacement.X() += 0.012 * nMouseMovedX * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += -0.012 * nMouseMovedX * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.X() += 0.012 * nMouseMovedY * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					oDisplacement.Y() += 0.012 * nMouseMovedY * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
				} else if (3 == nMouseButtonsDown) {
					oDisplacement.Z() += 0.012 * nMouseMovedY;
				}*/
				else if (2 == nMouseButtonsDown)
				{
					Wm5::Vector3d ToObject = MyScene.GetSelectedObject().GetPosition() - Wm5::Vector3d(camera.x, camera.y, camera.z); ToObject.Normalize();

					Wm5::Vector3d Horizontal(Wm5::Vector3d::ZERO), Vertical(Wm5::Vector3d::ZERO);
					Horizontal.X() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					Horizontal.Y() += -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					Vertical.X() += Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					Vertical.Y() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					Vertical.Z() += -Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);

					Horizontal = Vertical.Cross(ToObject);
					Vertical = ToObject.Cross(Horizontal);

					{
						Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Vertical, -0.008 * nMouseMovedX);
						MyScene.GetSelectedObject().ModifyRotation() = RotateBy * MyScene.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
						MyScene.GetSelectedObject().ModifyRotation().Normalize();
					}
					{
						Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Horizontal, -0.008 * nMouseMovedY);
						MyScene.GetSelectedObject().ModifyRotation() = RotateBy * MyScene.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
						MyScene.GetSelectedObject().ModifyRotation().Normalize();
					}

					{
						Wm5::Quaterniond RotateBy;
						Wm5::Vector3d V1(1, 0, 0);
						Wm5::Vector3d V2(0, 1, 0); V2.Normalize();
						RotateBy.Align(V1, V2);
						Wm5::Vector3d Axis;
						double Angle;
						//RotateBy.ToAxisAngle(Axis, Angle);
						MyScene.GetSelectedObject().ModifyRotation().ToAxisAngle(Axis, Angle);
						printf("Angle %f\n", Angle * 180 / 3.141593653759743746);
					}

					ModelRotatedByUser = true;
				}

				// Move the Selected Object(s)
				//MyScene.GetSelectedObject().MoveBy(oDisplacement);
				std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
				for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
					if ((*it0)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
						(*it0)->MoveBy(oDisplacement);

				m_UnderCursorPosition += oDisplacement;

				if (1 == nMouseButtonsDown &&
					oDisplacement != Wm5::Vector3d::ZERO)
				{
					bModelMovedByUser = true;
					//printf("m_SecondConstraint.Normal.Length() == %f, %d == m_StickToPlaneModePossible\n", m_SecondConstraint.Normal.Length(), m_StickToPlaneModePossible);
				}
			}
		}
	}
}

void UnderCursorMode::ProcessMouseWheel(int MouseWheel)
{
	static int nPrevMouseWheel = MouseWheel;

	int nMouseWheelMoved = MouseWheel - nPrevMouseWheel;
	nPrevMouseWheel = MouseWheel;

	if (0 != MyScene.GetSelectedObjectId())
	{
		bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
		bool bAltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

		if (!bAltPressed && GLFW_PRESS == glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT))
		{
			Wm5::Vector3d oDisplacement;
			oDisplacement = ZoomSelectedModel(0.30 * nMouseWheelMoved, m_CursorRayDirection);
			//MyScene.GetSelectedObject().MoveBy(oDisplacement);
			std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
				for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
					if ((*it0)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
						(*it0)->MoveBy(oDisplacement);
			m_UnderCursorPosition += oDisplacement;
			if (bShiftPressed) {
				camera.x += oDisplacement.X();
				camera.y += oDisplacement.Y();
				camera.z += oDisplacement.Z();
			}
		}
		else
		{
			/*glPushMatrix();
				// Look at the selected model
				glLoadIdentity();
				if (camera.x != MyScene.GetSelectedObject().GetPosition().X() || camera.y != MyScene.GetSelectedObject().GetPosition().Y())
					gluLookAt(camera.x, camera.y, camera.z, MyScene.GetSelectedObject().GetPosition().X(),
															MyScene.GetSelectedObject().GetPosition().Y(),
															MyScene.GetSelectedObject().GetPosition().Z(), 0, 0, 1);
				else
					gluLookAt(camera.x, camera.y, camera.z, MyScene.GetSelectedObject().GetPosition().X(),
															MyScene.GetSelectedObject().GetPosition().Y(),
															MyScene.GetSelectedObject().GetPosition().Z(), 1, 0, 0);
				double globalModelviewMatrixForRot[16]; glGetDoublev(GL_MODELVIEW_MATRIX, globalModelviewMatrixForRot);
			glPopMatrix();
			Bell & TB = MyScene.GetSelectedObject().ModifyTB();
			TB.begin_drag(0, 10, globalModelviewMatrixForRot);
			TB.drag(-0.5 * nMouseWheelMoved, 10);
			TB.get_current_rotation();*/
			/*Wm5::Vector3d Horizontal(Wm5::Vector3d::ZERO), Vertical(Wm5::Vector3d::ZERO);
			Horizontal.X() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
			Horizontal.Y() += -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
			Vertical.X() += Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
			Vertical.Y() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
			Vertical.Z() += -Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);*/

			if (2 == nMouseButtonsDown)
			{
				//Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Horizontal.Cross(Vertical), -0.10 * nMouseWheelMoved);
				Wm5::Vector3d ToObject = MyScene.GetSelectedObject().GetPosition() - Wm5::Vector3d(camera.x, camera.y, camera.z); ToObject.Normalize();
				Wm5::Quaterniond RotateBy = Wm5::Quaterniond(ToObject, -0.10 * nMouseWheelMoved);
				MyScene.GetSelectedObject().ModifyRotation() = RotateBy * MyScene.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
				MyScene.GetSelectedObject().ModifyRotation().Normalize();

				ModelRotatedByUser = true;
			}
		}
	}
}

void UnderCursorMode::ProcessKey(int Key, int Action)
{
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
				if (MyScene.GetSelectedObjectId()) {
					m_UnderCursorPosition += m_OriginalSelectedObjectPosition - MyScene.GetSelectedObject().GetPosition();
					MyScene.GetSelectedObject().ModifyPosition() = m_OriginalSelectedObjectPosition;
				}
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
			printf("nTESTMode2 = %d\n", nTESTMode2);
			break;
		case 'X':
			//m_StickToPlaneModePossible = true;
			break;
		case GLFW_KEY_SPACE:
			/*m_SecondConstraint.Normal = Wm5::Vector3d::ZERO;
			m_StickToPlaneModePossible = false;*/

			// Record the end study time
			if (StartedSTUDY) {
				if (EndTimeSTUDY <= StartTimeSTUDY) EndTimeSTUDY = glfwGetTime();
				double TimeTakenSTUDY = EndTimeSTUDY - StartTimeSTUDY;
				printf("| Study Ended!\n", TimeTakenSTUDY);
				printf("| ------------\n", TimeTakenSTUDY);
				printf("| Study Time: %f seconds.\n", TimeTakenSTUDY);
			}
			else printf("Error: Study hasn't started yet!");

			// Result error calculation
			{
				const uint8 NumPieces = 7;
				uint32 NumPoints = 0;
				for (uint8 Piece = 2; Piece < 2 + NumPieces; ++Piece)
				{
					NumPoints += 3 * MyScene.GetObject(Piece).GetTriangleCount();
				}

				Wm5::Vector3d * Points = new Wm5::Vector3d[NumPoints];
				uint32 NextPoint = 0;

				for (uint8 Piece = 2; Piece < 2 + NumPieces; ++Piece)
				{
					for (uint16 TriangleNumber = 0; TriangleNumber < MyScene.GetObject(Piece).GetTriangleCount(); ++TriangleNumber)
					{
						Points[NextPoint++] = MyScene.GetObject(Piece).GetTriangle(TriangleNumber).V[0];
						Points[NextPoint++] = MyScene.GetObject(Piece).GetTriangle(TriangleNumber).V[1];
						Points[NextPoint++] = MyScene.GetObject(Piece).GetTriangle(TriangleNumber).V[2];
					}
				}

				Wm5::Box3d MinBox = Wm5::MinBox3<double>(NumPoints, Points, 0.001, Wm5::Query::QT_REAL);
				//Wm5::Box3d MinBox = Wm5::ContOrientedBox<double>(NumPoints, Points);
				//Wm5::Box3d MinBox = Wm5::ContAlignedBox<double>(NumPoints, Points);

				delete[] Points;

				printf("| Puzzle Diagonal: %f%%\n", 100 * Wm5::Vector3d(MinBox.Extent[0], MinBox.Extent[1], MinBox.Extent[2]).Length() / Wm5::Vector3d(0.6, 0.6, 0.6).Length());
			}
			break;
		case 'R':
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
	else /// if (GLFW_RELEASE == Action)
	{
		/*if ('X' == Key) {
			m_SecondConstraint.Normal = Wm5::Vector3d::ZERO;
			m_StickToPlaneModePossible = false;
		}*/
	}
}

// Input: two triangles Tri0 and Tri1 (such that they are intersecting or touching), a unit vector Direction
// Output: the minimum non-negative value k, such that after moving triangle Tri0 by k*Direction results in the two triangles no longer intersecting (within some small value)
double UnderCursorMode::ResolveTriTriCollisionAlongVector(Wm5::Triangle3d Tri0, Wm5::Triangle3d Tri1, Wm5::Vector3d Direction)
{
	// TODO: Remove magic consts and calculate them based on triangle size/bounding box
	const double InitialBacktrack = 10;
	double EarliestContactTime = InitialBacktrack;

	// Move Tri0 back before the collision could have occurred
	for (int Vertex = 0; Vertex < 3; ++Vertex)
		Tri0.V[Vertex] += Direction * InitialBacktrack;

	Wm5::IntrTriangle3Triangle3d Intersection(Tri0, Tri1);

	// Now test how much it can move forward (towards its original position) before there is a collision
	bool Result = Intersection.Test(EarliestContactTime, -Direction, Wm5::Vector3d::ZERO);
	if (Result) {
if ((InitialBacktrack - Intersection.GetContactTime()) < 0) printf(" ++++++++++++++ POST-CONDITION FAILED in ResolveTriTriCollisionAlongVector: returning negative k\n");
		// Return the difference between collision time and Tri0's starting position, this is the value of k
		return (InitialBacktrack - Intersection.GetContactTime());
	}

	// The two triangles were not intersecting at all
	return 0;
}

void UnderCursorMode::SnapOrtho(SnapMode SnapMode)
{
	Wm5::Vector3d SnapOrigin(camera.x, camera.y, camera.z);
	Wm5::Vector3d SnapDirection(m_CursorRayDirection);

	SnapOrtho(SnapMode, SnapOrigin, SnapDirection);
}
void UnderCursorMode::SnapOrtho(SnapMode SnapMode, Wm5::Vector3d SnapOrigin, Wm5::Vector3d SnapDirection)
{
if (Back == SnapMode) printf("SnapOrtho(Back): ");
if (Front == SnapMode) printf("SnapOrtho(Front): ");

	SnapDirection.Normalize();

	Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);

	glPushMatrix();

	// Set a smaller viewport with same aspect ratio
	uint32 nCustomViewportWidth = std::min<uint32>(400, nViewportWidth);
	//uint32 nCustomViewportWidth = nViewportWidth;
	uint32 nCustomViewportHeight = nCustomViewportWidth * nViewportHeight / nViewportWidth;
	glViewport(0, 0, nCustomViewportWidth, nCustomViewportHeight);

	// Perspective projection
	//SetOpenGLProjectionMatrix(Perspective);
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, static_cast<double>(nViewportWidth) / nViewportHeight, 0.1, 100);
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();

		/*glRotated(camera.rv + 90, -1, 0, 0);
		glRotated(camera.rh, 0, 0, 1);
		glTranslated(-camera.x, -camera.y, -camera.z);*/
		Wm5::Vector3d CameraOrigin(SnapOrigin);
		Wm5::Vector3d CameraTarget(SnapOrigin + SnapDirection);
		if (CameraOrigin.X() != CameraTarget.X() || CameraOrigin.Y() != CameraTarget.Y())
			gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 0, 0, 1);
		else
			gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 1, 0, 0);
	}

	// Check that object is visible, using perspective projection
	bool IsObjectVisible = false;
	{
		uint8 * pPixels; float * pDepths; uint8 * pSPixels; float * pSDepths;

		do
		{
			MyScene.Render(Scene::StaticSceneGeometry);

			pPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pPixels));
			pDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths));

			// Render the selected model
			MyScene.Render(Scene::SelectedObjectBackFace);

			pSPixels = new uint8[4 * nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels));
			pSDepths = new float[nCustomViewportWidth * nCustomViewportHeight]; glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths));

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
					double dDepthDiff = d - sd;

					if (dDepthDiff >= 0) {
						IsObjectVisible = true;
						break;
					}
				}
				else if ((sr+sg) != 0 && (r+g) == 0)
				{
					IsObjectVisible = true;
					break;
				}
			}
		}
		while (false);

		delete[] pPixels; delete[] pDepths; delete[] pSPixels; delete[] pSDepths;
	}
	//printf("IsObjectVisible = %d\n", IsObjectVisible);

	if (Front == SnapMode && IsObjectVisible)
	{
		// TODO: Remove magic consts and calculate them based on triangle size/bounding box
		const double InitialBacktrack = 10;
		double EarliestContactTime = InitialBacktrack;

		for (auto it0 = m_IntersectingPairs.begin(); it0 != m_IntersectingPairs.end(); ++it0)
		{
			//printf("*Intersecting pair: %d/%d and %d/%d\n", it0->first.first, it0->first.second, it0->second.first, it0->second.second);

			Wm5::Triangle3d Tri0 = MyScene.GetObject(it0->first.first).GetTriangle(it0->first.second);
			for (int Vertex = 0; Vertex < 3; ++Vertex) Tri0.V[Vertex] += SnapDirection * -InitialBacktrack;
			Wm5::Triangle3d Tri1 = MyScene.GetObject(it0->second.first).GetTriangle(it0->second.second);

			Wm5::IntrTriangle3Triangle3d Intersection(Tri0, Tri1);

			bool Result = Intersection.Test(EarliestContactTime, SnapDirection, Wm5::Vector3d::ZERO);
			if (Result) {
				//printf("  %d -> %f\n", Result, Intersection.GetContactTime());
				if (Intersection.GetContactTime() < EarliestContactTime) {
					EarliestContactTime = Intersection.GetContactTime();
					MyScene.m_SlidingObject = it0->second.first;
					MyScene.m_SlidingTriangle = it0->second.second;
					MyScene.m_SelectedTriangle = it0->first.second;
				}
			}
		}

		//printf("EarliestContactTime: %f\n", EarliestContactTime);
		Wm5::Vector3d MoveVector1 = SnapDirection * (EarliestContactTime - InitialBacktrack);
		if (InitialBacktrack == EarliestContactTime) MoveVector1 = Wm5::Vector3d::ZERO;		// If there is no collision, make MoveVector zero

		// New approach that uses a function
		// TODO: Remove old one after a few days (current date is 8:33 PM 27/12/2010) if new one works fine
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
		Wm5::Vector3d MoveVector2 = -SnapDirection * MaxResolutionDistance;
		if (0 == MaxResolutionDistance) MoveVector2 = Wm5::Vector3d::ZERO;		// If there is no collision, make MoveVector zero (this is only needed to compare MoveVector1 to MoveVector2, can be removed afterwards)
//printf("\nMaxResolutionDistance = %.20f\n", MaxResolutionDistance);

		if (MoveVector1 != MoveVector2)
		{
			printf("\n\n\n\n\n  +++++       NOT THE SAME!!!!!!!!!!!\n MoveV1 = %.20f %.20f %.20f\n MoveV2 = %.20f %.20f %.20f\n", MoveVector1.X(), MoveVector1.Y(), MoveVector1.Z(), MoveVector2.X(), MoveVector2.Y(), MoveVector2.Z());
			//exit(5);
		}

		MoveVector = MoveVector2;

/*{
	Wm5::Vector3d SelObjFuturePos = (MyScene.GetSelectedObject().GetPosition() + MoveVector);
	printf("FutureSelObj.Z() = %f (%f, %f)\n", SelObjFuturePos.Z(), SelObjFuturePos.X(), SelObjFuturePos.Y());
}*/
	}
	else
	{
		// Ortho projection
		{
			//SetOpenGLProjectionMatrix(Ortho);
			glMatrixMode(GL_PROJECTION);
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
			//printf("ProjectedBoundingBox size = %f, %f\n", ProjectedBoundingBox[2] - ProjectedBoundingBox[0], ProjectedBoundingBox[3] - ProjectedBoundingBox[1]);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(ProjectedBoundingBox[0] - 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]), ProjectedBoundingBox[2] + 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]),
					ProjectedBoundingBox[1] - 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), ProjectedBoundingBox[3] + 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), 0, 100);
			glMatrixMode(GL_MODELVIEW);
		}

		if (Back == SnapMode)
		{
			// Render the static scene
			/*if (0 != Model::nSlidingObject)
			{
				// Just the model we're sliding on, if any
				MyScene.GetObject(Model::nSlidingObject).Render(2, false);
			}
			else
			{
				MyScene.Render(Scene::StaticSceneGeometry);
			}*/
			if (IsObjectVisible) {
				MyScene.Render(Scene::StaticSceneGeometry);
			} else {
				SnapMode = Front;
			}
		}
		if (Front == SnapMode)
		{
			if (IsObjectVisible) {
				// Render only the colliding static scene
				MyScene.Render(Scene::StaticCollidingSceneGeometry);
			} else {
				MyScene.Render(Scene::StaticSceneGeometry);
			}
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

		double dMinBackDistance = 0;
		double dMinPositiveDistance = -1;		// Positive distance means away from the camera (i.e. to snap back to background); valid range is (0, +oo)

		MyScene.m_SlidingObject = 0;
		MyScene.m_SlidingTriangle = 0;
		MyScene.m_SelectedTriangle = 0;
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
				double dDepthDiff = d - sd;

				if (Front == SnapMode)
				{
					if (dDepthDiff < dMinBackDistance)
					{
						dMinBackDistance = dDepthDiff;
						MoveVector = 100 * dDepthDiff * SnapDirection;

						MyScene.m_SlidingObject = static_cast<uint16>(g) << 8 | r;
						MyScene.m_SlidingTriangle = static_cast<uint16>(a) << 8 | b;
						MyScene.m_SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
						ClosestPixel = nPixel;
					}
				}
				else if (Back == SnapMode)
				{
					if (dDepthDiff >= 0.0 && (dDepthDiff < dMinPositiveDistance || dMinPositiveDistance == -1))
					{
						dMinPositiveDistance = dDepthDiff;
						MoveVector = 100 * dDepthDiff * SnapDirection;

						MyScene.m_SlidingObject = static_cast<uint16>(g) << 8 | r;
						MyScene.m_SlidingTriangle = static_cast<uint16>(a) << 8 | b;
						MyScene.m_SelectedTriangle = static_cast<uint16>(sa) << 8 | sb;
						ClosestPixel = nPixel;
					}
				}
			}
		}
//printf("SnapPersp() processed pixels in %f secs.\n", glfwGetTime() - t0);
		delete[] pPixels; delete[] pDepths; delete[] pSPixels; delete[] pSDepths;
//printf("MyScene.m_SlidingObject = %d\n MyScene.m_SlidingTriangle = %d\n MyScene.m_SelectedTriangle = %d\n ClosestPixel = %d\n", MyScene.m_SlidingObject, MyScene.m_SlidingTriangle, MyScene.m_SelectedTriangle, ClosestPixel);

		// Geometry distance correction
		do if (dMinBackDistance != 0 || dMinPositiveDistance != -1)		// Use <do> loop to make use of <break> keyword
		{
#if 1
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
#endif
		} while (false);
	}

	//SetOpenGLProjectionMatrix(Perspective);
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, static_cast<double>(nViewportWidth) / nViewportHeight, 0.1, 100);
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();

		glRotated(camera.rv + 90, -1, 0, 0);
		glRotated(camera.rh, 0, 0, 1);
		glTranslated(-camera.x, -camera.y, -camera.z);
	}
	glViewport(0, 0, nViewportWidth, nViewportHeight);

	glPopMatrix();

	{
		//MyScene.GetSelectedObject().MoveBy(MoveVector);
		std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
		for (auto it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
			if ((*it0)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
				(*it0)->MoveBy(MoveVector);
		m_UnderCursorPosition += MoveVector;
	}

	{
		if (glfwGetKey(GLFW_KEY_RALT) && Front == SnapMode)
		{
			printf("Ok sup (let's debug)\n");
		}
		double Epsilon = -0.000025;
		//double Epsilon = -0.000010;
		//double Epsilon = -0.01;
		if (0 != MyScene.m_SlidingObject) {
			Wm5::Vector3d SlidingNormal = MyScene.GetSlidingPlane().Normal;		// For GetSlidingPlane() to work properly, objects have to be in proper place
//SlidingNormal = MyScene.GetSlidingPlane().Normal;
			/// Target = Epsilon / cos(Theta)
			double FAbsCosTheta = Wm5::Mathd::FAbs(SnapDirection.Dot(SlidingNormal));
			if (FAbsCosTheta < 0.000001)
				FAbsCosTheta = 1;
			Epsilon = Epsilon / FAbsCosTheta;
			//printf("%f\n", SlidingNormal.Length());
		}
		MoveVector = ZoomSelectedModel(Epsilon, SnapDirection);

		//MyScene.GetSelectedObject().MoveBy(MoveVector);
		std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
		for (auto it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
			if ((*it0)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
				(*it0)->MoveBy(MoveVector);
		m_UnderCursorPosition += MoveVector;
	}

	printf("SelObj.Z() = %f (%f, %f)\n", MyScene.GetSelectedObject().GetPosition().Z(), MyScene.GetSelectedObject().GetPosition().X(), MyScene.GetSelectedObject().GetPosition().Y());
}
