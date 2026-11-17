#ifndef __MY_ENCODER_H__
#define __MY_ENCODER_H__

#include "zf_common_headfile.h"

#define ENCODER_L_TIMER (TIM4_ENCODER)
#define ENCODER_L_COUNT_PIN (TIM4_ENCODER_CH1_P02_8)
#define ENCODER_L_DIR_PIN (TIM4_ENCODER_CH2_P00_9)

#define ENCODER_R_TIMER (TIM6_ENCODER)
#define ENCODER_R_COUNT_PIN (TIM6_ENCODER_CH1_P20_3)
#define ENCODER_R_DIR_PIN (TIM6_ENCODER_CH2_P20_0)

#define ENCODER_L_REVERSE (0)
#define ENCODER_R_REVERSE (1)
/* 参数 */

#define ENCODER_SPEED_UNIT_US (10000)

#define ENCODER_FILTER_N (7) /* 本次轮速测量占七成，历史反馈占三成 */

#define ENCODER_MM_PER_COUNT (0.0606f)

/*==================================================== 运行数据 ====================================================*/
extern volatile int16 enc_speed_l;  /* 左轮滤波速度 */
extern volatile int16 enc_speed_r;  /* 右轮滤波速度 */
extern volatile int32 enc_total_l;  /* 左轮累计脉冲 */
extern volatile int32 enc_total_r;  /* 右轮累计脉冲 */
/*==================================================== 运行数据 ====================================================*/

/*==================================================== 编码器控制 ====================================================*/
void    Encoder_Init          (void);
void    Encoder_Update        (void);
float   Encoder_GetDistanceMM (void);
void    Encoder_ClearCount    (void);
/*==================================================== 编码器控制 ====================================================*/

#endif
