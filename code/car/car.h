#ifndef __CAR_H__
#define __CAR_H__

#include "zf_common_headfile.h"

#define BTN1_PIN        P20_9 /* 第一个启动按键 */
#define BTN2_PIN        P20_2 /* 第二个启动按键 */
#define BTN3_PIN        P20_7 /* 第三个启动按键 */
#define BUTTON_PRESSED (0U)   /* 按键有效电平 */
#define DEBOUNCE_FRAMES (3U)  /* 按键确认次数 */
#define COUNTDOWN_MS   (3000U) /* 发车倒计时 */
#define VISION_FRAME_LOST_MS (400U) /* 无新图像停车时长 */

#define CAR_IDLE      (0U) /* 等待发车 */
#define CAR_COUNTDOWN (1U) /* 发车倒计时 */
#define CAR_RUNNING   (2U) /* 正在运行 */

#define ROUND_TURN_SAMPLES  6       /* 转向记录数量 */
#define ROUND_TURN_HOLD_US  400000U /* 缺线续弯时长 */
#define ROUND_TURN_FRESH_US 200000U /* 转向记录有效期 */

/* 慢速档：保存当前代码参数，三档分别调整 */
#define CAR_SLOW_PWM_FF               2000
#define CAR_SLOW_STRAIGHT             250
#define CAR_SLOW_LEFT_ROUND           250
#define CAR_SLOW_RIGHT_ROUND          260
#define CAR_SLOW_SPDEND               240
#define CAR_SLOW_DIFF_GAIN            0.0f
#define CAR_SLOW_RUN_S                60
#define CAR_SLOW_STEER_KP             15.0f
#define CAR_SLOW_STEER_KD             36.0f
#define CAR_SLOW_TURN_LIMIT           600
#define CAR_SLOW_SPEED_KP             2.5f
#define CAR_SLOW_SPEED_KI             0.5f

/* 中速档：保留原恢复档参数 */
#define CAR_MID_PWM_FF                2000
#define CAR_MID_STRAIGHT              280
#define CAR_MID_LEFT_ROUND            270
#define CAR_MID_RIGHT_ROUND           290
#define CAR_MID_SPDEND                270
#define CAR_MID_DIFF_GAIN             0.027f
#define CAR_MID_RUN_S                 60
#define CAR_MID_STEER_KP              14.0f
#define CAR_MID_STEER_KD              35.0f
#define CAR_MID_TURN_LIMIT            600
#define CAR_MID_SPEED_KP              2.5f
#define CAR_MID_SPEED_KI              0.5f

/* 高速档：暂以中速档数值初始化，可单独调整 */
#define CAR_HIGH_PWM_FF               2000
#define CAR_HIGH_STRAIGHT             320
#define CAR_HIGH_LEFT_ROUND           310
#define CAR_HIGH_RIGHT_ROUND          340
#define CAR_HIGH_SPDEND               300
#define CAR_HIGH_DIFF_GAIN            0.027f
#define CAR_HIGH_RUN_S                60
#define CAR_HIGH_STEER_KP             14.0f
#define CAR_HIGH_STEER_KD             36.0f
#define CAR_HIGH_TURN_LIMIT           600
#define CAR_HIGH_SPEED_KP             2.5f
#define CAR_HIGH_SPEED_KI             0.5f

/* 开机使用慢速档，菜单恢复操作不会改写各档保存值 */
#define CAR_DEFAULT_PWM_FF            CAR_SLOW_PWM_FF
#define CAR_DEFAULT_STRAIGHT          CAR_SLOW_STRAIGHT
#define CAR_DEFAULT_LEFT_ROUND        CAR_SLOW_LEFT_ROUND
#define CAR_DEFAULT_RIGHT_ROUND       CAR_SLOW_RIGHT_ROUND
#define CAR_DEFAULT_SPDEND            CAR_SLOW_SPDEND
#define CAR_DEFAULT_DIFF_GAIN         CAR_SLOW_DIFF_GAIN
#define CAR_DEFAULT_RUN_S             CAR_SLOW_RUN_S
#define CAR_DEFAULT_STEER_KP          CAR_SLOW_STEER_KP
#define CAR_DEFAULT_STEER_KD          CAR_SLOW_STEER_KD
#define CAR_DEFAULT_TURN_LIMIT        CAR_SLOW_TURN_LIMIT
#define CAR_DEFAULT_SPEED_KP          CAR_SLOW_SPEED_KP
#define CAR_DEFAULT_SPEED_KI          CAR_SLOW_SPEED_KI

/* 运行参数 */
extern int16 CAR_BASE_SPEED;      /* 电机前馈占空比 */
extern int16 LEFT_ROUND_SPEED;    /* 左环目标速度 */
extern int16 RIGHT_ROUND_SPEED;   /* 右环目标速度 */
extern int16 SPDEND;              /* 右环结束后目标速度 */
extern float STEER_KP;            /* 转向比例系数 */
extern float STEER_KD;            /* 转向微分系数 */
extern int16 STEER_OFFSET_LIMIT;  /* 舵机偏移限幅 */
extern uint8 STEER_REVERSE;       /* 转向取反开关 */
extern int16 CAR_RUN_S;           /* 运行时长秒数 */
extern int16 CAR_TEST_MODE;       /* 固定中位调试 */

#define CAR_MODE_DRIVE (0)
#define CAR_MODE_SPEED_TUNE (1)

#define RACE_SHOW_DIV (10)

extern uint8 CAR_MODE;            /* 行驶或坡道调试 */
extern int16 LOST_HOLD_MS;        /* 普通丢线保持毫秒 */
extern int16 LOST_HOLD_MS_ELEM;   /* 元素丢线保持毫秒 */
extern volatile uint8  car_state;    /* 当前车辆状态，供视觉侧限制圆环运行 */
extern volatile uint32 Car_state_us; /* 当前状态起点，运行时即本次发车时刻 */
/*==================================================== 基础函数 ====================================================*/
void    Car_Init        (void);
void    Car_Task        (void);
void    Car_SteerTask   (void);
void    Car_Loop        (void);
/*==================================================== 基础函数 ====================================================*/

/*==================================================== 参数管理 ====================================================*/
void    Car_Mode_Toggle (void);
void    Car_ResetSlow   (void);
void    Car_ResetParams (void);
void    Car_ResetHigh   (void);
/*==================================================== 参数管理 ====================================================*/

#endif
