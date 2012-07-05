//----------------------------------------------------------------------
/**
 *	Contains code for random generators.
 *	\file		IceRandom.h
 *	\author		Pierre Terdiman
 *	\date		August, 9, 2001
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Include Guard
#ifndef __ICERANDOM_H__
#define __ICERANDOM_H__

	ICECORE_API	void	OpcodeSRand(udword seed);
	ICECORE_API	udword	OpcodeRand();

	//! Returns a unit random floating-point value
	ICECORE_API	float UnitRandomFloat();

	//! Returns a random index so that 0<= index < max_index
	ICECORE_API	udword GetRandomIndex(udword max_index);

	class ICECORE_API BasicRandom
	{
		public:

		//! Constructor
		inline_				BasicRandom(udword seed=0)	: mRnd(seed)	{}
		//! Destructor
		inline_				~BasicRandom()								{}

		inline_	void		SetSeed(udword seed)		{ mRnd = seed;											}
		inline_	udword		GetCurrentValue()	const	{ return mRnd;											}
		inline_	udword		Randomize()					{ mRnd = mRnd * 2147001325 + 715136305; return mRnd;	}

		private:
				udword		mRnd;
	};

#endif // __ICERANDOM_H__
