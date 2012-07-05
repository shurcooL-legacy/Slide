#pragma once
#ifndef __Globals_H__
#define __Globals_H__

#pragma warning(disable : 4351)
#pragma warning(disable : 4482)

#ifdef WIN32
	typedef signed __int8		sint8;
	typedef signed __int16		sint16;
	typedef signed __int32		sint32;
	typedef signed __int64		sint64;
	typedef unsigned __int8		uint8;
	typedef unsigned __int16	uint16;
//	typedef unsigned __int32	uint32;
	typedef unsigned long		uint32;
	typedef unsigned __int64	uint64;
#elif defined(__APPLE__) && defined(__MACH__)
	typedef signed char			sint8;
	typedef signed short		sint16;
	typedef signed int			sint32;
	typedef signed long long	sint64;
	typedef unsigned char		uint8;
	typedef unsigned short		uint16;
	typedef unsigned int		uint32;
	typedef unsigned long long	uint64;
#endif // WIN32

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

// Standard Includes
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>

// Library Includes
// {
	// FCollada
	#define NO_LIBXML
	#include "FCollada.h"
	#include "FCDocument/FCDocument.h"
	#include "FCDocument/FCDLibrary.h"
	#include "FCDocument/FCDGeometry.h"
	#include "FCDocument/FCDGeometryMesh.h"
	#include "FCDocument/FCDGeometryPolygons.h"
	#include "FCDocument/FCDGeometryPolygonsInput.h"
	#include "FCDocument/FCDGeometrySource.h"
	#include "FUtils/FUObject.h"

	#include "Wm5/Wm5Math.h"
	#include "Wm5/Wm5Vector3.h"
	#include "Wm5/Wm5Vector4.h"
	#include "Wm5/Wm5Plane3.h"
	#include "Wm5/Wm5IntrTriangle3Triangle3.h"
	#include "Wm5/Wm5IntrLine3Plane3.h"
	#include "Wm5/Wm5ContMinBox3.h"
	#include "Wm5/Wm5ContBox3.h"
	#include "Wm5/Wm5Matrix4.h"
	#include "Wm5/Wm5Quaternion.h"
	#include "Wm5/Wm5Segment3.h"
	#include "Wm5/Wm5DistSegment3Segment3.h"

	#include <Opcode.h>
	#ifdef __APPLE__
	#define IceMaths Opcode
	#endif

	#include "Arcball/trackball_bell.h"

	#define GLEW_STATIC 1
	#include <GL/glew.h>
	#include <GL/glfw.h>
// }

#ifdef WIN32
#include <irrKlang.h>

typedef unsigned char byte;
#include "Bamboo/Bamboo.h"
#endif

#if 1
#define CHECK_GL_ERRORS { GLenum err = glGetError(); if (err) printf( "Error %x at line %d of file %s\n", err, __LINE__, __FILE__); }
#else
#define CHECK_GL_ERRORS {}
#endif

class SceneObject;
class Scene;
class InputListener;
class SystemInputListener;
class InputManager;
class ControlMode;
class ControlModule;
class UnrealCameraModule;
class Slide;
class SlideTools;

// Project Includes
#include "Vector2.h"
#include "MultiRange.h"
#include "Shaders/GLSLProgramObject.h"
#include "Input/InputManager.h"
#include "Input/InputListener.h"
#include "Input/SystemInputListener.h"
#include "Slide/Slide.h"
#include "Slide/SlideTools.h"
#include "ControlModules/ControlModule.h"
#include "ControlModules/UnrealCameraModule.h"
#include "ControlModules/UnderCursorTranslationModule.h"
#include "ControlModules/TwoAxisValuatorModule.h"
#include "Input/ControlModuleMapping.h"
//#include "ControlModes/ControlMode.h"
#include "ControlModes/DepthIntervalsMode.h"
//#include "ControlModes/InputTestMode.h"
//#include "ControlModes/UnderCursorMode.h"
#include "Scene.h"
#include "SceneObject.h"

struct Camera {
	double x;
	double y;
	double z;

	double rh;
	double rv;
};

extern bool KeepRunning;

extern Scene MyScene;
extern DepthIntervalsMode * MyControlMode;
extern SlideTools * MySlideTools;
extern Slide * MySlide0;
#ifdef WIN32
extern irrklang::ISoundEngine * SoundEngine;
extern Bamboo * BambooEngine;
#endif

extern Camera camera;

extern uint32 nViewportWidth, nViewportHeight;

extern InputManager *	g_InputManager;

extern bool m_DebugSwitches[12];
extern int m_DebugValues[12];
extern bool bTESTMode0Performed;
extern bool bTESTMode1Performed;
extern bool bTESTMode2Performed;
extern Wm5::Vector3d DebugVector[10];

// STUDY
//extern bool StartedSTUDY;

// Utilities
void SavePPM(string Filename, uint32 Width, uint32 Height, uint8 * pPixels);

// Slide Flags
//#define SLIDE_TRANSLATIONAL_SNAPPING_ENABLED
//#define SLIDE_USE_BAMBOO

enum DebugSwitch
{
	DEBUG_SWITCH_SLIDE_RESOLUTION_OFF,
	DEBUG_SWITCH_DISPLAY_DEPTH_PEELING_VIEWPORT,
	DEBUG_SWITCH_HIGHLIGHT_SLIDING_TRIANGLE,
	DEBUG_SWITCH_VISUALIZE_SLIDING_SURFACE_NORMAL,
	DEBUG_SWITCH_HIDE_SHADOWS,
	DEBUG_SWITCH_VISUALIZE_DEPTH_BUFFER,
	DEBUG_SWITCH_FOCUS_CAMERA_ON_SELOBJ,
	DEBUG_SWITCH_SKIP_INNER_SELOBJ_SNAP_POSITIONS
};
inline bool GetDebugSwitch(DebugSwitch DebugSwitch)
{
	return m_DebugSwitches[DebugSwitch];
}

#endif // __Globals_H__
