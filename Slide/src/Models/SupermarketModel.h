#pragma once
#ifndef __SupermarketModel_H__
#define __SupermarketModel_H__

class SupermarketModelExport {};
class SupermarketModelImport {};

class SupermarketModel : public ColladaModel
{
public:
	SupermarketModel(std::string sFilename);
	SupermarketModel(std::string sFilename, SupermarketModelExport);
	SupermarketModel(std::string sFilename, SupermarketModelImport);
	virtual ~SupermarketModel();

	virtual void ReserveObject(Scene & Scene);
	virtual void PopulateObject(Scene & Scene);

private:
	SupermarketModel(const SupermarketModel &);
	SupermarketModel & operator =(const SupermarketModel &);

	GLuint LoadTextureByName(const fm::string & sName);
	GLuint LoadTextureByFilename(const fstring & sFilename);
	enum InitializeMode { Normal, Export, Import };
	void Initialize(InitializeMode Mode, std::string sFilename = "");

	uint32			m_TriangleCount;
	float *			m_pEntireVertexData;
	float *			m_pEntireNormalData;
	uint8 *			m_pEntireColorData;
	float *			m_pEntireTextureCoordData;

	size_t			m_nMeshCount;
	size_t			m_nTrianglesPerMesh[5000];
	size_t			m_nFirstIndexForMesh[5000];
	GLuint			m_nMeshTexture[5000];
	std::map<fstring, GLuint>	m_oLoadedTextures;
};

#endif // __SupermarketModel_H__
