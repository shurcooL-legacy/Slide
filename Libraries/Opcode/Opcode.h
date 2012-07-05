//----------------------------------------------------------------------
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
/**
 *	Main file for Opcode.dll.
 *	\file		Opcode.h
 *	\author		Pierre Terdiman
 *	\date		March, 20, 2001
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Include Guard
#ifndef __OPCODE_H__
#define __OPCODE_H__

// MT COMMENTS
//============
//
// Use min-max in AABB (instead of center-extent)
// #define USE_MINMAX  1
//
// See also OPC_Settings for vertex/triangle-buffer access


#define OPCODE_API
#define ICEMATHS_API
#define ICECODE_API
#define ICECORE_API
#define MESHMERIZER_API

#define inline_ inline

///#include <stdlib.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cfloat>
#include <cmath>

#include <string>

#if __GNUC__ == 4 && __GNUC_MINOR__ > 5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#else
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifndef ASSERT
#define	ASSERT(exp)  assert(exp)
#endif

#define SetIceError(err, ptr) false

namespace Opcode
{
// Ice
#include "Ice/IcePreprocessor.h"
#include "Ice/IceTypes.h"
#include "Ice/IceFPU.h"
#include "Ice/IceMemoryMacros.h"

// IceCore
#include "Ice/IceUtils.h"
#include "Ice/IceContainer.h"
#include "Ice/IcePairs.h"
#include "Ice/IceRevisitedRadix.h"
#include "Ice/IceRandom.h"

// IceMaths
#include "Ice/IceAxes.h"
#include "Ice/IcePoint.h"
#include "Ice/IceHPoint.h"
#include "Ice/IceMatrix3x3.h"
#include "Ice/IceMatrix4x4.h"
#include "Ice/IcePlane.h"
#include "Ice/IceRay.h"
#include "Ice/IceIndexedTriangle.h"
#include "Ice/IceTriangle.h"
#include "Ice/IceTriList.h"
#include "Ice/IceAABB.h"
#include "Ice/IceOBB.h"
#include "Ice/IceBoundingSphere.h"
#include "Ice/IceSegment.h"
#include "Ice/IceLSS.h"


// Bulk-of-the-work
#include "OPC_Settings.h"
#include "OPC_Common.h"
#include "OPC_MeshInterface.h"
// Builders
#include "OPC_TreeBuilders.h"
// Trees
#include "OPC_AABBTree.h"
#include "OPC_OptimizedTree.h"
// Models
#include "OPC_BaseModel.h"
#include "OPC_Model.h"
#include "OPC_HybridModel.h"
// Colliders
#include "OPC_Collider.h"
#include "OPC_VolumeCollider.h"
#include "OPC_TreeCollider.h"
#include "OPC_RayCollider.h"
#include "OPC_SphereCollider.h"
#include "OPC_OBBCollider.h"
#include "OPC_AABBCollider.h"
#include "OPC_LSSCollider.h"
#include "OPC_PlanesCollider.h"
// Usages
#include "OPC_Picking.h"
// Sweep-and-prune
#include "OPC_BoxPruning.h"
#include "OPC_SweepAndPrune.h"

FUNCTION OPCODE_API bool InitOpcode();
FUNCTION OPCODE_API bool CloseOpcode();
}

#if __GNUC__ == 4 && __GNUC_MINOR__ > 5
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wstrict-aliasing"
#endif

#endif // __OPCODE_H__
