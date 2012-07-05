#include "../Globals.h"

SlideTools::SlideTools()
{
	glGenQueries(1, &m_QueryId);

	// Initialize FBOs (Render Targets)
	{
		glGenFramebuffers(1, &m_PickingFboId);
		glGenTextures(1, &m_PickingColorTexId);
		glGenTextures(1, &m_PickingDepthTexId);

		{
			glBindTexture(GL_TEXTURE_2D, m_PickingColorTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			glBindTexture(GL_TEXTURE_2D, m_PickingDepthTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			glBindFramebuffer(GL_FRAMEBUFFER, m_PickingFboId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PickingColorTexId, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_PickingDepthTexId, 0);
		}
	}

	{
		//m_DepthPeelImageWidth = nViewportWidth;
		//m_DepthPeelImageHeight = nViewportHeight;
		m_DepthPeelImageWidth = std::min<uint32>(400, nViewportWidth);
		m_DepthPeelImageHeight = m_DepthPeelImageWidth * nViewportHeight / nViewportWidth;

		glGenFramebuffers(2, m_DepthPeelFboId);
		glGenTextures(2, m_DepthPeelColorTexId);
		glGenTextures(2, m_DepthPeelDepthTexId);

		for (int Texture = 0; Texture < 2; ++Texture)
		{
			glBindTexture(GL_TEXTURE_RECTANGLE, m_DepthPeelColorTexId[Texture]);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, m_DepthPeelImageWidth, m_DepthPeelImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			glBindTexture(GL_TEXTURE_RECTANGLE, m_DepthPeelDepthTexId[Texture]);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT32F, m_DepthPeelImageWidth, m_DepthPeelImageHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			glBindFramebuffer(GL_FRAMEBUFFER, m_DepthPeelFboId[Texture]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, m_DepthPeelColorTexId[Texture], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, m_DepthPeelDepthTexId[Texture], 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		CHECK_GL_ERRORS
	}

	BuildShaders();
}

SlideTools::~SlideTools()
{
	// Delete FBOs (Render Targets)
	if (m_PickingFboId) glDeleteFramebuffers(1, &m_PickingFboId);
	if (m_PickingColorTexId) glDeleteTextures(1, &m_PickingColorTexId);
	if (m_PickingDepthTexId) glDeleteTextures(1, &m_PickingDepthTexId);
	if (m_QueryId) glDeleteQueries(1, &m_QueryId);
	if (m_DepthPeelFboId[0]) glDeleteFramebuffers(2, m_DepthPeelFboId);
	if (m_DepthPeelColorTexId[0]) glDeleteTextures(2, m_DepthPeelColorTexId);
	if (m_DepthPeelDepthTexId[0]) glDeleteTextures(2, m_DepthPeelDepthTexId);
}

void SlideTools::ReloadShaders()
{
	DestroyShaders();
	BuildShaders();
}

void SlideTools::BuildShaders()
{
	m_DepthPeelShader.attachVertexShader("src/Shaders/depth_peel_vertex.glsl");
	m_DepthPeelShader.attachFragmentShader("src/Shaders/depth_peel_fragment.glsl");
	m_DepthPeelShader.link();

	CHECK_GL_ERRORS
}

void SlideTools::DestroyShaders()
{
	m_DepthPeelShader.destroy();

	CHECK_GL_ERRORS
}
