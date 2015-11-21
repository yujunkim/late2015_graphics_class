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
float line_points[2][3];

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

  for (int i = 0; i < 2; i++)
  {
    float tx = line_points[i][xx] - x;
    float ty = line_points[i][yy] - y;
    if ((tx * tx + ty * ty) < min)
    {
      min = (tx * tx + ty * ty);
          minp = 100 + i;
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

  SET_PT3(line_points[0], 70, 170, 70);
  SET_PT3(line_points[1], 170, 70, 130);
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
  sm = ((sir + ser) / 2);
  vm = ((vir + ver) / 2);
  if (
      ((sir + ser) % 2 == 0) &&
      ((vir + ver) % 2 == 0) &&
      (calc_distance(sir, vir, ser, ver, sm, vm, points[sir][vir], points[ser][vir], points[sir][ver], points[ser][ver]) > 1))
  {
    draw_bi(sir, vir, sm, vm);
    draw_bi(sir, vm, sm, ver);
    draw_bi(sm, vir, ser, vm);
    draw_bi(sm, vm, ser, ver);
  } else {
    draw_bi2(sir,vir,ser,ver);
  }
}

float line_equation(int given, float value, int want){
  return (line_points[1][want] - line_points[0][want]) * (value - line_points[0][given]) / (line_points[1][given] - line_points[0][given]) + line_points[0][want];
}

bool between(float left, float middle, float right) {
  return left <= middle && right >= middle;
}

bool determine_penetration(Point min, Point max){
  float min_y_x, max_y_x, min_z_x, max_z_x, maxs_min, mins_max;
  float ctrl_min, ctrl_max;

  if (line_points[0][0] == line_points[1][0] && line_points[0][1] == line_points[1][1]) {
    if (between(min[0], line_points[0][0], max[0]) && between(min[1], line_points[0][1], max[1])) {
      if (
          between(line_points[0][2], min[2], line_points[1][2]) ||
          between(line_points[1][2], max[2], line_points[0][2]) ||
          between(min[2], line_points[0][2], max[2])
         )
        return true;
    }
    return false;
  }

  if (line_points[0][0] == line_points[1][0] && line_points[0][2] == line_points[1][2]) {
    if (between(min[0], line_points[0][0], max[0]) && between(min[2], line_points[0][2], max[2])) {
      if (
          between(line_points[0][1], min[1], line_points[1][1]) ||
          between(line_points[1][1], max[1], line_points[0][1]) ||
          between(min[1], line_points[0][1], max[1])
         )
        return true;
    }
    return false;
  }

  if (line_points[0][1] == line_points[1][1] && line_points[0][2] == line_points[1][2]) {
    if (between(min[1], line_points[0][1], max[1]) && between(min[2], line_points[0][2], max[2])) {
      if (
          between(line_points[0][0], min[0], line_points[1][0]) ||
          between(line_points[1][0], max[0], line_points[0][0]) ||
          between(min[0], line_points[0][0], max[0])
         ) {
        return true;
      }
    }
    return false;
  }

  if (line_points[0][0] == line_points[1][0]) {
    if (between(min[0], line_points[0][0], max[0])){
      mins_max = line_equation(2, min[2], 1);
      maxs_min = line_equation(2, max[2], 1);
      if (maxs_min < mins_max) {
        mins_max = line_equation(2, max[2], 1);
        maxs_min = line_equation(2, min[2], 1);
      }

      if (mins_max < min[1])
        mins_max = min[1];
      if (maxs_min > max[1])
        maxs_min = max[1];

      ctrl_min = line_points[0][1];
      ctrl_max = line_points[1][1];
      if (ctrl_min > ctrl_max) {
        ctrl_min = line_points[1][1];
        ctrl_max = line_points[0][1];
      }
      if (
          mins_max < maxs_min &&
          (
            between(ctrl_min, mins_max, ctrl_max) ||
            between(ctrl_min, maxs_min, ctrl_max) ||
            between(mins_max, ctrl_min, maxs_min)
          )
         )
        return true;
    }
    return false;
  }

  if (line_points[0][1] == line_points[1][1]) {
    if (between(min[1], line_points[0][1], max[1])){
      mins_max = line_equation(2, min[2], 0);
      maxs_min = line_equation(2, max[2], 0);
      if (maxs_min < mins_max) {
        mins_max = line_equation(2, max[2], 0);
        maxs_min = line_equation(2, min[2], 0);
      }

      if (mins_max < min[0])
        mins_max = min[0];
      if (maxs_min > max[0])
        maxs_min = max[0];

      ctrl_min = line_points[0][0];
      ctrl_max = line_points[1][0];
      if (ctrl_min > ctrl_max) {
        ctrl_min = line_points[1][0];
        ctrl_max = line_points[0][0];
      }
      if (
          mins_max < maxs_min &&
          (
            between(ctrl_min, mins_max, ctrl_max) ||
            between(ctrl_min, maxs_min, ctrl_max) ||
            between(mins_max, ctrl_min, maxs_min)
          )
         )
        return true;
    }
    return false;
  }

  if (line_points[0][2] == line_points[1][2]) {
    if (between(min[2], line_points[0][2], max[2])){
      mins_max = line_equation(1, min[1], 0);
      maxs_min = line_equation(1, max[1], 0);
      if (maxs_min < mins_max) {
        mins_max = line_equation(1, max[1], 0);
        maxs_min = line_equation(1, min[1], 0);
      }

      if (mins_max < min[0])
        mins_max = min[0];
      if (maxs_min > max[0])
        maxs_min = max[0];

      ctrl_min = line_points[0][0];
      ctrl_max = line_points[1][0];
      if (ctrl_min > ctrl_max) {
        ctrl_min = line_points[1][0];
        ctrl_max = line_points[0][0];
      }
      if (
          mins_max < maxs_min &&
          (
            between(ctrl_min, mins_max, ctrl_max) ||
            between(ctrl_min, maxs_min, ctrl_max) ||
            between(mins_max, ctrl_min, maxs_min)
          )
         )
        return true;
    }
    return false;
  }

  min_y_x = line_equation(1, min[1], 0);
  max_y_x = line_equation(1, max[1], 0);
  min_z_x = line_equation(2, min[2], 0);
  max_z_x = line_equation(2, max[2], 0);
  if (min_y_x > max_y_x) {
    min_y_x = line_equation(1, max[1], 0);
    max_y_x = line_equation(1, min[1], 0);
  }
  if (min_z_x > max_z_x) {
    min_z_x = line_equation(2, max[2], 0);
    max_z_x = line_equation(2, min[2], 0);
  }

  mins_max = min[0];
  if (mins_max < min_y_x)
    mins_max = min_y_x;
  if (mins_max < min_z_x)
    mins_max = min_z_x;

  maxs_min = max[0];
  if (maxs_min > max_y_x)
    maxs_min = max_y_x;
  if (maxs_min > max_z_x)
    maxs_min = max_z_x;

  ctrl_min = line_points[0][0];
  ctrl_max = line_points[1][0];
  if (ctrl_min > ctrl_max) {
    ctrl_min = line_points[1][0];
    ctrl_max = line_points[0][0];
  }
  if (
      mins_max < maxs_min &&
      (
        between(ctrl_min, mins_max, ctrl_max) ||
        between(ctrl_min, maxs_min, ctrl_max) ||
        between(mins_max, ctrl_min, maxs_min)
      )
     )
    return true;


  return false;
}

void draw_intersect(int sir, int vir, int ser, int ver){
  int sm, vm;
  Point min;
  Point max;
  float p[4][3];
  int i = 0, j=0;

  for(i=0;i<3;i++){
    p[0][i] = points[sir][vir][i];
    p[1][i] = points[ser][vir][i];
    p[2][i] = points[sir][ver][i];
    p[3][i] = points[ser][ver][i];
    min[i] = 10000;
    max[i] = 0;
  }

  for(i=0;i<4;i++){
    for(j=0;j<3;j++){
      if(p[i][j] < min[j]) {
        min[j] = p[i][j];
      }
      if(p[i][j] > max[j]) {
        max[j] = p[i][j];
      }
    }
  }
  sm = ((sir + ser) / 2);
  vm = ((vir + ver) / 2);
  if (((sir + ser) % 2 != 0) && ((vir + ver) % 2 != 0)) {
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_POINTS);
    glVertex3f(p[0][0], p[0][1], p[0][2]);
    glEnd();
  } else if (determine_penetration(min, max)) {
    draw_intersect(sir, vir, sm, vm);
    draw_intersect(sir, vm, sm, ver);
    draw_intersect(sm, vir, ser, vm);
    draw_intersect(sm, vm, ser, ver);
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

  glColor3f(0.0f, 1.0f, 0.0f);
  glBegin(GL_POINTS);
  glVertex2f(line_points[0][0], line_points[0][1]);
  glVertex2f(line_points[1][0], line_points[1][1]);
  glEnd();

  glBegin(GL_LINES);
  glVertex2f(line_points[0][0], line_points[0][1]);
  glVertex2f(line_points[1][0], line_points[1][1]);
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

  glColor3f(0.0f, 1.0f, 0.0f);
  glBegin(GL_POINTS);
  glVertex2f(line_points[0][0], line_points[0][2]);
  glVertex2f(line_points[1][0], line_points[1][2]);
  glEnd();

  glBegin(GL_LINES);
  glVertex2f(line_points[0][0], line_points[0][2]);
  glVertex2f(line_points[1][0], line_points[1][2]);
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

  glColor3f(0.0f, 1.0f, 0.0f);
  glBegin(GL_POINTS);
  glVertex2f(line_points[0][1], line_points[0][2]);
  glVertex2f(line_points[1][1], line_points[1][2]);
  glEnd();

  glBegin(GL_LINES);
  glVertex2f(line_points[0][1], line_points[0][2]);
  glVertex2f(line_points[1][1], line_points[1][2]);
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

  // glEnable(GL_DEPTH_TEST);
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

  glColor3f(0.0f, 1.0f, 0.0f);
  glBegin(GL_POINTS);
  glVertex3f(line_points[0][0], line_points[0][1], line_points[0][2]);
  glVertex3f(line_points[1][0], line_points[1][1], line_points[1][2]);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(line_points[0][0], line_points[0][1], line_points[0][2]);
  glVertex3f(line_points[1][0], line_points[1][1], line_points[1][2]);
  glEnd();

  // draw_bi(0, 0, RES, RES);
  draw_intersect(0, 0, RES, RES);
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
      line_points[selected % 100][xx] = static_cast<float>(x);
      line_points[selected % 100][yy] = static_cast<float>(y);
    } else {
      surface.control_pts[selected / 10][selected % 10][xx] = static_cast<float>(x);
      surface.control_pts[selected / 10][selected % 10][yy] = static_cast<float>(y);
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
