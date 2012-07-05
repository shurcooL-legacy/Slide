#pragma once
#ifndef __ModelLoader_H__
#define __ModelLoader_H__

class ModelLoader
{
public:
	ModelLoader();
	virtual ~ModelLoader();

	virtual void ReserveObject(Scene & Scene) = 0;
	virtual void PopulateObject(Scene & Scene) = 0;

private:
	ModelLoader(const ModelLoader &);
	ModelLoader & operator =(const ModelLoader &);
};

#endif // __ModelLoader_H__
