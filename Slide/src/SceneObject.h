#pragma once
#ifndef __SceneObject_H__
#define __SceneObject_H__

class SceneObject
{
public:
	SceneObject(uint16 Id, uint32 StartingIndex, uint32 TriangleCount, uint32 LineIndex, uint32 LineCount, GLuint TextureId, float * pVertexData, float * pNormalData, uint8 * pColorData, float * pTextureCoordData, float * pLineVertexData);
	~SceneObject();

	const uint16 GetId() const { return m_Id; }
	void SetSelectable(bool Selectable) { m_Selectable = Selectable; }
	const bool IsSelectable() const { return m_Selectable; }

	// nMode values:
	// 0 -> Normal rendering mode (for displaying an image)
	// 3 -> Shadow rendering mode (for creating a stencil shadow mask)
	// 4 -> Wireframe rendering mode (for drawing on top of solids)
	// 1 -> Polygon Ids rendering mode (for picking)
	// 2 -> Snap back Ortho rendering mode (with front-face culling enabled for selected object)
	void Render(Scene::RenderMode Mode, Slide * MySlide);
	void RenderLinesTEST(Scene::RenderMode Mode);

	const Wm5::Vector3d GetPosition() const;
	Wm5::Vector3d & ModifyPosition();
	void MoveBy(Wm5::Vector3d oDisplacement);
	const Wm5::Matrix4d GetRotation();// const;
	Wm5::Quaterniond & ModifyRotation();
	Bell & GetTB();
	Bell & ModifyTB();

	Wm5::Tuple<4, double> GetProjectedBoundingBox();
	Wm5::Vector3d GetProjectedOutermostBoundingPoint(Wm5::Vector3d Direction);

	Wm5::Triangle3d		GetTriangle(uint16 TriangleNumber);
	Wm5::Vector3d		GetTriangleNormal(uint16 TriangleNumber);
	uint32				GetTriangleCount() const { return m_TriangleCount; }

	std::set<uint32>	m_oIntersectingTris; // TODO: Change to a bool for optimization?
	Opcode::Model		m_oOpModel;
	Opcode::MeshInterface	m_oOpMesh;
	IceMaths::IndexedTriangle *		m_pIndexedTriangles;

protected:
	SceneObject(const SceneObject &);
	SceneObject & operator =(const SceneObject &);

private:
	uint16			m_Id;

	Wm5::Vector3d		m_Position;
	Wm5::Quaterniond	m_Rotation;
	Bell				m_oTB;

	uint32			m_StartingIndex;
	uint32			m_TriangleCount;

	uint32			m_LineIndex;
	uint32			m_LineCount;

	GLuint			m_TextureId;

	float *			m_pVertexData;
	float *			m_pNormalData;
	uint8 *			m_pColorData;
	float *			m_pTextureCoordData;
	float *			m_pLineVertexData;

	Wm5::Vector3f	m_BoundingBoxMin;
	Wm5::Vector3f	m_BoundingBoxSize;
	bool			m_SmallObject;
	bool			m_Selectable;

	public:std::set<uint16>	m_ContainedObjects;private:
	public:uint16				m_ContainedByObject;private:

	public:Wm5::Vector3d GetBoundingBoxCorner(uint8 Corner);private:
};

#endif // __SceneObject_H__
