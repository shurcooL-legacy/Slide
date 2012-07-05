#pragma once
#ifndef __Scene_H__
#define __Scene_H__

class Scene
{
public:
	Scene();
	~Scene();

	void ReserveObject(uint32 TriangleCount, uint32 LineCount);
	void DoneReserving();
	void PopulateObjectPointers(float *& pVertexData, float *& pNormalData, uint8 *& pColorData, float *& pTextureCoordData, float *& pLineVertexData);
	void PopulatedObject(uint32 TriangleCount, uint32 LineCount, GLuint TextureId = 0);
	void Finalize();

	SceneObject & GetObject(uint16 Id);

	enum RenderMode { VisibleGeometry, VisibleGeometryWithBoundingBoxes, ObjectPicking, SelectedObjectVisibilityQuery, StaticSceneGeometry, StaticSceneGeometryBackFace, StaticCollidingSceneGeometry, SelectedObjectClosestFrontFace, SelectedObjectFurthestFrontFace, SelectedObjectClosestBackFace, SelectedObjectFurthestBackFace, IntersectingTrianglesTEST, BoundingBox, ShadowTEST };
	void Render(RenderMode Mode, Slide * MySlide);

	Wm5::Vector3d CalculateNavigationOffset();

	void Reset();

	static void InitializeScene(int SceneNumber);

private:
	Scene(const Scene &);
	Scene & operator =(const Scene &);

	uint16		m_NextId;
	uint32		m_TrianglesAdded;
	uint32		m_LinesAdded;

	uint32		m_TriangleCount;
	uint32		m_LineCount;

	public:std::vector<SceneObject *> m_Objects;private:
	std::set<GLuint>		m_LoadedTextureIds;

	//uint16		m_SelectedObjectId;

	GLuint		m_VertexVbo;
	GLuint		m_NormalVbo;
	GLuint		m_ColorVbo;
	GLuint		m_IdColorVbo;
	GLuint		m_TextureCoordVbo;
	GLuint		m_LineVertexVbo;

	float *		m_pVertexData;
	float *		m_pNormalData;
	uint8 *		m_pColorData;
	uint8 *		m_pIdColorData;
	float *		m_pTextureCoordData;
	float *		m_pLineVertexData;
};

#endif // __Scene_H__
