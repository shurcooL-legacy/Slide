#include "../Globals.h"

#include <cstdio>
#include <string>
#include <fstream>

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
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUObject.h"

#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDTexture.h"
#include "FCDocument/FCDImage.h"
#include "FUtils/FUUri.h"

#include "../Wm5/Wm5Math.h"
#include "../Wm5/Wm5Vector4.h"
#include "../Wm5/Wm5Plane3.h"
#include "../Wm5/Wm5Matrix4.h"
#include "../Wm5/Wm5Quaternion.h"
#include "../Wm5/Wm5Triangle3.h"

#include "Opcode.h"

#include "../Arcball/trackball_bell.h"

#include "ModelLoader.h"
#include "ColladaModel.h"
#include "SupermarketModel.h"
#include "../Scene.h"
#include "../SceneObject.h"

SupermarketModel::SupermarketModel(std::string sFilename)
	: ColladaModel(sFilename),
	  m_TriangleCount(0),
	  m_pEntireVertexData(nullptr),
	  m_pEntireNormalData(nullptr),
	  m_pEntireColorData(nullptr),
	  m_pEntireTextureCoordData(nullptr),
	  m_nTrianglesPerMesh(),
	  m_nFirstIndexForMesh(),
	  m_nMeshTexture(),
	  m_oLoadedTextures()
{
	//printf("SupermarketModel(\"%s\") Ctor.\n", sFilename.c_str());

	Initialize(Normal);
}

SupermarketModel::SupermarketModel(std::string sFilename, SupermarketModelExport)
	: ColladaModel(sFilename),
	  m_TriangleCount(0),
	  m_pEntireVertexData(nullptr),
	  m_pEntireNormalData(nullptr),
	  m_pEntireColorData(nullptr),
	  m_pEntireTextureCoordData(nullptr),
	  m_nMeshCount(0),
	  m_nTrianglesPerMesh(),
	  m_nFirstIndexForMesh(),
	  m_nMeshTexture(),
	  m_oLoadedTextures()
{
	//printf("SupermarketModel(\"%s\", SupermarketModelExport) Ctor.\n", sFilename.c_str());

	Initialize(Export, sFilename);
}

SupermarketModel::SupermarketModel(std::string sFilename, SupermarketModelImport)
	: ColladaModel(),
	  m_TriangleCount(0),
	  m_pEntireVertexData(nullptr),
	  m_pEntireNormalData(nullptr),
	  m_pEntireColorData(nullptr),
	  m_pEntireTextureCoordData(nullptr),
	  m_nMeshCount(0),
	  m_nTrianglesPerMesh(),
	  m_nFirstIndexForMesh(),
	  m_nMeshTexture(),
	  m_oLoadedTextures()
{
	//printf("SupermarketModel(\"%s\", SupermarketModelImport) Ctor.\n", sFilename.c_str());

	Initialize(Import, sFilename);
}

SupermarketModel::~SupermarketModel()
{
	delete[] m_pEntireVertexData;
	delete[] m_pEntireNormalData;
	delete[] m_pEntireColorData;
	delete[] m_pEntireTextureCoordData;

	//printf("SupermarketModel() ~Dtor.\n");
}

GLuint SupermarketModel::LoadTextureByName(const fm::string & sName)
{
	FCDImageLibrary * imagelib = m_pDocument->GetImageLibrary();
	int nImageCount = (int)imagelib->GetEntityCount();

	FCDImage * pFoundImage = nullptr;
	for (int nImage = 0; nImage < nImageCount; ++nImage) {
		FCDImage * pImage = imagelib->GetEntity(nImage);
		if (pImage->GetDaeId() == sName) {
			pFoundImage = pImage;
			break;
		}
	}

	if (nullptr == pFoundImage)
		return -1;

	fstring sFilename = pFoundImage->GetFilename().substr(0, pFoundImage->GetFilename().length() - 4) + ".tga";

	return LoadTextureByFilename(sFilename);
}

GLuint SupermarketModel::LoadTextureByFilename(const fstring & sFilename)
{
	// See if the image is already loaded
	for (std::map<fstring, GLuint>::iterator it0 = m_oLoadedTextures.begin(); it0 != m_oLoadedTextures.end(); ++it0) {
		if (it0->first == sFilename)
			return it0->second;
	}

	GLuint nTextureId;
	glGenTextures(1, &nTextureId);
	glBindTexture(GL_TEXTURE_2D, nTextureId);

	if (GL_TRUE == glfwLoadTexture2D(sFilename.c_str(), GLFW_BUILD_MIPMAPS_BIT))
	{
		m_oLoadedTextures.insert(std::pair<fstring, GLuint>(sFilename, nTextureId));

		printf("Loaded texture: %s\n", sFilename.c_str());
		return nTextureId;
	} else {
		printf("WARNING: Failed to load texture: %s\n", sFilename.c_str());
		//exit(111);
		return -1;
	}
}

void SupermarketModel::Initialize(InitializeMode Mode, std::string sFilename)
{
	size_t max_triangles = 0;

	std::fstream * CustomFile = nullptr;
	if (Export == Mode) {
		CustomFile = new std::fstream(sFilename + ".custom", std::fstream::binary | std::fstream::out | std::fstream::trunc);
	} else if (Import == Mode) {
		CustomFile = new std::fstream(sFilename + ".custom", std::fstream::binary | std::fstream::in);
	}

	// Calculate the total triangle count
	m_TriangleCount = 0;
	if (Import != Mode) {
		m_nMeshCount = m_pDocument->GetGeometryLibrary()->GetEntityCount();
		for (size_t nMesh = 0; nMesh < m_nMeshCount; ++nMesh)
		{
			FCDGeometryMesh * pMesh = m_pDocument->GetGeometryLibrary()->GetEntity(nMesh)->GetMesh();

			if (pMesh->IsTriangles())
			{
				size_t nPolygonsCount = m_pDocument->GetGeometryLibrary()->GetEntity(nMesh)->GetMesh()->GetPolygonsCount();
				for (size_t nPolygon = 0; nPolygon < nPolygonsCount; ++nPolygon)
				{
					size_t nTriangleCount = pMesh->GetPolygons(nPolygon)->GetFaceCount();
					m_TriangleCount += nTriangleCount;
				}
			}
		}
	} if (Export == Mode) {
#define CUSTOM_EXPORT(Variable) CustomFile->write(reinterpret_cast<const char *>(&(Variable)), sizeof(Variable));
#define CUSTOM_IMPORT(Variable) CustomFile->read(reinterpret_cast<char *>(&(Variable)), sizeof(Variable));

		CUSTOM_EXPORT(m_TriangleCount);
		CUSTOM_EXPORT(m_nMeshCount);
	} else if (Import == Mode) {
		CUSTOM_IMPORT(m_TriangleCount);
		CUSTOM_IMPORT(m_nMeshCount);
	}

	m_pEntireVertexData = new float[3 * 3 * m_TriangleCount];
	m_pEntireNormalData = new float[3 * 3 * m_TriangleCount];
	m_pEntireColorData = new uint8[3 * 3 * m_TriangleCount];
	m_pEntireTextureCoordData = new float[2 * 3 * m_TriangleCount];

	if (Import != Mode) {
		FCDSceneNode * pNode = m_pDocument->GetVisualSceneRoot();

		size_t nTriangleNumber = 0;
		for (size_t nMesh = 0; nMesh < m_nMeshCount; ++nMesh)
		{
			FCDGeometryMesh * pMesh = m_pDocument->GetGeometryLibrary()->GetEntity(nMesh)->GetMesh();

			if (pMesh->IsTriangles())
			{
				size_t nPolygonsCount = m_pDocument->GetGeometryLibrary()->GetEntity(nMesh)->GetMesh()->GetPolygonsCount();
				for (size_t nPolygon = 0; nPolygon < nPolygonsCount; ++nPolygon)
				{
					size_t nTriangleCount = pMesh->GetPolygons(nPolygon)->GetFaceCount();
					m_nTrianglesPerMesh[nMesh] += nTriangleCount;
					float * pVertexData = pMesh->GetVertexSource(0)->GetData();
					float * pNormalData = pMesh->GetSource(1)->GetData();
					float * pTexCoordsData = pMesh->GetSource(2)->GetData();
					uint32 * pVertexIndicies = pMesh->GetPolygons(nPolygon)->GetInput(0)->GetIndices();
					uint32 * pNormalIndicies = pMesh->GetPolygons(nPolygon)->GetInput(1)->GetIndices();
					uint32 * pTexCoordsIndicies = pMesh->GetPolygons(nPolygon)->GetInput(2)->GetIndices();
					//size_t d = pMesh->GetPolygons(nPolygon)->GetInputCount();
					//printf("d = %d\n", d);

					// Material info
					FCDEffectStandard * effect = nullptr;
					for (size_t nMaterial = 0; nMaterial < m_pDocument->GetMaterialLibrary()->GetEntityCount(); ++nMaterial) {
						if (pMesh->GetPolygons(nPolygon)->GetMaterialSemantic() == m_pDocument->GetMaterialLibrary()->GetEntity(nMaterial)->GetDaeId()) {
							effect = dynamic_cast<FCDEffectStandard *>(m_pDocument->GetMaterialLibrary()->GetEntity(nMaterial)->GetEffect()->GetProfile(0));
							break;
						}
					}
					//if (nullptr == effect) printf("no effect: mesh %d, poly %d\n", nMesh, nPolygon);
					if (nullptr == effect) effect = dynamic_cast<FCDEffectStandard *>(m_pDocument->GetMaterialLibrary()->GetEntity(0)->GetEffect()->GetProfile(0));

					// diffuse texture
					if (1 <= effect->GetTextureCount(FUDaeTextureChannel::DIFFUSE)) {
						FCDTexture * texture = effect->GetTexture(FUDaeTextureChannel::DIFFUSE, 0);
						if (nullptr != texture) {
							FCDImage * image = texture->GetImage();
							if (nullptr != image) {
								GLuint m_texture_diffuse = LoadTextureByName(image->GetDaeId());
								m_nMeshTexture[nMesh] = m_texture_diffuse;
							}
						}
					}

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

					Wm5::Matrix4f oMatrix(pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[0][3],
										  pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[1][3],
										  pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[2][3],
										  pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][0], pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][1], pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][2], pNode->GetChild(nMesh)->CalculateWorldTransform().m[3][3]);
					if (pNode->GetChild(nMesh)->GetChildrenCount() > 0) {
						Wm5::Matrix4f oMatrix2(pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[0][3],
											   pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[1][3],
											   pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[2][3],
											   pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][0], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][1], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][2], pNode->GetChild(nMesh)->GetChild(0)->CalculateLocalTransform().m[3][3]);
						oMatrix = oMatrix2 * oMatrix;
					}

					// Scale down and rotate the entire supermarket model
					{
						//{const double dScale = 0.025; glScaled(dScale, dScale, dScale);}
						//glRotated(90, 1, 0, 0);
						const float fScale = 0.025f;
						Wm5::Matrix4f oMatrix3(fScale, 0, 0, 0,
											   0, 0, fScale, 0,
											   0, -fScale, 0, 0,
											   0, 0, 0, 1);
						oMatrix = oMatrix * oMatrix3;
					}

					for (size_t nTriangle = 0; nTriangle < nTriangleCount; ++nTriangle)
					{
#define PREAPPLYMATRIX
						//glColor3ub(GetId(), nTriangle / 256, nTriangle % 256);
									/*(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0] - Wm5::Mathf::Floor(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0])) * 255,
									(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0] - Wm5::Mathf::Floor(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0])) * 255);*/
						//printf("Triangle %d: vertices %d %d %d\n", nTriangle, pVertexIndicies[3 * nTriangle + 0], pVertexIndicies[3 * nTriangle + 1], pVertexIndicies[3 * nTriangle + 2]);
						//glNormal3f(pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 0],
						//			pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 1],
						//			pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 2]);
						//glVertex3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0],
						//			pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1],
						//			pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2]);
						m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 0] = pTexCoordsData[2 * pTexCoordsIndicies[3 * nTriangle + 0] + 0];
						m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 1] = pTexCoordsData[2 * pTexCoordsIndicies[3 * nTriangle + 0] + 1];
						m_pEntireColorData[3 * 3 * nTriangleNumber + 0] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().x);// + effect->GetAmbientColor().x - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
						m_pEntireColorData[3 * 3 * nTriangleNumber + 1] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().y);// + effect->GetAmbientColor().y - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
						m_pEntireColorData[3 * 3 * nTriangleNumber + 2] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().z);// + effect->GetAmbientColor().z - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
#ifdef PREAPPLYMATRIX
						{Wm5::Vector4f oPoint(pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 0], pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 1], pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 2], 0); oPoint = oPoint * oMatrix; oPoint.Normalize();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 0] = oPoint.X();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 1] = oPoint.Y();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 2] = oPoint.Z();}
						Wm5::Vector4f oPoint1(pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2], 1); oPoint1 = oPoint1 * oMatrix;
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 0] = oPoint1.X();
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 1] = oPoint1.Y();
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 2] = oPoint1.Z();
#else
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 0] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 0];
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 1] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 1];
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 2] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 0] + 2];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 0] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 0];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 1] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 1];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 2] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 0] + 2];
#endif
						//glNormal3f(pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 0],
						//			pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 1],
						//			pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 2]);
						//glVertex3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0],
						//			pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1],
						//			pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2]);
						m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 2] = pTexCoordsData[2 * pTexCoordsIndicies[3 * nTriangle + 1] + 0];
						m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 3] = pTexCoordsData[2 * pTexCoordsIndicies[3 * nTriangle + 1] + 1];
						m_pEntireColorData[3 * 3 * nTriangleNumber + 3] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().x);// + effect->GetAmbientColor().x - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
						m_pEntireColorData[3 * 3 * nTriangleNumber + 4] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().y);// + effect->GetAmbientColor().y - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
						m_pEntireColorData[3 * 3 * nTriangleNumber + 5] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().z);// + effect->GetAmbientColor().z - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
#ifdef PREAPPLYMATRIX
						{Wm5::Vector4f oPoint(pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 0], pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 1], pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 2], 0); oPoint = oPoint * oMatrix; oPoint.Normalize();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 3] = oPoint.X();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 4] = oPoint.Y();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 5] = oPoint.Z();}
						Wm5::Vector4f oPoint2(pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2], 1); oPoint2 = oPoint2 * oMatrix;
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 3] = oPoint2.X();
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 4] = oPoint2.Y();
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 5] = oPoint2.Z();
#else
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 3] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 0];
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 4] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 1];
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 5] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 1] + 2];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 3] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 0];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 4] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 1];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 5] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 1] + 2];
#endif
						//glNormal3f(pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 0],
						//			pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 1],
						//			pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 2]);
						//glVertex3f(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0],
						//			pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1],
						//			pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2]);
						m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 4] = pTexCoordsData[2 * pTexCoordsIndicies[3 * nTriangle + 2] + 0];
						m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 5] = pTexCoordsData[2 * pTexCoordsIndicies[3 * nTriangle + 2] + 1];
						m_pEntireColorData[3 * 3 * nTriangleNumber + 6] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().x);// + effect->GetAmbientColor().x - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
						m_pEntireColorData[3 * 3 * nTriangleNumber + 7] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().y);// + effect->GetAmbientColor().y - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
						m_pEntireColorData[3 * 3 * nTriangleNumber + 8] = static_cast<uint8>(255.999 * effect->GetDiffuseColor().z);// + effect->GetAmbientColor().z - rand()%1000 * 0.0001f;//rand()%1000 * 0.001f;
#ifdef PREAPPLYMATRIX
						{Wm5::Vector4f oPoint(pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 0], pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 1], pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 2], 0); oPoint = oPoint * oMatrix; oPoint.Normalize();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 6] = oPoint.X();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 7] = oPoint.Y();
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 8] = oPoint.Z();}
						Wm5::Vector4f oPoint3(pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1], pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2], 1); oPoint3 = oPoint3 * oMatrix;
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 6] = oPoint3.X();
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 7] = oPoint3.Y();
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 8] = oPoint3.Z();
#else
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 6] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 0];
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 7] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 1];
						m_pEntireNormalData[3 * 3 * nTriangleNumber + 8] = pNormalData[3 * pNormalIndicies[3 * nTriangle + 2] + 2];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 6] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 0];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 7] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 1];
						m_pEntireVertexData[3 * 3 * nTriangleNumber + 8] = pVertexData[3 * pVertexIndicies[3 * nTriangle + 2] + 2];
#endif

						++nTriangleNumber;
					}
				}
			}

			if (nMesh + 1 < m_nMeshCount)
				m_nFirstIndexForMesh[nMesh + 1] = m_nFirstIndexForMesh[nMesh] + 3 * m_nTrianglesPerMesh[nMesh];
		}
	} if (Export == Mode) {
		size_t nTriangleNumber = 0;
		for (size_t nMesh = 0; nMesh < m_nMeshCount; ++nMesh)
		{
			CUSTOM_EXPORT(m_nTrianglesPerMesh[nMesh]);
			CUSTOM_EXPORT(m_nFirstIndexForMesh[nMesh]);

			if (m_nMeshTexture[nMesh]) {
				for (std::map<fstring, GLuint>::iterator it0 = m_oLoadedTextures.begin(); it0 != m_oLoadedTextures.end(); ++it0) {
					if (it0->second == m_nMeshTexture[nMesh]) {
						fstring TextureFilename = it0->first; size_t LastSlash = TextureFilename.find_last_of("\\/"); TextureFilename = TextureFilename.substr(LastSlash);
						size_t Length = TextureFilename.length();
						CUSTOM_EXPORT(Length);
						CustomFile->write(TextureFilename.c_str(), Length);
						break;
					}
				}
			}
			else CUSTOM_EXPORT(m_nMeshTexture[nMesh]);

			for (size_t nTriangle = 0; nTriangle < m_nTrianglesPerMesh[nMesh]; ++nTriangle)
			{
				CUSTOM_EXPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 0]);
				CUSTOM_EXPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 1]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 0]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 1]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 2]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 0]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 1]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 2]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 0]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 1]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 2]);

				CUSTOM_EXPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 2]);
				CUSTOM_EXPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 3]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 3]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 4]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 5]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 3]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 4]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 5]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 3]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 4]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 5]);

				CUSTOM_EXPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 4]);
				CUSTOM_EXPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 5]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 6]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 7]);
				CUSTOM_EXPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 8]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 6]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 7]);
				CUSTOM_EXPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 8]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 6]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 7]);
				CUSTOM_EXPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 8]);

				++nTriangleNumber;
			}
		}
	} else if (Import == Mode) {
		size_t nTriangleNumber = 0;
		for (size_t nMesh = 0; nMesh < m_nMeshCount; ++nMesh)
		{
			CUSTOM_IMPORT(m_nTrianglesPerMesh[nMesh]);
			CUSTOM_IMPORT(m_nFirstIndexForMesh[nMesh]);

			size_t Length;
			CUSTOM_IMPORT(Length);
			if (Length) {
				char * szFilename = new char[Length + 1]; szFilename[Length] = '\0';
				CustomFile->read(szFilename, Length);
				size_t LastSlash = sFilename.find_last_of("\\/"); fstring TextureFilename = sFilename.substr(0, LastSlash + 1).c_str(); TextureFilename += szFilename;
				delete szFilename;
				m_nMeshTexture[nMesh] = LoadTextureByFilename(TextureFilename);
			}

			for (size_t nTriangle = 0; nTriangle < m_nTrianglesPerMesh[nMesh]; ++nTriangle)
			{
				CUSTOM_IMPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 0]);
				CUSTOM_IMPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 1]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 0]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 1]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 2]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 0]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 1]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 2]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 0]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 1]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 2]);

				CUSTOM_IMPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 2]);
				CUSTOM_IMPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 3]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 3]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 4]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 5]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 3]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 4]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 5]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 3]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 4]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 5]);

				CUSTOM_IMPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 4]);
				CUSTOM_IMPORT(m_pEntireTextureCoordData[2 * 3 * nTriangleNumber + 5]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 6]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 7]);
				CUSTOM_IMPORT(m_pEntireColorData[3 * 3 * nTriangleNumber + 8]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 6]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 7]);
				CUSTOM_IMPORT(m_pEntireNormalData[3 * 3 * nTriangleNumber + 8]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 6]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 7]);
				CUSTOM_IMPORT(m_pEntireVertexData[3 * 3 * nTriangleNumber + 8]);

				++nTriangleNumber;
			}
		}
	}

	printf("m_TriangleCount = %d\n", m_TriangleCount);
	printf("m_oLoadedTextures.size() = %d\n", m_oLoadedTextures.size());
	for (size_t nMesh = 0; nMesh < m_nMeshCount; ++nMesh)
		max_triangles = std::max<size_t>(max_triangles, m_nTrianglesPerMesh[nMesh]);
	printf("max_triangles = %d\n", max_triangles);

	// OPCODE
	/*m_pIndexedTriangles = new IceMaths::IndexedTriangle[m_TriangleCount];
	for (size_t nTriangle = 0; nTriangle < m_TriangleCount; ++nTriangle) {
		m_pIndexedTriangles[nTriangle].mVRef[0] = 3 * nTriangle + 0;
		m_pIndexedTriangles[nTriangle].mVRef[1] = 3 * nTriangle + 1;
		m_pIndexedTriangles[nTriangle].mVRef[2] = 3 * nTriangle + 2;
	}
	m_oOpMesh.SetNbTriangles(0);
	m_oOpMesh.SetNbVertices(0);
	m_oOpMesh.SetPointers(m_pIndexedTriangles, reinterpret_cast<const IceMaths::Point *>(m_pEntireVertexData));
	Opcode::OPCODECREATE Create;
	Create.mIMesh			= &m_oOpMesh;
	Create.mSettings.mLimit	= 1;
	Create.mSettings.mRules	= Opcode::SPLIT_SPLATTER_POINTS | Opcode::SPLIT_GEOM_CENTER;
	Create.mNoLeaf			= true;
	Create.mQuantized		= true;
	Create.mKeepOriginal	= false;
	Create.mCanRemap		= false;
	m_oOpModel.Build(Create);*/

	if (CustomFile) {
		CustomFile->close();
		delete CustomFile;
	}
}

void SupermarketModel::ReserveObject(Scene & Scene)
{
	Scene.ReserveObject(m_TriangleCount, 0);
}

void SupermarketModel::PopulateObject(Scene & Scene)
{
	// Populate Object
	float * pVertexData, * pNormalData, * pTextureCoordData, * pLineVertexData;
	uint8 * pColorData;
	Scene.PopulateObjectPointers(pVertexData, pNormalData, pColorData, pTextureCoordData, pLineVertexData);
	memcpy(pVertexData, m_pEntireVertexData, 3 * 3 * m_TriangleCount * sizeof(*m_pEntireVertexData));
	memcpy(pNormalData, m_pEntireNormalData, 3 * 3 * m_TriangleCount * sizeof(*m_pEntireNormalData));
	memcpy(pColorData, m_pEntireColorData, 3 * 3 * m_TriangleCount * sizeof(*m_pEntireColorData));
	memcpy(pTextureCoordData, m_pEntireTextureCoordData, 2 * 3 * m_TriangleCount * sizeof(*m_pEntireTextureCoordData));
	//Scene.PopulatedObject(m_TriangleCount);
	for (size_t nMesh = 0; nMesh < m_nMeshCount; ++nMesh)
		Scene.PopulatedObject(m_nTrianglesPerMesh[nMesh], 0, m_nMeshTexture[nMesh]);
}
