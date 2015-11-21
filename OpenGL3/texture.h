#ifndef JH_TEXTURE_H
#define JH_TEXTURE_H

#include <gl/freeglut.h>

unsigned char *loadTGA(const char* filepath, int &width, int &height);
void initTGA(GLuint *tex, const char *name, int &width, int &height);
void initPNG(GLuint *tex, const char *name, int &width, int &height);
void initTex();

#endif