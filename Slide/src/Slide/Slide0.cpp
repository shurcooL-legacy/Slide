#include "../Globals.h"

// Went back to 025 thanks to additional Wm5 collision detection filtering
// Started using 100 instead of 025, because scene 2 had problems
// Right now went back to using 025, because 010 sometimes takes too many SnapGeometry(Front) calls to resolve collisions.
//const double Slide::m_kBufferDistance = 0.010000;
//const double Slide::m_kBufferDistance = 0.001000;
//const double Slide::m_kBufferDistance = 0.000100;
  const double Slide::m_kBufferDistance = 0.000025;
//const double Slide::m_kBufferDistance = 0.000010;

Slide::Slide()
	: m_SelectedObjectId(0),
	  m_SlidingObject(0),
	  m_SlidingTriangle(0),
	  m_SelectedTriangle(0)
{
}

Slide::~Slide()
{
}

bool Slide::CheckForCollision()
{
	bool bCollisionExists = false;

	for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
		(*it0)->m_oIntersectingTris.clear();
	m_IntersectingPairs.clear();

	//printf("\nCheckForCollision\n¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\n");

	//std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
	std::set<uint16> SubSelectedObjects;
	for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
	{
		if ((*it0)->GetId() == GetSelectedObjectId() || (GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
		{
			//SceneObject * pSelectedModel = &MyScene.GetSelectedObject();
			SceneObject * pSelectedModel = (*it0);

			static Opcode::AABBTreeCollider Collider;
			Collider.SetFirstContact(false);
			Collider.SetFullBoxBoxTest(true);
			Collider.SetFullPrimBoxTest(true);
			Collider.SetTemporalCoherence(false);
			Opcode::BVTCache ColCache;
			ColCache.Model0 = &pSelectedModel->m_oOpModel;

			// http://knol.google.com/k/matrices-for-3d-applications-translation-rotation
			IceMaths::Matrix4x4 oMatrix0( static_cast<float>(pSelectedModel->GetRotation()[0][0]), static_cast<float>(pSelectedModel->GetRotation()[1][0]), static_cast<float>(pSelectedModel->GetRotation()[2][0]), static_cast<float>(pSelectedModel->GetRotation()[3][0]),
										  static_cast<float>(pSelectedModel->GetRotation()[0][1]), static_cast<float>(pSelectedModel->GetRotation()[1][1]), static_cast<float>(pSelectedModel->GetRotation()[2][1]), static_cast<float>(pSelectedModel->GetRotation()[3][1]),
										  static_cast<float>(pSelectedModel->GetRotation()[0][2]), static_cast<float>(pSelectedModel->GetRotation()[1][2]), static_cast<float>(pSelectedModel->GetRotation()[2][2]), static_cast<float>(pSelectedModel->GetRotation()[3][2]),
										  static_cast<float>(pSelectedModel->GetPosition().X()),   static_cast<float>(pSelectedModel->GetPosition().Y()),   static_cast<float>(pSelectedModel->GetPosition().Z()),   1 );

			for (std::vector<SceneObject *>::iterator it1 = MyScene.m_Objects.begin(); it1 != MyScene.m_Objects.end(); ++it1)
			{
				if ((*it1) == pSelectedModel) continue;
				//if ((*it1)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it1)->GetId()))) continue;

				IceMaths::Matrix4x4 oMatrix1( static_cast<float>((*it1)->GetRotation()[0][0]), static_cast<float>((*it1)->GetRotation()[1][0]), static_cast<float>((*it1)->GetRotation()[2][0]), static_cast<float>((*it1)->GetRotation()[3][0]),
											  static_cast<float>((*it1)->GetRotation()[0][1]), static_cast<float>((*it1)->GetRotation()[1][1]), static_cast<float>((*it1)->GetRotation()[2][1]), static_cast<float>((*it1)->GetRotation()[3][1]),
											  static_cast<float>((*it1)->GetRotation()[0][2]), static_cast<float>((*it1)->GetRotation()[1][2]), static_cast<float>((*it1)->GetRotation()[2][2]), static_cast<float>((*it1)->GetRotation()[3][2]),
											  static_cast<float>((*it1)->GetPosition().X()),   static_cast<float>((*it1)->GetPosition().Y()),   static_cast<float>((*it1)->GetPosition().Z()),   1 );

				ColCache.Model1 = &(*it1)->m_oOpModel;

				bool IsOk = Collider.Collide(ColCache, &oMatrix0, &oMatrix1);
				if (IsOk)
				{
					if (Collider.GetContactStatus())
					{
						udword NbPairs = Collider.GetNbPairs();
						const Pair * pPairs = Collider.GetPairs();

						for (udword nPair = 0; nPair < NbPairs; ++nPair)
						{
							// Verify the triangles are indeed intersecting
							Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetObject(pSelectedModel->GetId()).GetTriangle(pPairs[nPair].id0);		// Selected Object, to be moved
							Wm5::Triangle3d CollidingObjectTriangle = MyScene.GetObject((*it1)->GetId()).GetTriangle(pPairs[nPair].id1);			// Colliding Object, static
							Wm5::IntrTriangle3Triangle3d Intersection(SelectedObjectTriangle, CollidingObjectTriangle);
							bool TrianglesIntersecting = Intersection.Test();
							if (!TrianglesIntersecting)
								continue;

							bCollisionExists = true;

							pSelectedModel->m_oIntersectingTris.insert(pPairs[nPair].id0);
							(*it1)->m_oIntersectingTris.insert(pPairs[nPair].id1);
							//printf("Intersecting pair: %d/%d and %d/%d\n", pSelectedModel->GetId(), pPairs[nPair].id0, (*it1)->GetId(), pPairs[nPair].id1);

							std::pair<std::pair<uint16, uint16>, std::pair<uint16, uint16>> IntersectingPair;		// { (Obj0, Tri), (Obj1, Tri) }
							IntersectingPair.first.first = pSelectedModel->GetId();
							IntersectingPair.first.second = pPairs[nPair].id0;
							IntersectingPair.second.first = (*it1)->GetId();
							IntersectingPair.second.second = pPairs[nPair].id1;
							m_IntersectingPairs.push_back(IntersectingPair);
						}
					}
				}
			}
		}
	}

	return bCollisionExists;
}

// Returns true of a collision is found when moving Selected Object by ResolutionVector
// Returns false if no collision
bool Slide::CheckForCollisionTemporary(Wm5::Vector3d ResolutionVector)
{
	//std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();
	std::set<uint16> SubSelectedObjects;
	for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
	{
		if ((*it0)->GetId() == GetSelectedObjectId() || (GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
		{
			//SceneObject * pSelectedModel = &MyScene.GetSelectedObject();
			SceneObject * pSelectedModel = (*it0);

			static Opcode::AABBTreeCollider Collider;
			Collider.SetFirstContact(/*true*/false);
			Collider.SetFullBoxBoxTest(true);
			Collider.SetFullPrimBoxTest(true);
			Collider.SetTemporalCoherence(false);
			Opcode::BVTCache ColCache;
			ColCache.Model0 = &pSelectedModel->m_oOpModel;

			// http://knol.google.com/k/matrices-for-3d-applications-translation-rotation
			IceMaths::Matrix4x4 oMatrix0( static_cast<float>(pSelectedModel->GetRotation()[0][0]), static_cast<float>(pSelectedModel->GetRotation()[1][0]), static_cast<float>(pSelectedModel->GetRotation()[2][0]), static_cast<float>(pSelectedModel->GetRotation()[3][0]),
										  static_cast<float>(pSelectedModel->GetRotation()[0][1]), static_cast<float>(pSelectedModel->GetRotation()[1][1]), static_cast<float>(pSelectedModel->GetRotation()[2][1]), static_cast<float>(pSelectedModel->GetRotation()[3][1]),
										  static_cast<float>(pSelectedModel->GetRotation()[0][2]), static_cast<float>(pSelectedModel->GetRotation()[1][2]), static_cast<float>(pSelectedModel->GetRotation()[2][2]), static_cast<float>(pSelectedModel->GetRotation()[3][2]),
										  static_cast<float>(pSelectedModel->GetPosition().X() + ResolutionVector.X()), static_cast<float>(pSelectedModel->GetPosition().Y() + ResolutionVector.Y()), static_cast<float>(pSelectedModel->GetPosition().Z() + ResolutionVector.Z()), 1 );

			for (std::vector<SceneObject *>::iterator it1 = MyScene.m_Objects.begin(); it1 != MyScene.m_Objects.end(); ++it1)
			{
				if ((*it1) == pSelectedModel) continue;
				//if ((*it1)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it1)->GetId()))) continue;

				IceMaths::Matrix4x4 oMatrix1( static_cast<float>((*it1)->GetRotation()[0][0]), static_cast<float>((*it1)->GetRotation()[1][0]), static_cast<float>((*it1)->GetRotation()[2][0]), static_cast<float>((*it1)->GetRotation()[3][0]),
											  static_cast<float>((*it1)->GetRotation()[0][1]), static_cast<float>((*it1)->GetRotation()[1][1]), static_cast<float>((*it1)->GetRotation()[2][1]), static_cast<float>((*it1)->GetRotation()[3][1]),
											  static_cast<float>((*it1)->GetRotation()[0][2]), static_cast<float>((*it1)->GetRotation()[1][2]), static_cast<float>((*it1)->GetRotation()[2][2]), static_cast<float>((*it1)->GetRotation()[3][2]),
											  static_cast<float>((*it1)->GetPosition().X()),   static_cast<float>((*it1)->GetPosition().Y()),   static_cast<float>((*it1)->GetPosition().Z()),   1 );

				ColCache.Model1 = &(*it1)->m_oOpModel;

				bool IsOk = Collider.Collide(ColCache, &oMatrix0, &oMatrix1);
				if (IsOk)
				{
					if (Collider.GetContactStatus())
					{
						//return true;

						udword NbPairs = Collider.GetNbPairs();
						const Pair * pPairs = Collider.GetPairs();

						for (udword nPair = 0; nPair < NbPairs; ++nPair)
						{
							// Verify the triangles are indeed intersecting
							Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetObject(pSelectedModel->GetId()).GetTriangle(pPairs[nPair].id0);		// Selected Object, to be moved
							for (int Vertex = 0; Vertex < 3; ++Vertex)
								SelectedObjectTriangle.V[Vertex] += ResolutionVector;
							Wm5::Triangle3d CollidingObjectTriangle = MyScene.GetObject((*it1)->GetId()).GetTriangle(pPairs[nPair].id1);			// Colliding Object, static
							Wm5::IntrTriangle3Triangle3d Intersection(SelectedObjectTriangle, CollidingObjectTriangle);
							bool TrianglesIntersecting = Intersection.Test();
							if (!TrianglesIntersecting)
								continue;

							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void Slide::MoveSelectedObject(Wm5::Vector3d MoveVector)
{
	GetSelectedObject().MoveBy(MoveVector);
	/*std::set<uint16> SubSelectedObjects = MySlide->GetSubSelectedObjects();
	for (auto it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0)
		if ((*it0)->GetId() == MyScene.GetSelectedObjectId() || (MyScene.GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find((*it0)->GetId())))
			(*it0)->MoveBy(MoveVector);*/
	m_UnderCursorPosition += MoveVector;
}

Wm5::Vector3d Slide::ZoomSelectedModel(double dDistance, Wm5::Vector3d oDirection)
{
#if 0
	Wm5::Vector3d oView(Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD));

	oDirection.Normalize();
	if (oView.Dot(oDirection) < 0)
		oDirection *= -1;
#endif

	Wm5::Vector3d oDisplacement(oDirection * dDistance);
	return oDisplacement;
}

void Slide::SlideResolution()
{
	if (0 != GetSelectedObjectId())
	{
		if (m_ModelMovedByUser)
		{
#if 1
			// DEBUG: This needs to be redone properly
			{
				GLuint SampleCount; glGetQueryObjectuiv(MySlideTools->m_QueryId, GL_QUERY_RESULT, &SampleCount);
				bool SelectedObjectVisible = (0 != SampleCount);
				//printf("VISIBLE > %d\n", SelectedObjectVisible);

				if (!SelectedObjectVisible) {
					printf("Object became invisible, bringing it to front...\n");
					SnapOrtho(SnapMode::Front, StartingPosition::Invisible);
					printf("Brought it to front.\n");
					m_ModelMovedByUser = false;
					return;
				}
			}
#endif

			bool CollisionExists = CheckForCollision();

#if 1
			//if (IsModeActive(StickToPlane) && 0 != MyScene.m_SlidingObject && Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CollisionExists)
			if (true && 0 != m_SlidingObject && Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CollisionExists)
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
			//if (!(IsModeActive(StickToPlane) && 0 != MyScene.m_SlidingObject && Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CollisionExists))
			if (!(true && 0 != m_SlidingObject && Wm5::Vector3d::ZERO != m_SlidingConstraint.Normal && !CollisionExists))
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
				m_ModelMovedByUser = false;
		}
		else if (m_ModelRotatedByUser)
		{
#if 1
			{
				bool CollisionExists = CheckForCollision();

				//Wm5::Vector3d Normal(0, 0, 1);
				Wm5::Vector3d Normal(m_SlidingConstraint.Normal);

				if (!CollisionExists)
				{
					Wm5::Vector3d ProjectedOutermostPoint(GetSelectedObject().GetProjectedOutermostBoundingPoint(Normal));

					Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);
					Wm5::Vector3d SnapDirection(-Normal);

					SnapOrtho(Back, SnapOrigin, SnapDirection, StartingPosition::NonColliding, true);
				}
				else
				{
					Wm5::Vector3d ProjectedOutermostPoint(GetSelectedObject().GetProjectedOutermostBoundingPoint(-Normal));

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

			m_ModelRotatedByUser = false;
		}
	}
}

void Slide::PickModelAndComputeUnderCursorPosition(int16 PositionX, int16 PositionY)
{
//if (MyScene.GetSelectedObjectId()) MyScene.GetSelectedObject().m_oIntersectingTris.clear();

	// Focus viewport on the specified pixel position
	glViewport(-PositionX, -PositionY, nViewportWidth, nViewportHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, MySlideTools->m_PickingFboId);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);

	MyScene.Render(Scene::ObjectPicking, this);

	uint8 cPixel[4]; glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, cPixel);
	uint16 NewSelectedObject = cPixel[1] << 8 | cPixel[0];

	// Compute under cursor position
	{
		GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
		GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
		GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);

		float Depth; glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(&Depth));
		gluUnProject(0, 0, Depth, ModelMatrix, ProjectionMatrix, Viewport, &m_UnderCursorPosition.X(), &m_UnderCursorPosition.Y(), &m_UnderCursorPosition.Z());
	}

	// Return to normal framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glDrawBuffer(GL_BACK);
	glViewport(0, 0, nViewportWidth, nViewportHeight);

	// Find the new cursor ray
	ComputeCursorRayDirection(PositionX, PositionY);

	if (1 == NewSelectedObject) NewSelectedObject = 0;

	// If we selected a different object, clear the sliding object
	if (NewSelectedObject != GetSelectedObjectId() && 0 != NewSelectedObject) {
		m_SlidingObject = MyScene.GetObject(NewSelectedObject).m_ContainedByObject;
		if (0 != m_SlidingObject) exit(0);
		m_SlidingTriangle = 0;		// DEBUG: This needs to be either set to correct value if 0 != SlidingObject, or else ignored
		m_SelectedTriangle = 0;		// DEBUG: This needs to be either set to correct value if 0 != SlidingObject, or else ignored

// DEBUG: This is a temporary hack, I should think about where to reset the sliding constraint (or perhaps keep it on a per-object basis)
m_SlidingConstraint.Normal = Wm5::Vector3d::ZERO;
	}

	SetSelectedObjectId(NewSelectedObject);

	uint32 Triangle = cPixel[3] << 8 | cPixel[2];
//if (MySlide->GetSelectedObjectId()) MySlide->GetSelectedObject().m_oIntersectingTris.insert(Triangle);

	if (GetSelectedObjectId()) {
		//m_OriginalSelectedObjectPosition = MySlide->GetSelectedObject().GetPosition();

		//printf("  selected model #%d at %d, %d, triangle = %d.\n", GetSelectedObjectId(), PositionX, PositionY, Triangle);

		m_ModelMovedByUser = true;
	}
}

// Find the new cursor ray
void Slide::ComputeCursorRayDirection(int16 PositionX, int16 PositionY)
{
	Wm5::Vector3d Camera(camera.x, camera.y, camera.z), FarPoint;
	GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
	GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);

	gluUnProject(PositionX, PositionY, 1.0f, ModelMatrix, ProjectionMatrix, Viewport, &FarPoint.X(), &FarPoint.Y(), &FarPoint.Z());

	m_CursorRayDirection = FarPoint - Camera;
	m_CursorRayDirection.Normalize();
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
		attitude = -Wm5::Mathd::PI / 2;
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

void Slide::RotationalSnap()
{
	// Snap to nearest rotation
	if (0 != GetSelectedObjectId())
	{
#if 0
		Wm5::Quaterniond ClosestRotateBy;
		double ClosestAngle = 10;
		for (uint8 Direction = 0; Direction < 6; ++Direction) {
#define			SLIDE_TOTAL_ORIENTATIONS (4)
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
				Wm5::Quaterniond RotateBy2 = MySlide->GetSelectedObject().ModifyRotation().Inverse() * RotateBy;

				Wm5::Vector3d Axis; double Angle;
				RotateBy2.ToAxisAngle(Axis, Angle);
#undef PI
				Angle += 2 * Wm5::Mathd::PI; while (Angle > Wm5::Mathd::PI) Angle -= 2 * Wm5::Mathd::PI; if (Angle < 0) Angle *= -1;		// Get angle in [0, +180) range
				//printf("Direction %d got Angle %f\n", Direction, Angle * 180 / 3.141592);

				if (Angle < ClosestAngle) {
					ClosestAngle = Angle;
					ClosestRotateBy = RotateBy;
				}
			}
		}

		MySlide->GetSelectedObject().ModifyRotation() = ClosestRotateBy;// * MySlide->GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
#else
		/*Wm5::Vector3d Axis;
		double Angle;
		MySlide->GetSelectedObject().ModifyRotation().ToAxisAngle(Axis, Angle);

		printf("Axis.Length() == %f\n", Axis.Length());
		//Wm5::Vector3d ViewDirection(Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD), Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD));
		double AxisVAngle = Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ASin(Axis.Z());
		double RAxisVAngle = std::floor(AxisVAngle / 90 + 0.5) * 90;
		Axis.Z() = Wm5::Mathd::Sin(RAxisVAngle * Wm5::Mathd::DEG_TO_RAD);
		Axis.Normalize();

		Wm5::Quaterniond SnappedRotation;
		SnappedRotation.FromAxisAngle(Axis, Angle);
		MySlide->GetSelectedObject().ModifyRotation() = SnappedRotation;*/

		/*{
			double cz, sz, cx, sx, cy, sy;
			MySlide->GetSelectedObject().ModifyRotation().FactorZXY(cz, sz, cx, sx, cy, sy);
			printf("z, x, y: %f %f %f", Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ACos(cz), Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ACos(cx), Wm5::Mathd::RAD_TO_DEG * Wm5::Mathd::ACos(cy));
		}*/

#define SLIDE_ROTATE_INCREMENT (90.0/3.0)
		if (1)
		{
			double heading, attitude, bank;
			QuaternionToEulerAngles(GetSelectedObject().ModifyRotation(), heading, attitude, bank);
			heading = floor(heading * Wm5::Mathd::RAD_TO_DEG / SLIDE_ROTATE_INCREMENT + 0.5) * SLIDE_ROTATE_INCREMENT * Wm5::Mathd::DEG_TO_RAD;
			attitude = floor(attitude * Wm5::Mathd::RAD_TO_DEG / SLIDE_ROTATE_INCREMENT + 0.5) * SLIDE_ROTATE_INCREMENT * Wm5::Mathd::DEG_TO_RAD;
			bank = floor(bank * Wm5::Mathd::RAD_TO_DEG / SLIDE_ROTATE_INCREMENT + 0.5) * SLIDE_ROTATE_INCREMENT * Wm5::Mathd::DEG_TO_RAD;

			//printf("heading, attitude, bank = %f %f %f\n", heading * Wm5::Mathd::RAD_TO_DEG, attitude * Wm5::Mathd::RAD_TO_DEG, bank * Wm5::Mathd::RAD_TO_DEG);

			GetSelectedObject().ModifyRotation() = QuaternionFromEulerAngles(heading, attitude, bank);
			//GetSelectedObject().ModifyRotation() = QuaternionFromEulerAngles(0 * Wm5::Mathd::DEG_TO_RAD, 0 * Wm5::Mathd::DEG_TO_RAD, 30 * Wm5::Mathd::DEG_TO_RAD);
		}
#endif

		m_ModelRotatedByUser = true;
	}
}

SceneObject & Slide::GetSelectedObject()
{
	return MyScene.GetObject(GetSelectedObjectId());
}
uint16 Slide::GetSelectedObjectId()
{
	return m_SelectedObjectId;
}
void Slide::SetSelectedObjectId(uint16 Id)
{
	m_SelectedObjectId = Id;
}

/*std::set<uint16> Slide::GetSubSelectedObjects()
{
	if (0 == GetSelectedObjectId()) return std::set<uint16>();

	std::set<uint16> SubSelectedObjects;
	std::queue<uint16> Frontier;

	Frontier.push(GetSelectedObjectId());
	while (!Frontier.empty())
	{
		SubSelectedObjects.insert(Frontier.front());
		for (std::set<uint16>::iterator it0 = GetObject(Frontier.front()).m_ContainedObjects.begin(); it0 != GetObject(Frontier.front()).m_ContainedObjects.end(); ++it0)
			Frontier.push(*it0);
		Frontier.pop();
	}
	SubSelectedObjects.erase(GetSelectedObjectId());

	return SubSelectedObjects;
}*/

/*Wm5::Vector3d Slide::SnapOrtho(SnapMode SnapMode, StartingPosition StartingPosition, int8 TargetDepthLayer)
{
	return Wm5::Vector3d::ZERO;
}*/
Wm5::Vector3d Slide::SnapOrtho(SnapMode SnapMode, StartingPosition StartingPosition, int8 TargetDepthLayer)
{
	Wm5::Vector3d SnapDirection;
	if (Back == SnapMode)
		SnapDirection = m_CursorRayDirection;
	else if (Front == SnapMode)
		SnapDirection = -m_CursorRayDirection;

	Wm5::Vector3d ProjectedOutermostPoint(GetSelectedObject().GetProjectedOutermostBoundingPoint(-SnapDirection));
	Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);

	return SnapOrtho(SnapMode, SnapOrigin, SnapDirection, StartingPosition, false, TargetDepthLayer);
}
Wm5::Vector3d Slide::SnapOrtho(SnapMode SnapMode, Wm5::Vector3d SnapOrigin, Wm5::Vector3d SnapDirection, StartingPosition StartingPosition, bool SkipVisibilityCheck, int8 TargetDepthLayer)
{
//if (Back == SnapMode) printf("SnapOrtho(Back):  ");
//if (Front == SnapMode) printf("SnapOrtho(Front): ");

	SnapDirection.Normalize();

	const Wm5::Vector3d kOriginalSelectedObjectPosition = GetSelectedObject().GetPosition();
	Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);

#if 1
	// Try a quick collision resolve in a special simple case
	if (Colliding == StartingPosition && (Front == SnapMode && 1 == TargetDepthLayer))
	{
		Wm5::Vector3d OriginalSelectedObjectPosition = GetSelectedObject().GetPosition();		// TODO: Make this function operate using temporary object positions, instead of physically moving selected object inside; after that, this variable is no longer needed

		MoveSelectedObject(SnapGeometry(-SnapDirection));

		if (!CheckForCollision() && (SkipVisibilityCheck ? true : CheckForVisibility()))
			//return MoveVector;			// Because I'm actually moving the object within this function...
			return Wm5::Vector3d::ZERO;		// TODO: Clean this up
		else
			MoveSelectedObject(OriginalSelectedObjectPosition - GetSelectedObject().GetPosition());		// Reset Object Position
	}
#endif

	// Set a smaller viewport with same aspect ratio
	/*uint32 nCustomViewportWidth = std::min<uint32>(400, nViewportWidth);
	//uint32 nCustomViewportWidth = nViewportWidth;
	uint32 nCustomViewportHeight = nCustomViewportWidth * nViewportHeight / nViewportWidth;*/
	uint32 nCustomViewportWidth = MySlideTools->m_DepthPeelImageWidth, nCustomViewportHeight = MySlideTools->m_DepthPeelImageHeight;
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

		Wm5::Tuple<4, double> ProjectedBoundingBox = GetSelectedObject().GetProjectedBoundingBox();

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
		Wm5::Vector3d OriginalSelectedObjectPosition = GetSelectedObject().GetPosition();		// TODO: Make this function operate using temporary object positions, instead of physically moving selected object inside; after that, this variable is no longer needed
		Wm5::Vector3d PreviousSelectedObjectPosition = GetSelectedObject().GetPosition();
		uint8 DepthLayersAchieved = 0;
		uint8 MaxLayersToPeel = 16;
		for (uint8 DepthLayer = 0; DepthLayersAchieved < TargetDepthLayer && DepthLayer <= MaxLayersToPeel; ++DepthLayer)
		{
			// Reset Object Position
			MoveSelectedObject(OriginalSelectedObjectPosition - GetSelectedObject().GetPosition());

			// Set target FBO
			glBindFramebuffer(GL_FRAMEBUFFER, MySlideTools->m_DepthPeelFboId[DepthLayer % 2]);
			//glDrawBuffer(GL_COLOR_ATTACHMENT0);

			if (0 == DepthLayer)
			{
				if (Front == SnapMode) {
					MyScene.Render(Scene::SelectedObjectBackFace, this);

					glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels2));
					glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths2));
				}

				// Render the selected model
				if (Back == SnapMode)
					MyScene.Render(Scene::SelectedObjectBackFace, this);
				else if (Front == SnapMode)
					MyScene.Render(Scene::SelectedObjectFrontFace, this);

				glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void *>(pSPixels));
				glReadPixels(0, 0, nCustomViewportWidth, nCustomViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pSDepths));
			}
			else
			{
				MySlideTools->m_DepthPeelShader.bind();
				MySlideTools->m_DepthPeelShader.bindTextureRECT("DepthTex", MySlideTools->m_DepthPeelDepthTexId[(DepthLayer - 1) % 2], 0);
				if (Back == SnapMode)
					MyScene.Render(Scene::StaticSceneGeometry, this);
				else if (Front == SnapMode)
					MyScene.Render(Scene::StaticSceneGeometryBackFace, this);
				MySlideTools->m_DepthPeelShader.unbind();

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

					int Iterations = 5;
					//int Iterations = 50;
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
					bool EndedUpInPreviousPosition = (0.000001 * m_kBufferDistance * m_kBufferDistance) >= (PreviousSelectedObjectPosition - GetSelectedObject().GetPosition()).SquaredLength();
					bool WorkingOnFirstLayer = (0 == DepthLayersAchieved);
					if (!CollisionExists && (!EndedUpInPreviousPosition || (WorkingOnFirstLayer && Back == SnapMode)))
					{
#if 0
						PreviousSelectedObjectPosition = MyScene.GetSelectedObject().GetPosition();
						++DepthLayersAchieved;
#else
						// Set target FBO
						glBindFramebuffer(GL_FRAMEBUFFER, MySlideTools->m_DepthPeelFboId[(DepthLayer + 1) % 2]);
						//glDrawBuffer(GL_COLOR_ATTACHMENT0);

						bool SelectedObjectVisible = SkipVisibilityCheck ? true : CheckForVisibility();

						if (SelectedObjectVisible)
						{
							PreviousSelectedObjectPosition = GetSelectedObject().GetPosition();
							++DepthLayersAchieved;
						}
						else if (Back == SnapMode)
						{
							//MoveSelectedObject(PreviousSelectedObjectPosition - GetSelectedObject().GetPosition());
							MoveSelectedObject(OriginalSelectedObjectPosition - GetSelectedObject().GetPosition());
							break;
						}
						else if (Front == SnapMode)
						{
							//MoveSelectedObject(PreviousSelectedObjectPosition - GetSelectedObject().GetPosition());
							//break;
							PreviousSelectedObjectPosition = GetSelectedObject().GetPosition();
						}
#endif
					}
					else if (CollisionExists)
					{
						printf("Problem! Unhandled exception. Unable to Geo-resolve collision (took too many iterations, giving up).\n");
					}
				}

				// Geometry distance correction
#if 0
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
	return kOriginalSelectedObjectPosition - GetSelectedObject().GetPosition();		// TODO: Doing this temporary for depth-pop sounds to work
}

Wm5::Vector3d Slide::SnapGeometry(SnapMode SnapMode)
{
	Wm5::Vector3d SnapDirection;
	if (Back == SnapMode)
		SnapDirection = -m_CursorRayDirection;
	else if (Front == SnapMode)
		SnapDirection = m_CursorRayDirection;

	return SnapGeometry(SnapDirection);
}
Wm5::Vector3d Slide::SnapGeometry(Wm5::Vector3d SnapDirection)
{
//printf("SnapGeometry(): ");

	SnapDirection.Normalize();

	Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);

	// Geometry resolution
	{
		m_SlidingObject = 0;		// Reset m_SlidingObject

		double MaxResolutionDistance = 0;
		for (auto it0 = m_IntersectingPairs.begin(); it0 != m_IntersectingPairs.end(); ++it0)
		{
			Wm5::Triangle3d SelectedObjectTriangle = MyScene.GetObject(it0->first.first).GetTriangle(it0->first.second);		// Selected Object, to be moved
			Wm5::Triangle3d CollidingObjectTriangle = MyScene.GetObject(it0->second.first).GetTriangle(it0->second.second);		// Colliding Object, static

			double ResolutionDistance = ResolveTriTriCollisionAlongVector(SelectedObjectTriangle, CollidingObjectTriangle, -SnapDirection);

			// Find the maximum resolution distance, because we need to make sure all triangle collisions are resolved
			if (ResolutionDistance > MaxResolutionDistance) {
				MaxResolutionDistance = ResolutionDistance;
				m_SlidingObject = it0->second.first;
				m_SlidingTriangle = it0->second.second;
				m_SelectedTriangle = it0->first.second;
			}
		}

		MoveVector = -SnapDirection * MaxResolutionDistance;
	}

//printf("preObj.Z() = %.20f (%f, %f)\nSnapGeome(Front): ", (MyScene.GetSelectedObject().GetPosition() + MoveVector).Z(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).X(), (MyScene.GetSelectedObject().GetPosition() + MoveVector).Y());
	{
		double BufferDistance = m_kBufferDistance;
		if (0 != m_SlidingObject) {
			Wm5::Vector3d SlidingNormal = GetSlidingPlane(MoveVector).Normal;		// For GetSlidingPlane() to work properly, objects have to be in proper place
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

bool Slide::CheckForVisibility()
{
	return true;
}

template <typename Real>
int8 WhichSide(const Wm5::Plane3<Real> & plane, const Wm5::Vector3<Real> & p, const Real epsilon = 0.000001)
{
    Real distance = plane.DistanceTo(p);

    if (distance > (Real)epsilon)
    {
        return +1;
    }
    else if (distance < (Real)-epsilon)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

Wm5::Plane3d Slide::GetSlidingPlane(Wm5::Vector3d SelectedObjectOffset)
{
	Slide * MySlide = nullptr;

	Wm5::Plane3d SlidingPlane(Wm5::Vector3d::ZERO, 0);

	if (0 != m_SlidingObject && 0 != GetSelectedObjectId())
	{
#if 0
		SlidingPlane.Normal = GetSlidingPlane().Normal;
#else
		Wm5::Triangle3d SlidingTriangle = MyScene.GetObject(m_SlidingObject).GetTriangle(m_SlidingTriangle);
		Wm5::Plane3d SlidingTrianglePlane(SlidingTriangle.V[0], SlidingTriangle.V[1], SlidingTriangle.V[2]);
		Wm5::Triangle3d SelectedTriangle = GetSelectedObject().GetTriangle(m_SelectedTriangle);
		for (uint8 Vertex = 0; Vertex < 3; ++Vertex)
			SelectedTriangle.V[Vertex] += SelectedObjectOffset;		// Shift Selected Triangle by Offset
		Wm5::Plane3d SelectedTrianglePlane(SelectedTriangle.V[0], SelectedTriangle.V[1], SelectedTriangle.V[2]);

		int TotalSide = 0;
		for (int Vertex = 0; Vertex < 3; ++Vertex)
		{
			int Side = WhichSide(SlidingTrianglePlane, SelectedTriangle.V[Vertex]);
			if (abs(TotalSide + Side) >= abs(TotalSide))
				TotalSide += Side;
			else {
				TotalSide = 10;
				break;
			}
		}

		if (10 != TotalSide)
		{
			SlidingPlane = SlidingTrianglePlane;
		}
		else
		{
			TotalSide = 0;
			for (uint8 Vertex = 0; Vertex < 3; ++Vertex)
			{
				int Side = WhichSide(SelectedTrianglePlane, SlidingTriangle.V[Vertex]);
				if (abs(TotalSide + Side) >= abs(TotalSide))
					TotalSide += Side;
				else {
					TotalSide = 10;
					break;
				}
			}

			if (10 != TotalSide)
			{
				SlidingPlane = SelectedTrianglePlane;
				SlidingPlane.Normal *= -1;
			}
			else
			{
				// Figure out the closest 2 edges
				uint8 ClosestSlidingEdge = 0;
				uint8 ClosestSelectedEdge = 0;
				double ClosestDistanceSquared = 1000000;
				for (uint8 SlidingEdge = 0; SlidingEdge < 3; ++SlidingEdge)
				{
					Wm5::Segment3d SlidingSegment(SlidingTriangle.V[SlidingEdge], SlidingTriangle.V[(SlidingEdge + 1) % 3]);

					for (uint8 SelectedEdge = 0; SelectedEdge < 3; ++SelectedEdge)
					{
						Wm5::Segment3d SelectedSegment(SelectedTriangle.V[SelectedEdge], SelectedTriangle.V[(SelectedEdge + 1) % 3]);

						Wm5::DistSegment3Segment3d Distance(SlidingSegment, SelectedSegment);
						double DistanceSquared = Distance.GetSquared();

						if (DistanceSquared < ClosestDistanceSquared) {
							ClosestDistanceSquared = DistanceSquared;
							ClosestSlidingEdge = SlidingEdge;
							ClosestSelectedEdge = SelectedEdge;
						}
					}
				}

				Wm5::Segment3d ClosestSlidingSegment(SlidingTriangle.V[ClosestSlidingEdge], SlidingTriangle.V[(ClosestSlidingEdge + 1) % 3]);
				Wm5::Segment3d ClosestSelectedSegment(SelectedTriangle.V[ClosestSelectedEdge], SelectedTriangle.V[(ClosestSelectedEdge + 1) % 3]);

				// The sliding plane is the cross product of the two closest edges
				SlidingPlane.Normal = (ClosestSelectedSegment.P1 - ClosestSelectedSegment.P0).Cross(ClosestSlidingSegment.P1 - ClosestSlidingSegment.P0);
				SlidingPlane.Normal.Normalize();
				const double Epsilon = 0.000003;
/*printf("\nSlidingPN.Dot(SelTriN) %.20f\n", SlidingPlane.Normal.Dot(SelectedTrianglePlane.Normal));
printf("SlidingPN.Dot(SlidingTriN) %.20f\n", SlidingPlane.Normal.Dot(SlidingTrianglePlane.Normal));
printf("SlidingPN.Dot(SelTri.V[3] - SelTri.V[1]) = %f\n", SlidingPlane.Normal.Dot(SelectedTriangle.V[(ClosestSelectedEdge + 2) % 3] - SelectedTriangle.V[ClosestSelectedEdge]));*/
				if (Epsilon < SlidingPlane.Normal.Dot(SelectedTrianglePlane.Normal))		// Assuming the selected triangle normal is facing outside, ensure the sliding plane faces the other direction, by flipping it if necessary
					SlidingPlane.Normal *= -1;
				else if (Epsilon >= Wm5::Mathd::FAbs(SlidingPlane.Normal.Dot(SelectedTrianglePlane.Normal)) && -Epsilon > SlidingPlane.Normal.Dot(SlidingTrianglePlane.Normal))		// If the selected triangle normal is orthogonal to sliding plane (i.e. dot product is 0), use sliding triangle normal instead
					SlidingPlane.Normal *= -1;
				else if (Epsilon >= Wm5::Mathd::FAbs(SlidingPlane.Normal.Dot(SelectedTrianglePlane.Normal)) && Epsilon >= Wm5::Mathd::FAbs(SlidingPlane.Normal.Dot(SlidingTrianglePlane.Normal))		// If both normals are orthogonal to the sliding plane, then fall back to unused 3rd vertex
					&& -0 > SlidingPlane.Normal.Dot(SelectedTriangle.V[(ClosestSelectedEdge + 2) % 3] - SelectedTriangle.V[ClosestSelectedEdge]))														// Flip the SlidinePlane.Normal if it's not facing the same way as the "3rd" vertex of Selected Triangle
					SlidingPlane.Normal *= -1;
			}
		}
#endif
	}

	return SlidingPlane;
}

// Input: Two triangles Tri0 and Tri1 (such that they are intersecting or touching), a unit vector Direction
// Output: The minimum non-negative value k, such that after moving triangle Tri0 by k*Direction results in the two triangles no longer intersecting (within some small value)
double Slide::ResolveTriTriCollisionAlongVector(Wm5::Triangle3d Tri0, Wm5::Triangle3d Tri1, Wm5::Vector3d Direction)
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
