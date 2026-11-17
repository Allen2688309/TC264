#ifndef __MY_KEY_H__
#define __MY_KEY_H__

#include "zf_common_headfile.h"

/* 接线 */
#define SWITCH_ENCODER_A_PIN P02_6 /* 编码器A相 */
#define SWITCH_ENCODER_B_PIN P02_7 /* 编码器B相 */
#define SWITCH_ENCODER_KEY_PIN P02_5

#define MY_BTN_SW5_PIN P20_9
#define MY_BTN_SW1_PIN P20_2
#define MY_BTN_SW2_PIN P20_7

#define MY_KEY_1 (0)
#define MY_KEY_2 (1)
#define MY_KEY_3 (2)
#define MY_KEY_4 (3)
#define MY_KEY_NUM (4)

#define MY_KEY_RELEASE (0) /* 没动作 */
#define MY_KEY_SHORT_PRESS (1) /* 短按 */
#define MY_KEY_LONG_PRESS (2) /* 长按 */

/* 参数 */
#define MY_KEY_DEBOUNCE (3U)

#define MY_KEY_MIN_GAP_US (5000U)
#define MY_KEY_LONG_MS (600U)
#define ENC_EDGES_PER_STEP (4)

extern int16 switch_encoder_change_num;
void MyKey_Init(void);/* 上电初始化一次 */
void MyKey_Scan(void);
uint8 my_key_get_state(uint8 key);
uint8 If_Switch_Encoder_Change(void);

#endif
