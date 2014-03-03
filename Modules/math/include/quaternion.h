#ifndef __QUATERNION_H
#define __QUATERNION_H

/* Standard includes */
#include "linear_algebra.h"

/* System includes */

/* Scheduler includes */

/* KFly includes */

/* Includes */

/* Defines */
#define PI ( 3.14159265359f )

/* Typedefs */
typedef struct
{
	float q0;
	float q1;
	float q2;
	float q3;
} quaternion_t;

typedef struct
{
	float x;
	float y;
	float z;
} vector3f_t;

/* Global variable defines */

/* Global function defines */

/* Includes */
#include "trigonometry.h"

/* Private Defines */

/* Private Typedefs */

/* Global variable defines */

/* Private function defines */

/* Inline functions */
static inline quaternion_t qmult(quaternion_t a, quaternion_t b)
{
	/**
	 * q = [a(1)*b(1) - a(2)*b(2) - a(3)*b(3) - a(4)*b(4);
     * 		a(1)*b(2) + a(2)*b(1) + a(3)*b(4) - a(4)*b(3);
     * 		a(1)*b(3) - a(2)*b(4) + a(3)*b(1) + a(4)*b(2);
     * 		a(1)*b(4) + a(2)*b(3) - a(3)*b(2) + a(4)*b(1)]
     * */
	quaternion_t r;

	r.q0 = a.q0*b.q0 - a.q1*b.q1 - a.q2*b.q2 - a.q3*b.q3;
	r.q1 = a.q0*b.q1 + a.q1*b.q0 + a.q2*b.q3 - a.q3*b.q2;
	r.q2 = a.q0*b.q2 - a.q1*b.q3 + a.q2*b.q0 + a.q3*b.q1;
	r.q3 = a.q0*b.q3 + a.q1*b.q2 - a.q2*b.q1 + a.q3*b.q0;

	return r;
}

static inline vector3f_t vector_rotation(float R[3][3], vector3f_t v)
{
	vector3f_t rot;

	rot.x = R[0][0] * v.x + R[0][1] * v.y + R[0][2] * v.z;
 	rot.y = R[1][0] * v.x + R[1][1] * v.y + R[1][2] * v.z;
 	rot.z = R[2][0] * v.x + R[2][1] * v.y + R[2][2] * v.z;

 	return rot;
}

static inline vector3f_t vector_rotation_transposed(float R[3][3], vector3f_t v)
{
	vector3f_t rot;

	rot.x = R[0][0] * v.x + R[1][0] * v.y + R[2][0] * v.z;
 	rot.y = R[0][1] * v.x + R[1][1] * v.y + R[2][1] * v.z;
 	rot.z = R[0][2] * v.x + R[1][2] * v.y + R[2][2] * v.z;

 	return rot;
}

/* Global function defines */
void qconj(quaternion_t *);
float qnorm(quaternion_t *);
void qnormalize(quaternion_t *);
void qvqc(quaternion_t *, vector3f_t *, vector3f_t *);
void euler2quat(float, float, float, quaternion_t *);
void q2dcm(float *a, quaternion_t *q);

#endif
