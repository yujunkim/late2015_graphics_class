#include <GL/glut.h>
#include "curve.h"
#include "viewport.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <vector>
#include <math.h>

BicubicBezierSurface surface[2];
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

float points[2][RES + 1][RES + 1][3];
// float line_points[2][3];

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
  for (int k = 0; k < 2; k++)
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        float tx = surface[k].control_pts[i][j][xx] - x;
        float ty = surface[k].control_pts[i][j][yy] - y;
        if ((tx * tx + ty * ty) < min)
        {
          min = (tx * tx + ty * ty);
          minp = k * 100 + i * 10 + j;
        }
      }
    }
  }

  return minp;
}

void calc_surface()
{
  for (int k = 0; k < 2; k++)
    for (int i = 0; i <= RES; i++)
      for (int j = 0; j <= RES; j++)
      {
        evaluate(&surface[k], i / (float)RES, j / (float)RES, points[k][i][j]);
      }
}

void init()
{
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      SET_PT3(surface[0].control_pts[i][j], 50 * i + 50, 20 * i - 75 * (i == 2) + 200 - j * 50, j * 50 + 50);

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      SET_PT3(surface[1].control_pts[i][j], 50 * i + 45 * (i == 2) + 60 - j * 20, 70 * i + 20, j * 30 + 30);

  calc_surface();

  eye = Vector3d(750, 750, 750);
  center = Vector3d(0, 0, 0);
  upVector = Vector3d(0, 1, 0);

}

void draw_intersect2(int s0i, int v0i, int s0e, int v0e, int s1i, int v1i, int s1e, int v1e){
  int s0m, v0m, s1m, v1m;
  Point min[2];
  Point max[2];
  float p[2][4][3];
  int i = 0, j=0, k=0;

  for(i=0;i<3;i++){
    p[0][0][i] = points[0][s0i][v0i][i];
    p[0][1][i] = points[0][s0e][v0i][i];
    p[0][2][i] = points[0][s0i][v0e][i];
    p[0][3][i] = points[0][s0e][v0e][i];
    min[0][i] = 10000;
    max[0][i] = 0;
  }

  for(i=0;i<3;i++){
    p[1][0][i] = points[1][s1i][v1i][i];
    p[1][1][i] = points[1][s1e][v1i][i];
    p[1][2][i] = points[1][s1i][v1e][i];
    p[1][3][i] = points[1][s1e][v1e][i];
    min[1][i] = 10000;
    max[1][i] = 0;
  }

  for(k=0;k<2;k++){
    for(i=0;i<4;i++){
      for(j=0;j<3;j++){
        if(p[k][i][j] < min[k][j]) {
          min[k][j] = p[k][i][j];
        }
        if(p[k][i][j] > max[k][j]) {
          max[k][j] = p[k][i][j];
        }
      }
    }
  }

  if (
      min[0][0] < max[1][0] && max[0][0] > min[1][0] &&
      min[0][1] < max[1][1] && max[0][1] > min[1][1] &&
      min[0][2] < max[1][2] && max[0][2] > min[1][2]
     )
  {
    if (
        ((s0i + s0e) % 2 != 0) && ((v0i + v0e) % 2 != 0) &&
        ((s1i + s1e) % 2 != 0) && ((v1i + v1e) % 2 != 0)
       )
    {
      glPointSize(5.0f);
      glColor3f(0.0f, 1.0f, 0.0f);
      glBegin(GL_POINTS);
      glVertex3f(p[0][0][0], p[0][0][1], p[0][0][2]);
      glEnd();
    }
    else if (((s0i + s0e) % 2 != 0) && ((v0i + v0e) % 2 != 0))
    {
      s1m = ((s1i + s1e) / 2);
      v1m = ((v1i + v1e) / 2);
      draw_intersect2(s0i, v0i, s0e, v0e, s1i, v1i, s1m, v1m);
      draw_intersect2(s0i, v0i, s0e, v0e, s1i, v1m, s1m, v1e);
      draw_intersect2(s0i, v0i, s0e, v0e, s1m, v1i, s1e, v1m);
      draw_intersect2(s0i, v0i, s0e, v0e, s1m, v1m, s1e, v1e);
    }
    else if (((s1i + s1e) % 2 != 0) && ((v1i + v1e) % 2 != 0))
    {
      s0m = ((s0i + s0e) / 2);
      v0m = ((v0i + v0e) / 2);
      draw_intersect2(s0i, v0i, s0m, v0m, s1i, v1i, s1e, v1e);
      draw_intersect2(s0i, v0m, s0m, v0e, s1i, v1i, s1e, v1e);
      draw_intersect2(s0m, v0i, s0e, v0m, s1i, v1i, s1e, v1e);
      draw_intersect2(s0m, v0m, s0e, v0e, s1i, v1i, s1e, v1e);
    }
    else
    {
      if (
          pow(max[0][0] - min[0][0], 2) + pow(max[0][1] - min[0][1], 2) + pow(max[0][2] - min[0][2], 2) >
          pow(max[1][0] - min[1][0], 2) + pow(max[1][1] - min[1][1], 2) + pow(max[1][2] - min[1][2], 2)
         )
      {
        s0m = ((s0i + s0e) / 2);
        v0m = ((v0i + v0e) / 2);
        draw_intersect2(s0i, v0i, s0m, v0m, s1i, v1i, s1e, v1e);
        draw_intersect2(s0i, v0m, s0m, v0e, s1i, v1i, s1e, v1e);
        draw_intersect2(s0m, v0i, s0e, v0m, s1i, v1i, s1e, v1e);
        draw_intersect2(s0m, v0m, s0e, v0e, s1i, v1i, s1e, v1e);
      }
      else
      {
        s1m = ((s1i + s1e) / 2);
        v1m = ((v1i + v1e) / 2);
        draw_intersect2(s0i, v0i, s0e, v0e, s1i, v1i, s1m, v1m);
        draw_intersect2(s0i, v0i, s0e, v0e, s1i, v1m, s1m, v1e);
        draw_intersect2(s0i, v0i, s0e, v0e, s1m, v1i, s1e, v1m);
        draw_intersect2(s0i, v0i, s0e, v0e, s1m, v1m, s1e, v1e);
      }
    }
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
  for (int k = 0; k < 2; k++)
  {
    if (k == 0)
      glColor3f(1.0f, 0.0f, 0.0f);
    else
      glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        glVertex2f(surface[k].control_pts[i][j][0], surface[k].control_pts[i][j][1]);
    glEnd();
  }

  for (int k = 0; k < 2; k++)
  {
    if (k == 0)
      glColor3f(1.0f, 0.0f, 0.0f);
    else
      glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 3; j++)
      {
        glVertex2f(surface[k].control_pts[i][j][0], surface[k].control_pts[i][j][1]);
        glVertex2f(surface[k].control_pts[i][j + 1][0], surface[k].control_pts[i][j + 1][1]);
        glVertex2f(surface[k].control_pts[j][i][0], surface[k].control_pts[j][i][1]);
        glVertex2f(surface[k].control_pts[j + 1][i][0], surface[k].control_pts[j + 1][i][1]);
      }
    glEnd();
  }

  // XZ
  glViewport(0, 0, viewportwidth, viewportheight);
  glLoadIdentity();
  gluOrtho2D(0, (double)viewportwidth, 0, (double)viewportheight);
  for (int k = 0; k < 2; k++)
  {
    if (k == 0)
      glColor3f(1.0f, 0.0f, 0.0f);
    else
      glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        glVertex2f(surface[k].control_pts[i][j][0], surface[k].control_pts[i][j][2]);
    glEnd();
  }

  for (int k = 0; k < 2; k++)
  {
    if (k == 0)
      glColor3f(1.0f, 0.0f, 0.0f);
    else
      glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 3; j++)
      {
        glVertex2f(surface[k].control_pts[i][j][0], surface[k].control_pts[i][j][2]);
        glVertex2f(surface[k].control_pts[i][j + 1][0], surface[k].control_pts[i][j + 1][2]);
        glVertex2f(surface[k].control_pts[j][i][0], surface[k].control_pts[j][i][2]);
        glVertex2f(surface[k].control_pts[j + 1][i][0], surface[k].control_pts[j + 1][i][2]);
      }
    glEnd();
  }

  // YZ
  glViewport(viewportwidth, 0, viewportwidth, viewportheight);
  glLoadIdentity();
  gluOrtho2D(0, (double)viewportwidth, 0, (double)viewportheight);
  for (int k = 0; k < 2; k++)
  {
    if (k == 0)
      glColor3f(1.0f, 0.0f, 0.0f);
    else
      glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        glVertex2f(surface[k].control_pts[i][j][1], surface[k].control_pts[i][j][2]);
    glEnd();
  }

  for (int k = 0; k < 2; k++)
  {
    if (k == 0)
      glColor3f(1.0f, 0.0f, 0.0f);
    else
      glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 3; j++)
      {
        glVertex2f(surface[k].control_pts[i][j][1], surface[k].control_pts[i][j][2]);
        glVertex2f(surface[k].control_pts[i][j + 1][1], surface[k].control_pts[i][j + 1][2]);
        glVertex2f(surface[k].control_pts[j][i][1], surface[k].control_pts[j][i][2]);
        glVertex2f(surface[k].control_pts[j + 1][i][1], surface[k].control_pts[j + 1][i][2]);
      }
    glEnd();
  }

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

  glColor3f(1.0f, 0.75f, 0.75f);
  glBegin(GL_QUADS);
  for (int i = 0; i < RES; i++)
  {
    for (int j = 0; j < RES; j++)
    {
      glVertex3f(points[0][i][j][0], points[0][i][j][1], points[0][i][j][2]);
      glVertex3f(points[0][i + 1][j][0], points[0][i + 1][j][1], points[0][i + 1][j][2]);
      glVertex3f(points[0][i + 1][j + 1][0], points[0][i + 1][j + 1][1], points[0][i + 1][j + 1][2]);
      glVertex3f(points[0][i][j + 1][0], points[0][i][j + 1][1], points[0][i][j + 1][2]);
    }
  }
  glEnd();

  glColor3f(0, 0.75f, 0.75f);
  glBegin(GL_QUADS);
  for (int i = 0; i < RES; i++)
  {
    for (int j = 0; j < RES; j++)
    {
      glVertex3f(points[1][i][j][0], points[1][i][j][1], points[1][i][j][2]);
      glVertex3f(points[1][i + 1][j][0], points[1][i + 1][j][1], points[1][i + 1][j][2]);
      glVertex3f(points[1][i + 1][j + 1][0], points[1][i + 1][j + 1][1], points[1][i + 1][j + 1][2]);
      glVertex3f(points[1][i][j + 1][0], points[1][i][j + 1][1], points[1][i][j + 1][2]);
    }
  }
  glEnd();

  draw_intersect2(0, 0, RES, RES, 0, 0, RES, RES);
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
    if (selected >= 100) {
      surface[1].control_pts[(selected - 100) / 10][selected % 10][xx] = static_cast<float>(x);
      surface[1].control_pts[(selected - 100) / 10][selected % 10][yy] = static_cast<float>(y);
    } else {
      surface[0].control_pts[selected / 10][selected % 10][xx] = static_cast<float>(x);
      surface[0].control_pts[selected / 10][selected % 10][yy] = static_cast<float>(y);
    }
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
