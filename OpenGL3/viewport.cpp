#include "viewport.h"

using namespace std;

#ifdef DEBUG
void PRINT_CTRLPTS(CubicBezierCurve* crv) {
	int i;
	printf("curve %p\n[\n", crv);
	for (i=0; i<4; ++i)
		printf("[%f, %f]\n", crv->control_pts[i][X], crv->control_pts[i][Y]);
	printf("]\n");
}
#endif

Vector3d rotate(Vector3d input, Vector3d rotateVector, float angle){
	Matrix3d matrix;
	Vector3d rotated = input;
	matrix.set(angle, rotateVector);
	matrix.transform(rotated);
	return rotated;
}

Vector3d unProjectToEye(Vector3d vector, Vector3d& eye, Vector3d& center, Vector3d& upVector){
	Vector3d zAxis;
	zAxis.sub(center, eye);
	zAxis.normalize();
	Vector3d xAxis;
	xAxis.cross(upVector, zAxis);
	xAxis.normalize();
	Vector3d yAxis;
	yAxis.cross(zAxis, xAxis);
	yAxis.normalize();

	Vector3d newVector;
	xAxis.scale(vector.x);
	newVector.add(xAxis);
	yAxis.scale(vector.y);
	newVector.add(yAxis);
	zAxis.scale(vector.z);
	newVector.add(zAxis);
	return newVector;
}

Vector3d getMousePoint(int mouseX, int mouseY, int width, int height, float radius){
	float x = mouseX - width/2.0;
	float y = mouseY - height/2.0;
	float zs = radius*radius - x*x - y*y;
	if (zs <= 0){
		zs = 0;
	}
	float z = std::sqrt(zs);
	return Vector3d(x, y, z);
}