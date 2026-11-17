#ifndef __MY_HEADFILE_H__
#define __MY_HEADFILE_H__

#include<stdbool.h>

/* 逐飞库总头文件 */
#include "zf_common_headfile.h"

#include "MyKey/MyKey.h" /* 旋转编码器+按键 */
#include "MyEncoder/MyEncoder.h"/* 车轮编码器 */
#include "Camera.h" /* 摄像头图像处理 */
#include "motor.h" /* 电机 */
#include "servo.h" /* 舵机 */
#include "car.h" /* 小车状态机 */
#include "Beep.h" /* 蜂鸣器 */
#include "ips-show/ips-show.h" /* 跑车状态页 */

#endif
