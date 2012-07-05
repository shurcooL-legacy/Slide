#pragma once
#ifndef __SketchUpModel_H__
#define __SketchUpModel_H__

class SketchUpModel : public ColladaModel
{
public:
	SketchUpModel(std::string sFilename);
	virtual ~SketchUpModel();

	virtual void ReserveObject(Scene & Scene);
	virtual void PopulateObject(Scene & Scene);

private:
	SketchUpModel(const SketchUpModel &);
	SketchUpModel & operator =(const SketchUpModel &);

	uint32		m_TriangleCount;
	uint32		m_LineCount;
};

#endif // __SketchUpModel_H__
