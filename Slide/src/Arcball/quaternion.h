#ifndef _QUATERNION_H
#define _QUATERNION_H

//#include "matrix3x3.h"
#include "vector3.h"
#include <cmath>
#include <iosfwd>


class Quaternion
{
    public:

      typedef size_t                 index_type;    // TODO should the index type be template parameterized?

    protected:

      double   m_s;      ///< The real part.
      Vector3 m_v;      ///< The R3 part or imaginary part.

    public:

      double       & s()       { return m_s; }
      double const & s() const { return m_s; }

      Vector3       & v()       { return m_v; }
      Vector3 const & v() const { return m_v; }

    public:

      Quaternion()
        : m_s(1.0)
        , m_v(0.0,0.0,0.0)
      {}

      ~Quaternion(){}

      Quaternion(Quaternion const & q) {  *this = q;    }

      //explicit Quaternion(Matrix3x3 const & M){  *this = M; }

      explicit Quaternion(double const & s_val, double const & x, double const & y, double const & z)
        : m_s(s_val)
        , m_v(x,y,z)
      { }

      explicit Quaternion(double const & s_val, Vector3 const & v_val)
        : m_s(s_val)
        , m_v(v_val)
      { }

      Quaternion & operator=(Quaternion const & cpy)
      {
        m_s = cpy.m_s;
        m_v = cpy.m_v;
        return *this;
      }

      /**
      * Assignment operator.
      * This method is a little speciel. Quaternions can be used to represent rotations, so can matrices.
      * This method transforms a rotation matrix into a Quaternion and then it assigns it to this Quaternion.
      * In short this method converts matrices into quaternions.
      *
      * @param M       A reference to a matrix. This matrix should be a rotation matrix. That is an othogonal matrix.
      */
      /*Quaternion & operator=(Matrix3x3 const & M)
      {
        using std::sqrt;

        double const & M00 = M(0,0);
        double const & M01 = M(0,1);
        double const & M02 = M(0,2);
        double const & M10 = M(1,0);
        double const & M11 = M(1,1);
        double const & M12 = M(1,2);
        double const & M20 = M(2,0);
        double const & M21 = M(2,1);
        double const & M22 = M(2,2);

        double tr = M00 + M11 + M22;
        double r;

        double const half = 1.0/2.0;

        if(tr>=0.0)
        {
          r      = sqrt(tr + 1.0);
          m_s    = half*r;
          r      = half/r;
          m_v[0] = (M21 - M12) * r;
          m_v[1] = (M02 - M20) * r;
          m_v[2] = (M10 - M01) * r;
        }
        else
        {
          int i = 0;
          if(M11>M00)
            i = 1;
          if(M22>M(i,i))
            i = 2;
          switch(i)
          {
          case 0:
            r      = sqrt((M00 - (M11+M22)) + 1.0);
            m_v[0] = half*r;
            r      = half/r;
            m_v[1] = (M01 + M10) * r;
            m_v[2] = (M20 + M02) * r;
            m_s    = (M21 - M12) * r;
            break;
          case 1:
            r      = sqrt((M11 - (M22+M00)) + 1.0);
            m_v[1] = half*r;
            r      = half/r;
            m_v[2] = (M12 + M21)*r;
            m_v[0] = (M01 + M10)*r;
            m_s    = (M02 - M20)*r;
            break;
          case 2:
            r      = sqrt((M22 - (M00+M11)) + 1.0);
            m_v[2] = half*r;
            r      = half/r;
            m_v[0] = (M20 + M02) * r;
            m_v[1] = (M12 + M21) * r;
            m_s    = (M10 - M01) * r;
            break;
          };
        }
        return *this;
      }*/

    public:

      // TODO: Comparing floats with == or != is not safe NOTE double might not be a float type it could be anything? This suggest that we need some kinf of metaprogramming technique to deal with ths problem?
     // bool operator==(Quaternion const & cmp) const    {      return m_s==cmp.m_s && m_v==cmp.m_v;    }
     // bool operator!=(Quaternion const & cmp) const    {      return !(*this==cmp);                   }

    public:

      Quaternion & operator+= (Quaternion const & q )
      {
        m_s += q.m_s;
        m_v += q.m_v;
        return *this;
      }

      Quaternion & operator-= (Quaternion const & q )
      {
        m_s -= q.m_s;
        m_v -= q.m_v;
        return *this;
      }

      Quaternion operator+ ( Quaternion const &q ) const    {      return Quaternion(m_s + q.m_s, m_v + q.m_v);    }
      Quaternion operator- ( Quaternion const &q ) const    {      return Quaternion(m_s - q.m_s, m_v - q.m_v);    }

      Quaternion & operator*=(double const &s_val )
      {
        m_s *= s_val;
        m_v *= s_val;
        return *this;
      }

      Quaternion & operator/=( double const &s_val )
      {
        assert(s_val || !"quaterion::operator/=(): division by zero");
        m_s /= s_val;
        m_v /= s_val;
        return *this;
      }

      Quaternion operator-() const  {  return Quaternion(-m_s,-m_v); }

      double operator*(Quaternion const &q) const  {  return m_s*q.m_s + m_v*q.m_v;  }

      /**
      * Multiplication of quaternions.
      * This method multiplies two quaterions with each other
      * and assigns the result to this Quaternion.
      *
      * @param b       The second Quaternion.
      */
      /*Quaternion operator%(Quaternion const & b)
      {
        return Quaternion( m_s*b.s()  - m_v.dot(b.v()),  
							m_v.cross(b.v()) + m_s*b.v() + b.s()*m_v );
      }
*/
      /**
      * Quaternion Vector Multiplication.
      *
      * @param v    A Quaternion as a vector, i.e. zero scalar value.
      */
      /*Quaternion operator%(Vector3 const & v_val)
      {
        return Quaternion( - dot( m_v() , v_val) ,  cross(m_v , v_val) + v_val*m_s  );
      }

    public:

      void random(double const & min_value, double const & max_value)
      {
        Random<double> value(min_value,max_value);
        m_v(0) = value();
        m_v(1) = value();
        m_v(2) = value();
        m_s = value();
      }

      void random()    {      random(0.0,1.0);    }
*/
      /**
      * A rotation around an arbitary axe.
      * This method constructs a unit Quaternion which represents the specified rotation.
      *
      * @param rad           The rotation angle in radians around the axe.
      * @param axe           The axe of rotation.
      */
      /*void Ru(double const & rad,Vector3 const & axe)
      {
        using std::cos;
        using std::sin;

        double teta = rad/2.0;

        double cteta = boost::numeric_cast<double>( cos(teta) );
        double steta = boost::numeric_cast<double>( sin(teta) );
        m_s = cteta;

        m_v = unit(axe) * steta;
      }
		*/
      /**
      * Assigns the quaterion to the identity rotation.
      * This is a commonly used constant. It corresponds to the identity matrix.
      */
      void identity()
      {
        m_s = 1.0;
        m_v.clear();
      }

      /**
      * This method constructs a unit Quaternion which represents the specified rotation around the x-axe.
      *
      * @param rad    The rotation angle in radians around the axe.
      */
      void Rx(double const & rad)
      {
        using std::cos;
        using std::sin;

        double teta  = rad/2.0;
        double cteta = (double)( cos(teta) );
        double steta = (double)( sin(teta) );

        m_s    = cteta; 
        m_v.x = steta;
        m_v.y = 0.0;
        m_v.z = 0.0;
      }

      /**
      * This method constructs a unit Quaternion which represents the specified rotation around the y-axe.
      *
      * @param rad   The rotation angle in radians around the axe.
      */
      void Ry(double const & rad)
      {
        using std::cos;
        using std::sin;

        double teta  = rad/2.0;
        double cteta = (double)( cos(teta) );
        double steta = (double)( sin(teta) );

        m_s    = cteta;
        m_v.x = 0.0;
        m_v.y = steta;
        m_v.z = 0.0;
      }

      /**
      * This method constructs a unit Quaternion which represents the specified rotation around the z-axe.
      *
      * @param rad    The rotation angle in radians around the axe.
      */
      void Rz(double const & rad)
      {
        using std::cos;
        using std::sin;

        double teta  = rad/2.0;
        double cteta = (double)( cos(teta) );
        double steta = (double)( sin(teta) );

        m_s    = cteta;
        m_v.x = 0.0;
        m_v.y = 0.0;
        m_v.z = steta;
      }

      /**
      * Rotate Vector by Quaternion.
      *
      *  computes  r' = q*r*conj(q)
      */
//      Vector3 rotate(Vector3 const & r) const    {      return ((*this) % r  % conj(*this)).v();    }

    public:

      /**
      * Equality comparison with an error bound.
      * The test is exactly the same as with the method without errorbound the only difference
      * is that the corresponding terms of the quaternions are said to be equal if they do not
      * differ by more than the error bound value.
      *
      * @param q           A reference to a Quaternion. This is the
      *                    Quaternion to compare with.
      * @param threshold   A reference to the acceptable error bound.
      *
      * @return            The test result. The return value is one if
      *                    the quaternions are equal otherwise the return
      *                    value is zero.
      */
      /*bool is_equal(Quaternion const & q,double const & threshold) const
      {
        using std::fabs;

        assert( threshold>=0.0 || !"is_equal(): threshold must be non-negative");

        return fabs(m_s-q.m_s)<threshold && m_v.is_equal(q.v(),threshold);
      }

    };*/
    
  /*  inline Vector3 rotate(Quaternion const & q, Vector3 const & r)
    {
      return prod( prod(q , r)  , conj(q)).v();
    }
*/
    inline Quaternion prod(Quaternion const & a, Quaternion const & b)
    {
		Quaternion temp1 (a.s(),a.v());
		Quaternion temp2 (b.s(),b.v());
		
		return Quaternion(  temp1.s()*temp2.s()  - temp1.v().dot(temp2.v()),  temp1.v().cross(temp2.v()) + temp2.v()*temp1.s() + temp1.v()*temp2.s()  );
	}

   /* inline Quaternion prod(Quaternion const & a, Vector3 const & b)
    {
      return Quaternion(  - dot(a.v() , b),   cross(a.v() , b) + b*a.s()  );
    }

    
    inline Quaternion prod(Vector3 const & a, Quaternion const & b)
    {
      return Quaternion( - dot(a , b.v()),  cross(a , b.v()) + a*b.s()  );
    }
*/
    
   // inline Quaternion operator%(Quaternion const & a, Quaternion const & b)  { return prod(a,b); }
   // inline Quaternion operator%(Quaternion const & a, Vector3 const & b)     { return prod(a,b); }
   // inline Quaternion operator%(Vector3 const & a, Quaternion const & b)     { return prod(a,b); }
   // inline Quaternion operator*( const Quaternion &q, const double &s_val )  {    return Quaternion( q.s()*s_val, q.v()*s_val);  }
  //  inline Quaternion operator*( const double &s_val, const Quaternion &q )  {    return Quaternion( q.s()*s_val, q.v()*s_val);  }
  //  inline Quaternion operator/( const Quaternion &q, const double &s_val )  {    return Quaternion( q.s()/s_val, q.v()/s_val);  }
  //  inline Quaternion operator/( const double &s_val, const Quaternion &q )  {    return Quaternion( q.s()/s_val, q.v()/s_val);  }

    
    inline double const length(Quaternion const & q)
    { 
      using std::sqrt;
      return sqrt( q*q );
    }

    
    inline Quaternion unit(Quaternion const & q)
    {
     
      using std::sqrt;
      using std::fabs;

      double l = length(q);
	  Vector3 temp=q.v();
      if(fabs(l) > 0.0)
        return Quaternion (q.s()/l,temp.divby(l));
      return Quaternion (0.0,0.0,0.0,0.0);
    }
	    
    inline Quaternion normalize(Quaternion const & q)  {    return unit(q);      }

    /**
    * Natural Logarithm
    * Returns the Quaternion equal to the natural logarithm of
    * the specified Quaternion.
    *
    * @param q   A reference to an unit quaterion.
    * @return
    */
    
    /*inline Quaternion log(Quaternion const & q)
    {
      typedef typename Quaternion::value_traits   value_traits;

      using std::acos;
      using std::sin;

      if(q.s()==1.0 && is_zero(q.v()))
        return Quaternion(0.0,0.0,0.0,0.0);

      T teta = boost::numeric_cast<T>( acos(q.s()) );
      T st   = boost::numeric_cast<T>( sin(teta)   );
      return Quaternion( 0.0, q.v()*(teta/st) );
    }
*/
    /**
    * Orthogonal Quaternion.
    * This method sets this Quaternion to an orthogonal Quaternion
    * of the specififed Quaternion. In otherwords the resulting
    * angle between the specified Quaternion and this Quaternion
    * is pi/2.
    */
    
    inline Quaternion hat(Quaternion const & q)
    {
      return Quaternion( q.v().z, - q.v().y , q.v().x, -q.s());
    }

    /**
    * Expoent
    * Sets the Quaternion equal to the expoent of
    * the specified Quaternion.
    *
    * @param q    A reference to a pure Quaternion (zero
    *             T part).
    */
    
   /* inline Quaternion exp(Quaternion const & q)
    {
      using std::sqrt;
      using std::cos;
      using std::sin;

      //--- teta^2 x^2 + teta^2 y^2 +teta^2 z^2 =
      //--- teta^2 (x^2 + y^2 + z^2) = teta^2
      T teta = boost::numeric_cast<T>(  sqrt(q.v() *q.v())  );
      T ct   = boost::numeric_cast<T>(  cos(teta)           );
      T st   = boost::numeric_cast<T>(  sin(teta)           );

      return Quaternion(ct,q.v()*st);
    }
*/
    /**
    * QLERP - Linear Interpolation of Quaterions.
    *
    * @param A		Quaternion A
    * @param B		Quaternion B
    * @param w    The weight
    *
    * @return     The resulting Quaternion, (1-w)A+w B. Note the resulting
    *             Quaternion may not be a unit quaterion eventhough A and B are
    *             unit quaterions. If a unit Quaternion is needed write
    *             unit(qlerp(A,B,w)).
    *
    *		-Added by spreak for the SBS algorithm, see OpenTissue/kinematics/skinning/SBS
    */
    
    // TODO why not simply call this function lerp?
/*    inline Quaternion qlerp(Quaternion const & A,Quaternion const & B,T const & w)
    {
      typedef typename Quaternion::value_traits   value_traits;

      assert(w>=0.0 || !"qlerp(): w must not be less than 0");
      assert(w<=1.0  || !"qlerp(): w must not be larger than 1");	  
      T mw = 1.0 - w; 
      return ((mw * A) + (w * B));
    }
*/
    /**
    * Spherical Linear Interpolation of Quaterions.
    *
    * @param A
    * @param B
    * @param w        The weight
    *
    * @return          The resulting Quaternion.
    */
    
  /*  inline Quaternion slerp(Quaternion const & A,Quaternion const & B,T const & w)
    {
      typedef typename Quaternion::value_traits   value_traits;

      using std::acos;
      using std::sin;

      assert(w>=0.0 || !"slerp(): w must not be less than 0");
      assert(w<=1.0  || !"slerp(): w must not be larger than 1");	  

      T q_tiny = boost::numeric_cast<T>( 10e-7 );  // TODO prober constant type conversion?

      T norm = A*B;

      bool flip = false;
      if( norm < 0.0 )
      {
        norm = -norm;
        flip = true;
      }
      T weight = w;
      T inv_weight;
      if(1.0 - norm < q_tiny)
      {
        inv_weight = 1.0 - weight;
      }
      else
      {
        T theta    = boost::numeric_cast<T>( acos(norm)                                          );
        T s_val    = boost::numeric_cast<T>( 1.0 / sin(theta)                    ); 
        inv_weight = boost::numeric_cast<T>( sin((1.0 - weight) * theta) * s_val );
        weight     = boost::numeric_cast<T>( sin(weight * theta) * s_val                         );
      }
      if(flip)
      {
        weight = -weight;
      }
      return ( inv_weight * A + weight * B);
    }
*/
    /**
    * "Cubical" Sphereical Interpolation.
    * In popular terms this correpons to a cubic spline in
    * ordinary 3D space. However it is really a serie of
    * spherical linear interpolations, which defines a
    * cubic on the unit Quaternion sphere.
    *
    * @param q0
    * @param q1
    * @param q2
    * @param q3
    * @param u
    *
    * @return
    */
  /*  
    inline Quaternion squad(
      Quaternion const & q0
      , Quaternion const & q1
      , Quaternion const & q2
      , Quaternion const & q3
      , T const & u
      )
    {
      typedef typename Quaternion::value_traits   value_traits;

      assert(u>=0.0 || !"squad(): u must not be less than 0");
      assert(u<=1.0  || !"squad(): u must not be larger than 1");	  

      T u2 = 2.0 *u*(1.0 -u); 
      return slerp( slerp(q0,q3,u), slerp(q1,q2,u), u2);
    }
*/
    /**
    * Quaternion Conjugate.
    */
    
    inline Quaternion conj(Quaternion const & q)
    {
      return Quaternion(q.s(),-q.v());
    }

    /**
    * Get Axis Angle Representation.
    * This function converts a unit-quaternion into the
    * equivalent angle-axis representation.
    *
    * @param Q       The quaternion
    * @param axis    Upon return this argument holds value of the equivalent rotation axis.
    * @param theta   Upon return this argument holds value of the equivalent rotation angle.
    */
    
    /*inline void get_axis_angle(Quaternion const & Q,Vector3 & axis, T & theta)
    {
      using std::atan2;

      typedef typename Quaternion::value_traits     value_traits;
      typedef          Vector3                      V;

      //
      // By definition a unit quaternion Q can be written as
      //
      //    Q = [s,v] = [cos(theta/2), n sin(theta/2)]
      //
      // where n is a unit vector. This is the same as a rotation of
      // theta radian around the axis n.
      //
      //
      // Rotations are difficult to work with for several reasons.
      //
      // Firstly both Q and -Q represent the same rotation. This is
      // easily proven, rotate a arbitary vector r by Q then we have
      //
      //   r^\prime = Q r Q^*
      //
      // Now rotate the same vector by -Q
      //
      //   r^\prime = (-Q) r (-Q)^* = Q r Q^*
      //
      // because -Q = [-s,-v] and (-Q)^* = [-s , v] = - [s,-v]^* = - Q^*.
      //
      // Thus the quaternion representation of a single rotation is not unique.
      //
      // Secondly the rotation it self is not well-posed. A rotation of theta
      // radians around the unit axis n could equally well be done as a rotation
      // of -theta radians around the negative unit axis n.
      //
      // This is seen by straightforward substitution
      //
      //  [ cos(-theta/2), sin(-theta/2) (-n) ] = [ cos(theta/2), sin(theta/2) n ]
      // 
      // Thus we get the same quaternion regardless of whether we
      // use (+theta,+n) or (-theta,-n).
      //
      //
      // From the Quaternion we see that
      //
      //   \frac{v}{\norm{v}}  = \frac{ sin(theta/2) n }{| sin(theta/2) | } = sign(sin(theta/2)) n
      //
      // Thus we can easily get the rotation axis. However, we can not immediately
      // determine the positive rotation axis direction. The problem boils down to the
      // fact that we can not see the sign of the sinus-factor.
      //
      // Let us proceed by setting
      //
      //   x =    cos(theta/2)   =  s
      //   y =  | sin(theta/2) | =  \norm{v}
      //
      // Then we basically have two possibilities for finding theta
      //
      //  theta_1 = 2 atan2( y, x)        equivalent to      sign(sin(theta/2)) = 1
      //
      // or 
      //
      //  theta_2 = 2 atan2( -y, x)       equivalent to      sign(sin(theta/2)) = -1
      //
      // If theta_1 is the solution we have
      //
      //  n = \frac{v}{\norm{v}}
      //
      // If theta_2 is the solution we must have
      //
      //  n = - \frac{v}{\norm{v}}
      //
      // Observe that we always have theta_2 = 2 pi - theta_1. Therefore theta_1 < theta_2.
      //
      // Let us imagine that we always choose $theta_1$ as the solution then
      // the correspoding quaternion for that solution would be
      //
      //
      //         Q_1 = [cos(theta_1/2),  sin(theta_1/2)   \frac{v}{\norm{v}}]
      //             = [s ,  \norm{v}   \frac{v}{\norm{v}}] 
      //             = Q
      //
      // Now if we choose theta_2 as the solution we would have
      //
      //         Q_2 = [cos(theta_2/2),  sin(theta_2/2)   -\frac{v}{\norm{v}}]
      //             = [s ,  -\norm{v}   -\frac{v}{\norm{v}}] 
      //             = [s ,  \norm{v}   \frac{v}{\norm{v}}] 
      //             = Q
      //
      // Thus we observe that regardless of which solution we pick we always have Q = Q_1 = Q_2.
      //
      // At this point one may be confused. However, it should be clear that theta_2 is equivalent
      // to the theta_1 rotation. The difference is simply that theta_2 corresponds to flipping the
      // rotation axis of the theta_1 case.
      //
      double const ct2   = Q.s();           //---   cos(theta/2)
      double const st2   = length( Q.v() ); //---  |sin(theta/2)|

      theta = 2.0* atan2(st2,ct2);

      assert( st2 >= 0.0   || !"get_axis_angle(): |sin(theta/2)| must be non-negative");
      assert( theta >= 0.0 || !"get_axis_angle(): theta must be non-negative");
      assert( is_number(theta)              || !"get_axis_angle(): NaN encountered");

      axis = st2 > 0.0 ? Q.v() / st2 : V(0.0, 0.0, 0.0);
    }
	*/

    /**
    *  Get Rotation Angle wrt. an Axis.
    * Think of it as if you have a fixated body A so only body B is allowd to move.
    *
    * Let BF_B' indicate the initial orientation of body B's body frame
    * and BF_B the current orientation, now BF_A should be thought of as
    * being immoveable ie. constant.
    *
    *   Q_initial : BF_A -> BF_B'
    *   Q_cur     : BF_A -> BF_B
    *
    * Now we must have the relation
    *
    *   Q_rel Q_initial = Q_cur
    *
    * From which we deduce
    *
    *   Q_rel = Q_cur conj(Q_initial)
    *
    * And we see that
    *
    *   Q_rel : BF_B' -> BF_B
    *
    * That is how much the body frame of body B have rotated (measured
    * with respect to the fixed body frame A).
    *
    *
    * @param Q_rel    Rotation from initial orientation of B to current orientation of B.
    * @param axis     The rotation axis in the body frame of body B.
    *
    * @return     The angle in radians.
    */
    
    /*inline T get_angle(Quaternion const & Q_rel,Vector3 const & axis)
    {
      typedef typename Quaternion::value_traits   value_traits;

      using std::atan2;

      //--- The angle between the two bodies is extracted from the Quaternion Q_rel
      //---
      //---    [s,v] = [ cos(theta/2) , sin(theta/2) * u ]
      //---
      //--- where s is a double and v is a 3-vector. u is a unit length axis and
      //--- theta is a rotation along that axis.
      //---
      //--- we can get theta/2 by:
      //---
      //---    theta/2 = atan2 ( sin(theta/2) , cos(theta/2) )
      //---
      //--- but we can not get sin(theta/2) directly, only its absolute value:
      //---
      //---    |v| = |sin(theta/2)| * |u|  = |sin(theta/2)|
      //---
      //--- using this value will have a strange effect.
      //---
      //--- Recall that there are two Quaternion representations of a given
      //--- rotation, q and -q.
      //---
      //--- Typically as a body rotates along the axis it will go through a
      //--- complete cycle using one representation and then the next cycle
      //--- will use the other representation.
      //---
      //--- This corresponds to u pointing in the direction of the joint axis and
      //--- then in the opposite direction. The result is that theta
      //--- will appear to go "backwards" every other cycle.
      //---
      //--- Here is a fix: if u points "away" from the direction of the joint
      //--- axis (i.e. more than 90 degrees) then use -q instead of q. This
      //--- represents the same rotation, but results in the cos(theta/2) value
      //--- being sign inverted.
      T ct2 = Q_rel.s();           //---   cos(theta/2)
      T st2 = length( Q_rel.v() ); //---  |sin(theta/2)|
      T theta = 0.0;

      //--- Remember that Q_rel : BF_B' -> BF_B, so we need the axis in body B's local frame
      if( Q_rel.v() * axis  >= 0.0)
      {
        //--- u points in direction of axis.
        //std::cout << "u points in direction of axis" << std::endl;
        theta = 2.0* atan2(st2,ct2);
      }
      else
      {
        //--- u points in opposite direction.
        //std::cout << "u points in opposite direction" << std::endl;
        theta = 2.0 * atan2(st2,-ct2);
      }
      //--- The angle we get will be between 0..2*pi, but we want
      //--- to return angles between -pi..pi
      if (theta > value_traits::pi())   
        theta -= 2.0*value_traits::pi();

      //--- The angle we've just extracted has the wrong sign (Why???).
      theta = -theta;
      //
      // Say we have a rotation, R, relating the coordinates of two frames X and Y, now
      // let the coordinates of the vector v be given in frame X as
      //
      //    [v]_X
      //
      // And in frame Y as
      //
      //    [v]_Y
      //
      // Now R is defined such that
      //
      //    R [v]_X = [v]_Y
      //
      // That is it chagnes the coordinates of v from X to Y.
      //
      // This is pretty straightforward, but there is some subtlety in it, say
      // frame Y is rotated theta radians around the z-axis, then the rotation
      // matrix relating the coordinates is the opposite rotation.
      //
      // What we want to measure is how much the frame axis have rotated, but
      // what we are given is a rotation transforming coordinates, therefor
      // we need to flip the sign of the extracted angle!!!
      return theta;
    }
*/
    
   /* inline std::ostream & operator<< (std::ostream & o,Quaternion const & q)
    {
      o << "[" << q.s() << "," << q.v()(0) << "," << q.v()(1) << "," << q.v()(2) << "]";
      return o;
    }

    
    inline std::istream & operator>>(std::istream & i,Quaternion & q)
    {
      char dummy;
      i >> dummy;
      i >> q.s();
      i >> dummy;
      i >> q.v()(0);
      i >> dummy;
      i >> q.v()(1);
      i >> dummy;
      i >> q.v()(2);
      i >> dummy;
      return i;
    }
*/
};

#endif
