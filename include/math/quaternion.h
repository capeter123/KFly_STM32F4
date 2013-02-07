#ifndef __QUATERNION_H
#define __QUATERNION_H

/* Standard includes */
#include <math.h>

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
	float v0;
	float v1;
	float v2;
} vector_t;

/* Global variable defines */

/* Global function defines */
/* *
 *
 *
 * */

/* Includes */
#include "quaternion.h"

/* Private Defines */

/* Private Typedefs */

/* Global variable defines */

/* Private function defines */

/* Global function defines */
void qmult(quaternion_t *, quaternion_t *, quaternion_t *);
void qconj(quaternion_t *);
float qnorm(quaternion_t *);
void qnormalize(quaternion_t *);
void qvqc(quaternion_t *, vector_t *, vector_t *);
float sine(float);
float cosine(float);
float tangent(float);
float invSqrt(float);


#endif
