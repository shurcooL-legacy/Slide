//----------------------------------------------------------------------
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
/**
 *	Contains code for box pruning.
 *	\file		IceBoxPruning.h
 *	\author		Pierre Terdiman
 *	\date		January, 29, 2000
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Include Guard
#ifndef __OPC_BOXPRUNING_H__
#define __OPC_BOXPRUNING_H__

// Optimized versions
FUNCTION OPCODE_API bool CompleteBoxPruning(udword nb, const AABB** array, Pairs& pairs, const Axes& axes);
FUNCTION OPCODE_API bool BipartiteBoxPruning(udword nb0, const AABB** array0, udword nb1, const AABB** array1, Pairs& pairs, const Axes& axes);

class BipartiteBoxPruner
{
protected:
  Axes         mAxes;
  udword       mN[2];
  const AABB** mBoxList[2];
  void**       mUserData[2];
  float*       mMinPosList[2];
  float        mMaxXExtent[2];
  RadixSort*   mSorter[2];

  void ClearList(int l);

public:
  BipartiteBoxPruner();
  ~BipartiteBoxPruner();

  void         SetAxes(const Axes& a) { mAxes = a; }

  udword       ListSize(int l) const { return mN       [l]; }
  const AABB** BoxList (int l) const { return mBoxList [l]; }
  void**       UserData(int l) const { return mUserData[l]; }

  void* GetUserData(int l, int e) const { return mUserData[l][e]; }

  void InitList(int l, int n);
  void SetElement(int l, int e, const AABB& box, void* ud)
  { mBoxList[l][e] = &box; mUserData[l][e] = ud; }

  void Sort(int l);
  void BipartitePruning(Pairs& pairs, int l0, int l1);
  void CompletePruning(Pairs& pairs, int l);

  int  FindNearestSmallerMinPos(int l, float value, int below=-1, int above=-1);

  void SinglePruning(Container& hits, int l, const AABB& box);
};

// Brute-force versions
FUNCTION OPCODE_API bool BruteForceCompleteBoxTest(udword nb, const AABB** array, Pairs& pairs);
FUNCTION OPCODE_API bool BruteForceBipartiteBoxTest(udword nb0, const AABB** array0, udword nb1, const AABB** array1, Pairs& pairs);

#endif //__OPC_BOXPRUNING_H__
