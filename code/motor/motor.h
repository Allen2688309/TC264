#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "zf_common_headfile.h"

#define MOTOR_L_PWM_PIN ATOM0_CH2_P33_11
#define MOTOR_L_DIR_PIN P33_13
#define MOTOR_R_PWM_PIN ATOM2_CH1_P33_5
#define MOTOR_R_DIR_PIN P33_9

#define MOTOR_PWM_HZ (17000U)

#define MOTOR_DUTY_MAX (6000U)

#define MOTOR_LEFT        (0U) /* 左轮编号 */
#define MOTOR_RIGHT       (1U) /* 右轮编号 */
#define MOTOR_WHEEL_COUNT (2U) /* 电机数量 */

/*==================================================== 电机控制 ====================================================*/
void    Motor_Init            (void);
void    Motor_SetSpeed        (int32 Left_duty,int32 Right_duty);
void    Motor_Stop            (void);
void    Motor_SpeedTick       (void);
/*==================================================== 电机控制 ====================================================*/
extern uint8 SPD_EN;          /* 速度闭环开关 */
extern int16 SPD_BASE;        /* 普通赛道目标速度 */
extern int16 SPD_TRIM;        /* 前馈最大修正量 */
extern int16 SPD_BRAKE_TRIM;  /* 反向制动限幅 */
extern int16 SPD_DEAD_SPD;    /* 低速判断门槛 */
extern int16 SPD_DEAD_FRAMES; /* 低速回退计数 */
extern float DIFF_GAIN;       /* 转向差速系数 */
extern float SPD_TUNE_KP;     /* 左右轮共用比例 */
extern float SPD_TUNE_KI;     /* 左右轮共用积分 */

/*==================================================== 速度控制 ====================================================*/
void    Motor_Drive           (int32 Dir_offset,int16 Base_target);
void    Motor_UpdateDisplay   (int32 Dir_offset,int16 Base_target);
void    Motor_SpeedLoop_Reset (void);
/*==================================================== 速度控制 ====================================================*/

/*==================================================== 单轮计算 ====================================================*/
void    motor_set_one         (uint32 Pwm_pin,uint32 Dir_pin,int32 Duty);
float   Motor_LimitOut        (float Output,float Low,float High);
int32   spd_pid_run           (uint8 Side,float Target,float Actual);
/*==================================================== 单轮计算 ====================================================*/

/*==================================================== 运行数据 ====================================================*/
extern volatile int16 spd_out_l;    /* 左轮当前输出 */
extern volatile int16 spd_out_r;    /* 右轮当前输出 */
extern volatile int16 spd_tgt_l;    /* 左轮目标速度 */
extern volatile int16 spd_tgt_r;    /* 右轮目标速度 */
extern int16 spd_diff;              /* 当前差速分量 */
extern int16 spd_diff_peak;         /* 差速绝对峰值 */
/*==================================================== 运行数据 ====================================================*/

#endif
