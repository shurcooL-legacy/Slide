//----------------------------------------------------------------------
/**
 *	Planes-triangle overlap test.
 *	\param          in_clip_mask	[in] bitmask for active planes
 *	\return         true if triangle overlap planes
 *	\warning THIS IS A CONSERVATIVE TEST !! Some triangles will be
 *	returned as intersecting, while they're not!
 */
//----------------------------------------------------------------------
inline_ bool PlanesCollider::PlanesTriOverlap(udword in_clip_mask)
{
  // Stats
  mNbVolumePrimTests++;

  const Plane* p = mPlanes;
  udword Mask = 1;

  while(Mask<=in_clip_mask)
    {
      if(in_clip_mask & Mask)
        {
          float d0 = p->Distance(*mVP.Vertex[0]);
          float d1 = p->Distance(*mVP.Vertex[1]);
          float d2 = p->Distance(*mVP.Vertex[2]);
          if(d0>0.0f && d1>0.0f && d2>0.0f) return false;
        }
      Mask+=Mask;
      p++;
    }
  return true;
}
