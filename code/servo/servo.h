#ifndef __SERVO_H__
#define __SERVO_H__

#include "zf_common_headfile.h"

#define SERVO_PWM_PIN ATOM1_CH2_P10_5
#define SERVO_PWM_HZ (300U)
#define SERVO_HARD_MIN (3300U) /* 防堵转下限 */
#define SERVO_HARD_MAX (5400U) /* 防堵转上限 */

extern uint16 SERVO_CENTER_DUTY;/* 中 */
extern uint16 SERVO_MIN_DUTY;/* 右 */
extern uint16 SERVO_MAX_DUTY;/* 左 */
extern volatile uint16 servo_duty;/* 最近限幅后输出 */

/*==================================================== 舵机控制 ====================================================*/
void    Servo_Init    (void);
void    Servo_SetDuty (uint16 Duty);
void    Servo_Center  (void);
/*==================================================== 舵机控制 ====================================================*/

#endif
