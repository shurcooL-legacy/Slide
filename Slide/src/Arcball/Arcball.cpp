#include <math.h>
#include <GL/glfw.h>

#define OT_M_PI       3.14159265358979323846

void NormalizeCoordinates(double & x, double & y, int Width, int Height)
{
	// std::cout << "(x, y) before = (" << x << ", " << y << ")" << std::endl;

	x = 2.0 * x / Width  - 1.0;
	if (x < -1.0) x = -1.0;
	if (x >  1.0) x =  1.0;

	y = (2.0 * y / Height - 1.0);
	if (y < -1.0) y = -1.0;
	if (y >  1.0) y =  1.0;

	// std::cout << "(x, y) after  = (" << x << ", " << y << ")" << std::endl;
}

void DisplaySphereRim(const float Radius, const unsigned int M)
{
  static GLuint sphereRim = 0;
  if (sphereRim == 0) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    sphereRim = glGenLists(1);
    glNewList(sphereRim, GL_COMPILE);
    double  R          = 1.0;
    double  TWO_PI     = 2.0 * OT_M_PI;
    double  Theta      = 0.0;
    double  DeltaTheta = TWO_PI / double(M);
    double  ThetaStop  = TWO_PI;

    GLfloat P[3];

    glBegin(GL_LINE_LOOP);
    for (Theta = 0.0; Theta < ThetaStop; Theta += DeltaTheta) {
      P[0] = static_cast<GLfloat>(R * cos(Theta));
      P[1] = static_cast<GLfloat>(R * sin(Theta));
      P[2] = static_cast<GLfloat>(0.0);

      glVertex3f(P[0], P[2], P[1]);
    }
    glEnd();
    glEndList();
    glPopMatrix();
  }

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glLineWidth(1.0);
  glColor3d(0.0, 0.0, 0.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  if (Radius != 1.0) {
    glScalef(Radius, Radius, Radius);
  }
  glCallList(sphereRim);
  glPopMatrix();
  glPopAttrib();
}

void DisplayWireSphere(const float Radius, const unsigned int M, const unsigned int N)
{
  static GLuint wireSphere = 0;
  if (wireSphere == 0) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    wireSphere = glGenLists(1);
    glNewList(wireSphere, GL_COMPILE);
    double  R          = 1.0;
    double  TWO_PI     = 2.0 * OT_M_PI;
    double  Theta      = 0.0;
    double  DeltaTheta = TWO_PI / double(M);
    double  ThetaStop  = TWO_PI;
    double  Phi        = 0.0;
    double  DeltaPhi   = OT_M_PI   / double(N);
    double  PhiStop    = OT_M_PI - DeltaPhi - 0.001;

    GLfloat P1[3], P2[3], P3[3], P4[3];

    for (Theta = 0.0; Theta < ThetaStop; Theta += DeltaTheta) {
      for (Phi = DeltaPhi; Phi < PhiStop; Phi += DeltaPhi) {
        P1[0] = static_cast<GLfloat>(R * sin(Phi) * sin(Theta));
        P1[1] = static_cast<GLfloat>(R * cos(Phi));
        P1[2] = static_cast<GLfloat>(R * sin(Phi) * cos(Theta));

        P2[0] = static_cast<GLfloat>(R * sin(Phi + DeltaPhi) * sin(Theta));
        P2[1] = static_cast<GLfloat>(R * cos(Phi + DeltaPhi));
        P2[2] = static_cast<GLfloat>(R * sin(Phi + DeltaPhi) * cos(Theta));

        P3[0] = static_cast<GLfloat>(R * sin(Phi + DeltaPhi) * sin(Theta + DeltaTheta));
        P3[1] = static_cast<GLfloat>(R * cos(Phi + DeltaPhi));
        P3[2] = static_cast<GLfloat>(R * sin(Phi + DeltaPhi) * cos(Theta + DeltaTheta));

        P4[0] = static_cast<GLfloat>(R * sin(Phi) * sin(Theta + DeltaTheta));
        P4[1] = static_cast<GLfloat>(R * cos(Phi));
        P4[2] = static_cast<GLfloat>(R * sin(Phi) * cos(Theta + DeltaTheta));

        glBegin(GL_POLYGON);
        glNormal3f(P1[0], P1[1], P1[2]);
        glVertex3f(P1[0], P1[1], P1[2]);
        glNormal3f(P2[0], P2[1], P2[2]);
        glVertex3f(P2[0], P2[1], P2[2]);
        glNormal3f(P3[0], P3[1], P3[2]);
        glVertex3f(P3[0], P3[1], P3[2]);
        glNormal3f(P4[0], P4[1], P4[2]);
        glVertex3f(P4[0], P4[1], P4[2]);
        glEnd();
      }
    }
#if 0
    // This is just a white quadrilateral
    Theta = 0.0 - DeltaTheta / 2.0;
    Phi   = OT_M_PI / 2.0 - DeltaPhi / 2.0;

    P1[0] = R * sin(Phi) * sin(Theta);
    P1[1] = R * cos(Phi);
    P1[2] = R * sin(Phi) * cos(Theta);

    P2[0] = R * sin(Phi + DeltaPhi) * sin(Theta);
    P2[1] = R * cos(Phi + DeltaPhi);
    P2[2] = R * sin(Phi + DeltaPhi) * cos(Theta);

    P3[0] = R * sin(Phi + DeltaPhi) * sin(Theta + DeltaTheta);
    P3[1] = R * cos(Phi + DeltaPhi);
    P3[2] = R * sin(Phi + DeltaPhi) * cos(Theta + DeltaTheta);

    P4[0] = R * sin(Phi) * sin(Theta + DeltaTheta);
    P4[1] = R * cos(Phi);
    P4[2] = R * sin(Phi) * cos(Theta + DeltaTheta);

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3d(1.0, 1.0, 1.0);
    glBegin(GL_POLYGON);
    glNormal3f(P1[0], P1[1], P1[2]);
    glVertex3f(P1[0], P1[1], P1[2]);
    glNormal3f(P2[0], P2[1], P2[2]);
    glVertex3f(P2[0], P2[1], P2[2]);
    glNormal3f(P3[0], P3[1], P3[2]);
    glVertex3f(P3[0], P3[1], P3[2]);
    glNormal3f(P4[0], P4[1], P4[2]);
    glVertex3f(P4[0], P4[1], P4[2]);
    glEnd();
#endif
    glEndList();
    glPopMatrix();
  }

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glDisable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glLineWidth(1.0);
  glColor3d(0.0, 0.0, 0.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  if (Radius != 1.0) {
    glScalef(Radius, Radius, Radius);
  }
  glCallList(wireSphere);
  glPopMatrix();
  glPopAttrib();
}
