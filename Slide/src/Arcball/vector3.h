#ifndef _VECTOR3_H
#define _VECTOR3_H

//#include <vector>
#include <cmath> // for sqrt
#include <cassert>
using namespace std;
	
	

class Vector3 
{
  public:

	typedef size_t         index_type;
    double x,y,z;
	
    Vector3() {}
    Vector3( double xx, double yy, double zz )
    { x=xx; y=yy; z=zz; }
	
    /*inline Vector3 operator + (Vector3 t) // addition
    { return Vector3(x+t.x,y+t.y,z+t.z); }
    inline Vector3 operator - (Vector3 t) // subtraction
    { return Vector3(x-t.x,y-t.y,z-t.z); }
    inline Vector3 operator * (double t) // dot product
    { return Vector3(x*t,y*t,z*t); }
    inline double operator * (Vector3 t) // scalar product
    { return x*t.x + y*t.y + z*t.z; }
    inline Vector3 operator ^ (Vector3 t) // cross product
    { return Vector3( y*t.z-z*t.y, t.x*z-x*t.z, x*t.y-y*t.x ); }
	*/
    //inline double dot( Vector3 const & a, Vector3 const & b )  { return a.x*b.x + a.y*b.y + a.z*b.z;  }
	inline double dot( Vector3 const & b )  { return x*b.x + y*b.y + z*b.z;  }
	
	/*inline Vector3 cross( Vector3 const & a, Vector3 const & b )
    {
      return Vector3( a.y * b.z - b.y * a.z, -a.x * b.z + b.x * a.z,  a.x * b.y - b.x * a.y );
    }
	*/
	inline Vector3 cross( Vector3 const & b )
    {
      return Vector3( y * b.z - b.y * z, -x * b.z + b.x * z,  x * b.y - b.x * y );
    }

	Vector3 & operator+= ( Vector3 const & v )
      {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
      }

      Vector3 & operator-= (  Vector3 const & v )
      {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
      }

      Vector3 & operator*=( double const & s )
      {
        x *= s;
        y *= s;
        z *= s;
        return *this;
      }

      Vector3 & operator/=( double const & s )
      {
        assert(s || !"Vector3::/=(): division by zero");
        x /= s;
        y /= s;
        z /= s;
        return *this;
      }

      Vector3    operator+ ( Vector3 const & v ) const {  return Vector3( x+v.x, y+v.y, z+v.z);                      }
      Vector3    operator- ( Vector3 const & v ) const {  return Vector3( x-v.x, y-v.y, z-v.z);                      }
      Vector3    operator- (                   ) const {  return Vector3(-x,-y,-z);                                     }
      //Vector3    operator% ( Vector3 const & v ) const {  return Vector3(y*v.z-v.y*z, v(0)*z-x*v.z, x*v.y-v.x*y);  }
      double  operator* ( Vector3 const & v ) const {  return x*v.x + y*v.y + z*v.z;                              }
     // inline Vector3 operator*( Vector3 const& v, double const& s )  {    return Vector3( v.x*s, v.y*s, v.z*s );  }
	  Vector3 operator*( double const& s)  {    return Vector3( x*s, y*s, z*s );  }
	  Vector3 divby( double const& s)  {    return Vector3( x/s, y/s, z/s );  }

	  
	  bool       operator<=( Vector3 const & v ) const {  return x <= v.x && y <= v.y && z <= v.z;                   }
	  bool       operator>=( Vector3 const & v ) const {  return x >= v.x && y >= v.y && z >= v.z; }
	 double & operator() ( index_type index )
      {
        assert( (index>=0 && index<3)  || !"vecto3():index should be in range [0..2]");
        return *((&x) + index);
      }

      double const & operator() ( index_type index ) const
      {
        assert( (index>=0 && index<3)  || !"vecto3():index should be in range [0..2]");
        return *((&x) + index);
      }

      double & operator[] ( index_type index )
      {
        assert( (index>=0 && index<3)  || !"vecto3[]:index should be in range [0..2]");
        return *((&x) + index);
      }

      double const & operator[] ( index_type index ) const
      {
        assert( (index>=0 && index<3)  || !"vecto3[]:index should be in range [0..2]");
        return *((&x) + index);
      }


	
	inline double length() // pythagorean length
    { return sqrt(x*x + y*y + z*z); }
    
	inline void unit() // normalized to a length of 1
    { 
	  double l = this->length();
	  if (l == 0.0) {x=y=z=0;}
	  else{
	  x/=l;
	  y/=l;
	  z/=l;
	  }
	}
	
	
	/*inline Vector3 unit(Vector3 const & v) // normalized to a length of 1
    { 
	  double l = v.length();
      if (l == 0.0) return Vector3(0.0,0.0,0.0);
      return Vector3(v.x/l,v.y/l,v.z/l); 
	}*/

    inline bool zero() // returns true if a zero vector
    { return x==0 && y==0 && z==0; }
    inline bool equals(Vector3 t) // returns true if exactly equal
    { return x==t.x && y==t.y && z==t.z; }
	inline void clear(){x=y=z=0.0;}

};
	
	

#endif