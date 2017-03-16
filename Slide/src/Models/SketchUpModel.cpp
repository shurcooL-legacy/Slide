#include "../Globals.h"

#include <cstdio>
#include <string>

#pragma warning(disable : 4351)

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
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FUtils/FUObject.h"
#include "FUtils/FUUri.h"

#include "../Wm5/Wm5Math.h"
#include "../Wm5/Wm5Vector4.h"
#include "../Wm5/Wm5Matrix4.h"
#include "../Wm5/Wm5Plane3.h"
#include "../Wm5/Wm5Quaternion.h"
#include "../Wm5/Wm5Triangle3.h"

#include "Opcode.h"

#include "../Arcball/trackball_bell.h"

#include "ModelLoader.h"
#include "ColladaModel.h"
#include "SketchUpModel.h"
#include "../Scene.h"
#include "../SceneObject.h"

SketchUpModel::SketchUpModel(std::string sFilename)
	: ColladaModel(sFilename),
	  m_TriangleCount(0),
	  m_LineCount(0)
{
	//printf("SketchUpModel(\"%s\") Ctor.\n", sFilename.c_str());
}

SketchUpModel::~SketchUpModel()
{
	//printf("SketchUpModel() ~Dtor.\n");
}

// Returns true if a triangle is degenerate (i.e. zero area)
bool IsTriangleDegenerate(Wm5::Triangle3f & Triangle)
{
	float Cross = (Triangle.V[1] - Triangle.V[0]).Cross(Triangle.V[2] - Triangle.V[0]).SquaredLength();
	return (0.000000000001f >= Cross);
}

void SketchUpModel::ReserveObject(Scene & Scene)
{
	// Calculate the total triangle and line counts
	m_TriangleCount = 0;
	m_LineCount = 0;

	size_t nMeshCount = m_pDocument->GetGeometryLibrary()->GetEntityCount();
	for (size_t nMesh = 0; nMesh < nMeshCount; ++nMesh)
	{
		FCDGeometryMesh * pMesh = m_pDocument->GetGeometryLibrary()->GetEntity(nMesh)->GetMesh();

		size_t nPolygonsCount = pMesh->GetPolygonsCount();
		for (size_t nPolygon = 0; nPolygon < nPolygonsCount; ++nPolygon)
		{
			if (pMesh->GetPolygons(nPolygon)->IsTriangles())
			{
				size_t nTriangleCount = pMesh->GetPolygons(nPolygon)->GetFaceCount();
				//float * pVertexData = pMesh->GetVertexSource(0)->GetData();
				float * pVertexData = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetSource()->GetData();
				uint32 * pVertexIndicies = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndices();

				for (size_t nTriangle = 0; nTriangle < nTriangleCount; ++nTriangle)
				{
					// Check for degenerate (i.e. zero area) triangles
					Wm5::Triangle3f Triangle(Wm5::Vector3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2]),
											 Wm5::Vector3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2]),
											 Wm5::Vector3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2]));
					bool DegenerateTriangle = IsTriangleDegenerate(Triangle);

					if (DegenerateTriangle) printf("Warning:\n \"%s\":\n  Zero-area triangle %d found and ignored!\n", m_pDocument->GetFileUrl().c_str(), nTriangle);
					else
						++m_TriangleCount;
				}
			}
			else if (FCDGeometryPolygons::PrimitiveType::LINES == pMesh->GetPolygons(nPolygon)->GetPrimitiveType())
			{
				size_t nLineCount = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndexCount() / 2;
				//float * pVertexData = pMesh->GetVertexSource(0)->GetData();
				float * pVertexData = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetSource()->GetData();
				uint32 * pVertexIndicies = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndices();

				for (size_t nLine = 0; nLine < nLineCount; ++nLine)
				{
					// Check for degenerate (i.e. zero length) lines
					// TODO: Finish that
					if (false) printf("Warning:\n \"%s\":\n  Zero-length line %d found and ignored!\n", m_pDocument->GetFileUrl().c_str(), nLine);
					else
						++m_LineCount;
				}
			}
		}
	}

	printf("m_TriangleCount = %d, m_LineCount = %d\n", m_TriangleCount, m_LineCount);

	Scene.ReserveObject(m_TriangleCount, m_LineCount);
}

void SketchUpModel::PopulateObject(Scene & Scene)
{
	float * pEntireVertexData = new float[3 * 3 * m_TriangleCount];
	float * pEntireNormalData = new float[3 * 3 * m_TriangleCount];
	uint8 * pEntireColorData = new uint8[3 * 3 * m_TriangleCount];
	float * pEntireLineVertexData = new float[3 * 3 * m_LineCount];
	size_t nTriangleNumber = 0;
	size_t nLineNumber = 0;

	FCDSceneNode * pNode = m_pDocument->GetVisualSceneRoot();

	size_t nMeshCount = m_pDocument->GetGeometryLibrary()->GetEntityCount();
	for (size_t nMesh = 0; nMesh < nMeshCount; ++nMesh)
	{
		FCDGeometryMesh * pMesh = m_pDocument->GetGeometryLibrary()->GetEntity(nMesh)->GetMesh();

		size_t nPolygonsCount = pMesh->GetPolygonsCount();
		for (size_t nPolygon = 0; nPolygon < nPolygonsCount; ++nPolygon)
		{
			if (pMesh->GetPolygons(nPolygon)->IsTriangles())
			{
				size_t nTriangleCount = pMesh->GetPolygons(nPolygon)->GetFaceCount();
				//float * pVertexData = pMesh->GetVertexSource(0)->GetData();
				float * pVertexData = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetSource()->GetData();
				//float * pNormalData = pMesh->GetSource(1)->GetData();
				float * pNormalData = pMesh->GetPolygons(nPolygon)->GetInput(1)->GetSource()->GetData();
				uint32 * pVertexIndicies = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndices();
				uint32 * pNormalIndicies = pMesh->GetPolygons(nPolygon)->GetInput(1)->GetIndices();
				//size_t d = pMesh->GetPolygons(nPolygon)->GetInputCount();
				//printf("d = %d\n", d);

				// Material info
				FCDEffectStandard * effect = nullptr;
				for (size_t nGeometryInstance = 0; nGeometryInstance < m_pDocument->GetVisualSceneLibrary()->GetEntity(0)->GetChild(0)->GetInstanceCount(); ++nGeometryInstance) {
					if (pMesh->GetDaeId() == m_pDocument->GetVisualSceneLibrary()->GetEntity(0)->GetChild(0)->GetInstance(nGeometryInstance)->GetEntity()->GetDaeId()
						&& FCDEntityInstance::Type::GEOMETRY == m_pDocument->GetVisualSceneLibrary()->GetEntity(0)->GetChild(0)->GetInstance(nGeometryInstance)->GetType()
						&& 1 <= reinterpret_cast<FCDGeometryInstance *>(m_pDocument->GetVisualSceneLibrary()->GetEntity(0)->GetChild(0)->GetInstance(nGeometryInstance))->GetMaterialInstanceCount())
					{
						effect = dynamic_cast<FCDEffectStandard *>(reinterpret_cast<FCDGeometryInstance *>(m_pDocument->GetVisualSceneLibrary()->GetEntity(0)->GetChild(0)->GetInstance(nGeometryInstance))->GetMaterialInstance(0)->GetMaterial()->GetEffect()->GetProfile(0));
						break;
					}
				}
				//if (nullptr == effect) printf("no effect: mesh %d, poly %d\n", nMesh, nPolygon);
				if (nullptr == effect) effect = dynamic_cast<FCDEffectStandard *>(m_pDocument->GetMaterialLibrary()->GetEntity(0)->GetEffect()->GetProfile(0));

				//if (pMesh->GetSourceCount() != 3 || 1 != pMesh->GetVertexSourceCount())
				//	printf("wth %d source, %d vertex source\n", pMesh->GetSourceCount(), pMesh->GetVertexSourceCount());

				/*size_t x = 0;
				FCDEntityInstance * inst = nullptr;
				if (pNode->GetChild(nMesh)->GetChildrenCount() == 0)
					inst = pNode->GetChild(nMesh)->GetInstance(0);
				else
					inst = pNode->GetChild(nMesh)->GetChild(0)->GetInstance(0);
				if (nMesh < 50) {
					//printf("%d\n", x);
					//printf("%s\n", inst->GetEntityReference()->GetObjectType().GetTypeName());
					printf("%d: %s\n", nMesh, !pNode->GetChild(nMesh)->GetChildrenCount() ? pNode->GetChild(nMesh)->GetDaeId().c_str() : pNode->GetChild(nMesh)->GetChild(0)->GetDaeId().c_str());
					printf("    %s\n", pMesh->GetPolygons(nPolygon)->GetMaterialSemantic().c_str());
					
					for (size_t Material = 0; Material < m_pDocument->GetMaterialLibrary()->GetEntityCount(); ++Material)
					{
						if (m_pDocument->GetMaterialLibrary()->GetEntity(Material)->GetDaeId() == pMesh->GetPolygons(nPolygon)->GetMaterialSemantic())
						{
							FCDEffectStandard * standardProfile = dynamic_cast<FCDEffectStandard *>(m_pDocument->GetMaterialLibrary()->GetEntity(Material)->GetEffect()->GetProfile(0));
							printf("         DiffuseColor().x = %f\n", standardProfile->GetDiffuseColor().x);
						}
					}
				}*/
				//printf("GetEffectParameterCount %d\n", m_pDocument->GetEffectLibrary()->GetEntity(400)->GetEffectParameterCount());

				/*Wm5::Matrix4f oMatrix(pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][3],
									  pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][3],
									  pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][3],
									  pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][3]);
				if (pNode->GetChild(nMesh)->GetChildrenCount() > 0) {
					Wm5::Matrix4f oMatrix1(pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][3],
										   pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][3],
										   pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][3],
										   pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][3]);
					oMatrix = oMatrix1 * oMatrix;
				}*/
				Wm5::Matrix4f oMatrix(Wm5::Matrix4f::IDENTITY);
				if (fstring::npos != m_pDocument->GetAsset()->GetContributor(0)->GetAuthoringTool().find("Google SketchUp 8"))
					for (uint8 i = 0; i < 3; ++i)
						oMatrix[i][i] *= 0.4f;

				for (size_t nTriangle = 0; nTriangle < nTriangleCount; ++nTriangle)
				{
					// Check for and skip degenerate (i.e. zero area) triangles
					{
						Wm5::Triangle3f Triangle(Wm5::Vector3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2]),
												 Wm5::Vector3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2]),
												 Wm5::Vector3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2]));
						bool DegenerateTriangle = IsTriangleDegenerate(Triangle);

						if (DegenerateTriangle) 
							continue;
					}

#define PREAPPLYMATRIX
					pEntireColorData[3 * 3 * nTriangleNumber + 0] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().x);// + effect->GetAmbientColor().x - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
					pEntireColorData[3 * 3 * nTriangleNumber + 1] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().y);// + effect->GetAmbientColor().y - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
					pEntireColorData[3 * 3 * nTriangleNumber + 2] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().z);// + effect->GetAmbientColor().z - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
#ifdef PREAPPLYMATRIX
					{Wm5::Vector4f oPoint(pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 0], pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 1], pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 2], 0); oPoint = oPoint * oMatrix; oPoint.Normalize();
					pEntireNormalData[3 * 3 * nTriangleNumber + 0] = oPoint.X();
					pEntireNormalData[3 * 3 * nTriangleNumber + 1] = oPoint.Y();
					pEntireNormalData[3 * 3 * nTriangleNumber + 2] = oPoint.Z();}
					Wm5::Vector4f oPoint1(pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2], 1); oPoint1 = oPoint1 * oMatrix;
					pEntireVertexData[3 * 3 * nTriangleNumber + 0] = oPoint1.X();
					pEntireVertexData[3 * 3 * nTriangleNumber + 1] = oPoint1.Y();
					pEntireVertexData[3 * 3 * nTriangleNumber + 2] = oPoint1.Z();
#else
					pEntireNormalData[3 * 3 * nTriangleNumber + 0] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 0];
					pEntireNormalData[3 * 3 * nTriangleNumber + 1] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 1];
					pEntireNormalData[3 * 3 * nTriangleNumber + 2] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 2];
					pEntireVertexData[3 * 3 * nTriangleNumber + 0] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0];
					pEntireVertexData[3 * 3 * nTriangleNumber + 1] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1];
					pEntireVertexData[3 * 3 * nTriangleNumber + 2] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2];
#endif
					pEntireColorData[3 * 3 * nTriangleNumber + 3] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().x);// + effect->GetAmbientColor().x - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
					pEntireColorData[3 * 3 * nTriangleNumber + 4] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().y);// + effect->GetAmbientColor().y - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
					pEntireColorData[3 * 3 * nTriangleNumber + 5] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().z);// + effect->GetAmbientColor().z - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
#ifdef PREAPPLYMATRIX
					{Wm5::Vector4f oPoint(pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 0], pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 1], pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 2], 0); oPoint = oPoint * oMatrix; oPoint.Normalize();
					pEntireNormalData[3 * 3 * nTriangleNumber + 3] = oPoint.X();
					pEntireNormalData[3 * 3 * nTriangleNumber + 4] = oPoint.Y();
					pEntireNormalData[3 * 3 * nTriangleNumber + 5] = oPoint.Z();}
					Wm5::Vector4f oPoint2(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2], 1); oPoint2 = oPoint2 * oMatrix;
					pEntireVertexData[3 * 3 * nTriangleNumber + 3] = oPoint2.X();
					pEntireVertexData[3 * 3 * nTriangleNumber + 4] = oPoint2.Y();
					pEntireVertexData[3 * 3 * nTriangleNumber + 5] = oPoint2.Z();
#else
					pEntireNormalData[3 * 3 * nTriangleNumber + 3] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 0];
					pEntireNormalData[3 * 3 * nTriangleNumber + 4] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 1];
					pEntireNormalData[3 * 3 * nTriangleNumber + 5] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 2];
					pEntireVertexData[3 * 3 * nTriangleNumber + 3] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0];
					pEntireVertexData[3 * 3 * nTriangleNumber + 4] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1];
					pEntireVertexData[3 * 3 * nTriangleNumber + 5] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2];
#endif
					pEntireColorData[3 * 3 * nTriangleNumber + 6] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().x);// + effect->GetAmbientColor().x - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
					pEntireColorData[3 * 3 * nTriangleNumber + 7] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().y);// + effect->GetAmbientColor().y - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
					pEntireColorData[3 * 3 * nTriangleNumber + 8] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().z);// + effect->GetAmbientColor().z - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
#ifdef PREAPPLYMATRIX
					{Wm5::Vector4f oPoint(pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 0], pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 1], pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 2], 0); oPoint = oPoint * oMatrix; oPoint.Normalize();
					pEntireNormalData[3 * 3 * nTriangleNumber + 6] = oPoint.X();
					pEntireNormalData[3 * 3 * nTriangleNumber + 7] = oPoint.Y();
					pEntireNormalData[3 * 3 * nTriangleNumber + 8] = oPoint.Z();}
					Wm5::Vector4f oPoint3(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2], 1); oPoint3 = oPoint3 * oMatrix;
					pEntireVertexData[3 * 3 * nTriangleNumber + 6] = oPoint3.X();
					pEntireVertexData[3 * 3 * nTriangleNumber + 7] = oPoint3.Y();
					pEntireVertexData[3 * 3 * nTriangleNumber + 8] = oPoint3.Z();
#else
					pEntireNormalData[3 * 3 * nTriangleNumber + 6] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 0];
					pEntireNormalData[3 * 3 * nTriangleNumber + 7] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 1];
					pEntireNormalData[3 * 3 * nTriangleNumber + 8] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 2];
					pEntireVertexData[3 * 3 * nTriangleNumber + 6] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0];
					pEntireVertexData[3 * 3 * nTriangleNumber + 7] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1];
					pEntireVertexData[3 * 3 * nTriangleNumber + 8] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2];
#endif

					++nTriangleNumber;
				}
			}
			else if (FCDGeometryPolygons::PrimitiveType::LINES == pMesh->GetPolygons(nPolygon)->GetPrimitiveType())
			{
				size_t nLineCount = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndexCount() / 2;
				//float * pLineVertexData = pMesh->GetVertexSource(0)->GetData();
				float * pLineVertexData = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetSource()->GetData();
				uint32 * pLineVertexIndicies = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndices();

				Wm5::Matrix4f oMatrix(Wm5::Matrix4f::IDENTITY);
				if (fstring::npos != m_pDocument->GetAsset()->GetContributor(0)->GetAuthoringTool().find("Google SketchUp 8"))
					for (uint8 i = 0; i < 3; ++i)
						oMatrix[i][i] *= 0.4f;

				for (size_t nLine = 0; nLine < nLineCount; ++nLine)
				{
#ifdef PREAPPLYMATRIX
					{Wm5::Vector4f oPoint(pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 0] + 0], pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 0] + 1], pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 0] + 2], 0); oPoint = oPoint * oMatrix;
					pEntireLineVertexData[2 * 3 * nLineNumber + 0] = oPoint.X();
					pEntireLineVertexData[2 * 3 * nLineNumber + 1] = oPoint.Y();
					pEntireLineVertexData[2 * 3 * nLineNumber + 2] = oPoint.Z();}

					{Wm5::Vector4f oPoint(pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 1] + 0], pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 1] + 1], pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 1] + 2], 0); oPoint = oPoint * oMatrix;
					pEntireLineVertexData[2 * 3 * nLineNumber + 3] = oPoint.X();
					pEntireLineVertexData[2 * 3 * nLineNumber + 4] = oPoint.Y();
					pEntireLineVertexData[2 * 3 * nLineNumber + 5] = oPoint.Z();}
#else
					pEntireLineVertexData[2 * 3 * nLineNumber + 0] = pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 0] + 0];
					pEntireLineVertexData[2 * 3 * nLineNumber + 1] = pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 0] + 1];
					pEntireLineVertexData[2 * 3 * nLineNumber + 2] = pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 0] + 2];

					pEntireLineVertexData[2 * 3 * nLineNumber + 3] = pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 1] + 0];
					pEntireLineVertexData[2 * 3 * nLineNumber + 4] = pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 1] + 1];
					pEntireLineVertexData[2 * 3 * nLineNumber + 5] = pLineVertexData[3 * pLineVertexIndicies[2 * nLine + 1] + 2];
#endif

					++nLineNumber;
				}
			}
		}
	}

	// Populate Object
	float * pVertexData, * pNormalData, * pTextureCoordData, * pLineVertexData;
	uint8 * pColorData;
	Scene.PopulateObjectPointers(pVertexData, pNormalData, pColorData, pTextureCoordData, pLineVertexData);
	memcpy(pVertexData, pEntireVertexData, 3 * 3 * m_TriangleCount * sizeof(*pEntireVertexData));
	memcpy(pNormalData, pEntireNormalData, 3 * 3 * m_TriangleCount * sizeof(*pEntireNormalData));
	memcpy(pColorData, pEntireColorData, 3 * 3 * m_TriangleCount * sizeof(*pEntireColorData));
	memset(pTextureCoordData, 0, 2 * 3 * m_TriangleCount * sizeof(*pTextureCoordData));
	memcpy(pLineVertexData, pEntireLineVertexData, 2 * 3 * m_LineCount * sizeof(*pEntireLineVertexData));
	Scene.PopulatedObject(m_TriangleCount, m_LineCount);

	delete[] pEntireVertexData;
	delete[] pEntireNormalData;
	delete[] pEntireColorData;
	delete[] pEntireLineVertexData;
}
