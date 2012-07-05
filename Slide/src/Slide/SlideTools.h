#pragma once
#ifndef __SlideTools_H__
#define __SlideTools_H__

class SlideTools
{
public:
	SlideTools();
	~SlideTools();

	void ReloadShaders();

private:
	SlideTools(const SlideTools &);
	SlideTools & operator =(const SlideTools &);

	void BuildShaders();
	void DestroyShaders();

	public:GLuint		m_PickingFboId;
	GLuint		m_PickingColorTexId;
	GLuint		m_PickingDepthTexId;

	GLuint		m_QueryId;

	GLuint		m_DepthPeelFboId[2];
	GLuint		m_DepthPeelColorTexId[2];
	GLuint		m_DepthPeelDepthTexId[2];
	GLsizei		m_DepthPeelImageWidth;
	GLsizei		m_DepthPeelImageHeight;

	GLSLProgramObject	m_DepthPeelShader;private:
};

#endif // __SlideTools_H__
