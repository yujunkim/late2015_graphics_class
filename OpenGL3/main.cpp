#include <algorithm>
#include <GL/freeglut.h>
#include "texture.h"
#include <windows.h>
#include "viewport.h"

#define M_PI 3.14159265358979323846

const int OOCP = 3;	//order of control points
const int LOD = 128;	//level of detail, the number of lines each curve is divided into
const int DIM = 3;

int combinations[(OOCP+1)/2+1];	//nCr
double bezierConstants[LOD+1][OOCP+1];	//nCr * t^r * (1-t)^(n-r)
double controlPoints[OOCP+1][OOCP+1][DIM];
double intermediatePoints[OOCP+1][LOD+1][DIM];
double surfacePoints[LOD+1][LOD+1][DIM];
double surfaceNormals[LOD+1][LOD+1][DIM];
double texCoords[LOD+1][LOD+1][2];

int selectedView, selectedPoint[2];
const int INIT_SIZE = 800;
int width = INIT_SIZE;
int height = INIT_SIZE;

int mouseButton = -1;
int lastX, lastY;
double radius;
Vector3d eye, center, upVector;

void CalculateCombinations()
{
  combinations[0] = 1;
  for (int i = 1; i <= OOCP; i++)
  {
    for (int j = i / 2; j >= 0; j--)
      combinations[j] += combinations[j-1];
    if (i % 2 == 1)
      combinations[i/2+1] = combinations[i/2];
  }
}

void CalculateBezierConstants()
{
  for (int i = 0; i <= LOD; i++)
  {
    double t = (double) i / LOD;
    double it = 1-t;

    for (int j = 0; j <= OOCP / 2; j++)
    {
      bezierConstants[i][j] = combinations[j] * pow(t, j) * pow(it, OOCP - j);
      bezierConstants[i][OOCP-j] = combinations[j] * pow(t, OOCP - j) * pow(it, j);
    }
  }
}

double Length(double x, double y, double z)
{
  return std::sqrt(x*x + y*y + z*z);
}

void Normalize(double* v)
{
  double l = Length(v[0], v[1], v[2]);
  if (l > 0)
    for (int i = 0; i < DIM; i++)
      v[i] /= l;
}

void EvaluateSurface()
{
  for (int i = 0; i <= OOCP; i++)
    for (int j = 0; j <= LOD; j++)
      for (int axis = 0; axis < DIM; axis++)
      {
        intermediatePoints[i][j][axis] = 0;
        for (int l = 0; l <= OOCP; l++)
          intermediatePoints[i][j][axis] += bezierConstants[j][l] * controlPoints[l][i][axis];
      }

  for (int i = 0; i <= LOD; i++)
    for (int j = 0; j <= LOD; j++)
      for (int axis = 0; axis < DIM; axis++)
      {
        surfacePoints[i][j][axis] = 0;
        for (int l = 0; l <= OOCP; l++)
          surfacePoints[i][j][axis] += bezierConstants[j][l] * intermediatePoints[l][i][axis];
      }

  for (int i = 0; i <= LOD; i++)
    for (int j = 0; j <= LOD; j++)
    {
      double tangents[4][DIM], normals[4][DIM];

      for (int axis = 0; axis < DIM; axis++)
      {
        tangents[0][axis] = j > 0? surfacePoints[i][j-1][axis] - surfacePoints[i][j][axis] : 0;
        tangents[1][axis] = i < LOD? surfacePoints[i+1][j][axis] - surfacePoints[i][j][axis] : 0;
        tangents[2][axis] = j < LOD? surfacePoints[i][j+1][axis] - surfacePoints[i][j][axis] : 0;
        tangents[3][axis] = i > 0? surfacePoints[i-1][j][axis] - surfacePoints[i][j][axis] : 0;
      }

      for (int d = 0; d < 4; d++)
      {
        int c = (d + 1) % 4;
        normals[d][0] = tangents[c][1]*tangents[d][2] - tangents[c][2]*tangents[d][1];
        normals[d][1] = tangents[c][2]*tangents[d][0] - tangents[c][0]*tangents[d][2];
        normals[d][2] = tangents[c][0]*tangents[d][1] - tangents[c][1]*tangents[d][0];
        Normalize(normals[d]);
      }

      for (int axis = 0; axis < DIM; axis++)
      {
        surfaceNormals[i][j][axis] = 0;
        for (int d = 0; d < 4; d++)
          surfaceNormals[i][j][axis] += normals[d][axis];
      }

      Normalize(surfaceNormals[i][j]);
    }
}

void InitPoints()
{
  for (int i = 0; i <= OOCP; i++)
    for (int j = 0; j <= OOCP; j++)
    {
      controlPoints[i][j][0] = INIT_SIZE * 0.7 * (2 * i / (double) OOCP - 1);
      controlPoints[i][j][1] = INIT_SIZE * 0.7 * (1 - 2 * j / (double) OOCP);
      controlPoints[i][j][2] = INIT_SIZE * (0.5 - std::abs((double)i / OOCP - 0.5) - std::abs((double)j / OOCP - 0.5));
    }

  EvaluateSurface();
  selectedView = -1;
}

float thickness = 1.5;

void Init()
{
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowPosition(300, 100);
  glutInitWindowSize(width, height);
  glutCreateWindow("2015 Fall Computer Graphics HW #3-4 Example");

  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glPointSize(6 * thickness);

  CalculateCombinations();
  CalculateBezierConstants();
  InitPoints();

  unsigned int tex;
  int width, height;
  initPNG(&tex, "cubemap.png", width, height);


  eye = Vector3d(0, 0, 1000);
  center = Vector3d(0, 0, 0);
  upVector = Vector3d(0, 1, 0);
}

enum {FRONT, UP, LEFT, ROTATE};

void SelectViewport(int view, bool clear)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-width, width, -height, height, -100000, 100000);

  if (view == FRONT)
    gluLookAt(0, 0, 1, 0, 0, 0, 0, 1, 0);
  else if (view == UP)
    gluLookAt(0, 1, 0, 0, 0, 0, 0, 0, -1);
  else if (view == LEFT)
    gluLookAt(1, 0, 0, 0, 0, 0, 0, 1, 0);
  else
  {
    gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, upVector.x, upVector.y, upVector.z);
  }

  int w = width / 2;
  int h = height / 2;
  int x = (view == LEFT || view == ROTATE)? w : 0;
  int y = (view == UP || view == ROTATE)? h : 0;
  glViewport(x, y, w, h);

  if (clear)
  {
    glScissor(x, y, w, h);
    glClearColor(view < 2? 0.9f : 1, view % 2 == 0? 0.9f : 1, view > 0 && view < 3? 0.9f : 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

bool showAxes = false;

void DrawGrid()
{
  double length = INIT_SIZE / 2;

  glLineWidth(2 * thickness);
  glBegin(GL_LINES);

  if (showAxes)
    for (int axis = 0; axis < DIM; axis++)
    {
      glColor3f(axis == 0, axis == 1, axis == 2);
      glVertex3d(0, 0, 0);
      glVertex3d(length*(axis == 0), length*(axis == 1), length*(axis == 2));
    }

  glEnd();
}


void DrawSurface()
{
  for (int i = 0; i <= LOD; i++)
    for (int j = 0; j <= LOD; j++)
    {
      // calculating reflecting vector
      Vector3d mapped;
      mapped.sub(center, eye);
      Vector3d n = Vector3d(surfaceNormals[i][j][0], surfaceNormals[i][j][1], surfaceNormals[i][j][2]);
      n.scale(-2 * n.dot(mapped));
      mapped.add(n);
      mapped.normalize();

      // for sphere mapping
      float padding_y = 1/3.0;
      float padding_x = 1/4.0;
      if (mapped.x >= std::abs(mapped.y) && mapped.x >= std::abs(mapped.z)) { // right
        texCoords[i][j][0] = (-mapped.z / mapped.x + 1) / 2 / 4 + padding_x * 2;
        texCoords[i][j][1] = (-mapped.y / mapped.x + 1) / 2 / 3 + padding_y;
      } else if (mapped.y >= std::abs(mapped.x) && mapped.y >= std::abs(mapped.z)) { // top
        texCoords[i][j][0] = (mapped.x / mapped.y + 1) / 2 / 4 + padding_x;
        texCoords[i][j][1] = (mapped.z / mapped.y + 1) / 2 / 3;
      } else if (mapped.z >= std::abs(mapped.x) && mapped.z >= std::abs(mapped.y)) { // front
        texCoords[i][j][0] = (mapped.x / mapped.z + 1) / 2 / 4 + padding_x;
        texCoords[i][j][1] = (-mapped.y / mapped.z + 1) / 2 / 3 + padding_y;
      } else if (mapped.x <= -std::abs(mapped.y) && mapped.x <= -std::abs(mapped.z)) { // left
        texCoords[i][j][0] = (-mapped.z / mapped.x + 1) / 2 / 4;
        texCoords[i][j][1] = (mapped.y / mapped.x + 1) / 2 / 3 + padding_y;
      } else if (mapped.y <= -std::abs(mapped.x) && mapped.y <= -std::abs(mapped.z)) { // bottom
        texCoords[i][j][0] = (-mapped.x / mapped.y + 1) / 2 / 4 + padding_x;
        texCoords[i][j][1] = (mapped.z / mapped.y + 1) / 2 / 3 + padding_y * 2;
      } else if (mapped.z <= -std::abs(mapped.x) && mapped.z <= -std::abs(mapped.y)) { // back
        texCoords[i][j][0] = (mapped.x / mapped.z + 1) / 2 / 4 + padding_x * 3;
        texCoords[i][j][1] = (mapped.y / mapped.z + 1) / 2 / 3 + padding_y;
      }
    }

  glEnable(GL_TEXTURE_2D);
  for (int i = 0; i < LOD; i++)
  {
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= LOD; j++)
    {
      glTexCoord2dv(texCoords[i][j]);
      glVertex3dv(surfacePoints[i][j]);
      glTexCoord2dv(texCoords[i+1][j]);
      glVertex3dv(surfacePoints[i+1][j]);
    }
    glEnd();
  }
  glDisable(GL_TEXTURE_2D);
}

void DrawFrame()
{
  float hue = 0.7f;
  glLineWidth(1 * thickness);
  glColor3f(hue, hue, hue);

  for (int i = 0; i <= OOCP; i++)
  {
    glBegin(GL_LINE_STRIP);
    for (int j = 0; j <= OOCP; j++)
      glVertex3dv(controlPoints[i][j]);
    glEnd();
  }

  for (int j = 0; j <= OOCP; j++)
  {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= OOCP; i++)
      glVertex3dv(controlPoints[i][j]);
    glEnd();
  }
}

void DrawControlPoints()
{
  glDisable(GL_DEPTH_TEST);
  glBegin(GL_POINTS);
  for (int j = 0; j <= OOCP; j++)
  {
    float k = j / (float) OOCP;
    float a = 0.6f + 0.4f*k;
    float b = 0.5f*k;
    for (int i = 0; i <= OOCP; i++)
    {
      if (i == 0)
        glColor3f(a, b, a);
      else if (i == 1)
        glColor3f(a, a, b);
      else if (i == 2)
        glColor3f(b, a, b);
      else if (i == 3)
        glColor3f(b, b, a);
      else if (i == 4)
        glColor3f(b, a, a);
      else
        glColor3f(a, b, b);
      glVertex3dv(controlPoints[i][j]);
    }
  }
  glEnd();
  glEnable(GL_DEPTH_TEST);
}

int n = 0;

int ScreenShot()
{
  if (n > 1000)
    return 1;

  // we will store the image data here
  unsigned char *pixels;
  // the thingy we use to write files
  FILE * shot;
  // we get the width/height of the screen into this array
  int screenStats[4] = {0, 0, width, height};

  // get the width/height of the window
  //glGetIntegerv(GL_VIEWPORT, screenStats);

  // generate an array large enough to hold the pixel data 
  // (width*height*bytesPerPixel)
  pixels = new unsigned char[screenStats[2]*screenStats[3]*3];
  // read in the pixel data, TGA's pixels are BGR aligned
  glReadPixels(0, 0, screenStats[2], screenStats[3], 0x80E0, //GL_BGR=0x80E0
      GL_UNSIGNED_BYTE, pixels);

  // open the file for writing. If unsucessful, return 1
  char path[] = "screenshots/frame000.tga";

  _itoa_s(n, path+(n < 10? 19 : n < 100? 18 : 17), 4, 10);
  path[20] = '.';
  path[21] = 't';
  path[22] = 'g';
  if((fopen_s(&shot, path, "wb")) != 0) return 1;
  n++;

  // this is the tga header it must be in the beginning of 
  // every (uncompressed) .tga
  unsigned char TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};
  // the header that is used to get the dimensions of the .tga
  // header[1]*256+header[0] - width
  // header[3]*256+header[2] - height
  // header[4] - bits per pixel
  // header[5] - ?
  unsigned char header[6]={((int)(screenStats[2]%256)),
    ((int)(screenStats[2]/256)),
    ((int)(screenStats[3]%256)),
    ((int)(screenStats[3]/256)),24,0};

  // write out the TGA header
  fwrite(TGAheader, sizeof(unsigned char), 12, shot);
  // write out the header
  fwrite(header, sizeof(unsigned char), 6, shot);
  // write the pixels
  fwrite(pixels, sizeof(unsigned char), 
      screenStats[2]*screenStats[3]*3, shot);

  // close the file
  fclose(shot);
  // ROT the memory
  delete [] pixels;

  // return success
  return 0;
}

bool saveScreens = false;

void displayCallback()
{
  glEnable(GL_SCISSOR_TEST);
  for (int view = 0; view < 4; view++)
  {
    SelectViewport(view, true);
    if (view == ROTATE)
      DrawSurface();
    else
      DrawFrame();
    DrawGrid();
    if (view != ROTATE)
      DrawControlPoints();
  }
  glDisable(GL_SCISSOR_TEST);

  if (saveScreens)
    ScreenShot();

  glutSwapBuffers();
}

void reshapeCallback(int nw, int nh)
{
  width = nw;
  height = nh;

  radius = std::sqrt(width * width + height * height) / 4;
}

void keyboardCallback(unsigned char key, int x, int y)
{
  if (key == 27)
    exit(0);
  else if (key == 's' || key == 'S')
    saveScreens = !saveScreens;
  else if (key == 'a' || key == 'A')
    showAxes = !showAxes;
  glutPostRedisplay();
}

double ToLocalX(int x)
{
  return 4*x - width;
}

double ToLocalY(int y)
{
  return height - 4*y;
}

void mouseCallback(int button, int action, int x, int y)
{
  if (action == GLUT_DOWN)
  {
    if (x > width / 2 && y < height / 2)
    {
      selectedView = ROTATE;
      lastX = x - width / 2;
      lastY = y;
    }
    else
    {
      for (int i = 0; i <= OOCP; i++)
        for (int j = 0; j <= OOCP; j++)
        {
          int possibleView = -1;
          double cx, cy;

          if (x < width / 2)
            if (y > height / 2)
            {
              possibleView = FRONT;
              cx = controlPoints[i][j][0];
              cy = controlPoints[i][j][1];
            }
            else
            {
              possibleView = UP;
              cx = controlPoints[i][j][0];
              cy = -controlPoints[i][j][2];
            }
          else if (y > height / 2)
          {
            possibleView = LEFT;
            cx = -controlPoints[i][j][2];
            cy = controlPoints[i][j][1];
          }


          if (possibleView > -1 && Length(cx - ToLocalX(x % (width / 2)), cy - ToLocalY(y % (height / 2)), 0) < 20)
          {
            selectedView = possibleView;
            selectedPoint[0] = i;
            selectedPoint[1] = j;
          }
        }
    }
    mouseButton = button;
  }
  else if (action == GLUT_UP)
  {
    selectedView = -1;
    mouseButton = -1;
  }
}

void motionCallback(int x, int y)
{
  if (selectedView == FRONT)
  {
    controlPoints[selectedPoint[0]][selectedPoint[1]][0] = ToLocalX(x);
    controlPoints[selectedPoint[0]][selectedPoint[1]][1] = ToLocalY(y - height/2);
  }
  else if (selectedView == UP)
  {
    controlPoints[selectedPoint[0]][selectedPoint[1]][0] = ToLocalX(x);
    controlPoints[selectedPoint[0]][selectedPoint[1]][2] = -ToLocalY(y);
  }
  else if (selectedView == LEFT)
  {
    controlPoints[selectedPoint[0]][selectedPoint[1]][2] = -ToLocalX(x - width/2);
    controlPoints[selectedPoint[0]][selectedPoint[1]][1] = ToLocalY(y - height/2);
  }
  else if (selectedView == ROTATE)
  {
    Vector3d lastP = getMousePoint(lastX, lastY, width / 2.0f, height / 2.0f, radius);
    Vector3d currentP = getMousePoint(x - width / 2.0f, y, width / 2.0f, height / 2.0f, radius);

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
    else if (mouseButton == GLUT_RIGHT_BUTTON) {
      Vector3d dEye;
      dEye.sub(center, eye);
      double offset = 0.025;
      if ((y - lastY) < 0) {
        dEye.scale(1 - offset);
      }
      else {
        dEye.scale(1 + offset);
      }
      eye.sub(center, dEye);
    }
    else if (mouseButton == GLUT_MIDDLE_BUTTON) {
      double dx = x - width / 2.0f - lastX;
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
    lastX = x - width / 2;
    lastY = y;
  }

  EvaluateSurface();
  glutPostRedisplay();
}

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  Init();
  glutDisplayFunc(displayCallback);
  glutReshapeFunc(reshapeCallback);
  glutKeyboardFunc(keyboardCallback);
  glutMouseFunc(mouseCallback);
  glutMotionFunc(motionCallback);
  glutMainLoop();
  return 0;
}
