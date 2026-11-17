#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "zf_common_headfile.h"

#define IMG_W MT9V03X_W /* 图像宽188 */
#define IMG_H MT9V03X_H /* 图像高120 */

#define IMG_WHITE (255U) /* 二值图的白，赛道 */
#define IMG_BLACK (0U) /* 二值图的黑，边线 */

extern volatile uint16 vision_camera_timeouts;/* 等帧超时次数 */

/* 赛道元素 */
#define ROAD_NORMAL (0U) /* 普通赛道 */
#define ROAD_CROSS (1U) /* 十字 */
#define ROAD_ROUND_LEFT (2U) /* 左圆环 */
#define ROAD_ROUND_RIGHT (3U) /* 右圆环 */

#define ROUND_NONE (0U) /* 等入口 */
#define ROUND_FAR (1U) /* 近点补线 */
#define ROUND_STRAIGHT (2U) /* 等远点 */
#define ROUND_INTO (3U) /* 远点连对侧底部 */
#define ROUND_IN (4U) /* 环内巡线 */
#define ROUND_OUT (5U) /* 出口引导 */
#define ROUND_ALIGN (6U) /* 远点补线 */
#define ROUND_LEAVE (7U) /* 出环 */

extern uint8 bin_image[IMG_H][IMG_W];/* 二值图 */
extern uint8 l_border[IMG_H];
extern uint8 r_border[IMG_H];/* 右边线 */
extern uint8 center_line[IMG_H];/* 255为无效 */
extern uint8 l_valid[IMG_H]; /* 左侧每行标记：零为无效，一为实测，二为补线 */
extern uint8 r_valid[IMG_H]; /* 右侧每行标记：零为无效，一为实测，二为补线 */
extern volatile int16 vision_error;
extern volatile int16 vision_error_raw;
extern volatile uint8 vision_valid;/* 这一帧是否找到线 */
extern volatile uint8 vision_turn_ok;/* 转向可信 */
extern volatile uint8 vision_exit_blend;/* 出口交接 */
extern volatile uint8 vision_lost;
extern volatile uint8 vision_out;
extern volatile int16 vision_no_start;
extern volatile uint8 vision_cross;/* 是否识别到十字 */
extern volatile uint8 vision_road_type;
extern volatile uint8 vision_round_state;
extern volatile uint8 vision_round_dir;/* 1左环2右环0无 */
extern volatile int16 vision_round_dist;
extern volatile uint8 vision_threshold;
extern int16 SHOW_DIV;
extern volatile uint16 T_OTSU;/* 大津法+二值化 */
extern volatile uint16 T_TRACE;
extern volatile uint16 T_FILL;/* 补洞和拐点 */
extern volatile uint16 T_ELEM;
extern volatile uint16 T_SHOW;/* 画屏 */

extern int16 START_IN_BLACK_MAX;
extern volatile uint16 vision_fps;/* 每秒处理帧数 */
extern volatile uint16 vision_proc_us;
extern volatile int16 vision_left_lost;
extern volatile int16 vision_right_lost;/* 右边线丢线行数 */

extern volatile int16 vision_offset;
extern int16 ERR_TOP;/* 前瞻上行 */
extern int16 ERR_BOTTOM;/* 前瞻下行 */
extern int16 CROSS_LOST_MIN;
extern int16 ROUND_LOST_MIN;
#define LOST_NOSTART_FRAMES_DEFAULT 240
extern int16 LOST_NOSTART_FRAMES;
extern volatile uint8 vision_show_enable;
extern volatile uint32 vision_frame_id;
extern volatile uint8 vision_snapshot_release;
typedef struct
{
    uint32 frame;
    float error;
    int16 no_start;
    uint8 valid,out,turn_ok,exit_blend,road_type,round_state,round_dir;
} VisionData;
uint8 Vision_Read(VisionData *data);
void Vision_RequestReset(void);

/* 总入口 */
uint8 Image_Show(void);
void Image_Display(void);
void Vision_Reset(void);

#endif
