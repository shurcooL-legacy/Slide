#ifndef _ABMatrix3x3_H
#define _ABMatrix3x3_H

#include <cmath>
#include <cassert>
#include <iosfwd>
#include "vector3.h"
#include "quaternion.h"


class ABMatrix3x3
{
   typedef size_t                index_type;    // TODO should the index type be template parameterized?

    public:

      Vector3  m_row0;  ///<   The 1st row of the matrix
      Vector3  m_row1;  ///<   The 2nd row of the matrix
      Vector3  m_row2;  ///<   The 3rd row of the matrix

    public:

      ABMatrix3x3()
        : m_row0(0.0 ,0.0 ,0.0 )
        , m_row1(0.0 ,0.0 ,0.0 )
        , m_row2(0.0 ,0.0 ,0.0 )
      {}

      ~ABMatrix3x3(){}

      explicit ABMatrix3x3(
          double const & m00      , double const & m01      , double const & m02
        , double const & m10      , double const & m11      , double const & m12
        , double const & m20      , double const & m21      , double const & m22
        )
        : m_row0(m00,m01,m02)
        , m_row1(m10,m11,m12)
        , m_row2(m20,m21,m22)
      {}


      explicit ABMatrix3x3(Vector3 const & row0, Vector3 const & row1, Vector3 const & row2)
        : m_row0(row0)
        , m_row1(row1)
        , m_row2(row2)
      {}

      explicit ABMatrix3x3(Quaternion const & q)    {      *this = q;    }

      ABMatrix3x3(ABMatrix3x3 const & M)
        : m_row0(M.m_row0)
        , m_row1(M.m_row1)
        , m_row2(M.m_row2)
      {}

    public:

      double & operator()(index_type i, index_type j)
      { 
        assert( ( i>=0 && i<3 ) || !"ABMatrix3x3::(i,j) i must be in range [0..2]");
        assert( ( j>=0 && j<3 ) || !"ABMatrix3x3::(i,j) j must be in range [0..2]");
        return (*(&m_row0+i))(j); 
      }

      double const & operator()(index_type i, index_type j) const 
      { 
        assert( ( i>=0 && i<3 ) || !"ABMatrix3x3::(i,j) i must be in range [0..2]");
        assert( ( j>=0 && j<3 ) || !"ABMatrix3x3::(i,j) j must be in range [0..2]");
        return (*(&m_row0+i))(j); 
      }

      Vector3 & operator[](index_type i)
      { 
        assert( ( i>=0 && i<3 ) || !"ABMatrix3x3::(i,j) i must be in range [0..2]");
        return *(&m_row0+i); 
      }

      Vector3 const & operator[](index_type i) const 
      { 
        assert( ( i>=0 && i<3 ) || !"ABMatrix3x3::(i,j) i must be in range [0..2]");
        return *(&m_row0+i); 
      }

      ABMatrix3x3 & operator=( ABMatrix3x3 const & cpy )
      {
        m_row0=cpy.m_row0;
        m_row1=cpy.m_row1;
        m_row2=cpy.m_row2; 
        return *this;
      }

      // TODO: Comparing floats with == or != is not safe NOTE T might not be a float type it could be anything? This suggest that we need some kinf of metaprogramming technique to deal with ths problem?
      /*bool operator==(ABMatrix3x3 const & cmp ) const 
      {
        return (m_row0==cmp.m_row0) && (m_row1==cmp.m_row1) && (m_row2==cmp.m_row2);
      }

      bool operator!=( ABMatrix3x3 const & cmp ) const 
      {
        return !(*this==cmp);
      }
*/
      ABMatrix3x3   operator+  ( ABMatrix3x3 const & m   ) const { return ABMatrix3x3( m_row0+m.m_row0, m_row1+m.m_row1, m_row2+m.m_row2); }
      ABMatrix3x3   operator-  ( ABMatrix3x3 const & m   ) const { return ABMatrix3x3( m_row0-m.m_row0, m_row1-m.m_row1, m_row2-m.m_row2); }
      ABMatrix3x3 & operator+= ( ABMatrix3x3 const & m   )       { m_row0+=m.m_row0; m_row1+=m.m_row1; m_row2+=m.m_row2; return *this; }
      ABMatrix3x3 & operator-= ( ABMatrix3x3 const & m   )       { m_row0-=m.m_row0; m_row1-=m.m_row1; m_row2-=m.m_row2; return *this; }
      ABMatrix3x3 & operator*= ( double const & s  )       {m_row0*=s;m_row1*=s;m_row2*=s;return *this;}

      ABMatrix3x3 & operator/= ( double const & s  )       
      {
        assert(s || !"ABMatrix3x3/=(): division by zero");
        m_row0*=(1.0/s);
        m_row1*=(1.0/s);
        m_row2*=(1.0/s);
        return *this;
      }

      Vector3 operator*(Vector3 const &v) const { return Vector3(m_row0*v, m_row1*v, m_row2*v); }
      ABMatrix3x3    operator-()                      const { return ABMatrix3x3(-m_row0,-m_row1,-m_row2);         }

      size_t size1() const { return 3u; }
      size_t size2() const { return 3u; }

      /**
      * Assigns this quaternion to a matrix.
      * This method performs a conversion of a quaternion that represents a rotation into the correponding rotationmatrix.
      *
      * @param q          A reference to a quaternion. This is the quaternion that represents a rotation.
      */
      ABMatrix3x3  & operator=(Quaternion const & q)
      {
        m_row0(0) = 1.0 - 2.0 * ( (q.v().y*q.v().y) + (q.v().z*q.v().z));
        m_row1(1) = 1.0 - 2.0 * ( (q.v().x*q.v().x) + (q.v().z*q.v().z));
        m_row2(2) = 1.0 - 2.0 * ( (q.v().y*q.v().y) + (q.v().x*q.v().x));
        m_row1(0) =       2.0 * ( (q.v().x*q.v().y) + (q.s()*q.v().z));
        m_row0(1) =       2.0 * ( (q.v().x*q.v().y) - (q.s()*q.v().z));
        m_row2(0) =       2.0 * (-(q.s()*q.v().y)    + (q.v().x*q.v().z));
        m_row0(2) =       2.0 * ( (q.s()*q.v().y)    + (q.v().x*q.v().z));
        m_row2(1) =       2.0 * ( (q.v().z*q.v().y) + (q.s()*q.v().x));
        m_row1(2) =       2.0 * ( (q.v().z*q.v().y) - (q.s()*q.v().x));
        return *this;
      }

      void clear()
      {
        m_row0.clear();
        m_row1.clear();
        m_row2.clear();
      }

    public:

      friend ABMatrix3x3 fabs(ABMatrix3x3 const & A)
      {
        using std::fabs;

        return ABMatrix3x3(
          fabs(A(0,0)),  fabs(A(0,1)), fabs(A(0,2)),
          fabs(A(1,0)),  fabs(A(1,1)), fabs(A(1,2)),
          fabs(A(2,0)),  fabs(A(2,1)), fabs(A(2,2))
          );
      }

    public:

      /*Vector3 column(index_type i) const
      {
        assert((i>=0 && i<3) || !"ABMatrix3x3::column(): index must be in range [0..2]");

        return Vector3(m_row0(i), m_row1(i), m_row2(i));
      }

      void set_column(index_type i, Vector3 const & column)
      {
        assert((i>=0 && i<3) || !"ABMatrix3x3::set_column(): index must be in range [0..2]");

        m_row0(i) = column(0);
        m_row1(i) = column(1);
        m_row2(i) = column(2);      
      }
	  */
      Vector3       & row(index_type i)       { return (*this)[i]; }
      Vector3 const & row(index_type i) const { return (*this)[i]; }

   // }; // class ABMatrix3x3


    /*
    inline void random(ABMatrix3x3 & m )
    {
      random(m.row(0));
      random(m.row(1));
      random(m.row(2));
    }

    
	*/
    inline ABMatrix3x3 operator*(double const &s )
    {
      return ABMatrix3x3(m_row0*s, m_row1*s, m_row2*s);
    }

   /* inline ABMatrix3x3 operator*( double const & s, ABMatrix3x3 const & m )
    {
      return ABMatrix3x3(m.row(0)*s, m.row(1)*s, m.row(2)*s);
    }
*/
    
   /* inline ABMatrix3x3 operator/( double const & s )
    {
      return ABMatrix3x3(m_row0/s, m_row1/s, m_row2/s);
    }
*/
    
    /*inline ABMatrix3x3 operator/( double const & s, ABMatrix3x3 const & m )
    {
      return ABMatrix3x3(m.row(0)/s, m.row(1)/s, m.row(2)/s);
    }
*/
    static inline ABMatrix3x3 multby(ABMatrix3x3 const & A,ABMatrix3x3 const & B)
    {
      
		
	  ABMatrix3x3 C;

      for (int c=0; c<3; ++c)
        for (int r=0; r<3; ++r){
          C(r,c) = 0.0;
          for(int i=0; i<3; ++i){
            C(r,c) += A(r,i) * B(i,c);
          }
        }
        return C;
    }

    /**
    * Creates a rotation matrix.
    * This method returns a rotation matrix around the x-axe. It
    * assumes that post-multiplication by colum vectors is used.
    *
    * @param radians           The rotation angle in radians.
    */
  /*  inline ABMatrix3x3 Rx(double const & radians)
    {
      using std::cos;
      using std::sin;

      double cosinus = boost::numeric_cast<double>( cos(radians) );
      double sinus   = boost::numeric_cast<double>( sin(radians) );

      return ABMatrix3x3(
        1.0, 0.0,        0.0,
        0.0,              cosinus,                      -sinus,
        0.0,                sinus,                      cosinus
        );
    }
*/
    /**
    * Creates a rotation matrix.
    * This method returns a rotation matrix around the y-axe. It
    * assumes that post-multiplication by colum vectors is used.
    *
    * @param radians           The rotation angle in radians.
    */
   /* inline ABMatrix3x3 Ry(double const & radians)
    {
      using std::cos;
      using std::sin;

      //double cosinus = boost::numeric_cast<double>( cos(radians) );
      //double sinus   = boost::numeric_cast<double>( sin(radians) );
      double cosinus = double( cos(radians) );
      double sinus   = double( sin(radians) );
      
	  
	  return ABMatrix3x3(
        cosinus,		0.0,          sinus,
        0.0,			1.0,			0.0,
        -sinus,			0.0,        cosinus
        );
    }
*/
    /**
    * Creates a rotation matrix.
    * This method returns a rotation matrix around the z-axe. It assumes that post-multiplication by colum vectors is used.
    *
    * @param radians           The rotation angle in radians.
    */
  /*  inline ABMatrix3x3 Rz(double const & radians)
    {
      
      using std::cos;
      using std::sin;

      //double cosinus = boost::numeric_cast<double>( cos(radians) );
      //double sinus   = boost::numeric_cast<double>( sin(radians) );
	  double cosinus = double( cos(radians) );
      double sinus   = double( sin(radians) );


      return ABMatrix3x3(
        cosinus,       -sinus,       0.0,
        sinus,        cosinus,       0.0,
        0.0,			  0.0,       1.0
        );
    }
*/
    /**
    * Creates a rotation matrix.
    * This method returns a general rotation matrix around a specified axe. It assumes that post-multiplication by colum vectors is used.
    *
    * @param radians           The rotation angle in radians.
    * @param axe               A vector. This is the rotation axe.
    */
  /*  inline ABMatrix3x3 Ru(double const & radians, Vector3 const & axis)
    {
      using std::cos;
      using std::sin;

      //double cosinus = boost::numeric_cast<double>( cos(radians) );
      //double sinus   = boost::numeric_cast<double>( sin(radians) );
      double cosinus = double( cos(radians) );
      double sinus   = double( sin(radians) );
	  Vector3 u = unit(axis);

      //Foley p.227 (5.76)
      return ABMatrix3x3(
        u.x*u.x + cosinus*(1.0 - u.x*u.x),   u.x*u.y*(1.0-cosinus) - sinus*u.z,   u.x*u.z*(1.0-cosinus) + sinus*u.y,
        u.x*u.y*(1.0-cosinus) + sinus*u.z,    u.y*u.y + cosinus*(1.0 - u.y*u.y),  u.y*u.z*(1.0-cosinus) - sinus*u.x,
        u.x*u.z*(1.0-cosinus) - sinus*u.y,    u.y*u.z*(1.0-cosinus) + sinus*u.x,   u.z*u.z + cosinus*(1.0 - u.z*u.z)
        );
    }
*/
    /**
    * Direction of Flight (DoF)
    *
    * @param k   The desired direction of flight.
    */
    /*inline ABMatrix3x3 z_dof(Vector3 const & k)
    {
      Vector3 i,j;
      orthonormal_vectors(i,j,unit(k));
      return ABMatrix3x3(
        i(0) , j(0), k(0)
        , i(1) , j(1), k(1)
        , i(2) , j(2), k(2)
        );
    }
*/
    
    inline bool is_orthonormal(ABMatrix3x3 const & M,double const & threshold)
    {
      
      using std::fabs;

      assert(threshold>=0.0 || !"is_orthonormal(): threshold must be non-negative");

      double dot01 = M.m_row0*M.m_row1;
      double dot02 = M.m_row0*M.m_row2;
      double dot12 = M.m_row1*M.m_row2;
      if(fabs(dot01)>threshold) return false;
      if(fabs(dot02)>threshold) return false;
      if(fabs(dot12)>threshold) return false;
      double dot00 = M.m_row0*M.m_row0;
      double dot11 = M.m_row1*M.m_row1;
      double dot22 = M.m_row2*M.m_row2;
      if((dot00-1.0)>threshold)        return false;
      if((dot11-1.0)>threshold)        return false;
      if((dot22-1.0)>threshold)        return false;
      return true;
    }

   
    inline bool is_orthonormal(ABMatrix3x3 const & M) 
    {  
      
      return is_orthonormal(M, 0.0); 
    }

   
    inline bool is_zero(ABMatrix3x3 M, double const & threshold)
    {
      
      using std::fabs;

      assert(threshold>=0.0 || !"is_zero(): threshold must be non-negative");

      if(fabs(M(0,0))>threshold)      return false;
      if(fabs(M(0,1))>threshold)      return false;
      if(fabs(M(0,2))>threshold)      return false;
      if(fabs(M(1,0))>threshold)      return false;
      if(fabs(M(1,1))>threshold)      return false;
      if(fabs(M(1,2))>threshold)      return false;
      if(fabs(M(2,0))>threshold)      return false;
      if(fabs(M(2,1))>threshold)      return false;
      if(fabs(M(2,2))>threshold)      return false;
      return true;
    }

    
    inline bool is_zero(ABMatrix3x3 const & M) 
    {  
      
      return is_zero(M, 0.0); 
    }

    
    inline bool is_symmetric(ABMatrix3x3 M, double const & threshold)
    {
      
      using std::fabs;

      assert(threshold>=0.0 || !"is_symmetric(): threshold must be non-negative");

      if(fabs(M(0,1)-M(1,0))>threshold)      return false;
      if(fabs(M(0,2)-M(2,0))>threshold)      return false;
      if(fabs(M(1,2)-M(2,1))>threshold)      return false;
      return true;
    }

    
    inline bool is_symmetric(ABMatrix3x3 const & M) 
    {  
      
      return is_symmetric(M, 0.0); 
    }

    inline bool is_diagonal(ABMatrix3x3 M, double const & threshold)
    {
      
      using std::fabs;

      assert(threshold>=0.0 || !"is_diagonal(): threshold must be non-negative");

      if(fabs(M(0,1))>threshold)      return false;
      if(fabs(M(0,2))>threshold)      return false;
      if(fabs(M(1,0))>threshold)      return false;
      if(fabs(M(1,2))>threshold)      return false;
      if(fabs(M(2,0))>threshold)      return false;
      if(fabs(M(2,1))>threshold)      return false;
      return true;
    }

    
    inline bool is_diagonal(ABMatrix3x3 const & M) 
    {  
      
      return is_diagonal(M, 0.0); 
    }

    
    inline bool is_identity(ABMatrix3x3 M, double const & threshold)
    {
      
      using std::fabs;

      assert(threshold>=0.0 || !"is_identity(): threshold must be non-negative");

      if(fabs(M(0,0)-1.0)>threshold)    return false;
      if(fabs(M(0,1))>threshold)      return false;
      if(fabs(M(0,2))>threshold)      return false;
      if(fabs(M(1,0))>threshold)      return false;
      if(fabs(M(1,1)-1.0)>threshold)    return false;
      if(fabs(M(1,2))>threshold)      return false;
      if(fabs(M(2,0))>threshold)      return false;
      if(fabs(M(2,1))>threshold)      return false;
      if(fabs(M(2,2)-1.0)>threshold)    return false;
      return true;
    }

    
    inline bool is_identity(ABMatrix3x3 const & M) 
    {  
     
      return is_identity(M, 0.0); 
    }

    
    inline ABMatrix3x3 outer_prod( Vector3 const & v1, Vector3 const & v2 )
    {
      return ABMatrix3x3(
        ( v1( 0 ) * v2( 0 ) ), ( v1( 0 ) * v2( 1 ) ), ( v1( 0 ) * v2( 2 ) ),
        ( v1( 1 ) * v2( 0 ) ), ( v1( 1 ) * v2( 1 ) ), ( v1( 1 ) * v2( 2 ) ),
        ( v1( 2 ) * v2( 0 ) ), ( v1( 2 ) * v2( 1 ) ), ( v1( 2 ) * v2( 2 ) ) 
        );
    }

    
    /*inline ABMatrix3x3 ortonormalize(ABMatrix3x3 const & A)
    {
  
      Vector3   row0(A(0,0),A(0,1),A(0,2));
      Vector3   row1(A(1,0),A(1,1),A(1,2));
      Vector3   row2(A(2,0),A(2,1),A(2,2));

      double const l0 = length(row0);
      if(l0) row0 /= l0;

      row1 -=  row0 * dot(row0 , row1);
      double const l1 = length(row1);
      if(l1) row1 /= l1;

      row2 = cross( row0 , row1);

      return ABMatrix3x3(
        row0(0), row0(1), row0(2),
        row1(0), row1(1), row1(2),
        row2(0), row2(1), row2(2)
        );
    }

   */
    static inline ABMatrix3x3 trans(ABMatrix3x3 const & M)
    {
      return ABMatrix3x3(
        M(0,0),  M(1,0), M(2,0),
        M(0,1),  M(1,1), M(2,1),
        M(0,2),  M(1,2), M(2,2)
        );
    }


    
    static inline ABMatrix3x3 diag(double const & d0,double const & d1,double const & d2 )
    {
    
      return ABMatrix3x3(
        d0,  0.0,  0.0,
        0.0,                    d1,  0.0,
        0.0,  0.0,                    d2
        );
    }

    
    static inline ABMatrix3x3 diag(Vector3 const & d)  {    return diag( d(0), d(1), d(2));  }

   
    static inline ABMatrix3x3 diag(double const & d )  {    return diag(d,d,d);  }

    /*inline ABMatrix3x3 inverse(ABMatrix3x3 const & A)
    {
      
      //From messer p.283 we know
      //
      //  -1     1
      // A   = -----  adj A
      //       det A
      //
      //                      i+j
      // ij-cofactor of A = -1    det A
      //                               ij
      //
      // i,j entry of the adjoint.
      //                                   i+j
      // adjoint A   = ji-cofactor = A = -1    det A
      //          ij                                ji
      //
      // As it can be seen the only numerical error
      // in these calculations is from the resolution
      // of the scalars. So it is a very accurate method.
      //
      ABMatrix3x3 adj;
      adj(0,0) = A(1,1)*A(2,2) - A(2,1)*A(1,2);
      adj(1,1) = A(0,0)*A(2,2) - A(2,0)*A(0,2);
      adj(2,2) = A(0,0)*A(1,1) - A(1,0)*A(0,1);
      adj(0,1) = A(1,0)*A(2,2) - A(2,0)*A(1,2);
      adj(0,2) = A(1,0)*A(2,1) - A(2,0)*A(1,1);
      adj(1,0) = A(0,1)*A(2,2) - A(2,1)*A(0,2);
      adj(1,2) = A(0,0)*A(2,1) - A(2,0)*A(0,1);
      adj(2,0) = A(0,1)*A(1,2) - A(1,1)*A(0,2);
      adj(2,1) = A(0,0)*A(1,2) - A(1,0)*A(0,2);
      double det = A(0,0)*adj(0,0) -  A(0,1)*adj(0,1) +   A(0,2)*adj(0,2);
      if(det)
      {
        adj(0,1) = -adj(0,1);
        adj(1,0) = -adj(1,0);
        adj(1,2) = -adj(1,2);
        adj(2,1) = -adj(2,1);
        return trans(adj)/det;
      }

      return diag(1.0);
    }

    */
    inline double max_value(ABMatrix3x3 const & A)
    {
      using std::max;

      return max(A(0,0), max( A(0,1), max( A(0,2), max ( A(1,0), max ( A(1,1), max ( A(1,2), max (A(2,0), max ( A(2,1) , A(2,2) ) ) ) ) ) ) ) );
    }

    
    inline double min_value(ABMatrix3x3 const & A)
    {
      using std::min;

      return min(A(0,0), min( A(0,1), min( A(0,2), min ( A(1,0), min ( A(1,1), min ( A(1,2), min (A(2,0), min ( A(2,1) , A(2,2) ) ) ) ) ) ) ) );
    }

    
    inline double det(ABMatrix3x3 const & A)
    {
      return A(0,0)*(A(1,1)*A(2,2) - A(2,1)*A(1,2)) -  A(0,1)*(A(1,0)*A(2,2) - A(2,0)*A(1,2)) +   A(0,2)*(A(1,0)*A(2,1) - A(2,0)*A(1,1));
    }

    
    inline double trace(ABMatrix3x3 const & A)
    {
      return (A(0,0) + A(1,1) + A(2,2));
    }

    
    inline double  norm_1(ABMatrix3x3 const & A)
    {
      using std::fabs;
      using std::max;

      double r0 = fabs(A(0,0)) + fabs(A(0,1)) +  fabs(A(0,2));
      double r1 = fabs(A(1,0)) + fabs(A(1,1)) +  fabs(A(1,2));
      double r2 = fabs(A(2,0)) + fabs(A(2,1)) +  fabs(A(2,2));

      return max ( r0, max( r1, r2 ) );
    }

    
   /* inline double norm_2(ABMatrix3x3 const & A)
    {
      using std::fabs;
      using std::max;
      using std::sqrt;

      ABMatrix3x3 V;
      typename ABMatrix3x3::Vector3 d;
      math::eigen(A,V,d);

      double lambda = max( fabs(d(0)), max( fabs(d(1)) , fabs(d(2)) ));
      return sqrt( lambda );
    }
*/
    
    inline double  norm_inf(ABMatrix3x3 const & A)
    {
      using std::fabs;
      using std::max;

      double c0 = fabs(A(0,0)) + fabs(A(1,0)) +  fabs(A(2,0));
      double c1 = fabs(A(0,1)) + fabs(A(1,1)) +  fabs(A(2,1));
      double c2 = fabs(A(0,2)) + fabs(A(1,2)) +  fabs(A(2,2));
      return max ( c0, max( c1, c2 ) );
    }

    
    inline ABMatrix3x3 truncate(ABMatrix3x3 const & A,double const & epsilon)
    {
      
      using std::fabs;

      assert(epsilon>0.0 || !"truncate(ABMatrix3x3,epsilon): epsilon must be positive");

      return ABMatrix3x3(
        fabs(A(0,0))<epsilon?0.0:A(0,0),
        fabs(A(0,1))<epsilon?0.0:A(0,1),
        fabs(A(0,2))<epsilon?0.0:A(0,2),

        fabs(A(1,0))<epsilon?0.0:A(1,0),
        fabs(A(1,1))<epsilon?0.0:A(1,1),
        fabs(A(1,2))<epsilon?0.0:A(1,2),

        fabs(A(2,0))<epsilon?0.0:A(2,0),
        fabs(A(2,1))<epsilon?0.0:A(2,1),
        fabs(A(2,2))<epsilon?0.0:A(2,2)
        );
    }

    /**
    * Sets up the cross product matrix.
    * This method is usefull for expression the cross product as a matrix multiplication.
    *
    * @param  v       A reference to a vector. This is first argument of the cross product.
    */
    inline ABMatrix3x3 star(Vector3 const &v)
    {
      //--- Changes a cross-product into a matrix multiplication.
      //--- Rewrites the component of a Vector3 cross-product as a matrix.
      //--- a x b = a*b = ba*
      return ABMatrix3x3(   
        0.0,                   -v.z,                     v.y,  
        v.z,					0.0,                    -v.x,
        -v.y,                   v.x,					 0.0
        );
    }

    
    static inline  void  printMatrix (std::ostream & o,ABMatrix3x3 const & A)
    {
      o << "[" << A(0,0)
        << " " << A(0,1)
        << " " << A(0,2)
        << " " << A(1,0)
        << " " << A(1,1)
        << " " << A(1,2)
        << " " << A(2,0)
        << " " << A(2,1)
        << " " << A(2,2)
        << "]";
    }

    
    /*inline std::istream & operator>>(std::istream & i,ABMatrix3x3 & A)
    {
      char dummy;
      i >> dummy;
      i >> A(0,0);
      i >> dummy;
      i >> A(0,1);
      i >> dummy;
      i >> A(0,2);
      i >> dummy;
      i >> A(1,0);
      i >> dummy;
      i >> A(1,1);
      i >> dummy;
      i >> A(1,2);
      i >> dummy;
      i >> A(2,0);
      i >> dummy;
      i >> A(2,1);
      i >> dummy;
      i >> A(2,2);
      i >> dummy;
      return i;
    }*/

};
#endif
