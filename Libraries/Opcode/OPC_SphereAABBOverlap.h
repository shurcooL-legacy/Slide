//----------------------------------------------------------------------
/**
 *	Sphere-AABB overlap test, based on Jim Arvo's code.
 *	\param		center		[in] box center
 *	\param		extents		[in] box extents
 *	\return		true on overlap
 */
//----------------------------------------------------------------------
inline_ bool SphereCollider::SphereAABBOverlap(const Point& center, const Point& extents)
{
  // Stats
  mNbVolumeBVTests++;

  float d = 0.0f, tmp, s;

  tmp = mCenter.x - center.x;
  s   = tmp + extents.x;

  if(s<0.0f)
    {
      d += s*s;
      if(d>mRadius2)  return false;
    }
  else
    {
      s = tmp - extents.x;
      if(s>0.0f)
        {
          d += s*s;
          if(d>mRadius2)  return false;
        }
    }

  tmp = mCenter.y - center.y;
  s   = tmp + extents.y;

  if(s<0.0f)
    {
      d += s*s;
      if(d>mRadius2)  return false;
    }
  else
    {
      s = tmp - extents.y;
      if(s>0.0f)
        {
          d += s*s;
          if(d>mRadius2)  return false;
        }
    }

  tmp = mCenter.z - center.z;
  s   = tmp + extents.z;

  if(s<0.0f)
    {
      d += s*s;
      if(d>mRadius2)  return false;
    }
  else
    {
      s = tmp - extents.z;
      if(s>0.0f)
        {
          d += s*s;
          if(d>mRadius2)  return false;
        }
    }

  return d <= mRadius2;
}
