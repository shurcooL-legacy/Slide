#include "Globals.h"

#include "Models/ModelLoader.h"
#include "Models/ColladaModel.h"
#include "Models/SupermarketModel.h"
#include "Models/SketchUpModel.h"
#include "ControlModes/ControlMode.h"
#include "ControlModes/ClassicMode.h"
#include "ControlModes/UnderCursorMode.h"

bool KeepRunning = true;

GLFWvidmode oDesktopMode;
uint32 nViewportWidth, nViewportHeight;

Camera camera;

DepthIntervalsMode * MyControlMode = nullptr;
SlideTools * MySlideTools = nullptr;
Slide * MySlide0 = nullptr;

#ifdef WIN32
irrklang::ISoundEngine * SoundEngine;

Bamboo * BambooEngine = nullptr;
#endif

bool m_DebugSwitches[12];
int m_DebugValues[12];
bool bTESTMode0Performed = false;
bool bTESTMode1Performed = false;
bool bTESTMode2Performed = false;
Wm5::Vector3d DebugVector[10];// = { Wm5::Vector3d::ZERO };

enum ProjectionMatrixType { Perspective, Ortho };
void SetOpenGLProjectionMatrix(ProjectionMatrixType ProjectionMatrix)
{
	if (Perspective == ProjectionMatrix)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, static_cast<double>(nViewportWidth) / nViewportHeight, 0.1, 100);
		glMatrixMode(GL_MODELVIEW);
	}
	else if (Ortho == ProjectionMatrix)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		//glOrtho(-8, 8, -5, 5, 0, 100);
		glOrtho(-1, 1, -1, 1, 0, 100);
		glMatrixMode(GL_MODELVIEW);
	}
}

void GLFWCALL ProcessWindowSize(int nWindowWidth, int nWindowHeight)
{
	printf("WindowResize to %dx%d\n", nWindowWidth, nWindowHeight);

	nViewportWidth = nWindowWidth;
	nViewportHeight = nWindowHeight;
	glViewport(0, 0, nViewportWidth, nViewportHeight);

	g_InputManager->UpdateWindowDimensions(Vector2n(nWindowWidth, nWindowHeight));

	SetOpenGLProjectionMatrix(Perspective);
}

void SetupOpenGL()
{
	GLfloat fZero[4] = {0, 0, 0, 1};
	GLfloat fOne[4] = {1, 1, 1, 1};

	GLfloat fAmbient[4] = {0.4f, 0.4f, 0.4f, 1}; glLightfv(GL_LIGHT0, GL_AMBIENT, fAmbient);
	GLfloat fDiffuse[4] = {0.85f, 0.85f, 0.85f, 1}; glLightfv(GL_LIGHT0, GL_DIFFUSE, fDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, fZero);
	GLfloat fLightPosition[4] = {0, 0, 0, 1}; glLightfv(GL_LIGHT0, GL_POSITION, fLightPosition);
	glEnable(GL_LIGHT0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, fOne);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fZero);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glDisable(GL_DITHER);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_FLAT);

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	//glEnable(GL_LINE_SMOOTH);
	//glLineWidth(0.5);
	glPolygonOffset(0.3f, 0);

	// Shadow related
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glStencilMask(0);		// Disable stencil writes by default

	glPointSize(10);

	printf("glError = %d\n", glGetError());
}

void SavePPM(string Filename, uint32 Width, uint32 Height, uint8 * pPixels)
{
	// Convert from RGBA to RGB
	uint8 * pPixels3 = new uint8[3 * Width * Height];
	for (uint32 PixelY = 0; PixelY < Height; ++PixelY) {
		for (uint32 PixelX = 0; PixelX < Width; ++PixelX) {
			pPixels3[3 * (PixelX + PixelY * Width) + 0] = pPixels[4 * ((Height - 1 - PixelY) * Width + PixelX) + 0];
			pPixels3[3 * (PixelX + PixelY * Width) + 1] = pPixels[4 * ((Height - 1 - PixelY) * Width + PixelX) + 1];
			pPixels3[3 * (PixelX + PixelY * Width) + 2] = pPixels[4 * ((Height - 1 - PixelY) * Width + PixelX) + 2];
		}
	}

	FILE * File = fopen(Filename.c_str(), "wb");
	fprintf(File, "P6 %d %d 255 ", Width, Height);
	fwrite(pPixels3, 3, Width * Height, File);
	fflush(File);
	fclose(File);

	delete[] pPixels3;
}

// Main function
int main(int argc, char * argv[])
{
	{
		MultiRange<double> mr;
		std::pair<double, double> a(9, 10); mr.insert(a);
		std::pair<double, double> b(5, 7); mr.insert(b);
		std::pair<double, double> c(1, 2); mr.insert(c);
		std::pair<double, double> d(6, 8); mr.insert(d);
		//std::cout << mr.size() << std::endl;
		//std::cout << mr.front().first << ", " << mr.front().second << std::endl;
	}









	glfwInit();
	// Verify the GLFW library and header versions match
	{ int Major, Minor, Revision; glfwGetVersion(&Major, &Minor, &Revision); bool Match = (GLFW_VERSION_MAJOR == Major && GLFW_VERSION_MINOR == Minor && GLFW_VERSION_REVISION == Revision); if (!Match) { std::cerr << "Error: GLFW library and header versions do not match." << std::endl; return 1; } else { std::cout << "Using GLFW v" << Major << "." << Minor << "." << Revision << "." << std::endl; } }
	FCollada::Initialize();

#if WIN32
	// start the sound engine with default parameters
	SoundEngine = irrklang::createIrrKlangDevice();
	if (nullptr == SoundEngine) {
		std::cerr << "Could not startup Sound Engine.\n";
		return 1;
	}

	// Bamboo Init
#ifdef SLIDE_USE_BAMBOO
	BambooEngine = new Bamboo;
	if (nullptr == BambooEngine) {
		std::cerr << "Could not startup Bamboo Engine.\n";
		return 1;
	}
#endif // SLIDE_USE_BAMBOO
#endif // WIN32

	glfwGetDesktopMode(&oDesktopMode);
	std::cout << "Desktop Mode: " << oDesktopMode.Width << "x" << oDesktopMode.Height << " @ " << (oDesktopMode.RedBits+oDesktopMode.GreenBits+oDesktopMode.BlueBits) << " bpp" << std::endl;

#	define DESKTOP
#ifdef DESKTOP
#	define WINDOW_WIDTH		1280
#	define WINDOW_HEIGHT	800

	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 32);
#else
#	define WINDOW_WIDTH		800
#	define WINDOW_HEIGHT	500
#endif // DESKTOP

//#	define FULLSCREEN
#ifndef FULLSCREEN
	glfwOpenWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 8, 8, 8, 8, 24, 8, GLFW_WINDOW); glfwSetWindowTitle("Slide Demo");
	glfwSetWindowPos((oDesktopMode.Width-WINDOW_WIDTH)/2, (oDesktopMode.Height-60-WINDOW_HEIGHT)/2);
	//glfwSetWindowPos(oDesktopMode.Width-WINDOW_WIDTH-30, (oDesktopMode.Height-60-WINDOW_HEIGHT)/2);
	nViewportWidth = WINDOW_WIDTH;
	nViewportHeight = WINDOW_HEIGHT;
#else
	glfwOpenWindow(oDesktopMode.Width, oDesktopMode.Height, 8, 8, 8, 8, 24, 8, GLFW_FULLSCREEN);
	nViewportWidth = oDesktopMode.Width;
	nViewportHeight = oDesktopMode.Height;
	glfwEnable(GLFW_MOUSE_CURSOR);
#endif // FULLSCREEN
	std::stringstream x;
	x << "CPU Count: " << glfwGetNumberOfProcessors()
	  << "\nGL Renderer: " << glGetString(GL_VENDOR) << " " << glGetString(GL_RENDERER) << " v" << glGetString(GL_VERSION)
	  << "\nGLFW_ACCELERATED: " << glfwGetWindowParam(GLFW_ACCELERATED)
	  << "\nGLFW_RED_BITS: " << glfwGetWindowParam(GLFW_RED_BITS)
	  << "\nGLFW_GREEN_BITS: " << glfwGetWindowParam(GLFW_GREEN_BITS)
	  << "\nGLFW_BLUE_BITS: " << glfwGetWindowParam(GLFW_BLUE_BITS)
	  << "\nGLFW_ALPHA_BITS: " << glfwGetWindowParam(GLFW_ALPHA_BITS)
	  << "\nGLFW_DEPTH_BITS: " << glfwGetWindowParam(GLFW_DEPTH_BITS)
	  << "\nGLFW_STENCIL_BITS: " << glfwGetWindowParam(GLFW_STENCIL_BITS)
	  << "\nGLFW_REFRESH_RATE: " << glfwGetWindowParam(GLFW_REFRESH_RATE)
	  << "\nGLFW_FSAA_SAMPLES: " << glfwGetWindowParam(GLFW_FSAA_SAMPLES);
	printf("%s\n", x.str().c_str());
#ifdef DESKTOP
	glfwSwapInterval(1);		// Vsync
#endif // DESKTOP

	if (GLEW_OK != glewInit()) {
		printf("ERROR: Failed to init GLEW.\n");
		glfwTerminate();
		return 0;
	}

	/*glfwSetMouseButtonCallback(&ProcessMouseButton);
	glfwSetMousePosCallback(&ProcessMousePos);
	glfwSetMouseWheelCallback(&ProcessMouseWheel);
	glfwSetKeyCallback(&ProcessKey);
	//glfwEnable(GLFW_KEY_REPEAT);*/
	g_InputManager = new InputManager();
	auto pSystemInputListener = new SystemInputListener();
	g_InputManager->RegisterListener(pSystemInputListener);

	glfwSetWindowSizeCallback(&ProcessWindowSize);

	SetupOpenGL();

	MySlideTools = new SlideTools();
	MySlide0 = new Slide();
	MyControlMode = new DepthIntervalsMode();

	Scene::InitializeScene(2);
	//Scene::InitializeScene(1);
	//Scene::InitializeScene(7);
	//Scene::InitializeScene(0);
	//Scene::InitializeScene(8);

	while (glfwGetWindowParam(GLFW_OPENED) && KeepRunning)
	{
		static double PreviousTime = glfwGetTime(); double CurrentTime = glfwGetTime(); double TimePassed = CurrentTime - PreviousTime; PreviousTime = CurrentTime;

		if (!GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_FOCUS_CAMERA_ON_SELOBJ) || 0 == MySlide0->GetSelectedObjectId())
		{
			SetOpenGLProjectionMatrix(Perspective);

			glLoadIdentity();

			GLfloat fLightPosition[4] = {0, 0, 0, 1};
			//GLfloat fLightPosition[4] = {0, 0, 1, 0};
			glLightfv(GL_LIGHT0, GL_POSITION, fLightPosition);

			glRotated(camera.rv + 90, -1, 0, 0);		// The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector)
			glRotated(camera.rh, 0, 0, 1);
			glTranslated(-camera.x, -camera.y, -camera.z);
		}
		else
		{
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(-100, 100, -100, 100, 0, 100);
			glMatrixMode(GL_MODELVIEW);

			Wm5::Vector3d ProjectedOutermostPoint(MySlide0->GetSelectedObject().GetProjectedOutermostBoundingPoint(-MySlide0->m_CursorRayDirection));

			// Look at the selected model
			Wm5::Vector3d CameraOrigin(ProjectedOutermostPoint);
			Wm5::Vector3d CameraTarget(CameraOrigin + MySlide0->m_CursorRayDirection);
			glLoadIdentity();
			if (CameraOrigin.X() != CameraTarget.X() || CameraOrigin.Y() != CameraTarget.Y())
				gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 0, 0, 1);
			else
				gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 1, 0, 0);

			Wm5::Tuple<4, double> ProjectedBoundingBox = MySlide0->GetSelectedObject().GetProjectedBoundingBox();

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(ProjectedBoundingBox[0] - 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]), ProjectedBoundingBox[2] + 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]),
					ProjectedBoundingBox[1] - 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), ProjectedBoundingBox[3] + 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), 0, 100);
			glMatrixMode(GL_MODELVIEW);
		}
		/*else// if (0 != MyScene.m_SlidingObject)
		{
			Wm5::Vector3d Normal(0, 0, 1);
			Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(Normal));

			Wm5::Vector3d SnapOrigin(ProjectedOutermostPoint);
			Wm5::Vector3d SnapDirection(MyScene.GetSelectedObject().GetPosition() - SnapOrigin);

			// Ortho projection
			//SetOpenGLProjectionMatrix(Ortho);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(-100, 100, -100, 100, 0, 100);
			glMatrixMode(GL_MODELVIEW);

			// Look at the selected model
			Wm5::Vector3d CameraOrigin(SnapOrigin);
			Wm5::Vector3d CameraTarget(SnapOrigin + SnapDirection);
			glLoadIdentity();
			if (CameraOrigin.X() != CameraTarget.X() || CameraOrigin.Y() != CameraTarget.Y())
				gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 0, 0, 1);
			else
				gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 1, 0, 0);

			Wm5::Tuple<4, double> ProjectedBoundingBox = MyScene.GetSelectedObject().GetProjectedBoundingBox();
			//printf("ProjectedBoundingBox size = %f, %f\n", ProjectedBoundingBox[2] - ProjectedBoundingBox[0], ProjectedBoundingBox[3] - ProjectedBoundingBox[1]);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(ProjectedBoundingBox[0] - 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]), ProjectedBoundingBox[2] + 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]),
					ProjectedBoundingBox[1] - 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), ProjectedBoundingBox[3] + 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), 0, 100);
			glMatrixMode(GL_MODELVIEW);
		}*/

		/*if (MyControlMode.bSelectPerformed || MyControlMode.m_DragPerformed)
			MyScene.Render(Scene::ObjectPicking);

		if (MyControlMode.bSelectPerformed) {
			MyControlMode.bSelectPerformed = false;
			MyControlMode.PickModel();
			MyControlMode.ComputeUnderCursorPosition();
		}

		if (MyControlMode.m_DragPerformed) {
			MyControlMode.m_DragPerformed = false;
			MyControlMode.ComputeUnderCursorPosition();
		}*/

		if (bTESTMode0Performed)
		{
			if (MySlide0->GetSelectedObjectId()) {
				MySlide0->CheckForCollision();
				double dStartTime = glfwGetTime();
				MySlide0->NewSnap(Slide::Back);
				printf("Snap-Back took %f secs.\n", glfwGetTime() - dStartTime);
				MySlide0->CheckForCollision();
			}

			bTESTMode0Performed = false;
		} else if (bTESTMode1Performed)
		{
			if (MySlide0->GetSelectedObjectId()) {
				MySlide0->CheckForCollision();
				double dStartTime = glfwGetTime();
				MySlide0->NewSnapB(Slide::Front, Slide::NonColliding);
				printf("Snap-Front took %f secs.\n", glfwGetTime() - dStartTime);
				MySlide0->CheckForCollision();
			}

			bTESTMode1Performed = false;
		} else if (bTESTMode2Performed)
		{
			if (MySlide0->GetSelectedObjectId()) {
				MySlide0->CheckForCollision();
				double dStartTime = glfwGetTime();
				MySlide0->NewSnapC(Slide::BackNextLevel);
				printf("Snap-Back-to-Next-Level took %f secs.\n", glfwGetTime() - dStartTime);
				MySlide0->CheckForCollision();
			}

			bTESTMode2Performed = false;
		}

#define SLIDE_ENABLED
#ifdef SLIDE_ENABLED
		if (!GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_SLIDE_RESOLUTION_OFF) && GLFW_RELEASE == glfwGetKey(GLFW_KEY_LALT))
			MySlide0->SlideResolution();
		else
		{
			MySlide0->CheckForCollision();

			MySlide0->m_SlidingConstraint.Normal = Wm5::Vector3d::ZERO;
			MySlide0->m_ModelMovedByUser = false;
		}
#else
		if (0 != MySlide0->GetSelectedObjectId() && MySlide0->m_ModelMovedByUser)
		{
			//double dStartTime = glfwGetTime();

			MySlide0->CheckForCollision();

			/*double dTimeTaken = glfwGetTime() - dStartTime;
			static double dMaxTimeTaken = 0;
			if (dTimeTaken > dMaxTimeTaken) dMaxTimeTaken = dTimeTaken;
			printf("CheckForCollision3() took %f secs (worst case was %f seconds).\n", dTimeTaken, dMaxTimeTaken);*/

			MySlide0->m_ModelMovedByUser = false;
		}
#endif

		/*{
			// Set a smaller viewport with square aspect ratio
			uint32 nCustomViewportSize = std::min<uint32>(200, std::min<uint32>(nViewportWidth, nViewportHeight));
			//uint32 nCustomViewportSize = std::min<uint32>(nViewportWidth, nViewportHeight);
			glViewport(0, 0, nCustomViewportSize, nCustomViewportSize);

			for (int i = 0; i < 6; ++i)
				MyScene.Render(Scene::ObjectPicking);

			glViewport(0, 0, nViewportWidth, nViewportHeight);
		}*/

		MyScene.Render(Scene::VisibleGeometry, MySlide0);
		//MyScene.Render(Scene::IntersectingTrianglesTEST);
			/*if (0 == MyControlMode.nTESTMode2)
				MyScene.Render(Scene::VisibleGeometry);
			else {
				glClearColor(0.600f, 0.741f, 0.565f, 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				MyScene.Render(Scene::IntersectingTrianglesTEST);
			}*/
		//MyScene.Render(Scene::VisibleGeometryWithBoundingBoxes);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//MyScene.Render(Scene::BoundingBox);
		//MyScene.Render(Scene::ObjectPicking);

		// Depth Peeling Test
		if (GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_DISPLAY_DEPTH_PEELING_VIEWPORT) && 0 != MySlide0->GetSelectedObjectId())
		{
			// Prepare viewport
			glViewport(0, 0, MySlideTools->m_DepthPeelImageWidth, MySlideTools->m_DepthPeelImageHeight);

			// Ortho projection
			{
				//SetOpenGLProjectionMatrix(Ortho);
				glPushMatrix();		// Push Modelview Matrix
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();		// Push Projection Matrix
				glLoadIdentity();
				glOrtho(-100, 100, -100, 100, 0, 100);
				glMatrixMode(GL_MODELVIEW);

				// Look at the selected model
				//Wm5::Vector3d CameraOrigin(camera.x, camera.y, camera.z);
				//Wm5::Vector3d CameraTarget(CameraOrigin + MySlide->m_CursorRayDirection);
				Wm5::Vector3d CameraOrigin(MySlide0->GetSelectedObject().GetProjectedOutermostBoundingPoint(-MySlide0->m_CursorRayDirection));
				Wm5::Vector3d CameraTarget(CameraOrigin + MySlide0->m_CursorRayDirection);
				glLoadIdentity();
				if (CameraOrigin.X() != CameraTarget.X() || CameraOrigin.Y() != CameraTarget.Y())
					gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 0, 0, 1);
				else
					gluLookAt(CameraOrigin.X(), CameraOrigin.Y(), CameraOrigin.Z(), CameraTarget.X(), CameraTarget.Y(), CameraTarget.Z(), 1, 0, 0);

				Wm5::Tuple<4, double> ProjectedBoundingBox = MySlide0->GetSelectedObject().GetProjectedBoundingBox();

				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(ProjectedBoundingBox[0] - 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]), ProjectedBoundingBox[2] + 0.01*(ProjectedBoundingBox[2]-ProjectedBoundingBox[0]),
						ProjectedBoundingBox[1] - 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), ProjectedBoundingBox[3] + 0.01*(ProjectedBoundingBox[3]-ProjectedBoundingBox[1]), 0, 100);
				glMatrixMode(GL_MODELVIEW);
			}

			// Peel Depth Layers away
			uint8 NumberDepthLayersToPeel = m_DebugValues[0];
			for (uint8 DepthLayer = 0; DepthLayer <= NumberDepthLayersToPeel; ++DepthLayer)
			{
				// Set target FBO
				glBindFramebuffer(GL_FRAMEBUFFER, MySlideTools->m_DepthPeelFboId[DepthLayer % 2]);
				//glDrawBuffer(GL_COLOR_ATTACHMENT0);

				if (0 == DepthLayer)
				{
					// Offset the object back, so that it goes behind the surface it is already on
					Wm5::Vector3d MoveVector(Wm5::Vector3d::ZERO);
					/*if (Wm5::Vector3d::ZERO != MySlide->m_SlidingConstraint.Normal)		// Only do it if it is actually on top of some object
					{
						double BufferDistance = MySlide->m_kBufferDistance;
						/// Target = m_kBufferDistance / cos(Theta)
						double FAbsCosTheta = Wm5::Mathd::FAbs(MySlide->m_CursorRayDirection.Dot(MySlide->m_SlidingConstraint.Normal));
						if (FAbsCosTheta < 0.000001)
							FAbsCosTheta = 1;
						BufferDistance = MySlide->m_kBufferDistance / FAbsCosTheta;
						MoveVector = ZoomSelectedModel(BufferDistance, MySlide->m_CursorRayDirection);
						//printf("preObj.Z() = %.20f (%f, %f)\n", MyScene.GetSelectedObject().GetPosition().Z(), MyScene.GetSelectedObject().GetPosition().X(), MyScene.GetSelectedObject().GetPosition().Y());
						MySlide->MoveSelectedObject(MoveVector * 2);
						//printf("SelObj.Z() = %.20f (%f, %f)\n", MyScene.GetSelectedObject().GetPosition().Z(), MyScene.GetSelectedObject().GetPosition().X(), MyScene.GetSelectedObject().GetPosition().Y());
					}*/

					MyScene.Render(Scene::SelectedObjectFurthestBackFace, MySlide0);

					// Restore its original position
					MySlide0->MoveSelectedObject(MoveVector * -2);
				}
				else
				{
					MySlideTools->m_DepthPeelShader.bind();
					MySlideTools->m_DepthPeelShader.bindTextureRECT("DepthTex", MySlideTools->m_DepthPeelDepthTexId[(DepthLayer - 1) % 2], 0);
					MyScene.Render(Scene::StaticSceneGeometry, MySlide0);
					MySlideTools->m_DepthPeelShader.unbind();
				}
			}

			{
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();		// Restore Projection Matrix
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();		// Restore Modelview Matrix
			}

			// Return to normal framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glDrawBuffer(GL_BACK);
			glViewport(0, 0, nViewportWidth, nViewportHeight);

			// Display a textured quad with results
			{
				glEnable(GL_TEXTURE_RECTANGLE);
				glBindTexture(GL_TEXTURE_RECTANGLE, MySlideTools->m_DepthPeelColorTexId[NumberDepthLayersToPeel % 2]);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				gluOrtho2D(0, nViewportWidth, 0, nViewportHeight);
				glMatrixMode(GL_MODELVIEW);

				glPushMatrix();
				glLoadIdentity();
				glClear(GL_DEPTH_BUFFER_BIT);
				glBegin(GL_QUADS);
					glColor3d(1, 1, 1);
					glTexCoord2d(0, 0); glVertex2d(0, 0);
					glTexCoord2d(MySlideTools->m_DepthPeelImageWidth, 0); glVertex2d(MySlideTools->m_DepthPeelImageWidth, 0);
					glTexCoord2d(MySlideTools->m_DepthPeelImageWidth, MySlideTools->m_DepthPeelImageHeight); glVertex2d(MySlideTools->m_DepthPeelImageWidth, MySlideTools->m_DepthPeelImageHeight);
					glTexCoord2d(0, MySlideTools->m_DepthPeelImageHeight); glVertex2d(0, MySlideTools->m_DepthPeelImageHeight);
				glEnd();
				glPopMatrix();

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);

				glDisable(GL_TEXTURE_RECTANGLE);
			}

			CHECK_GL_ERRORS
		}

		bool bCtrlPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LCTRL) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RCTRL));
		bool bShiftPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LSHIFT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RSHIFT));
		bool AltPressed = (GLFW_PRESS == glfwGetKey(GLFW_KEY_LALT) || GLFW_PRESS == glfwGetKey(GLFW_KEY_RALT));

		//if (2 == MySlide->nMouseButtonsDown && 0 != MyScene.GetSelectedObjectId() && !bCtrlPressed && !bShiftPressed)
		//if (TwoAxisValuatorModule::m_Active && 0 != MyScene.GetSelectedObjectId())
		//if (MyControlMode->GetTwoAxisValuatorModule().IsActive())
		if (nullptr != MyControlMode->GetControlModuleMapping().GetActiveModule() && typeid(TwoAxisValuatorModule) == typeid(*MyControlMode->GetControlModuleMapping().GetActiveModule()))
		{
			Wm5::Vector3d ToObject = MySlide0->GetSelectedObject().GetPosition() - Wm5::Vector3d(camera.x, camera.y, camera.z); ToObject.Normalize();

			Wm5::Vector3d Horizontal(Wm5::Vector3d::ZERO), Vertical(Wm5::Vector3d::ZERO);
			Horizontal.X() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
			Horizontal.Y() += -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
			Vertical.X() += Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
			Vertical.Y() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
			Vertical.Z() += -Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);

			Horizontal = Vertical.Cross(ToObject) * 2;
			Vertical = ToObject.Cross(Horizontal);

			glPushAttrib(GL_ALL_ATTRIB_BITS);
				//glEnable(GL_BLEND);
				glLineWidth(2);
				glBegin(GL_LINES);
					glColor3d(0.1, 0.3, 1);
					glVertex3d((MySlide0->GetSelectedObject().GetPosition() + Vertical).X(), (MySlide0->GetSelectedObject().GetPosition() + Vertical).Y(), (MySlide0->GetSelectedObject().GetPosition() + Vertical).Z());
					glVertex3d((MySlide0->GetSelectedObject().GetPosition() - Vertical).X(), (MySlide0->GetSelectedObject().GetPosition() - Vertical).Y(), (MySlide0->GetSelectedObject().GetPosition() - Vertical).Z());

					glColor3d(1, 0.3, 0.1);
					glVertex3d((MySlide0->GetSelectedObject().GetPosition() + Horizontal).X(), (MySlide0->GetSelectedObject().GetPosition() + Horizontal).Y(), (MySlide0->GetSelectedObject().GetPosition() + Horizontal).Z());
					glVertex3d((MySlide0->GetSelectedObject().GetPosition() - Horizontal).X(), (MySlide0->GetSelectedObject().GetPosition() - Horizontal).Y(), (MySlide0->GetSelectedObject().GetPosition() - Horizontal).Z());
				glEnd();
			glPopAttrib();
		}

#if 0
		glBegin(GL_LINES);
			glColor3d(1, 0, 0); glVertex3d(1, 0, 0); glVertex3d(0, 0, 0);		// X-axis
			glColor3d(0, 1, 0); glVertex3d(0, 1, 0); glVertex3d(0, 0, 0);		// Y-axis
			glColor3d(0, 0, 1); glVertex3d(0, 0, 1); glVertex3d(0, 0, 0);		// Z-axis

			glColor3d(0, 0, 0); glVertex3d(DebugVector[0].X(), DebugVector[0].Y(), DebugVector[0].Z()); glVertex3d(DebugVector[1].X(), DebugVector[1].Y(), DebugVector[1].Z());
		glEnd();
#endif

#if 0
		// Debug: Selected Object center and radius projected
		if (0 != MyScene.GetSelectedObjectId())
		{
			GLdouble ModelMatrix[16]; glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
			GLdouble ProjectionMatrix[16]; glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);
			GLint Viewport[4]; glGetIntegerv(GL_VIEWPORT, Viewport);
			Wm5::Vector3d ObjectCenter;
			Wm5::Vector3d ObjectCorner;

			//Wm5::Vector3d ObjectPosition(MyScene.GetSelectedObject().GetPosition());
			Wm5::Vector3d ObjectPosition(MySlide->m_OriginalSelectedObjectPosition);
			gluProject(ObjectPosition.X(), ObjectPosition.Y(), ObjectPosition.Z(), ModelMatrix, ProjectionMatrix, Viewport, &ObjectCenter.X(), &ObjectCenter.Y(), &ObjectCenter.Z());

			Wm5::Vector3d Horizontal(Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD), -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD), 0);
			Horizontal *= MyScene.GetSelectedObject().GetTB().radius();
			gluProject(ObjectPosition.X() + Horizontal.X(), ObjectPosition.Y() + Horizontal.Y(), ObjectPosition.Z() + Horizontal.Z(), ModelMatrix, ProjectionMatrix, Viewport, &ObjectCorner.X(), &ObjectCorner.Y(), &ObjectCorner.Z());

			double Radius = Wm5::Mathd::FAbs(ObjectCenter.X() - ObjectCorner.X());

			glPushMatrix();
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				gluOrtho2D(0, nViewportWidth, 0, nViewportHeight);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				glPushAttrib(GL_ALL_ATTRIB_BITS);
					glPolygonMode(GL_FRONT, GL_LINE);
					glBegin(GL_QUADS);
						glColor3d(0, 0, 1);
						glVertex2d(ObjectCenter.X() - Radius, ObjectCenter.Y() - Radius);
						glVertex2d(ObjectCenter.X() + Radius, ObjectCenter.Y() - Radius);
						glVertex2d(ObjectCenter.X() + Radius, ObjectCenter.Y() + Radius);
						glVertex2d(ObjectCenter.X() - Radius, ObjectCenter.Y() + Radius);
					glEnd();
				glPopAttrib();
			glPopMatrix();

			double X, Y;
			X = (MySlide->m_CursorX - (ObjectCenter.X() - Radius)) / (2 * Radius);
			Y = (MySlide->m_CursorY - (ObjectCenter.Y() - Radius)) / (2 * Radius);
			//printf("X, Y = %f, %f\n", X, Y);

			SetOpenGLProjectionMatrix(Perspective);
		}
#endif

		/*if (1 == MySlide->nTESTView)
		{
			// 2D Rendering Mode
			SetOpenGLProjectionMatrix(Ortho);
			glLoadIdentity();

			glBegin(GL_LINES);
				glColor3d(1, 0, 0);
				glVertex3d(1, 0, 0); glVertex3d(-1, 0, 0);		// X-axis
				glVertex3d(0, 1, 0); glVertex3d(0, -1, 0);		// Y-axis
			glEnd();
		}*/

		// Debug Points
		glBegin(GL_POINTS);
			glColor3d(1, 0, 0);
			//glVertex3d(MySlide->m_UnderCursorPosition.X(), MySlide->m_UnderCursorPosition.Y(), MySlide->m_UnderCursorPosition.Z());
			//printf("m_UnderCursorPosition: %f %f %f\n", MySlide->m_UnderCursorPosition.X(), MySlide->m_UnderCursorPosition.Y(), MySlide->m_UnderCursorPosition.Z());

#if 0
			if (0 != MyScene.GetSelectedObjectId() && 0 != MyScene.m_SlidingObject)
			{
				Wm5::Vector3d Normal(MyScene.GetSlidingPlane().Normal);
				//Wm5::Vector3d Normal(0, 0, 1);
				Wm5::Vector3d ProjectedOutermostPoint(MyScene.GetSelectedObject().GetProjectedOutermostBoundingPoint(Normal));
				glColor3d(0, 0, 1); glVertex3d(ProjectedOutermostPoint.X(), ProjectedOutermostPoint.Y(), ProjectedOutermostPoint.Z());
			}
#endif
		glEnd();

#define SLIDE_DRAW_SLIDING_SURFACE_NORMAL
#ifdef SLIDE_DRAW_SLIDING_SURFACE_NORMAL
		if (GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_VISUALIZE_SLIDING_SURFACE_NORMAL)) {
			// Debug: Sliding Surface Normal
			glBegin(GL_LINES);
				glColor3d(0, 0, 1);
				glVertex3d(MySlide0->m_UnderCursorPosition.X(), MySlide0->m_UnderCursorPosition.Y(), MySlide0->m_UnderCursorPosition.Z());
				//Wm5::Vector3d OtherEnd = MySlide->m_UnderCursorPosition + MyScene.GetSlidingPlane().Normal;
				Wm5::Vector3d OtherEnd = MySlide0->m_UnderCursorPosition + MySlide0->m_SlidingConstraint.Normal;
				glVertex3d(OtherEnd.X(), OtherEnd.Y(), OtherEnd.Z());
			glEnd();
		}
#endif

		// Debug: Render Bounding Box Corners
		//if (0 != MyScene.GetSelectedObjectId()) { for (uint8 CornerNumber = 0; CornerNumber < 8; ++CornerNumber) { Wm5::Vector3d Corner = MyScene.GetSelectedObject().GetBoundingBoxCorner(CornerNumber); glPushAttrib(GL_ALL_ATTRIB_BITS); glDisable(GL_DEPTH_TEST); glBegin(GL_POINTS); glColor3d(0, 0, 0); glVertex3d(Corner.X(), Corner.Y(), Corner.Z()); glEnd(); glPopAttrib(); } }

			/*if (bShiftPressed && MySlide->nMouseButtonsDown)
			{
				Wm5::Vector3d Dir(MySlide->m_CursorRayDirection);
				Dir.Normalize();
				Dir *= TimePassed * 5;

				if (1 == MySlide->nMouseButtonsDown) {
					camera.x += Dir.X(); camera.y += Dir.Y(); camera.z += Dir.Z();
				} else if (2 == MySlide->nMouseButtonsDown) {
					camera.x -= Dir.X(); camera.y -= Dir.Y(); camera.z -= Dir.Z();
				}
				camera.rh += TimePassed * 200 * (MySlide->m_CursorX - 0.5 * nViewportWidth) / nViewportWidth;
				camera.rv += TimePassed * 200 * (MySlide->m_CursorY - 0.5 * nViewportHeight) / nViewportHeight;
			}*/

		// Debug: Visualize depth buffer
		if (GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_VISUALIZE_DEPTH_BUFFER))
		{
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, nViewportWidth, 0, nViewportHeight);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			float * pDepths = new float[nViewportWidth * nViewportHeight];

			glReadPixels(0, 0, nViewportWidth, nViewportHeight, GL_DEPTH_COMPONENT, GL_FLOAT, reinterpret_cast<void *>(pDepths));

			glBegin(GL_POINTS);
			for (uint32 Y = 0; Y < nViewportHeight; ++Y)
			{
				for (uint32 X = 0; X < nViewportWidth; ++X)
				{
					double C = pDepths[Y * nViewportWidth + X];
					C = (1.0 - Wm5::Mathd::Pow(C, 32)) * 1;
					glColor3d(C, C, C);
					glVertex2d(X + 0.5, Y + 0.5);
				}
			}
			glEnd();

			delete[] pDepths;
		}

		// Reset back to pespective projection
		SetOpenGLProjectionMatrix(Perspective); glLoadIdentity(); glRotated(camera.rv + 90, -1, 0, 0); glRotated(camera.rh, 0, 0, 1); glTranslated(-camera.x, -camera.y, -camera.z);

		glfwSwapBuffers();
		glFinish();		// ATI 4000 Stuttering Fix

		MyControlMode->GetControlModuleMapping().ProcessTimePassed(TimePassed);

		if (!glfwGetWindowParam(GLFW_ACTIVE)) glfwSleep(0.016);

#ifdef WIN32
		if (BambooEngine) BambooEngine->update();
#endif

		// Collision avoidance
		/*Wm5::Vector3d NavOffset = MyScene.CalculateNavigationOffset();
		NavOffset *= 30 * TimePassed;
		camera.x += NavOffset.X(); camera.y += NavOffset.Y(); camera.z += NavOffset.Z();*/
	}

	MyScene.Reset();

	printf("glError = %d\n", glGetError());
	CHECK_GL_ERRORS

	delete MyControlMode; MyControlMode = nullptr;
	delete MySlide0; MySlide0 = nullptr;
	delete MySlideTools; MySlideTools = nullptr;
	delete pSystemInputListener; pSystemInputListener = nullptr;
	delete g_InputManager; g_InputManager = nullptr;

#ifdef WIN32
	SoundEngine->drop();
	delete BambooEngine;
#endif
	FCollada::Release();
	glfwTerminate();

	return 0;
}
