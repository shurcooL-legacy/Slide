#include "../Globals.h"

#include <cstring>
#include <set>
#include <vector>
#include <queue>

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
#include "../Wm5/Wm5Vector4.h"
#include "../Wm5/Wm5Matrix4.h"
#include "../Wm5/Wm5Quaternion.h"
#include "../Wm5/Wm5Triangle3.h"
#include "../Wm5/Wm5Line3.h"
#include "../Wm5/Wm5Plane3.h"
#include "../Wm5/Wm5Segment3.h"
#include "../Wm5/Wm5DistSegment3Segment3.h"

#include <Opcode.h>
#include "../Arcball/trackball_bell.h"

#include "Models/ModelLoader.h"
#include "Models/ColladaModel.h"
#include "Models/SketchUpModel.h"
#include "Models/SupermarketModel.h"

#include "Scene.h"

#include "SceneObject.h"

Scene MyScene;

Scene::Scene()
	: m_NextId(1),
	  m_TrianglesAdded(0),
	  m_LinesAdded(0),
	  m_TriangleCount(0),
	  m_LineCount(0),
	  m_Objects(),
	  m_LoadedTextureIds(),
	  m_VertexVbo(0),
	  m_NormalVbo(0),
	  m_ColorVbo(0),
	  m_IdColorVbo(0),
	  m_TextureCoordVbo(0),
	  m_LineVertexVbo(0),
	  m_pVertexData(nullptr),
	  m_pNormalData(nullptr),
	  m_pColorData(nullptr),
	  m_pIdColorData(nullptr),
	  m_pTextureCoordData(nullptr),
	  m_pLineVertexData(nullptr)
{
}

Scene::~Scene()
{
	Reset();
}

void Scene::ReserveObject(uint32 TriangleCount, uint32 LineCount)
{
//printf("Reserving an object with %d tris and %d lines.\n", TriangleCount, LineCount);
	m_TriangleCount += TriangleCount;
	m_LineCount += LineCount;
}

void Scene::DoneReserving()
{
printf("Done reserving all objects with a total of %d tris and %d lines.\n", m_TriangleCount, m_LineCount);
	m_pVertexData = new float[3 * 3 * m_TriangleCount];
	m_pNormalData = new float[3 * 3 * m_TriangleCount];
	m_pColorData = new uint8[3 * 3 * m_TriangleCount];
	m_pIdColorData = new uint8[3 * 4 * m_TriangleCount];
	m_pTextureCoordData = new float[2 * 3 * m_TriangleCount];
	m_pLineVertexData = new float[2 * 3 * m_LineCount];
}

void Scene::PopulateObjectPointers(float *& pVertexData, float *& pNormalData, uint8 *& pColorData, float *& pTextureCoordData, float *& pLineVertexData)
{
	pVertexData = m_pVertexData + 3 * 3 * m_TrianglesAdded;
	pNormalData = m_pNormalData + 3 * 3 * m_TrianglesAdded;
	pColorData = m_pColorData + 3 * 3 * m_TrianglesAdded;
	pTextureCoordData = m_pTextureCoordData + 2 * 3 * m_TrianglesAdded;
	pLineVertexData = m_pLineVertexData + 2 * 3 * m_LinesAdded;
}

void Scene::PopulatedObject(uint32 TriangleCount, uint32 LineCount, GLuint TextureId)
{
	float * pVertexData = m_pVertexData + 3 * 3 * m_TrianglesAdded;
	float * pNormalData = m_pNormalData + 3 * 3 * m_TrianglesAdded;
	uint8 * pColorData = m_pColorData + 3 * 3 * m_TrianglesAdded;
	float * pTextureCoordData = m_pTextureCoordData + 2 * 3 * m_TrianglesAdded;
	float * pLineVertexData = m_pLineVertexData + 2 * 3 * m_LinesAdded;

	SceneObject * pSceneObject = new SceneObject(m_NextId++, 3 * m_TrianglesAdded, TriangleCount, 2 * m_LinesAdded, LineCount, TextureId, pVertexData, pNormalData, pColorData, pTextureCoordData, pLineVertexData);
	m_Objects.push_back(pSceneObject);

	if (TextureId) m_LoadedTextureIds.insert(TextureId);		// Register Texture Id

	// Generate Id Color array
	uint8 * pIdColorData = m_pIdColorData + 3 * 4 * m_TrianglesAdded;
	for (uint32 Triangle = 0; Triangle < TriangleCount; ++Triangle)
	{
		for (uint8 Vertex = 0; Vertex < 3; ++Vertex) {
			pIdColorData[3 * 4 * Triangle + 4 * Vertex + 0] = static_cast<uint8>(pSceneObject->GetId());
			pIdColorData[3 * 4 * Triangle + 4 * Vertex + 1] = static_cast<uint8>(pSceneObject->GetId() >> 8);
			pIdColorData[3 * 4 * Triangle + 4 * Vertex + 2] = static_cast<uint8>(Triangle);
			pIdColorData[3 * 4 * Triangle + 4 * Vertex + 3] = static_cast<uint8>(Triangle >> 8);
		}
	}

	m_TrianglesAdded += TriangleCount;
	m_LinesAdded += LineCount;
}

void Scene::Finalize()
{
	glGenBuffers(1, &m_VertexVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * 3 * m_TriangleCount * sizeof(*m_pVertexData), m_pVertexData, GL_STATIC_DRAW);

	glGenBuffers(1, &m_NormalVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_NormalVbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * 3 * m_TriangleCount * sizeof(*m_pNormalData), m_pNormalData, GL_STATIC_DRAW);

	glGenBuffers(1, &m_ColorVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_ColorVbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * 3 * m_TriangleCount * sizeof(*m_pColorData), m_pColorData, GL_STATIC_DRAW);

	glGenBuffers(1, &m_IdColorVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_IdColorVbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * 4 * m_TriangleCount * sizeof(*m_pIdColorData), m_pIdColorData, GL_STATIC_DRAW);

	glGenBuffers(1, &m_TextureCoordVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_TextureCoordVbo);
	glBufferData(GL_ARRAY_BUFFER, 2 * 3 * m_TriangleCount * sizeof(*m_pTextureCoordData), m_pTextureCoordData, GL_STATIC_DRAW);

	glGenBuffers(1, &m_LineVertexVbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_LineVertexVbo);
	glBufferData(GL_ARRAY_BUFFER, 2 * 3 * m_LineCount * sizeof(*m_pLineVertexData), m_pLineVertexData, GL_STATIC_DRAW);
}

SceneObject & Scene::GetObject(uint16 Id)
{
	return *m_Objects[Id - 1];
}

void Scene::Render(RenderMode Mode, Slide * MySlide)
{
	//const std::set<uint16> SubSelectedObjects = MyScene.GetSubSelectedObjects();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	switch (Mode)
	{
	case VisibleGeometry:
	case VisibleGeometryWithBoundingBoxes:
	case IntersectingTrianglesTEST:
		if (IntersectingTrianglesTEST != Mode) {
			glClearColor(0.600f, 0.741f, 0.565f, 1);
			glStencilMask(255);		// Enable writing to stencil buffer
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glStencilMask(0);		// Disable stencil writes
		}

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_NORMAL_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_NormalVbo); glNormalPointer(GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_COLOR_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_ColorVbo); glColorPointer(3, GL_UNSIGNED_BYTE, 0, nullptr);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_TextureCoordVbo); glTexCoordPointer(2, GL_FLOAT, 0, nullptr);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		{
			glShadeModel(GL_SMOOTH);
			glEnable(GL_POLYGON_OFFSET_FILL);		// Enable this for line rendering
			glEnable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);

			// Render Static Objects
			for (auto it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0) {
				const uint8 Selected = ((*it0)->GetId() == MySlide->GetSelectedObjectId());
				if (0 == Selected)
					(*it0)->Render(Mode, MySlide);
			}

			// Render Selected Object(s)
			{
				glBeginQuery(GL_SAMPLES_PASSED, MySlideTools->m_QueryId);		// Occlusion query for selected object

				for (auto it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0) {
					const uint8 Selected = ((*it0)->GetId() == MySlide->GetSelectedObjectId());
					if (0 != Selected)
						(*it0)->Render(Mode, MySlide);
				}

				glEndQuery(GL_SAMPLES_PASSED);									// Occlusion query for selected object
			}
		}
		glPopAttrib();

		// Shadows
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		if (!GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_HIDE_SHADOWS))
		{
			glEnable(GL_POLYGON_OFFSET_FILL);		// Match the above

			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDepthMask(GL_FALSE);
			glStencilMask(255);		// Enable writing to stencil buffer

			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 0, 0);

			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			for (auto it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
				(*it0)->Render(ShadowTEST, MySlide);

			glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
			glCullFace(GL_FRONT);
			for (auto it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
				(*it0)->Render(ShadowTEST, MySlide);
			glCullFace(GL_BACK);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthMask(GL_TRUE);
			glStencilMask(0);		// Disable stencil writes

			// Ortho projection
			glPushMatrix();		// Push Modelview Matrix
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();		// Push Projection Matrix
			glLoadIdentity();
			glOrtho(-1, 1, -1, 1, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glStencilFunc(GL_NOTEQUAL, 0, 255);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			// Draw a screen filling quad
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glColor4d(0, 0, 0, 0.5);
			glBegin(GL_QUADS);
				glVertex2i(-1, -1);
				glVertex2i( 1, -1);
				glVertex2i( 1,  1);
				glVertex2i(-1,  1);
			glEnd();
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);

			glDisable(GL_STENCIL_TEST);

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();		// Restore Projection Matrix
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();		// Restore Modelview Matrix
		}
		glPopAttrib();

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_LineVertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		//glPushAttrib(GL_ALL_ATTRIB_BITS);
		{
			//glEnable(GL_BLEND);

			glColor3d(0, 0, 0);
			for (std::vector<SceneObject *>::iterator it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
				//(*it0)->Render(Mode, (*it0)->GetId() == GetSelectedObjectId());
				(*it0)->RenderLinesTEST(Mode);
		}
		//glPopAttrib();

		break;
	case BoundingBox:
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		{
			glEnable(GL_LIGHTING);

			for (std::vector<SceneObject *>::iterator it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
				//(*it0)->Render(Mode, (*it0)->GetId() == GetSelectedObjectId());
				(*it0)->Render(Mode, MySlide);
		}
		glPopAttrib();
		
		break;
	case ObjectPicking:
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_COLOR_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_IdColorVbo); glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);

		glBeginQuery(GL_SAMPLES_PASSED, MySlideTools->m_QueryId);		// Occlusion query for selected object

		for (std::vector<SceneObject *>::iterator it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
			(*it0)->Render(Mode, MySlide);

		glEndQuery(GL_SAMPLES_PASSED);									// Occlusion query for selected object

		CHECK_GL_ERRORS

		break;
	case SelectedObjectVisibilityQuery:
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);

		// Render Static Objects
		for (auto it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0) {
			const uint8 Selected = ((*it0)->GetId() == MySlide->GetSelectedObjectId());
			if (0 == Selected)
				(*it0)->Render(Mode, MySlide);
		}

		// Render Selected Object(s)
		{
			glBeginQuery(GL_SAMPLES_PASSED, MySlideTools->m_QueryId);		// Occlusion query for selected object

			for (auto it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0) {
				const uint8 Selected = ((*it0)->GetId() == MySlide->GetSelectedObjectId());
				if (0 != Selected)
					(*it0)->Render(Mode, MySlide);
			}

			glEndQuery(GL_SAMPLES_PASSED);									// Occlusion query for selected object
		}

		break;
	case StaticSceneGeometry:
	case StaticCollidingSceneGeometry:
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_COLOR_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_IdColorVbo); glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);

		for (std::vector<SceneObject *>::iterator it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
			if ((*it0)->GetId() != MySlide->GetSelectedObjectId())
				(*it0)->Render(Mode, MySlide);

		break;
	case StaticSceneGeometryBackFace:
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_COLOR_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_IdColorVbo); glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
			glCullFace(GL_FRONT);

			for (std::vector<SceneObject *>::iterator it0 = m_Objects.begin(); it0 != m_Objects.end(); ++it0)
				if ((*it0)->GetId() != MySlide->GetSelectedObjectId())
					(*it0)->Render(Mode, MySlide);
		glPopAttrib();

		break;
	/*case StaticCollidingSceneGeometry:
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bool bCollisionExists = false;
		/*for (std::vector<Model *>::iterator it0 = oModels.begin(); it0 != oModels.end(); ++it0)
			if ((*it0)->GetId() != oScene.GetSelectedObjectId() && !(*it0)->m_oIntersectingTris.empty()) {
				bCollisionExists = true;
				break;
			}* /

		// Render only the colliding the static scene
		for (std::vector<Model *>::iterator it0 = oModels.begin(); it0 != oModels.end(); ++it0)
			if ((*it0)->GetId() != oScene.GetSelectedObjectId() && (!(*it0)->m_oIntersectingTris.empty() || !bCollisionExists))
				(*it0)->Render(Mode, false);

		break;*/
	case SelectedObjectClosestFrontFace:
	case SelectedObjectFurthestFrontFace:
		glClearColor(0, 0, 0, 0);
		glPushAttrib(GL_ALL_ATTRIB_BITS);
			if (SelectedObjectFurthestFrontFace == Mode) glClearDepth(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPopAttrib();

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_COLOR_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_IdColorVbo); glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);

		// Render the selected model
		glPushAttrib(GL_ALL_ATTRIB_BITS);
			if (SelectedObjectFurthestFrontFace == Mode) glDepthFunc(GL_GEQUAL);

			MySlide->GetSelectedObject().Render(Mode, MySlide);
		glPopAttrib();

		break;
	case SelectedObjectClosestBackFace:
	case SelectedObjectFurthestBackFace:
		glClearColor(0, 0, 0, 0);
		glPushAttrib(GL_ALL_ATTRIB_BITS);
			if (SelectedObjectFurthestBackFace == Mode) glClearDepth(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPopAttrib();

		glEnableClientState(GL_VERTEX_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_VertexVbo); glVertexPointer(3, GL_FLOAT, 0, nullptr);
		glEnableClientState(GL_COLOR_ARRAY); glBindBuffer(GL_ARRAY_BUFFER, m_IdColorVbo); glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);

		// Render the selected model
		glPushAttrib(GL_ALL_ATTRIB_BITS);
			if (SelectedObjectFurthestBackFace == Mode) glDepthFunc(GL_GEQUAL);
			glCullFace(GL_FRONT);

			MySlide->GetSelectedObject().Render(Mode, MySlide);
		glPopAttrib();

		break;
	default:
		break;
	}
}

Wm5::Vector3d Scene::CalculateNavigationOffset()
{
	Wm5::Vector3d TotalOffset(Wm5::Vector3d::ZERO);

	// Set a smaller viewport with square aspect ratio
	uint32 CustomViewportSize = std::min<uint32>(32, std::min<uint32>(nViewportWidth, nViewportHeight));
	glViewport(0, 0, CustomViewportSize, CustomViewportSize);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(90, 1 / 1, 0.01, 0.5);
	gluPerspective(60, 1 / 1, 0.01, 0.25);
	glMatrixMode(GL_MODELVIEW);

	float * pDepths[6];
	for (int Side = 0; Side < 6; ++Side)
	{
		Wm5::Vector3d ViewDirection;
		if (0 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 1; ViewDirection.Z() = 0; }				// Forward
		else if (1 == Side) { ViewDirection.X() = -1; ViewDirection.Y() = 0; ViewDirection.Z() = 0; }		// Left
		else if (2 == Side) { ViewDirection.X() = 1; ViewDirection.Y() = 0; ViewDirection.Z() = 0; }		// Right
		else if (3 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = -1; ViewDirection.Z() = 0; }		// Back
		else if (4 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 0; ViewDirection.Z() = 1; }		// Up
		else if (5 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 0; ViewDirection.Z() = -1; }		// Down

		glLoadIdentity();
		if (Side <= 3)
			gluLookAt(camera.x, camera.y, camera.z, camera.x + ViewDirection.X(), camera.y + ViewDirection.Y(), camera.z + ViewDirection.Z(), 0, 0, 1);
		else if (4 == Side)
			gluLookAt(camera.x, camera.y, camera.z, camera.x + ViewDirection.X(), camera.y + ViewDirection.Y(), camera.z + ViewDirection.Z(), 0, -1, 0);
		else if (5 == Side)
			gluLookAt(camera.x, camera.y, camera.z, camera.x + ViewDirection.X(), camera.y + ViewDirection.Y(), camera.z + ViewDirection.Z(), 0, 1, 0);

		Render(Scene::ObjectPicking, nullptr);

		pDepths[Side] = new float[CustomViewportSize * CustomViewportSize]; glReadPixels(0, 0, CustomViewportSize, CustomViewportSize, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths[Side]));

		double Min = 1;
		for (uint32 y = 0; y < CustomViewportSize; ++y) {
			for (uint32 x = 0; x < CustomViewportSize; ++x) {
				Min = std::min<double>(Min, Wm5::Mathd::Pow(pDepths[Side][x + y * CustomViewportSize], 20));
			}
		}

		//TotalOffset += -ViewDirection * (1 - Min);
	}

	float * pNewDepths;
	for (int Side = 0; Side < 6; ++Side)
	{
		//glViewport(Side % 3 * CustomViewportSize, Side / 3 * CustomViewportSize, CustomViewportSize, CustomViewportSize);
		glViewport(Side * CustomViewportSize, 0, CustomViewportSize, CustomViewportSize);

		Wm5::Vector3d ViewDirection;
		if (0 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 1; ViewDirection.Z() = 0; }				// Forward
		else if (1 == Side) { ViewDirection.X() = -1; ViewDirection.Y() = 0; ViewDirection.Z() = 0; }		// Left
		else if (2 == Side) { ViewDirection.X() = 1; ViewDirection.Y() = 0; ViewDirection.Z() = 0; }		// Right
		else if (3 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = -1; ViewDirection.Z() = 0; }		// Back
		else if (4 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 0; ViewDirection.Z() = 1; }		// Up
		else if (5 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 0; ViewDirection.Z() = -1; }		// Down

		glLoadIdentity();
		if (Side <= 3)
			gluLookAt(camera.x, camera.y, camera.z, camera.x + ViewDirection.X(), camera.y + ViewDirection.Y(), camera.z + ViewDirection.Z(), 0, 0, 1);
		else if (4 == Side)
			gluLookAt(camera.x, camera.y, camera.z, camera.x + ViewDirection.X(), camera.y + ViewDirection.Y(), camera.z + ViewDirection.Z(), 0, -1, 0);
		else if (5 == Side)
			gluLookAt(camera.x, camera.y, camera.z, camera.x + ViewDirection.X(), camera.y + ViewDirection.Y(), camera.z + ViewDirection.Z(), 0, 1, 0);

		Render(Scene::ObjectPicking, nullptr);
	}

	glViewport(0, 0, CustomViewportSize, 6 * CustomViewportSize);

	for (int Side = 0; Side < 6; ++Side)
	{
		Wm5::Vector3d ViewDirection;
		if (0 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 1; ViewDirection.Z() = 0; }				// Forward
		else if (1 == Side) { ViewDirection.X() = -1; ViewDirection.Y() = 0; ViewDirection.Z() = 0; }		// Left
		else if (2 == Side) { ViewDirection.X() = 1; ViewDirection.Y() = 0; ViewDirection.Z() = 0; }		// Right
		else if (3 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = -1; ViewDirection.Z() = 0; }		// Back
		else if (4 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 0; ViewDirection.Z() = 1; }		// Up
		else if (5 == Side) { ViewDirection.X() = 0; ViewDirection.Y() = 0; ViewDirection.Z() = -1; }		// Down

		pNewDepths = new float[6 * CustomViewportSize * CustomViewportSize]; glReadPixels(0, 0, 6 * CustomViewportSize, CustomViewportSize, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pNewDepths));

		double Min = 1;
		for (uint32 y = 0; y < CustomViewportSize; ++y) {
			for (uint32 x = 0; x < CustomViewportSize; ++x) {
				Min = std::min<double>(Min, Wm5::Mathd::Pow(pNewDepths[x + Side * CustomViewportSize + y * 6 * CustomViewportSize], 20));
			}
		}

		TotalOffset += -ViewDirection * (1 - Min);
	}

	// Verify
	uint32 Good = 0;
	for (int Side = 0; Side < 6; ++Side) {
		for (uint32 y = 0; y < CustomViewportSize; ++y) {
			for (uint32 x = 0; x < CustomViewportSize; ++x) {
				float d1 = pDepths[Side][x + y * CustomViewportSize];
				float d2 = pNewDepths[x + Side * CustomViewportSize + y * 6 * CustomViewportSize];
				if (d1 != d2) {
					printf("Baaaaad on Side %d at %dx%d!\n", Side, x, y);
					//glfwSwapBuffers();
					//glfwSleep(10);
					//exit(1999);
				} else if (d1 != 0 && d1 != 1) Good++;
			}
		}
	}
	printf("Good %d\n", Good);

#if 0
	{
		glViewport(0, 0, nViewportWidth, nViewportHeight);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, nViewportWidth, 0, nViewportHeight);
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();

		for (int Side = 0; Side < 6; ++Side)
		{
			Wm5::Vector3d DisplayPos;
			if (0 == Side) { DisplayPos.X() = 1; DisplayPos.Y() = 2; }			// Forward
			else if (1 == Side) { DisplayPos.X() = 0; DisplayPos.Y() = 2; }		// Left
			else if (2 == Side) { DisplayPos.X() = 2; DisplayPos.Y() = 2; }		// Right
			else if (3 == Side) { DisplayPos.X() = 1; DisplayPos.Y() = 0; }		// Back
			else if (4 == Side) { DisplayPos.X() = 1; DisplayPos.Y() = 3; }		// Up
			else if (5 == Side) { DisplayPos.X() = 1; DisplayPos.Y() = 1; }		// Down

			glPushAttrib(GL_ALL_ATTRIB_BITS);
				glPointSize(1);
				glBegin(GL_POINTS);
					for (uint32 y = 0; y < CustomViewportSize; ++y) {
						for (uint32 x = 0; x < CustomViewportSize; ++x) {
							//glColor3d(1 - Wm5::Mathd::Pow(pDepths[Side][x + y * CustomViewportSize], 14), 0, 0);
							glColor3d(1 - Wm5::Mathd::Pow(pDepths[Side][x + y * CustomViewportSize], 1), 0, 0);
							glVertex2d(DisplayPos.X() * CustomViewportSize + x, DisplayPos.Y() * CustomViewportSize + y);
						}
					}
				glEnd();
			glPopAttrib();
		}
	}
#endif

	for (int Side = 0; Side < 6; ++Side)
		delete[] pDepths[Side];
	delete[] pNewDepths;

	glViewport(0, 0, nViewportWidth, nViewportHeight);

	return TotalOffset;
}

void Scene::Reset()
{
	// Delete textures
	while (!m_LoadedTextureIds.empty()) {
		glDeleteTextures(1, &(*m_LoadedTextureIds.begin()));
		printf("Unloaded texture with id: %d\n", *m_LoadedTextureIds.begin());
		m_LoadedTextureIds.erase(m_LoadedTextureIds.begin());
	}

	while (!m_Objects.empty()) {
		delete m_Objects.back();
		m_Objects.pop_back();
	}

	if (m_VertexVbo) { glDeleteBuffers(1, &m_VertexVbo); m_VertexVbo = 0; }
	if (m_NormalVbo) { glDeleteBuffers(1, &m_NormalVbo); m_NormalVbo = 0; }
	if (m_ColorVbo) { glDeleteBuffers(1, &m_ColorVbo); m_ColorVbo = 0; }
	if (m_IdColorVbo) { glDeleteBuffers(1, &m_IdColorVbo); m_IdColorVbo = 0; }
	if (m_TextureCoordVbo) { glDeleteBuffers(1, &m_TextureCoordVbo); m_TextureCoordVbo = 0; }
	if (m_LineVertexVbo) { glDeleteBuffers(1, &m_LineVertexVbo); m_LineVertexVbo = 0; }

	delete[] m_pVertexData; m_pVertexData = nullptr;
	delete[] m_pNormalData; m_pNormalData = nullptr;
	delete[] m_pColorData; m_pColorData = nullptr;
	delete[] m_pIdColorData; m_pIdColorData = nullptr;
	delete[] m_pTextureCoordData; m_pTextureCoordData = nullptr;
	delete[] m_pLineVertexData; m_pLineVertexData = nullptr;

	m_NextId = 1;
	m_TrianglesAdded = 0;
	m_TriangleCount = 0;
	m_LinesAdded = 0;
	m_LineCount = 0;

	if (nullptr != MySlide0) {
		MySlide0->SetSelectedObjectId(0);
		MySlide0->m_SlidingObject = 0;
		MySlide0->m_SlidingTriangle = 0;
		MySlide0->m_SelectedTriangle = 0;
	}
}

void Scene::InitializeScene(int SceneNumber)
{
	const std::string ModelsPath = "../Models/";
	std::vector<ModelLoader *> LoadedModels;

	MyScene.Reset();

	if (100 == SceneNumber) {
		//LoadedModels.push_back(new SketchUpModel(ModelsPath + "unit_box.dae"));
		//LoadedModels.push_back(new SketchUpModel(ModelsPath + "complex_shape.dae"));
		//LoadedModels.push_back(new SketchUpModel(ModelsPath + "Ship.dae"));
		LoadedModels.push_back(new SketchUpModel("/Users/Dmitri/Dropbox/Work/2013/GoLand/src/github.com/shurcooL/Hover/vehicle0.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		camera.x = 3.413633; camera.y = -3.883973; camera.z = 3.516000; camera.rh = 322.550000; camera.rv = -33.400000;
	} else
	if (1 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Raised_Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Table.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() += -0.912151; (*it0)->ModifyPosition().Y() += 5.152449; (*it0)->ModifyPosition().Z() += 0.000462;
		++it0; (*it0)->ModifyPosition().X() += -1.502566; (*it0)->ModifyPosition().Y() += 1.668853; (*it0)->ModifyPosition().Z() += 0.000642;

		camera.x = 0.480176; camera.y = -1.309128; camera.z = 3.516000; camera.rh = 325.250000; camera.rv = -27.700000;
	}
	else if (2 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Table.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Picture_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		//LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Vase.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Picture_2.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() += -0.896558; (*it0)->ModifyPosition().Y() += 6.994349; (*it0)->ModifyPosition().Z() += 0.000516;
		++it0; (*it0)->ModifyPosition().X() += -0.432860; (*it0)->ModifyPosition().Y() += 7.532712; (*it0)->ModifyPosition().Z() += 0.000528;
		++it0; (*it0)->ModifyPosition().X() += -1.156365; (*it0)->ModifyPosition().Y() += 7.506348; (*it0)->ModifyPosition().Z() += 0.000427;
		++it0; (*it0)->ModifyPosition().X() += 5.262230; (*it0)->ModifyPosition().Y() += 11.104565; (*it0)->ModifyPosition().Z() += 0.000590;
		++it0; (*it0)->ModifyPosition().X() += -0.076719; (*it0)->ModifyPosition().Y() += 6.990614; (*it0)->ModifyPosition().Z() += 0.920668;
		++it0; (*it0)->ModifyPosition().X() += -0.301037; (*it0)->ModifyPosition().Y() += 6.991926; (*it0)->ModifyPosition().Z() += 0.920906;
		++it0; (*it0)->ModifyPosition().X() += -0.525810; (*it0)->ModifyPosition().Y() += 6.988351; (*it0)->ModifyPosition().Z() += 0.920906;
		++it0; (*it0)->ModifyPosition().X() += 3.945961; (*it0)->ModifyPosition().Y() += 10.815588; (*it0)->ModifyPosition().Z() += 1.893604;
		++it0; (*it0)->ModifyPosition().X() += 4.060388; (*it0)->ModifyPosition().Y() += 10.818698; (*it0)->ModifyPosition().Z() += 1.893519;
		++it0; (*it0)->ModifyPosition().X() += 4.176796; (*it0)->ModifyPosition().Y() += 10.817491; (*it0)->ModifyPosition().Z() += 1.893144;
		++it0; (*it0)->ModifyPosition().X() += 4.290648; (*it0)->ModifyPosition().Y() += 10.816959; (*it0)->ModifyPosition().Z() += 1.893088;
		++it0; (*it0)->ModifyPosition().X() += 0.289435; (*it0)->ModifyPosition().Y() += 11.593086; (*it0)->ModifyPosition().Z() += 1.883482;
		++it0; (*it0)->ModifyPosition().X() += 1.405321; (*it0)->ModifyPosition().Y() += 6.799362; (*it0)->ModifyPosition().Z() += 0.100387;
		++it0; (*it0)->ModifyPosition().X() += 1.798147; (*it0)->ModifyPosition().Y() += 6.799410; (*it0)->ModifyPosition().Z() += 0.100414;
		++it0; (*it0)->ModifyPosition().X() += 1.479604; (*it0)->ModifyPosition().Y() += 7.341168; (*it0)->ModifyPosition().Z() += 0.100260;
		++it0; (*it0)->ModifyPosition().X() += 2.466465; (*it0)->ModifyPosition().Y() += 7.454488; (*it0)->ModifyPosition().Z() += 0.150345;
		//++it0; (*it0)->ModifyPosition().X() += 2.967736; (*it0)->ModifyPosition().Y() += 6.984870; (*it0)->ModifyPosition().Z() += 0.170521;
		++it0; (*it0)->ModifyPosition().X() += -0.458550; (*it0)->ModifyPosition().Y() += 6.360063; (*it0)->ModifyPosition().Z() += 0.470531;

		camera.x = 1.898164; camera.y = 2.479043; camera.z = 4.500000; camera.rh = 355.550000; camera.rv = -29.500000;
	}
	else if (7 == SceneNumber)
	{
		//LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "High Poly Test/Smooth Terrain.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Table.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		//LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Vase.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Picture_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Picture_2.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = -0.842697; (*it0)->ModifyPosition().Y() = 7.060841; (*it0)->ModifyPosition().Z() = 0.410516; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.418917; (*it0)->ModifyPosition().Y() = 7.516844; (*it0)->ModifyPosition().Z() = 0.555528; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -1.142422; (*it0)->ModifyPosition().Y() = 7.490480; (*it0)->ModifyPosition().Z() = 0.555427; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 9.222457; (*it0)->ModifyPosition().Y() = 8.937028; (*it0)->ModifyPosition().Z() = 0.823649; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.080506; (*it0)->ModifyPosition().Y() = 6.990475; (*it0)->ModifyPosition().Z() = 0.920668; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.423625; (*it0)->ModifyPosition().Y() = 6.992346; (*it0)->ModifyPosition().Z() = 1.121319; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.308891; (*it0)->ModifyPosition().Y() = 6.995296; (*it0)->ModifyPosition().Z() = 1.321582; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.195603; (*it0)->ModifyPosition().Y() = 6.994071; (*it0)->ModifyPosition().Z() = 1.121301; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.304824; (*it0)->ModifyPosition().Y() = 6.991787; (*it0)->ModifyPosition().Z() = 0.920906; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -0.529597; (*it0)->ModifyPosition().Y() = 6.988212; (*it0)->ModifyPosition().Z() = 0.920906; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 3.830650; (*it0)->ModifyPosition().Y() = 10.814218; (*it0)->ModifyPosition().Z() = 1.893594; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 3.947174; (*it0)->ModifyPosition().Y() = 10.815449; (*it0)->ModifyPosition().Z() = 1.893604; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 4.061601; (*it0)->ModifyPosition().Y() = 10.818559; (*it0)->ModifyPosition().Z() = 1.893519; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 4.178009; (*it0)->ModifyPosition().Y() = 10.817352; (*it0)->ModifyPosition().Z() = 1.893144; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 4.291861; (*it0)->ModifyPosition().Y() = 10.816820; (*it0)->ModifyPosition().Z() = 1.893088; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 0.285094; (*it0)->ModifyPosition().Y() = 11.588086; (*it0)->ModifyPosition().Z() = 1.880052; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -1.005442; (*it0)->ModifyPosition().Y() = 11.588089; (*it0)->ModifyPosition().Z() = 1.858350; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;

		camera.x = 5.168119; camera.y = 3.964099; camera.z = 7.548000; camera.rh = 43.250000; camera.rv = -43.600000;

		// DEBUG: Select an object
		{ glLoadIdentity(); glRotated(camera.rv + 90, -1, 0, 0); glRotated(camera.rh, 0, 0, 1); glTranslated(-camera.x, -camera.y, -camera.z); MySlide0->PickModelAndComputeUnderCursorPositionAndRayDirection(static_cast<int16>(nViewportWidth * 0.44), static_cast<int16>(nViewportHeight * 0.45)); }
	}
	else if (3 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Horizontal_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Horizontal_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Horizontal_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() += -0.145542; (*it0)->ModifyPosition().Y() += 10.885712; (*it0)->ModifyPosition().Z() += 0.049301;
		//++it0; (*it0)->ModifyPosition().X() += -2.884731; (*it0)->ModifyPosition().Y() += 10.761803; (*it0)->ModifyPosition().Z() += 0.430296;
		//++it0; (*it0)->ModifyPosition().X() += -2.656993; (*it0)->ModifyPosition().Y() += 10.823899; (*it0)->ModifyPosition().Z() += 0.430790;
		++it0; (*it0)->ModifyPosition().X() += -1.223507; (*it0)->ModifyPosition().Y() += 10.887698; (*it0)->ModifyPosition().Z() += 0.540759;
		++it0; (*it0)->ModifyPosition().X() += 0.917612; (*it0)->ModifyPosition().Y() += 10.898662; (*it0)->ModifyPosition().Z() += 0.540773;
		++it0; (*it0)->ModifyPosition().X() += -2.814384; (*it0)->ModifyPosition().Y() += 8.715672; (*it0)->ModifyPosition().Z() += 0.049028;//++it0; (*it0)->ModifyPosition().X() += -2.829703; (*it0)->ModifyPosition().Y() += 8.832493; (*it0)->ModifyPosition().Z() += 0.269960;
		++it0; (*it0)->ModifyPosition().X() += -2.755191; (*it0)->ModifyPosition().Y() += 8.794656; (*it0)->ModifyPosition().Z() += 0.159501;
		++it0; (*it0)->ModifyPosition().X() += -3.284392; (*it0)->ModifyPosition().Y() += 10.734854; (*it0)->ModifyPosition().Z() += 0.430814;
		++it0; (*it0)->ModifyPosition().X() += -3.068752; (*it0)->ModifyPosition().Y() += 10.758643; (*it0)->ModifyPosition().Z() += 0.430829;

		camera.x = -0.219997; camera.y = 3.836598; camera.z = 3.372000; camera.rh = 0.650000; camera.rv = -18.550000;
	}
	/*else if (8 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Horizontal_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Horizontal_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Horizontal_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Shelf_Assembly/Vertical_Part.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() += -0.148388; (*it0)->ModifyPosition().Y() += 10.896099; (*it0)->ModifyPosition().Z() += 1.990325;
		++it0; (*it0)->ModifyPosition().X() += -0.145542; (*it0)->ModifyPosition().Y() += 10.885712; (*it0)->ModifyPosition().Z() += 0.049301;
		++it0; (*it0)->ModifyPosition().X() += -0.144262; (*it0)->ModifyPosition().Y() += 10.900532; (*it0)->ModifyPosition().Z() += 1.019928;
		++it0; (*it0)->ModifyPosition().X() += -1.236003; (*it0)->ModifyPosition().Y() += 10.912714; (*it0)->ModifyPosition().Z() += 1.511279;
		++it0; (*it0)->ModifyPosition().X() += 0.918217; (*it0)->ModifyPosition().Y() += 10.892752; (*it0)->ModifyPosition().Z() += 1.511290;
		++it0; (*it0)->ModifyPosition().X() += -1.223507; (*it0)->ModifyPosition().Y() += 10.887698; (*it0)->ModifyPosition().Z() += 0.540759;
		++it0; (*it0)->ModifyPosition().X() += 0.917612; (*it0)->ModifyPosition().Y() += 10.898662; (*it0)->ModifyPosition().Z() += 0.540773;

		camera.x = -0.219997; camera.y = 3.836598; camera.z = 3.372000; camera.rh = 0.650000; camera.rv = -18.550000;
	}*/
	else if (8 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Teeth/Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Teeth/Teeth.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Teeth/Weight.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Table.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = 11.548100; (*it0)->ModifyPosition().Y() = 5.061959; (*it0)->ModifyPosition().Z() = 0.700025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 8.178390; (*it0)->ModifyPosition().Y() = 8.473480; (*it0)->ModifyPosition().Z() = 0.337525; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 9.177054; (*it0)->ModifyPosition().Y() = 8.499962; (*it0)->ModifyPosition().Z() = 0.410025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 17.369403; (*it0)->ModifyPosition().Y() = 17.381471; (*it0)->ModifyPosition().Z() = 0.555025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 16.342822; (*it0)->ModifyPosition().Y() = 1.253846; (*it0)->ModifyPosition().Z() = 0.555025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = 9.219638; (*it0)->ModifyPosition().Y() = 8.934523; (*it0)->ModifyPosition().Z() = 0.555025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;

		//camera.x = 24.232270; camera.y = 0.621070; camera.z = 3.288000; camera.rh = 306.200000; camera.rv = -13.750000;
		camera.x = 8.103851; camera.y = 8.454115; camera.z = 4.440000; camera.rh = 89.600000; camera.rv = -90.000000;
	}
	else if (4 == SceneNumber)
	{
		double dStartTime = glfwGetTime();

		LoadedModels.push_back(new SketchUpModel("../Models/Table_Chair_Scene/Raised_Platform.dae"));
		//SupermarketModel * pSupermarketModel = new SupermarketModel("../Models/collada_max_models/colladaModel_no_Box07.dae");
		//SupermarketModel * pSupermarketModel = new SupermarketModel("../Models/collada_max_models/colladaModel_no_Box07.dae", SupermarketModelExport());
		SupermarketModel * pSupermarketModel = new SupermarketModel("../Models/collada_max_models/colladaModel_no_Box07.dae", SupermarketModelImport());
		LoadedModels.push_back(pSupermarketModel);
		//pSupermarketModel->PopulateObjects(LoadedModels);

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		//camera.x = 4.181226; camera.y = 0.371067; camera.z = 1.944000; camera.rh = 183.300000; camera.rv = -13.200000;
		//camera.x = 2.385236; camera.y = -6.257709; camera.z = 3.097084; camera.rh = 11.100000; camera.rv = -41.400000;
		//camera.x = 0.288228; camera.y = -0.784797; camera.z = 1.417084; camera.rh = 138.600000; camera.rv = -13.500000;
		camera.x = 0.843329; camera.y = -0.110498; camera.z = 1.645084; camera.rh = 146.850000; camera.rv = -9.796775;

		printf("Loaded MyScene 4 in %f secs.\n", glfwGetTime() - dStartTime);
	}
	else if (5 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Raised_Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_T.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Tri.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Quad.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = -7.424656; (*it0)->ModifyPosition().Y() = -0.021802; (*it0)->ModifyPosition().Z() = 0.394177;
		++it0; (*it0)->ModifyPosition().X() = -7.416107; (*it0)->ModifyPosition().Y() = -0.569511; (*it0)->ModifyPosition().Z() = 0.394158;
		++it0; (*it0)->ModifyPosition().X() = -7.431186; (*it0)->ModifyPosition().Y() = 0.523722; (*it0)->ModifyPosition().Z() = 0.394180;
		++it0; (*it0)->ModifyPosition().X() = -7.440165; (*it0)->ModifyPosition().Y() = 1.065251; (*it0)->ModifyPosition().Z() = 0.394210;
		++it0; (*it0)->ModifyPosition().X() = -5.837915; (*it0)->ModifyPosition().Y() = 1.098874; (*it0)->ModifyPosition().Z() = 0.394226;
		++it0; (*it0)->ModifyPosition().X() = -4.484706; (*it0)->ModifyPosition().Y() = 1.111684; (*it0)->ModifyPosition().Z() = 0.394087;
		++it0; (*it0)->ModifyPosition().X() = -3.201551; (*it0)->ModifyPosition().Y() = 1.088340; (*it0)->ModifyPosition().Z() = 0.394168;

		camera.x = -6.627647; camera.y = 8.685808; camera.z = 5.808000; camera.rh = 170.900000; camera.rv = -52.150000;
	}
	else if (6 == SceneNumber)
	{
		/*LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Raised_Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_T.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Tri.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Quad.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Sputnik.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_2.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = -10.454637; (*it0)->ModifyPosition().Y() = 0.208028; (*it0)->ModifyPosition().Z() = 0.400025;
		++it0; (*it0)->ModifyPosition().X() = -3.759590; (*it0)->ModifyPosition().Y() = 0.222544; (*it0)->ModifyPosition().Z() = 0.400025;
		++it0; (*it0)->ModifyPosition().X() = -8.928240; (*it0)->ModifyPosition().Y() = 0.255272; (*it0)->ModifyPosition().Z() = 0.400025;
		++it0; (*it0)->ModifyPosition().X() = -7.568784; (*it0)->ModifyPosition().Y() = 0.265564; (*it0)->ModifyPosition().Z() = 0.400025;
		++it0; (*it0)->ModifyPosition().X() = -2.467028; (*it0)->ModifyPosition().Y() = 0.347497; (*it0)->ModifyPosition().Z() = 0.400025;
		++it0; (*it0)->ModifyPosition().X() = -4.976653; (*it0)->ModifyPosition().Y() = 0.265757; (*it0)->ModifyPosition().Z() = 0.400025;
		++it0; (*it0)->ModifyPosition().X() = -6.164581; (*it0)->ModifyPosition().Y() = 0.134127; (*it0)->ModifyPosition().Z() = 0.400025;

		camera.x = -4.949073; camera.y = 8.591082; camera.z = 4.353871; camera.rh = 194.200000; camera.rv = -40.600000;*/
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Raised_Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_T.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Tri.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Quad.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Sputnik.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_2.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = -9.327855; (*it0)->ModifyPosition().Y() = 4.296353; (*it0)->ModifyPosition().Z() = 0.200009; (*it0)->ModifyRotation().W() = 0.707107; (*it0)->ModifyRotation().X() = 0.707107; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -3.759590; (*it0)->ModifyPosition().Y() = 0.222544; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -8.928240; (*it0)->ModifyPosition().Y() = 0.255272; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -6.459079; (*it0)->ModifyPosition().Y() = -0.234820; (*it0)->ModifyPosition().Z() = 0.800041; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -2.467028; (*it0)->ModifyPosition().Y() = 0.347497; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -4.976653; (*it0)->ModifyPosition().Y() = 0.265757; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -6.164581; (*it0)->ModifyPosition().Y() = 0.134127; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;

		camera.x = -5.433915; camera.y = 1.391433; camera.z = 1.869871; camera.rh = 209.650000; camera.rv = -30.250000;
	}
	else if (9 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Ship.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();

		camera.x = -0.576260; camera.y = 0.801863; camera.z = 0.729871; camera.rh = 128.800000; camera.rv = -30.400000;
	}
	else if (11 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Table.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Chair.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Picture_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Small_Block.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Book.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Wall_Scene/Picture_2.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Trees/Tree 0.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() += -0.896558; (*it0)->ModifyPosition().Y() += 6.994349; (*it0)->ModifyPosition().Z() += 0.000516;
		++it0; (*it0)->ModifyPosition().X() += -0.432860; (*it0)->ModifyPosition().Y() += 7.532712; (*it0)->ModifyPosition().Z() += 0.000528;
		++it0; (*it0)->ModifyPosition().X() += -1.156365; (*it0)->ModifyPosition().Y() += 7.506348; (*it0)->ModifyPosition().Z() += 0.000427;
		++it0; (*it0)->ModifyPosition().X() += 5.262230; (*it0)->ModifyPosition().Y() += 11.104565; (*it0)->ModifyPosition().Z() += 0.000590;
		++it0; (*it0)->ModifyPosition().X() += -0.076719; (*it0)->ModifyPosition().Y() += 6.990614; (*it0)->ModifyPosition().Z() += 0.920668;
		++it0; (*it0)->ModifyPosition().X() += -0.301037; (*it0)->ModifyPosition().Y() += 6.991926; (*it0)->ModifyPosition().Z() += 0.920906;
		++it0; (*it0)->ModifyPosition().X() += -0.525810; (*it0)->ModifyPosition().Y() += 6.988351; (*it0)->ModifyPosition().Z() += 0.920906;
		++it0; (*it0)->ModifyPosition().X() += 3.945961; (*it0)->ModifyPosition().Y() += 10.815588; (*it0)->ModifyPosition().Z() += 1.893604;
		++it0; (*it0)->ModifyPosition().X() += 4.060388; (*it0)->ModifyPosition().Y() += 10.818698; (*it0)->ModifyPosition().Z() += 1.893519;
		++it0; (*it0)->ModifyPosition().X() += 4.176796; (*it0)->ModifyPosition().Y() += 10.817491; (*it0)->ModifyPosition().Z() += 1.893144;
		++it0; (*it0)->ModifyPosition().X() += 4.290648; (*it0)->ModifyPosition().Y() += 10.816959; (*it0)->ModifyPosition().Z() += 1.893088;
		++it0; (*it0)->ModifyPosition().X() += 0.289435; (*it0)->ModifyPosition().Y() += 11.593086; (*it0)->ModifyPosition().Z() += 1.883482;
		++it0; (*it0)->ModifyPosition().X() += 1.405321; (*it0)->ModifyPosition().Y() += 6.799362; (*it0)->ModifyPosition().Z() += 0.100387;
		++it0; (*it0)->ModifyPosition().X() += 1.798147; (*it0)->ModifyPosition().Y() += 6.799410; (*it0)->ModifyPosition().Z() += 0.100414;
		++it0; (*it0)->ModifyPosition().X() += 1.479604; (*it0)->ModifyPosition().Y() += 7.341168; (*it0)->ModifyPosition().Z() += 0.100260;
		++it0; (*it0)->ModifyPosition().X() += 2.466465; (*it0)->ModifyPosition().Y() += 7.454488; (*it0)->ModifyPosition().Z() += 0.150345;
		++it0; (*it0)->ModifyPosition().X() += -0.458550; (*it0)->ModifyPosition().Y() += 6.360063; (*it0)->ModifyPosition().Z() += 0.470531;
		++it0; (*it0)->ModifyPosition().X() = 0.330811; (*it0)->ModifyPosition().Y() = -0.340123; (*it0)->ModifyPosition().Z() = 3.396533; (*it0)->SetSelectable(false);

		camera.x = 5.995972; camera.y = -3.042715; camera.z = 6.828000; camera.rh = 318.350000; camera.rv = -30.700000;
	}
	else if (0 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Raised_Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_T.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Tri.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Quad.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Sputnik.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_2.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = -7.685461; (*it0)->ModifyPosition().Y() = 1.172332; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 0.965926; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.258819;
		++it0; (*it0)->ModifyPosition().X() = -7.664009; (*it0)->ModifyPosition().Y() = -5.016181; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 0.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = -1.000000;
		++it0; (*it0)->ModifyPosition().X() = -8.402479; (*it0)->ModifyPosition().Y() = -1.301249; (*it0)->ModifyPosition().Z() = 0.942736; (*it0)->ModifyRotation().W() = 0.000000; (*it0)->ModifyRotation().X() = -0.707107; (*it0)->ModifyRotation().Y() = -0.000000; (*it0)->ModifyRotation().Z() = -0.707107;
		++it0; (*it0)->ModifyPosition().X() = -7.879198; (*it0)->ModifyPosition().Y() = -0.319816; (*it0)->ModifyPosition().Z() = 0.600025; (*it0)->ModifyRotation().W() = 0.707107; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.707107; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -7.835089; (*it0)->ModifyPosition().Y() = -6.480685; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -7.876206; (*it0)->ModifyPosition().Y() = -3.380155; (*it0)->ModifyPosition().Z() = 0.400025; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;
		++it0; (*it0)->ModifyPosition().X() = -7.878604; (*it0)->ModifyPosition().Y() = -1.101149; (*it0)->ModifyPosition().Z() = 0.400100; (*it0)->ModifyRotation().W() = 1.000000; (*it0)->ModifyRotation().X() = 0.000000; (*it0)->ModifyRotation().Y() = 0.000000; (*it0)->ModifyRotation().Z() = 0.000000;

		camera.x = -9.255365; camera.y = -5.001759; camera.z = 0.501871; camera.rh = 15.700000; camera.rv = 3.350000;

		// DEBUG: Select an object
		{ glLoadIdentity(); glRotated(camera.rv + 90, -1, 0, 0); glRotated(camera.rh, 0, 0, 1); glTranslated(-camera.x, -camera.y, -camera.z); MySlide0->PickModelAndComputeUnderCursorPositionAndRayDirection(static_cast<int16>(nViewportWidth * 0.5), static_cast<int16>(nViewportHeight * 0.5)); }
	}
	else if (16 == SceneNumber)
	{
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Table_Chair_Scene/Raised_Platform.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_L.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_T.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Tri.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Quad.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Sputnik.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_1.dae"));
		LoadedModels.push_back(new SketchUpModel(ModelsPath + "Puzzle_Box/Piece_Snake_2.dae"));

		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->ReserveObject(MyScene);
		MyScene.DoneReserving();
		for (std::vector<ModelLoader *>::iterator it0 = LoadedModels.begin(); it0 != LoadedModels.end(); ++it0)
			(*it0)->PopulateObject(MyScene);
		MyScene.Finalize();

		std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();
		++it0; (*it0)->ModifyPosition().X() = -4.990611; (*it0)->ModifyPosition().Y() = 5.447445; (*it0)->ModifyPosition().Z() = 1.330075;
		++it0; (*it0)->ModifyPosition().X() = -4.991611; (*it0)->ModifyPosition().Y() = 4.643642; (*it0)->ModifyPosition().Z() = 1.330050;
		++it0; (*it0)->ModifyPosition().X() = -5.592726; (*it0)->ModifyPosition().Y() = 5.247445; (*it0)->ModifyPosition().Z() = 1.530147;
		++it0; (*it0)->ModifyPosition().X() = -5.192611; (*it0)->ModifyPosition().Y() = 4.847420; (*it0)->ModifyPosition().Z() = 1.730075;
		++it0; (*it0)->ModifyPosition().X() = -5.391636; (*it0)->ModifyPosition().Y() = 4.842692; (*it0)->ModifyPosition().Z() = 1.130025;
		++it0; (*it0)->ModifyPosition().X() = -4.990636; (*it0)->ModifyPosition().Y() = 5.247348; (*it0)->ModifyPosition().Z() = 1.130025;
		++it0; (*it0)->ModifyPosition().X() = -5.391726; (*it0)->ModifyPosition().Y() = 5.248348; (*it0)->ModifyPosition().Z() = 1.130050;

		camera.x = -4.949073; camera.y = 8.591082; camera.z = 4.353871; camera.rh = 194.200000; camera.rv = -40.600000;
	}

	while (!LoadedModels.empty()) {
		delete LoadedModels.back();
		LoadedModels.pop_back();
	}

	//StartedSTUDY = false;		// Reset the study
}
