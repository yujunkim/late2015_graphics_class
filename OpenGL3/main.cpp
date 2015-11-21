#include <GL/glut.h>
#include "curve.h"
#include "viewport.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <vector>
#include <math.h>

BicubicBezierSurface surface;
GLsizei width = 800, height = 600;
float viewportwidth = 400, viewportheight = 300;

int selectedscene = 0;
int selected = -1;
bool isDrawControlMesh = true;
bool isDottedLine = false;

Vector3d eye;
Vector3d center;
Vector3d upVector;
bool isDragging = false;
float radius;

#define RES 256

float points[RES + 1][RES + 1][3];

int mouseButton = -1;
int lastX = -1;
int lastY = -1;

Point target;

void RayTest(int mouse_x, int mouse_y)
{
  float x = mouse_x;
  float y = height - mouse_y;

  double model[16], proj[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, model);
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  int viewport[4] = { 0.0f, 0.0f, width, height };
  double ax, ay, az, bx, by, bz;
  gluUnProject(mouse_x, mouse_y, 0.0, model, proj, viewport, &ax, &ay, &az);
  gluUnProject(mouse_x, mouse_y, 1.0, model, proj, viewport, &bx, &by, &bz);

  double cx, cy, cz;
  cx = ax - bx;
  cy = ay - by;
  cz = az - bz;

  target[0] = ax - cx * ay / cy;
  target[1] = az - cz * ay / cy;
}

int hit_index(int x, int y, int scene)
{
  int xx, yy;
  switch (scene)
  {
    case 1:
      xx = 0, yy = 1;
      break;
    case 3:
      xx = 0, yy = 2;
      break;
    case 4:
      xx = 1, yy = 2;
      break;
  }
  int min = 30;
  int minp = -1;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      float tx = surface.control_pts[i][j][xx] - x;
      float ty = surface.control_pts[i][j][yy] - y;
      if ((tx * tx + ty * ty) < min)
      {
        min = (tx * tx + ty * ty);
        minp = i * 10 + j;
      }
    }
  }
  return minp;
}

void calc_surface()
{
  for (int i = 0; i <= RES; i++)
    for (int j = 0; j <= RES; j++)
    {
      evaluate(&surface, i / (float)RES, j / (float)RES, points[i][j]);
    }
}

void init()
{
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      SET_PT3(surface.control_pts[i][j], 50 * i + 50, 20 * i - 75 * (i == 2) + 200 - j * 50, j * 50 + 50);

  calc_surface();

  eye = Vector3d(750, 750, 750);
  center = Vector3d(0, 0, 0);
  upVector = Vector3d(0, 1, 0);
}

float calc_distance(int sir, int vir, int ser, int ver, int ss, int vv, Point p1, Point p2, Point p3, Point p4){
  float s, v;
  float dx, dy, dz;
  s = (ss - sir) / (float)(ser - sir);
  v = (vv - vir) / (float)(ver - vir);
  dx = (1 - s) * (1 - v) * p1[0] + s * (1 - v) * p2[0] + (1 - s) * v * p3[0] + s * v * p4[0] - points[ss][vv][0];
  dy = (1 - s) * (1 - v) * p1[1] + s * (1 - v) * p2[1] + (1 - s) * v * p3[1] + s * v * p4[1] - points[ss][vv][1];
  dz = (1 - s) * (1 - v) * p1[2] + s * (1 - v) * p2[2] + (1 - s) * v * p3[2] + s * v * p4[2] - points[ss][vv][2];

  return sqrt(dx*dx + dy*dy + dz*dz);
}
void draw_bi2(int sir, int vir, int ser, int ver){
  glDisable(GL_DEPTH_TEST);
  glColor3f(0, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex3f(points[sir][vir][0], points[sir][vir][1], points[sir][vir][2]);
  glVertex3f(points[ser][vir][0], points[ser][vir][1], points[ser][vir][2]);
  glVertex3f(points[ser][ver][0], points[ser][ver][1], points[ser][ver][2]);
  glVertex3f(points[sir][ver][0], points[sir][ver][1], points[sir][ver][2]);
  glEnd();
}

void draw_bi(int sir, int vir, int ser, int ver){
  int sm, vm;
  float distance = 0;
  float tmp_distance = 0;
  if (sir == ser || vir == ver)
    return;
  for (int i = sir; i < ser + 1; i++)
  {
    for (int j = vir; j < ver + 1; j++)
    {
      tmp_distance = calc_distance(sir, vir, ser, ver, i, j, points[sir][vir], points[ser][vir], points[sir][ver], points[ser][ver]);
      if (tmp_distance > distance) {
        sm = i;
        vm = j;
        distance = tmp_distance;
      }
    }
  }
  if ( distance > 1 )
  {
    // printf("distance : %f, sm: %d, vm: %d \n",distance, sm, vm);
    draw_bi(sir, vir, sm, vm);
    draw_bi(sir, vm, sm, ver);
    draw_bi(sm, vir, ser, vm);
    draw_bi(sm, vm, ser, ver);
  } else {
    draw_bi2(sir,vir,ser,ver);
    // printf("draw distance : %f, sir: %d, vir: %d, ser: %d, ver: %d \n",distance, sir, vir, ser, ver);
  }
}

void reshape_callback(GLint nw, GLint nh)
{
  width = nw;
  height = nh;
  viewportwidth = width / 2.0f;
  viewportheight = height / 2.0f;
  radius = std::sqrt(viewportwidth * viewportwidth + viewportheight * viewportheight) / 2;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
}

void display_callback()
{
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glColor3f(0, 0, 0);
  glBegin(GL_LINES);
  glVertex3f(-1, 0, 0);
  glVertex3f(1, 0, 0);
  glEnd();
  glBegin(GL_LINES);
  glVertex3f(0, -1, 0);
  glVertex3f(0, 1, 0);
  glEnd();

  glPointSize(8.0f);
  // XY
  glViewport(0, viewportheight, viewportwidth, viewportheight);
  glLoadIdentity();
  gluOrtho2D(0, (double)viewportwidth, 0, (double)viewportheight);
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_POINTS);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      glVertex2f(surface.control_pts[i][j][0], surface.control_pts[i][j][1]);
  glEnd();

  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 3; j++)
    {
      glVertex2f(surface.control_pts[i][j][0], surface.control_pts[i][j][1]);
      glVertex2f(surface.control_pts[i][j + 1][0], surface.control_pts[i][j + 1][1]);
      glVertex2f(surface.control_pts[j][i][0], surface.control_pts[j][i][1]);
      glVertex2f(surface.control_pts[j + 1][i][0], surface.control_pts[j + 1][i][1]);
    }
  glEnd();

  // XZ
  glViewport(0, 0, viewportwidth, viewportheight);
  glLoadIdentity();
  gluOrtho2D(0, (double)viewportwidth, 0, (double)viewportheight);
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_POINTS);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      glVertex2f(surface.control_pts[i][j][0], surface.control_pts[i][j][2]);
  glEnd();
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 3; j++)
    {
      glVertex2f(surface.control_pts[i][j][0], surface.control_pts[i][j][2]);
      glVertex2f(surface.control_pts[i][j + 1][0], surface.control_pts[i][j + 1][2]);
      glVertex2f(surface.control_pts[j][i][0], surface.control_pts[j][i][2]);
      glVertex2f(surface.control_pts[j + 1][i][0], surface.control_pts[j + 1][i][2]);
    }
  glEnd();

  // YZ
  glViewport(viewportwidth, 0, viewportwidth, viewportheight);
  glLoadIdentity();
  gluOrtho2D(0, (double)viewportwidth, 0, (double)viewportheight);
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_POINTS);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      glVertex2f(surface.control_pts[i][j][1], surface.control_pts[i][j][2]);
  glEnd();
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 3; j++)
    {
      glVertex2f(surface.control_pts[i][j][1], surface.control_pts[i][j][2]);
      glVertex2f(surface.control_pts[i][j + 1][1], surface.control_pts[i][j + 1][2]);
      glVertex2f(surface.control_pts[j][i][1], surface.control_pts[j][i][2]);
      glVertex2f(surface.control_pts[j + 1][i][1], surface.control_pts[j + 1][i][2]);
    }
  glEnd();

  // 3D
  glViewport(viewportwidth, viewportheight, viewportwidth, viewportheight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(25, width / (double)height, 0.1, 25000);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, upVector.x, upVector.y, upVector.z);

  glEnable(GL_DEPTH_TEST);
  glBegin(GL_LINES);
  glColor3f(1.0f, 0, 0);
  glVertex3f(0, 0, 0);
  glVertex3f(500.0f, 0, 0);
  glColor3f(0, 1.0f, 0);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 500.0f, 0);
  glColor3f(0, 0, 1.0f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 500.0f);
  glEnd();

  // glColor3f(0, 0, 0);
  // for (int i = 0; i <= RES; i += 4)
  // {
  // 	glBegin(GL_LINE_STRIP);
  // 	for (int j = 0; j <= RES; j++)
  // 		glVertex3f(points[i][j][0], points[i][j][1], points[i][j][2]);
  // 	glEnd();
  // }
  // for (int i = 0; i <= RES; i += 4)
  // {
  // 	glBegin(GL_LINE_STRIP);
  // 	for (int j = 0; j <= RES; j++)
  // 		glVertex3f(points[j][i][0], points[j][i][1], points[j][i][2]);
  // 	glEnd();
  // }
  //
  glColor3f(1.0f, 0.75f, 0.75f);
  glBegin(GL_QUADS);
  for (int i = 0; i < RES; i++)
  {
    for (int j = 0; j < RES; j++)
    {
      glVertex3f(points[i][j][0], points[i][j][1], points[i][j][2]);
      glVertex3f(points[i + 1][j][0], points[i + 1][j][1], points[i + 1][j][2]);
      glVertex3f(points[i + 1][j + 1][0], points[i + 1][j + 1][1], points[i + 1][j + 1][2]);
      glVertex3f(points[i][j + 1][0], points[i][j + 1][1], points[i][j + 1][2]);
    }
  }
  glEnd();
  draw_bi(0, 0, RES, RES);
  glDisable(GL_DEPTH_TEST);

  glutSwapBuffers();
}

// void glutMouseFunc(void (*func)(int button, int state, int x, int y));
void mouse_callback(GLint button, GLint action, GLint x, GLint y)
{
  int scene = 0;
  if (x < viewportwidth)
  {
    if (y < viewportheight)
      scene = 1;
    else
    {
      scene = 3;
      y -= (int)viewportheight;
    }
  }
  else
  {
    x -= (int)viewportwidth;
    if (y < viewportheight)
      scene = 2;
    else
    {
      scene = 4;
      y -= (int)viewportheight;
    }
  }

  if (action == GLUT_UP)
  {
    isDragging = false;
    mouseButton = -1;
  }

  if (scene == 2)
  {
    if (action == GLUT_DOWN)
    {
      mouseButton = button;
      isDragging = true;
      lastX = x;
      lastY = y;
    }
  }
  else
  {
    if (button == GLUT_LEFT_BUTTON)
    {
      switch (action)
      {
        case GLUT_DOWN:
          selectedscene = scene;
          selected = hit_index(x, (int)viewportheight - y, scene);
          break;
        case GLUT_UP:
          selected = -1;
          break;
        default: break;
      }
    }
  }
  glutPostRedisplay();
}

// void glutMotionFunc(void (*func)(int x, int y));
void mouse_move_callback(GLint x, GLint y)
{
  Vector3d lastP = getMousePoint(lastX, lastY, viewportwidth, viewportheight, radius);
  Vector3d currentP = getMousePoint(x - viewportwidth, y, viewportwidth, viewportheight, radius);

  if (mouseButton == GLUT_LEFT_BUTTON)
  {
    Vector3d rotateVector;
    rotateVector.cross(currentP, lastP);
    double angle = -currentP.angle(lastP) * 2;
    rotateVector = unProjectToEye(rotateVector, eye, center, upVector);

    Vector3d dEye;
    dEye.sub(center, eye);
    dEye = rotate(dEye, rotateVector, -angle);
    upVector = rotate(upVector, rotateVector, -angle);
    eye.sub(center, dEye);
  }
  else if (mouseButton == GLUT_RIGHT_BUTTON){
    Vector3d dEye;
    dEye.sub(center, eye);
    double offset = 0.025;
    if ((y - lastY) < 0){
      dEye.scale(1 - offset);
    }
    else {
      dEye.scale(1 + offset);
    }
    eye.sub(center, dEye);
  }
  else if (mouseButton == GLUT_MIDDLE_BUTTON){
    double dx = x - viewportwidth - lastX;
    double dy = y - lastY;
    if (dx != 0 || dy != 0)
    {
      Vector3d moveVector(dx, dy, 0);
      moveVector = unProjectToEye(moveVector, eye, center, upVector);
      moveVector.normalize();
      double eyeDistance = Vector3d(eye).distance(Vector3d(center));
      moveVector.scale(std::sqrt(dx*dx + dy*dy) / 1000 * eyeDistance);
      center.add(moveVector);
      eye.add(moveVector);
    }
  }

  lastX = x - viewportwidth;
  lastY = y;

  if (selected != -1)
  {
    int xx = 0;
    int yy = 0;
    switch (selectedscene)
    {
      case 1:
        xx = 0, yy = 1;
        break;
      case 3:
        xx = 0, yy = 2;
        y -= (int)viewportheight;
        break;
      case 4:
        xx = 1, yy = 2;
        x -= (int)viewportwidth;
        y -= (int)viewportheight;
        break;
    }
    x = std::max(x, 0);
    x = std::min(x, (int)viewportwidth);
    y = std::max((int)viewportheight - y, 0);
    y = std::min(y, (int)viewportheight);
    surface.control_pts[selected / 10][selected % 10][xx] = static_cast<float>(x);
    surface.control_pts[selected / 10][selected % 10][yy] = static_cast<float>(y);
    calc_surface();
  }

  glutPostRedisplay();
}

// void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y));
void keyboard_callback(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 'i': case 'I':
      init();
      break;
    case 'l': case 'L':
      isDottedLine ^= true;
      break;
    case 'c': case 'C':
      isDrawControlMesh ^= true;
      break;
    case (27) : exit(0); break;
    default: break;
  }
  glutPostRedisplay();
}

int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("Beizer Surface Editor");

  init();
  glutReshapeFunc(reshape_callback);
  glutMouseFunc(mouse_callback);
  glutMotionFunc(mouse_move_callback);
  glutDisplayFunc(display_callback);
  glutKeyboardFunc(keyboard_callback);
  glutMainLoop();

  return 0;
}
