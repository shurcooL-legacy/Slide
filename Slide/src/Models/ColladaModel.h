#pragma once
#ifndef __ColladaModel_H__
#define __ColladaModel_H__

class ColladaModel : public ModelLoader
{
public:
	ColladaModel(std::string sFilename);
	ColladaModel();
	virtual ~ColladaModel();

protected:
	FCDocument *	m_pDocument;

private:
	ColladaModel(const ColladaModel &);
	ColladaModel & operator =(const ColladaModel &);
};

#endif // __ColladaModel_H__
