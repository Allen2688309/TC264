#ifndef ROUND_H
#define ROUND_H

#include "../My_image/Camera.h"

#define ROUND_EXIT_CONFIRM 5U
#define ROUND_EXIT_NEAR_MIN_ROW 40U
#define ROUND_RIGHT_FAR_HOLD 8U
#define ROUND_LEFT_INTO_FAR_HOLD 16U
#define ROUND_LEFT_INTO_FAR_MISS 12U
#define ROUND_RIGHT_INTO_FAR_HOLD 28U
#define ROUND_RIGHT_INTO_FAR_MIN_ROW 8U  /* 右环入环远点最上行，与检测范围一致 */
#define ROUND_RIGHT_INTO_FAR_MAX_ROW 110U /* 右环入环远点最下行，与检测范围一致 */
#define ROUND_RIGHT_EXIT_CONFIRM 3U
#define ROUND_LEFT_START_DELAY_DEFAULT_S ROUND_SLOW_LEFT_DELAY_S
#define ROUND_RIGHT_START_DELAY_DEFAULT_S ROUND_SLOW_RIGHT_DELAY_S
extern int16 ROUND_LEFT_START_DELAY_S;
extern int16 ROUND_RIGHT_START_DELAY_S;

/* 出环补线端点 */
#define ROUND_LEFT_EXIT_TOP_X     50  /* 左环状态四顶部横坐标 */
#define ROUND_LEFT_RND5_TOP_X     30  /* 左环状态五顶部横坐标 */
#define ROUND_LEFT_EXIT_BOTTOM_X  100 /* 左环状态五底部横坐标 */
#define ROUND_RIGHT_EXIT_TOP_X    160 /* 右环顶部横坐标 */
#define ROUND_RIGHT_EXIT_BOTTOM_X 6   /* 右环底部横坐标 */

/* 阶段时间 */
#define ROUND_LEFT_INTO_MIN_DEFAULT_MS ROUND_SLOW_LEFT_INTO_MIN_MS
#define ROUND_LEFT_INTO_MAX_DEFAULT_MS ROUND_SLOW_LEFT_INTO_MAX_MS
#define ROUND_LEFT_RND4_MIN_DEFAULT_MS ROUND_SLOW_LEFT_RND4_MIN_MS
#define ROUND_LEFT_RND4_MAX_DEFAULT_MS ROUND_SLOW_LEFT_RND4_MAX_MS
#define ROUND_LEFT_RND5_MAX_DEFAULT_MS ROUND_SLOW_LEFT_RND5_MAX_MS
#define ROUND_LEFT_ALIGN_DEFAULT_MS ROUND_SLOW_LEFT_ALIGN_MS
#define ROUND_LEFT_LEAVE_DEFAULT_MS ROUND_SLOW_LEFT_LEAVE_MS
#define ROUND_RIGHT_RND2_MIN_DEFAULT_MS ROUND_SLOW_RIGHT_RND2_MIN_MS
#define ROUND_RIGHT_RND3_MIN_DEFAULT_MS ROUND_SLOW_RIGHT_RND3_MIN_MS
#define ROUND_TIME_MS(v) ((uint32)((v)>0?(v):0))

/* 慢速档圆环时间及发车延迟 */
#define ROUND_SLOW_LEFT_DELAY_S              4
#define ROUND_SLOW_RIGHT_DELAY_S             10
#define ROUND_SLOW_LEFT_INTO_MIN_MS          800
#define ROUND_SLOW_LEFT_INTO_MAX_MS          1000
#define ROUND_SLOW_LEFT_RND4_MIN_MS          800
#define ROUND_SLOW_LEFT_RND4_MAX_MS          1000
#define ROUND_SLOW_LEFT_RND5_MAX_MS          1000
#define ROUND_SLOW_LEFT_ALIGN_MS             800
#define ROUND_SLOW_LEFT_LEAVE_MS             200
#define ROUND_SLOW_RIGHT_RND2_MIN_MS         300
#define ROUND_SLOW_RIGHT_RND3_MIN_MS         400

/* 中速档圆环时间及发车延迟 */
#define ROUND_MID_LEFT_DELAY_S               3
#define ROUND_MID_RIGHT_DELAY_S              9
#define ROUND_MID_LEFT_INTO_MIN_MS           600
#define ROUND_MID_LEFT_INTO_MAX_MS           800
#define ROUND_MID_LEFT_RND4_MIN_MS           600
#define ROUND_MID_LEFT_RND4_MAX_MS           900
#define ROUND_MID_LEFT_RND5_MAX_MS           1000
#define ROUND_MID_LEFT_ALIGN_MS              800
#define ROUND_MID_LEFT_LEAVE_MS              200
#define ROUND_MID_RIGHT_RND2_MIN_MS          100
#define ROUND_MID_RIGHT_RND3_MIN_MS          400

/* 高速档圆环时间及发车延迟，暂以中速档数值初始化 */
#define ROUND_HIGH_LEFT_DELAY_S              3
#define ROUND_HIGH_RIGHT_DELAY_S             8
#define ROUND_HIGH_LEFT_INTO_MIN_MS          600
#define ROUND_HIGH_LEFT_INTO_MAX_MS          800
#define ROUND_HIGH_LEFT_RND4_MIN_MS          600
#define ROUND_HIGH_LEFT_RND4_MAX_MS          800
#define ROUND_HIGH_LEFT_RND5_MAX_MS          1000
#define ROUND_HIGH_LEFT_ALIGN_MS             800
#define ROUND_HIGH_LEFT_LEAVE_MS             200
#define ROUND_HIGH_RIGHT_RND2_MIN_MS         100
#define ROUND_HIGH_RIGHT_RND3_MIN_MS         350

extern int16 Right_rnd2_min_ms; /* 右环状态二最短毫秒 */
extern int16 Right_rnd3_min_ms; /* 右环状态三最短毫秒 */
extern int16 ROUND_LEFT_INTO_MIN_MS;
extern int16 ROUND_LEFT_INTO_MAX_MS;
extern int16 ROUND_LEFT_RND4_MIN_MS;
extern int16 ROUND_LEFT_RND4_MAX_MS;
extern int16 ROUND_LEFT_RND5_MAX_MS;
extern int16 ROUND_LEFT_ALIGN_MS;
extern int16 ROUND_LEFT_LEAVE_MS;

typedef struct {
    uint8 valid;/* 有效标志 */
    uint8 x,y;/* 点位 */
    uint8 hits;/* 确认帧数 */
    uint8 misses;/* 漏检帧数 */
    uint8 px,py;/* 候选位置 */
} RoundPoint;

#define ROUND_LEFT 0U
#define ROUND_RIGHT 1U
#define ROUND_LEFT_DIR 1U
#define ROUND_RIGHT_DIR 2U
#define ROUND_NEAR 0U
#define ROUND_MID 1U
#define ROUND_FAR_POINT 2U

#define ROUND_POINT_CAP (IMG_H*3U)
extern uint16 points_l[ROUND_POINT_CAP][2];
extern uint16 points_r[ROUND_POINT_CAP][2];
extern uint16 count_l,count_r;
extern uint8 round_dir;
extern uint8 round_state;
extern volatile uint8 round_right_done; /* 本轮右环已完成 */
extern uint8 round_line_rows[IMG_H];
extern RoundPoint round_points[2][3];
extern RoundPoint round_exit_point;
extern uint8 round_ok_frames,round_far_seen;
extern uint8 L_corner_flag,L_corner_row,L_corner_col;
extern uint8 R_corner_flag,R_corner_row,R_corner_col;
extern uint8 L_mid_flag,L_mid_row,L_mid_col;
extern uint8 R_mid_flag,R_mid_row,R_mid_col;
extern uint8 L_far_flag,L_far_row,L_far_col;
extern uint8 R_far_flag,R_far_row,R_far_col;
/*==================================================== 状态处理 ====================================================*/
void    ROUND_Init           (void);
void    ROUND_ClearEntry     (void);
void    ROUND_GetPoints      (void);
uint8   ROUND_Detect         (uint8 l_lost,uint8 r_lost);
void    ROUND_Process        (uint8 det,uint8 l_lost,uint8 r_lost);
void    ROUND_StateEnter     (uint8 state);
void    ROUND_LeftProcess    (uint8 l_lost,uint8 r_lost,uint32 ms);
void    ROUND_RightProcess   (uint8 l_lost,uint8 r_lost,uint32 ms);
/*==================================================== 状态处理 ====================================================*/

/*==================================================== 圆环补线 ====================================================*/
void    ROUND_Repair         (void);
void    ROUND_RepairReset    (void);
void    ROUND_RightStartLine (void);
uint8   round_check_exit     (const RoundPoint*corner);
/*==================================================== 圆环补线 ====================================================*/

#endif
