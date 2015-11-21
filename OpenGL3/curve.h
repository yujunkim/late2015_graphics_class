#ifndef _CURVE_H_
#define _CURVE_H_

#define PRECISION   1e-5
#define EPS         1e-6        /* data type is float */

typedef float Point[3];

typedef struct BicubicBezierSurface
{
	Point control_pts[4][4];
} BicubicBezierSurface;

#ifdef DEBUG
void PRINT_CTRLPTS(CubicBezierCurve* crv);
#else
#   define PRINT_CTRLPTS(X)
#endif

#define SET_PT3(V, V1, V2, V3) do { (V)[0] = (V1); (V)[1] = (V2); (V)[2] = (V3); } while (0)

void evaluate(const BicubicBezierSurface *curve, const float t1, const float t2, Point value);

#endif /* _CURVE_H_ */
