//----------------------------------------------------------------------
/**
 *	Contains a simple container class.
 *	\file		IceContainer.h
 *	\author		Pierre Terdiman
 *	\date		February, 5, 2000
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Include Guard
#ifndef __ICECONTAINER_H__
#define __ICECONTAINER_H__

#define CONTAINER_STATS

enum FindMode
  {
    FIND_CLAMP,
    FIND_WRAP,

    FIND_FORCE_DWORD = 0x7fffffff
  };

class ICECORE_API Container
{
public:
  // Constructor / Destructor
  Container();
  Container(const Container& object);
  Container(udword size, float growth_factor);
  ~Container();
  // Management
  //----------------------------------------------------------------------
  /**
   *	A O(1) method to add a value in the container. The container
   *	is automatically resized if needed.  The method is inline, not
   *	the resize. The call overhead happens on resizes only, which
   *	is not a problem since the resizing operation costs a lot more
   *	than the call overhead...
   *
   *	\param		entry		[in] a udword to store in the container
   *	\see		Add(float entry)
   *	\see		Empty()
   *	\see		Contains(udword entry)
   *	\return		Self-Reference
   */
  //----------------------------------------------------------------------
  Container&	Add(udword entry)
  {
    // Resize if needed
    if(mCurNbEntries==mMaxNbEntries)	Resize();

    // Add new entry
    mEntries[mCurNbEntries++]	= entry;
    return *this;
  }

  Container&	Add(const udword* entries, udword nb)
  {
    // Resize if needed
    if(mCurNbEntries+nb>mMaxNbEntries)	Resize(nb);

    // Add new entry
    CopyMemory(&mEntries[mCurNbEntries], entries, nb*sizeof(udword));
    mCurNbEntries+=nb;
    return *this;
  }

  //----------------------------------------------------------------------
  /**
   *	A O(1) method to add a value in the container. The container
   *	is automatically resized if needed.  The method is inline, not
   *	the resize. The call overhead happens on resizes only, which
   *	is not a problem since the resizing operation costs a lot more
   *	than the call overhead...
   *
   *	\param	entry		[in] a float to store in the container
   *	\see	Add(udword entry)
   *	\see	Empty()
   *	\see	Contains(udword entry)
   *	\return Self-Reference
   */
  //----------------------------------------------------------------------
  Container&	Add(float entry)
  {
    // Resize if needed
    if(mCurNbEntries==mMaxNbEntries)	Resize();

    // Add new entry
    mEntries[mCurNbEntries++]	= IR(entry);
    return *this;
  }

  Container&	Add(const float* entries, udword nb)
  {
    // Resize if needed
    if(mCurNbEntries+nb>mMaxNbEntries)	Resize(nb);

    // Add new entry
    CopyMemory(&mEntries[mCurNbEntries], entries, nb*sizeof(float));
    mCurNbEntries+=nb;
    return *this;
  }

  //! Add unique [slow]
  Container&	AddUnique(udword entry)
  {
    if(!Contains(entry))        Add(entry);
    return *this;
  }

  //----------------------------------------------------------------------
  /**
   *	Clears the container. All stored values are deleted, and it
   *	frees used ram.
   *	\see		Reset()
   *	\return		Self-Reference
   */
  //----------------------------------------------------------------------
  Container&	Empty();

  //----------------------------------------------------------------------
  /**
   *	Resets the container. Stored values are discarded but the
   *	buffer is kept so that further calls don't need resizing again.
   *	That's a kind of temporal coherence.
   *	\see		Empty()
   */
  //----------------------------------------------------------------------
  void	Reset()
  {
    // Avoid the write if possible
    // ### CMOV
    if(mCurNbEntries)	mCurNbEntries = 0;
  }

  // HANDLE WITH CARE
  void	ForceSize(udword size)
  {
    mCurNbEntries = size;
  }

  //----------------------------------------------------------------------
  /**
   *	Sets the initial size of the container. If it already contains something, it's discarded.
   *	\param		nb		[in] Number of entries
   *	\return		true if success
   */
  //----------------------------------------------------------------------
  bool	SetSize(udword nb);

  //----------------------------------------------------------------------
  /**
   *	Refits the container and get rid of unused bytes.
   *	\return		true if success
   */
  //----------------------------------------------------------------------
  bool	Refit();

  // Checks whether the container already contains a given value.
  bool	Contains(udword entry, udword* location=0) const;
  // Deletes an entry - doesn't preserve insertion order.
  bool	Delete(udword entry);
  // Deletes an entry - does preserve insertion order.
  bool	DeleteKeepingOrder(udword entry);
  //! Deletes the very last entry.
  void	DeleteLastEntry() { if(mCurNbEntries) mCurNbEntries--; }
  //! Deletes the entry whose index is given
  void	DeleteIndex(udword index) { mEntries[index] = mEntries[--mCurNbEntries];	}

  // Helpers
  Container&	FindNext(udword& entry, FindMode find_mode=FIND_CLAMP);
  Container&	FindPrev(udword& entry, FindMode find_mode=FIND_CLAMP);
  // Data access.
  udword	GetNbEntries()	   const { return mCurNbEntries; }	//!< Returns the current number of entries.
  udword	GetEntry(udword i) const { return mEntries[i]; }	//!< Returns ith entry
  udword*	GetEntries()	   const { return mEntries; }	        //!< Returns the list of entries.

  udword	GetFirst() const { return mEntries[0]; }
  udword	GetLast()  const { return mEntries[mCurNbEntries-1]; }

  // Growth control
  float	GetGrowthFactor() const	{ return mGrowthFactor; }	    //!< Returns the growth factor
  void	SetGrowthFactor(float growth)	{ mGrowthFactor = growth; } //!< Sets the growth factor
  bool	IsFull()     const { return mCurNbEntries==mMaxNbEntries; } //!< Checks the container is full
  bool	IsNotEmpty() const { return mCurNbEntries; }                //!< Checks the container is empty

  //! Read-access as an array
  udword	operator[](udword i) const { ASSERT(i>=0 && i<mCurNbEntries); return mEntries[i]; }
  //! Write-access as an array
  udword&	operator[](udword i)       { ASSERT(i>=0 && i<mCurNbEntries); return mEntries[i]; }

  // Stats
  udword	GetUsedRam()	const;

  //! Operator for "Container A = Container B"
  void		operator = (const Container& object);

#ifdef CONTAINER_STATS
  udword	GetNbContainers() const { return mNbContainers; }
  udword	GetTotalBytes()   const { return mUsedRam; }
private:

  static	udword	mNbContainers;	//!< Number of containers around
  static	udword	mUsedRam;	//!< Amount of bytes used by containers in the system
#endif
private:
  // Resizing
  bool		Resize(udword needed=1);
  // Data
  udword	mMaxNbEntries;	//!< Maximum possible number of entries
  udword	mCurNbEntries;	//!< Current number of entries
  udword*	mEntries;	//!< List of entries
  float		mGrowthFactor;	//!< Resize: new number of entries = old number * mGrowthFactor
};

#endif // __ICECONTAINER_H__
