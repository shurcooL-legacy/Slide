#include "Globals.h"

#include <cstdio>
#include <string>

#include "../Wm5/Wm5Math.h"
#include "../Wm5/Wm5Vector4.h"
#include "../Wm5/Wm5Plane3.h"
#include "../Wm5/Wm5Matrix4.h"
#include "../Wm5/Wm5Quaternion.h"
#include "../Wm5/Wm5Triangle3.h"
#include "../Wm5/Wm5IntrLine3Plane3.h"
#include "Opcode.h"
#include "../Arcball/trackball_bell.h"

#include "Scene.h"

#include "SceneObject.h"

SceneObject::SceneObject(uint16 Id, uint32 StartingIndex, uint32 TriangleCount, uint32 LineIndex, uint32 LineCount, GLuint TextureId, float * pVertexData, float * pNormalData, uint8 * pColorData, float * pTextureCoordData, float * pLineVertexData)
	: m_oIntersectingTris(),
	  m_oOpModel(),
	  m_oOpMesh(),
	  m_pIndexedTriangles(nullptr),
	  m_Id(Id),
	  m_Position(Wm5::Vector3d::ZERO),
	  m_Rotation(Wm5::Quaterniond::IDENTITY),
	  m_oTB(0.75),
	  m_StartingIndex(StartingIndex),
	  m_TriangleCount(TriangleCount),
	  m_LineIndex(LineIndex),
	  m_LineCount(LineCount),
	  m_TextureId(TextureId),
	  m_pVertexData(pVertexData),
	  m_pNormalData(pNormalData),
	  m_pColorData(pColorData),
	  m_pTextureCoordData(pTextureCoordData),
	  m_pLineVertexData(pLineVertexData),
	  m_BoundingBoxMin(pVertexData[0], pVertexData[1], pVertexData[2]),
	  m_BoundingBoxSize(),
	  m_SmallObject(false),
	  m_Selectable(true),
	  m_ContainedObjects(),
	  m_ContainedByObject(0)
{
	//printf("SceneObject(%d) Ctor.\n", m_Id);

	// Calculate Bounding Box
	Wm5::Vector3f BoundingBoxMax(m_BoundingBoxMin);
	for (uint32 Vertex = 0; Vertex < 3 * m_TriangleCount; ++Vertex) {
		if (pVertexData[3 * Vertex + 0] < m_BoundingBoxMin.X()) m_BoundingBoxMin.X() = pVertexData[3 * Vertex + 0];
		if (pVertexData[3 * Vertex + 1] < m_BoundingBoxMin.Y()) m_BoundingBoxMin.Y() = pVertexData[3 * Vertex + 1];
		if (pVertexData[3 * Vertex + 2] < m_BoundingBoxMin.Z()) m_BoundingBoxMin.Z() = pVertexData[3 * Vertex + 2];
		if (pVertexData[3 * Vertex + 0] > BoundingBoxMax.X()) BoundingBoxMax.X() = pVertexData[3 * Vertex + 0];
		if (pVertexData[3 * Vertex + 1] > BoundingBoxMax.Y()) BoundingBoxMax.Y() = pVertexData[3 * Vertex + 1];
		if (pVertexData[3 * Vertex + 2] > BoundingBoxMax.Z()) BoundingBoxMax.Z() = pVertexData[3 * Vertex + 2];
	}
	m_BoundingBoxSize = BoundingBoxMax - m_BoundingBoxMin;
	m_SmallObject = (m_BoundingBoxSize.X() <= 0.5 && m_BoundingBoxSize.Y() <= 0.5 && m_BoundingBoxSize.Z() <= 0.5);

	// Recenter the object at Bounding Box Center
	for (uint32 Vertex = 0; Vertex < 3 * m_TriangleCount; ++Vertex) {
		pVertexData[3 * Vertex + 0] -= m_BoundingBoxMin.X() + 0.5f * m_BoundingBoxSize.X();
		pVertexData[3 * Vertex + 1] -= m_BoundingBoxMin.Y() + 0.5f * m_BoundingBoxSize.Y();
		pVertexData[3 * Vertex + 2] -= m_BoundingBoxMin.Z() + 0.5f * m_BoundingBoxSize.Z();
	}
	for (uint32 Vertex = 0; Vertex < 2 * m_LineCount; ++Vertex) {
		pLineVertexData[3 * Vertex + 0] -= m_BoundingBoxMin.X() + 0.5f * m_BoundingBoxSize.X();
		pLineVertexData[3 * Vertex + 1] -= m_BoundingBoxMin.Y() + 0.5f * m_BoundingBoxSize.Y();
		pLineVertexData[3 * Vertex + 2] -= m_BoundingBoxMin.Z() + 0.5f * m_BoundingBoxSize.Z();
	}
	ModifyPosition().X() += m_BoundingBoxMin.X() + 0.5 * m_BoundingBoxSize.X();
	ModifyPosition().Y() += m_BoundingBoxMin.Y() + 0.5 * m_BoundingBoxSize.Y();
	ModifyPosition().Z() += m_BoundingBoxMin.Z() + 0.5 * m_BoundingBoxSize.Z();
	m_BoundingBoxMin = m_BoundingBoxSize * -0.5;

	m_oTB.radius() = 0.5 * m_BoundingBoxSize.Length();

	// OPCODE
	m_pIndexedTriangles = new IceMaths::IndexedTriangle[m_TriangleCount];
	for (uint32 Triangle = 0; Triangle < m_TriangleCount; ++Triangle) {
		m_pIndexedTriangles[Triangle].mVRef[0] = 3 * Triangle + 0;
		m_pIndexedTriangles[Triangle].mVRef[1] = 3 * Triangle + 1;
		m_pIndexedTriangles[Triangle].mVRef[2] = 3 * Triangle + 2;
	}
	m_oOpMesh.SetNbTriangles(m_TriangleCount);
	m_oOpMesh.SetNbVertices(3 * m_TriangleCount);
	m_oOpMesh.SetPointers(m_pIndexedTriangles, reinterpret_cast<const IceMaths::Point *>(m_pVertexData));
	Opcode::OPCODECREATE Create;
	Create.mIMesh			= &m_oOpMesh;
	Create.mSettings.mLimit	= 1;
	Create.mSettings.mRules	= Opcode::SPLIT_SPLATTER_POINTS | Opcode::SPLIT_GEOM_CENTER;
	Create.mNoLeaf			= true;
	Create.mQuantized		= true;
	Create.mKeepOriginal	= false;
	Create.mCanRemap		= false;
	bool IsOk = m_oOpModel.Build(Create); if (!IsOk) printf("m_oOpModel.Build(Create) failed.\n");
}

SceneObject::~SceneObject()
{
	delete[] m_pIndexedTriangles;

	//printf("SceneObject(%d) ~Dtor.\n", m_Id);
}

void SceneObject::Render(Scene::RenderMode Mode, Slide * MySlide)
{
	//const std::set<uint16> SubSelectedObjects = MySlide->GetSubSelectedObjects();
	//const uint8 Selected = (GetId() == MySlide->GetSelectedObjectId()) + 2 * (MySlide->GetSelectedObjectId() && SubSelectedObjects.end() != SubSelectedObjects.find(GetId()));
	const uint8 Selected = (GetId() == MySlide->GetSelectedObjectId());

	//if (0 != nMode) glColor3ub(static_cast<uint8>(GetId() >> 8), static_cast<uint8>(GetId()), 0);

	// Shadow related
	// DEBUG: Attend this
	//{
		const Wm5::Vector3f oShadowWorldDirection(7.5, 10, -10);

		Wm5::Matrix4d Matrix(GetRotation().Transpose());
		Wm5::Vector4d ShadowDirection(oShadowWorldDirection.X(), oShadowWorldDirection.Y(), oShadowWorldDirection.Z(), 1);
		ShadowDirection = Matrix * ShadowDirection;

		Wm5::Vector3f oShadowDirection(static_cast<float>(ShadowDirection.X()), static_cast<float>(ShadowDirection.Y()), static_cast<float>(ShadowDirection.Z()));
	//}

	glPushMatrix();
		glTranslated(GetPosition().X(), GetPosition().Y(), GetPosition().Z());
		//glMultMatrixd(GetTB().get_gl_current_rotation());
		glMultTransposeMatrixd(&GetRotation()[0][0]);		// Pass in a row-major matrix

			// DEBUG: Attend this
			if (Scene::ShadowTEST == Mode) {
				// Draw shadows to stencil buffer
				for (uint32 nTriangle = 0; nTriangle < m_TriangleCount; ++nTriangle)
				{
					Wm5::Plane3f oSurface(Wm5::Vector3f(m_pVertexData[3 * 3 * nTriangle + 0], m_pVertexData[3 * 3 * nTriangle + 1], m_pVertexData[3 * 3 * nTriangle + 2]),
										  Wm5::Vector3f(m_pVertexData[3 * 3 * nTriangle + 3], m_pVertexData[3 * 3 * nTriangle + 4], m_pVertexData[3 * 3 * nTriangle + 5]),
										  Wm5::Vector3f(m_pVertexData[3 * 3 * nTriangle + 6], m_pVertexData[3 * 3 * nTriangle + 7], m_pVertexData[3 * 3 * nTriangle + 8]));
					if (oSurface.Normal.Dot(-oShadowDirection) >= 0) continue;

					// Top and bottom caps
					glBegin(GL_TRIANGLES);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 0],
								   m_pVertexData[3 * 3 * nTriangle + 1],
								   m_pVertexData[3 * 3 * nTriangle + 2]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 3],
								   m_pVertexData[3 * 3 * nTriangle + 4],
								   m_pVertexData[3 * 3 * nTriangle + 5]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 6],
								   m_pVertexData[3 * 3 * nTriangle + 7],
								   m_pVertexData[3 * 3 * nTriangle + 8]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 0] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 1] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 2] + oShadowDirection.Z());
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 3] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 4] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 5] + oShadowDirection.Z());
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 6] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 7] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 8] + oShadowDirection.Z());
					glEnd();

					// Volume sides
					glBegin(GL_QUADS);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 0],
								   m_pVertexData[3 * 3 * nTriangle + 1],
								   m_pVertexData[3 * 3 * nTriangle + 2]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 3],
								   m_pVertexData[3 * 3 * nTriangle + 4],
								   m_pVertexData[3 * 3 * nTriangle + 5]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 3] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 4] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 5] + oShadowDirection.Z());
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 0] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 1] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 2] + oShadowDirection.Z());

						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 3],
								   m_pVertexData[3 * 3 * nTriangle + 4],
								   m_pVertexData[3 * 3 * nTriangle + 5]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 6],
								   m_pVertexData[3 * 3 * nTriangle + 7],
								   m_pVertexData[3 * 3 * nTriangle + 8]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 6] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 7] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 8] + oShadowDirection.Z());
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 3] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 4] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 5] + oShadowDirection.Z());

						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 6],
								   m_pVertexData[3 * 3 * nTriangle + 7],
								   m_pVertexData[3 * 3 * nTriangle + 8]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 0],
								   m_pVertexData[3 * 3 * nTriangle + 1],
								   m_pVertexData[3 * 3 * nTriangle + 2]);
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 0] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 1] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 2] + oShadowDirection.Z());
						glVertex3f(m_pVertexData[3 * 3 * nTriangle + 6] + oShadowDirection.X(),
								   m_pVertexData[3 * 3 * nTriangle + 7] + oShadowDirection.Y(),
								   m_pVertexData[3 * 3 * nTriangle + 8] + oShadowDirection.Z());
					glEnd();
				}
			} else if (Scene::BoundingBox == Mode) {
				glColor3d(0, 1, 0);
				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glDisable(GL_LIGHTING);		// TODO: Pull out the Disable-Lighting call
				glBegin(GL_LINES);
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glEnd();
				glPopAttrib();
			} else if (Scene::VisibleGeometryWithBoundingBoxes == Mode && m_SmallObject && GetId() != MySlide->m_SlidingObject) {
				glBindTexture(GL_TEXTURE_2D, 0);
				glColor3ub(m_pColorData[0], m_pColorData[1], m_pColorData[2]);
				glBegin(GL_QUADS);
					// Bottom
					glNormal3d(0, 0, -1);
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					// Top
					glNormal3d(0, 0, 1);
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					// Front X Side
					glNormal3d(0, -1, 0);
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

					// Back X Side
					glNormal3d(0, 1, 0);
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					
					// Front Y Side
					glNormal3d(-1, 0, 0);
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

					// Back Y Side
					glNormal3d(1, 0, 0);
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
					glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glEnd();
			} else if (Scene::IntersectingTrianglesTEST != Mode && Scene::StaticCollidingSceneGeometry != Mode) {

#define SLIDE_HIGHLIGHT_SELECTED_OBJECT
#ifdef SLIDE_HIGHLIGHT_SELECTED_OBJECT
		if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode) && Selected) { glDisableClientState(GL_COLOR_ARRAY); glColor3d(0.45 / Selected, 0.7 / Selected, 0.45 / Selected); }
#endif
#define SLIDE_HIGHLIGHT_COLLISIONS_IN_RED
#ifdef SLIDE_HIGHLIGHT_COLLISIONS_IN_RED
		if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode) && !m_oIntersectingTris.empty()) { glDisableClientState(GL_COLOR_ARRAY); glColor3d(1, 0.5, 0.5); }
#endif

		if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode)) glBindTexture(GL_TEXTURE_2D, m_TextureId);

		// Draw the object triangles
		glDrawArrays(GL_TRIANGLES, m_StartingIndex, 3 * m_TriangleCount);

#define SLIDE_HIGHLIGHT_SLIDING_TRIANGLES
#ifdef SLIDE_HIGHLIGHT_SLIDING_TRIANGLES
		if (GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_HIGHLIGHT_SLIDING_TRIANGLE)) {
			// Highlight the sliding triangles
			if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode) && GetId() == MySlide->m_SlidingObject) {		// Sliding triangle of Sliding Object
				glDisableClientState(GL_COLOR_ARRAY); glColor3d(1.1, 0.7, 0.7);
				glDrawArrays(GL_TRIANGLES, m_StartingIndex + 3 * MySlide->m_SlidingTriangle, 3);
				glEnableClientState(GL_COLOR_ARRAY);
			}
			if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode) && GetId() == MySlide->GetSelectedObjectId()) {		// Sliding triangle of Selected Object
				glDisableClientState(GL_COLOR_ARRAY); glColor3d(1.1, 1.1, 0.7);
				glDrawArrays(GL_TRIANGLES, m_StartingIndex + 3 * MySlide->m_SelectedTriangle, 3);
				glEnableClientState(GL_COLOR_ARRAY);
			}
		}
#endif

//#define SLIDE_DRAW_BOUNDING_BOX_FOR_SELECTED_OBJECT
#ifdef SLIDE_DRAW_BOUNDING_BOX_FOR_SELECTED_OBJECT
		// Draw the bounding box for selected object
		if (Scene::VisibleGeometry == Mode && GetId() == MySlide->GetSelectedObjectId()) {
			glColor3d(0, 1, 0);
			glPushAttrib(GL_ALL_ATTRIB_BITS);
			glDisable(GL_LIGHTING);		// TODO: Pull out the Disable-Lighting call
			glBegin(GL_LINES);
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 0*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 1*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());

				glVertex3f(m_BoundingBoxMin.X() + 1*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
				glVertex3f(m_BoundingBoxMin.X() + 0*m_BoundingBoxSize.X(), m_BoundingBoxMin.Y() + 0*m_BoundingBoxSize.Y(), m_BoundingBoxMin.Z() + 1*m_BoundingBoxSize.Z());
			glEnd();
			glPopAttrib();
		}
#endif

#ifdef SLIDE_HIGHLIGHT_SELECTED_OBJECT
		if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode) && Selected) glEnableClientState(GL_COLOR_ARRAY);
#endif
		if ((Scene::VisibleGeometry == Mode || Scene::VisibleGeometryWithBoundingBoxes == Mode) && !m_oIntersectingTris.empty()) glEnableClientState(GL_COLOR_ARRAY);

			} else {
				if (Scene::IntersectingTrianglesTEST == Mode) {
					glBindTexture(GL_TEXTURE_2D, m_TextureId);
					glDisableClientState(GL_COLOR_ARRAY); glColor3d(1, 0, 0);
				}
				for (std::set<uint32>::iterator it0 = m_oIntersectingTris.begin(); it0 != m_oIntersectingTris.end(); ++it0)
					glDrawArrays(GL_TRIANGLES, m_StartingIndex + 3 * (*it0), 3);
				if (Scene::IntersectingTrianglesTEST == Mode) {
					glEnableClientState(GL_COLOR_ARRAY);
				}
			}
	glPopMatrix();
}

void SceneObject::RenderLinesTEST(Scene::RenderMode Mode)
{
	glPushMatrix();
		glTranslated(GetPosition().X(), GetPosition().Y(), GetPosition().Z());
		//glMultMatrixd(GetTB().get_gl_current_rotation());
		glMultTransposeMatrixd(&GetRotation()[0][0]);		// Pass in a row-major matrix

		if (Scene::VisibleGeometry == Mode)
		{
			// Draw the object lines
			glDrawArrays(GL_LINES, m_LineIndex, 2 * m_LineCount);
		}
	glPopMatrix();
}

const Wm5::Vector3d SceneObject::GetPosition() const
{
	return m_Position;
}

Wm5::Vector3d & SceneObject::ModifyPosition()
{
	return m_Position;
}

void SceneObject::MoveBy(Wm5::Vector3d oDisplacement)
{
	m_Position += oDisplacement;
}

const Wm5::Matrix4d SceneObject::GetRotation()// const
{
#ifdef SLIDE_ARCBALL_ROTATIONS
	double Entries[16];
	memcpy(Entries, GetTB().get_gl_current_rotation(), 16 * sizeof(double));
	const Wm5::Matrix4d RotationMatrix(Entries, true);
	return RotationMatrix;
#else
	Wm5::Matrix3d Matrix3;
	m_Rotation.ToRotationMatrix(Matrix3);		// This gives me a row-major rotation matrix
	const Wm5::Matrix4d RotationMatrix( Matrix3[0][0], Matrix3[0][1], Matrix3[0][2], 0,
										Matrix3[1][0], Matrix3[1][1], Matrix3[1][2], 0,
										Matrix3[2][0], Matrix3[2][1], Matrix3[2][2], 0,
										0,             0,             0,             1 );

	return RotationMatrix;
#endif // SLIDE_ARCBALL_ROTATIONS
}

Wm5::Quaterniond & SceneObject::ModifyRotation()
{
	return m_Rotation;
}

Bell & SceneObject::GetTB()
{
	return m_oTB;
}

Bell & SceneObject::ModifyTB()
{
	return m_oTB;
}

Wm5::Vector3d SceneObject::GetBoundingBoxCorner(uint8 CornerNumber)
{
	if (CornerNumber < 0 || CornerNumber >= 8) {printf("SceneObject::GetBoundingBoxCorner(uint8 CornerNumber): param out of range\n"); throw -1; }

	Wm5::Vector4d Corner(m_BoundingBoxMin.X() + (CornerNumber / 1 % 2) * m_BoundingBoxSize.X(),
						 m_BoundingBoxMin.Y() + (CornerNumber / 2 % 2) * m_BoundingBoxSize.Y(),
						 m_BoundingBoxMin.Z() + (CornerNumber / 4 % 2) * m_BoundingBoxSize.Z(), 1);
	Wm5::Matrix4d Matrix(GetRotation());
	Matrix[0][3] = GetPosition().X();
	Matrix[1][3] = GetPosition().Y();
	Matrix[2][3] = GetPosition().Z();
	Corner = Matrix * Corner;

	return Wm5::Vector3d(Corner.X(), Corner.Y(), Corner.Z());
}

Wm5::Tuple<4, double> SceneObject::GetProjectedBoundingBox()
{
	GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
	GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);
Viewport[0] = -100; Viewport[1] = -100; Viewport[2] = 200; Viewport[3] = 200;
	GLdouble winX, winY, winZ;

	Wm5::Tuple<2, double> MinCorner; MinCorner[0] = Viewport[2] + Viewport[0]; MinCorner[1] = Viewport[3] + Viewport[1];
	Wm5::Tuple<2, double> MaxCorner; MaxCorner[0] = Viewport[0]; MaxCorner[1] = Viewport[1];

	for (uint8 CornerNumber = 0; CornerNumber < 8; ++CornerNumber)
	{
		Wm5::Vector3d Corner = GetBoundingBoxCorner(CornerNumber);

		gluProject(Corner.X(), Corner.Y(), Corner.Z(), ModelMatrix, ProjectionMatrix, Viewport, &winX, &winY, &winZ);
		if (winX < MinCorner[0]) MinCorner[0] = winX;
		if (winY < MinCorner[1]) MinCorner[1] = winY;
		if (winX > MaxCorner[0]) MaxCorner[0] = winX;
		if (winY > MaxCorner[1]) MaxCorner[1] = winY;
	}

	Wm5::Tuple<4, double> ProjectedBoundingBox;
	ProjectedBoundingBox[0] = MinCorner[0];
	ProjectedBoundingBox[1] = MinCorner[1];
	ProjectedBoundingBox[2] = MaxCorner[0];
	ProjectedBoundingBox[3] = MaxCorner[1];
	return ProjectedBoundingBox;
}

Wm5::Vector3d SceneObject::GetProjectedOutermostBoundingPoint(Wm5::Vector3d Direction)
{
	Wm5::Line3d Line(GetPosition(), Direction);

	double OutermostIntersection = 0;
	for (uint8 CornerNumber = 0; CornerNumber < 8; ++CornerNumber)
	{
		Wm5::Vector3d Corner = GetBoundingBoxCorner(CornerNumber);

		Wm5::Plane3d CornerPlane(Line.Direction, Corner);
		Wm5::IntrLine3Plane3d Intersection(Line, CornerPlane);
		if (Intersection.Find())
			OutermostIntersection = std::max<double>(OutermostIntersection, Intersection.GetLineParameter());
		else
			printf("Error: No intersection!\n");
	}

	Wm5::Vector3d IntersectionPoint = Line.Origin + Line.Direction * OutermostIntersection;
	return IntersectionPoint;
}

Wm5::Triangle3d SceneObject::GetTriangle(uint16 TriangleNumber)
{
	float * pVertexData = &m_pVertexData[3 * 3 * TriangleNumber];
	Wm5::Matrix4d Matrix(GetRotation());
	Matrix[0][3] = GetPosition().X();
	Matrix[1][3] = GetPosition().Y();
	Matrix[2][3] = GetPosition().Z();
	Wm5::Vector4d V0(pVertexData[0], pVertexData[1], pVertexData[2], 1); V0 = Matrix * V0;
	Wm5::Vector4d V1(pVertexData[3], pVertexData[4], pVertexData[5], 1); V1 = Matrix * V1;
	Wm5::Vector4d V2(pVertexData[6], pVertexData[7], pVertexData[8], 1); V2 = Matrix * V2;

	Wm5::Triangle3d Triangle(Wm5::Vector3d(V0.X(), V0.Y(), V0.Z()),
							 Wm5::Vector3d(V1.X(), V1.Y(), V1.Z()),
							 Wm5::Vector3d(V2.X(), V2.Y(), V2.Z()));

	return Triangle;
}

Wm5::Vector3d SceneObject::GetTriangleNormal(uint16 TriangleNumber)
{
	float * pNormalData = &m_pNormalData[3 * 3 * TriangleNumber];
	Wm5::Matrix4d Matrix(GetRotation());
	Wm5::Vector4d Normal(pNormalData[0], pNormalData[1], pNormalData[2], 1);
	Normal = Matrix * Normal;

	return Wm5::Vector3d(Normal.X(), Normal.Y(), Normal.Z());
}
