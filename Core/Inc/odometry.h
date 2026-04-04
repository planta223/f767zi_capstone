/*
 * odometry.h
 *
 *  Created on: Mar 26, 2026
 *      Author: kyubeom
 */

#ifndef INC_ODOMETRY_H_
#define INC_ODOMETRY_H_

#include <stdint.h>

typedef struct
{
    uint32_t t_us;
    float x_m;
    float y_m;
    float yaw_rad;
    float v_mps;
    float w_radps;
} Odometry_t;

void Odometry_Init(void);

void Odometry_Update(void);
void Odometry_Reset(void);

float Odometry_GetX(void);
float Odometry_GetY(void);
float Odometry_GetYaw(void);
float Odometry_GetV(void);
float Odometry_GetW(void);

#endif /* INC_ODOMETRY_H_ */
