#include "curve.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

enum XYZ { X = 0, Y, Z };

#define COPY_PT(DST, SRC)               do { (DST)[X] = SRC[X]; (DST)[Y] = SRC[Y]; } while (0)
#define VECTOR3_X_SCALA_ADD(O, V, S)    do { O[X] += (S) * (V)[X]; O[Y] += (S) * (V)[Y]; O[Z] += (S) * (V)[Z]; } while (0)

#ifdef DEBUG
void PRINT_CTRLPTS(CubicBezierCurve* crv) {
	int i;
	printf("curve %p\n[\n", crv);
	for (i=0; i<4; ++i)
		printf("[%f, %f]\n", crv->control_pts[i][X], crv->control_pts[i][Y]);
	printf("]\n");
}
#endif

void evaluate(const BicubicBezierSurface *curve, const float t1, const float t2, Point value)
{
	const float t1_inv = 1.0f - t1;
	const float t1_inv_sq = t1_inv * t1_inv;
	const float t1_sq = t1 * t1;
	const float t2_inv = 1.0f - t2;
	const float t2_inv_sq = t2_inv * t2_inv;
	const float t2_sq = t2 * t2;

	float b[2][4];
	b[0][0] = t1_inv_sq * t1_inv;
	b[0][1] = 3 * t1_inv_sq * t1;
	b[0][2] = 3 * t1_inv * t1_sq;
	b[0][3] = t1_sq * t1;
	b[1][0] = t2_inv_sq * t2_inv;
	b[1][1] = 3 * t2_inv_sq * t2;
	b[1][2] = 3 * t2_inv * t2_sq;
	b[1][3] = t2_sq * t2;

	SET_PT3(value, 0, 0, 0);
	for (int i = 0; i < 4; i++)
	for (int j = 0; j < 4; j++)
		VECTOR3_X_SCALA_ADD(value, curve->control_pts[i][j], b[0][i] * b[1][j]);
}
