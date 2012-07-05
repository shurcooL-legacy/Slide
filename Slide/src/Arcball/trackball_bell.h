#ifndef TRACKBALL_BELL_H
#define TRACKBALL_BELL_H

#include "vector3.h"
#include "Matrix3x3.h"
#include "quaternion.h"

void NormalizeCoordinates(double & x, double & y, int Width, int Height);
void DisplaySphereRim(const float Radius, const unsigned int M);
void DisplayWireSphere(const float Radius, const unsigned int M, const unsigned int N);

class Bell
  {
  public:
 	typedef  double gl_transform_type[16];


	double		m_radius;                 
    Vector3		m_anchor_position;                
    Vector3     m_current_position;       
    double		m_angle;                  
    Vector3		m_axis;                   
    ABMatrix3x3    m_xform_anchor;           ///< TBD
    ABMatrix3x3    m_xform_incremental;      ///< TBD
    ABMatrix3x3    m_xform_current;          ///< TBD
    ABMatrix3x3    m_xform_view_rotation;
	ABMatrix3x3	   m_xform_view_rotation_inv;
    gl_transform_type m_gl_xform_current;       ///< TBD
  
  public:

    Bell() : m_radius(1.0) 
    {
      reset();
    }

    Bell(double const & radius): m_radius(radius)
    {
      reset();
    }
	
    void reset()
    {
      m_anchor_position.clear();
      m_current_position.clear();
      m_axis.clear();
      m_angle     = double(0.0);
      m_xform_anchor      = ABMatrix3x3::diag(1.0);
      m_xform_incremental = ABMatrix3x3::diag(1.0);
      m_xform_current     = ABMatrix3x3::diag(1.0);
	  m_xform_view_rotation = ABMatrix3x3::diag(1.0);
	  m_xform_view_rotation_inv = ABMatrix3x3::diag(1.0);
      project_onto_surface(this->m_anchor_position);
      project_onto_surface(this->m_current_position);
    }

	double          & radius()        { return m_radius;                                                       }
    double const    & radius()  const { return m_radius;                                                       }
    Vector3       & anchor()        { m_anchor_position.unit(); return m_anchor_position;    }
//    Vector3 const & anchor()  const { m_anchor_position.unit(); return m_anchor_position;    }
    Vector3       & current()       { m_current_position.unit(); return m_current_position; }
  //  Vector3 const & current() const { m_current_position.unit(); return m_current_position; }
    double          & angle()         { return m_angle;                                                   }
    double const    & angle()   const { return m_angle;                                                   }
    Vector3       & axis()          { m_axis.unit(); return m_axis;             }
   // Vector3 const & axis()    const { m_axis.unit(); return m_axis;             }

    ABMatrix3x3 const & get_current_rotation()
    {
      // Compute the the rotation from Panchor to Pcurrent, i.e.
      // the rotation form (Xanchor, Yanchor, Zanchor) to
      // (Xcurrent, Ycurrent, Zcurrent) along a great circle.
      // Multiply the IncrementalTransformation and the AnchorTransformation
      // to get the CurrentTransformation.
      m_xform_current = ABMatrix3x3::multby(m_xform_incremental, m_xform_anchor);
	  m_xform_current = ABMatrix3x3::multby(m_xform_view_rotation_inv, m_xform_current);		// Added by Dmitri

      return m_xform_current;
    }

    gl_transform_type const & get_gl_current_rotation()
    {
      // Compute the the rotation from Panchor to Pcurrent, i.e.
      // the rotation form (Xanchor, Yanchor, Zanchor) to
      // (Xcurrent, Ycurrent, Zcurrent) along a great circle.
      // Multiply the IncrementalTransformation and the AnchorTransformation
      // to get the CurrentTransformation.
      m_xform_current = ABMatrix3x3::multby(m_xform_incremental, m_xform_anchor);
	  m_xform_current = ABMatrix3x3::multby(m_xform_view_rotation_inv, m_xform_current);

      m_gl_xform_current[ 0 ] = m_xform_current[ 0 ][ 0 ];
      m_gl_xform_current[ 1 ] = m_xform_current[ 1 ][ 0 ];
      m_gl_xform_current[ 2 ] = m_xform_current[ 2 ][ 0 ];
      m_gl_xform_current[ 3 ] = 0;
      m_gl_xform_current[ 4 ] = m_xform_current[ 0 ][ 1 ];
      m_gl_xform_current[ 5 ] = m_xform_current[ 1 ][ 1 ];
      m_gl_xform_current[ 6 ] = m_xform_current[ 2 ][ 1 ];
      m_gl_xform_current[ 7 ] = 0;
      m_gl_xform_current[ 8 ] = m_xform_current[ 0 ][ 2 ];
      m_gl_xform_current[ 9 ] = m_xform_current[ 1 ][ 2 ];
      m_gl_xform_current[ 10 ] = m_xform_current[ 2 ][ 2 ];
      m_gl_xform_current[ 11 ] = 0;
      m_gl_xform_current[ 12 ] = 0;
      m_gl_xform_current[ 13 ] = 0;
      m_gl_xform_current[ 14 ] = 0;
      m_gl_xform_current[ 15 ] = 1;

      return m_gl_xform_current;
    }

	void set_gl_current_rotation(gl_transform_type  & r)
    {
		m_xform_current[ 0 ][ 0 ] = r[ 0 ];
		m_xform_current[ 1 ][ 0 ] = r[ 1 ];
		m_xform_current[ 2 ][ 0 ] = r[ 2 ];

		m_xform_current[ 0 ][ 1 ] = r[ 4 ];
		m_xform_current[ 1 ][ 1 ] = r[ 5 ];
		m_xform_current[ 2 ][ 1 ] = r[ 6 ];

		m_xform_current[ 0 ][ 2 ] = r[ 8 ];
		m_xform_current[ 1 ][ 2 ] = r[ 9 ];
		m_xform_current[ 2 ][ 2 ] = r[ 10 ];

		//m_xform_current = m_xform_current.multby(m_xform_incremental , m_xform_anchor);
    }

    ABMatrix3x3 const & get_incremental_rotation() const { return m_xform_incremental;   }




    void begin_drag(double const & x, double const & y, gl_transform_type & view_rotation)
    {
      this->m_angle = 0.0;
      this->m_axis.clear();

      this->m_xform_view_rotation[0][0] = view_rotation[0]; this->m_xform_view_rotation[1][0] = view_rotation[1]; this->m_xform_view_rotation[2][0] = view_rotation[2];
	  this->m_xform_view_rotation[0][1] = view_rotation[4]; this->m_xform_view_rotation[1][1] = view_rotation[5]; this->m_xform_view_rotation[2][1] = view_rotation[6];
	  this->m_xform_view_rotation[0][2] = view_rotation[8]; this->m_xform_view_rotation[1][2] = view_rotation[9]; this->m_xform_view_rotation[2][2] = view_rotation[10];
	  this->m_xform_view_rotation_inv = ABMatrix3x3::trans(this->m_xform_view_rotation);

	  //ABMatrix3x3::printMatrix(cout, m_xform_current);

	  this->m_xform_anchor = ABMatrix3x3::multby(this->m_xform_view_rotation, this->m_xform_current);
      //this->m_xform_anchor = this->m_xform_current;
      this->m_xform_incremental = m_xform_incremental.diag(1.0);
      this->m_xform_current = m_xform_current.diag(1.0);

	  this->m_anchor_position = Vector3(x,y,0);
      project_onto_surface(this->m_anchor_position);
      this->m_current_position = Vector3(x,y,0);
      project_onto_surface(this->m_current_position);
    }

    void drag(double const & x, double const & y)
    {
      this->m_current_position = Vector3(x,y,0);
      project_onto_surface(this->m_current_position);
      compute_incremental(this->m_anchor_position,this->m_current_position,this->m_xform_incremental);
    }

    void end_drag(double const & x, double const & y)
    {
      this->m_current_position = Vector3(x,y,0);
      project_onto_surface(this->m_current_position);
      compute_incremental(this->m_anchor_position,this->m_current_position,this->m_xform_incremental);
    }

  private:

    void project_onto_surface(Vector3 & P)
    {
      using std::sqrt;
      const static double radius2 = this->m_radius * this->m_radius;
      double length2 = P.x*P.x + P.y*P.y;

      if (length2 <= radius2 / 2.0)
        P.z = sqrt(radius2 - length2);
      else
      {
        P.z = radius2 / (2.0 * sqrt(length2));
        double length = sqrt(length2 + P.z * P.z);
        //P /= length;
		P *= (1.0/length); 

      }
    }

    void compute_incremental(Vector3 const & anchor, Vector3 const & current, ABMatrix3x3 & transform)
    {
      
	  Vector3 temp(anchor.x,anchor.y,anchor.z);
	  Vector3 temp2(current.x,current.y,current.z);
	  temp.unit();
	  temp2.unit();
	  Quaternion  Q_anchor ( 0, temp  );
      Quaternion  Q_current( 0, temp2 );
      Quaternion Q_rot = -Q_rot.prod(Q_current,Q_anchor);
      this->m_axis = Q_rot.v();
      this->m_angle = atan2(this->m_axis.length(), Q_rot.s());
      transform = Q_rot;  //--- KE 20060307: Kaiip extracted axis and angle from this conversion!!!
    }
  };

#endif
