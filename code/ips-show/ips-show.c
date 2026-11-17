/* 状态显示 */
#include "car.h"
#include "motor.h"
#include "servo.h"
#include "Camera.h"
#include "MyEncoder/MyEncoder.h"
#include "ips-show.h"
#include "wifi/Wifi_Cam.h"
#include "zf_device_wifi_spi.h"

/***********************************************
 * @brief : 显示车辆运行状态及视觉和控制数据
 * @param : State 车辆当前运行状态
 * @param : Countdown_s 发车倒计时秒数
 * @param : Run_s 本次运行秒数
 * @param : Lost_now 当前连续丢线帧数
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Car_Show_Debug(uint8 State,uint8 Countdown_s,uint8 Run_s,uint16 Lost_now)
{
    if(State==0) /* CAR_IDLE */
    {
        ips200_show_string(2,124,"STATE: IDLE");
        ips200_show_string(146,124,"  ");
    }
    else if(State==1)
    {
        ips200_show_string(2,124,"STATE: COUNTDOWN");
        ips200_show_uint(146,124,Countdown_s,2);
    }
    else
    {
        ips200_show_string(2,124,"STATE: RUNNING  ");
        ips200_show_uint(146,124,Run_s,2);
    }
    ips200_show_string(2,142,"FPS:");
    ips200_show_uint(34,142,vision_fps,3);
    ips200_show_string(90,142,"T(us):");
    ips200_show_uint(142,142,vision_proc_us,5);
    ips200_show_string(2,160,"THRESH:");
    ips200_show_uint(70,160,vision_threshold,3);
    ips200_show_string(120,160,"OK:");
    ips200_show_uint(150,160,vision_valid,1);
    ips200_show_string(170,160,"X:");
    ips200_show_uint(186,160,vision_cross,1);
    ips200_show_string(200,160,"O:");
    ips200_show_uint(216,160,vision_out,1);
    ips200_show_string(2,178,"ERR:");
    ips200_show_int(36,178,vision_error,4);
    ips200_show_string(80,178,"RAW:");
    ips200_show_int(120,178,vision_error_raw,4);
    ips200_show_string(170,178,"L:");
    ips200_show_uint(186,178,Lost_now,2);
    ips200_show_string(2,196,"SERVO:");
    ips200_show_uint(62,196,servo_duty,4);
    ips200_show_string(110,196,"SPD:");
    ips200_show_int(146,196,enc_speed_l,3);
    ips200_show_string(176,196,"/");
    ips200_show_int(184,196,enc_speed_r,3);
    ips200_show_string(2,214,"ROAD:");
    ips200_show_uint(50,214,vision_road_type,1);
    ips200_show_string(74,214,"RND:");
    ips200_show_uint(110,214,vision_round_state,1);
    ips200_show_string(134,214,"DIR:");
    ips200_show_uint(170,214,vision_round_dir,1);
    ips200_show_string(178,214,"D:");
    ips200_show_int(194,214,vision_round_dist,4);
    ips200_show_string(2,232,"LLOST:");
    ips200_show_uint(58,232,(uint32)vision_left_lost,2);
    ips200_show_string(86,232,"RLOST:");
    ips200_show_uint(142,232,(uint32)vision_right_lost,2);
    if(State==2)
    {
        ips200_show_string(2,250,"KI:");
        ips200_show_float(34,250,(double)SPD_TUNE_KI,1,2);
        ips200_show_string(90,250,"KP:");
        ips200_show_float(122,250,(double)SPD_TUNE_KP,1,2);
    }
    else
    {
        ips200_show_string(2,250,"CNTL:");
        ips200_show_int(42,250,enc_total_l,6);
        ips200_show_string(106,250,"R:");
        ips200_show_int(122,250,enc_total_r,6);
    }
    ips200_show_string(178,250,"NS:");
    ips200_show_int(202,250,vision_no_start,3);
    ips200_show_string(2,268,"T:");
    ips200_show_int(18,268,spd_tgt_l,3);
    ips200_show_string(44,268,"/");
    ips200_show_int(52,268,spd_tgt_r,3);
    ips200_show_string(86,268,"O:");
    ips200_show_int(110,268,spd_out_l,4);
    ips200_show_string(152,268,"/");
    ips200_show_int(160,268,spd_out_r,4);
    /* 底部操作提示 */
    if(State==0) /* CAR_IDLE */
        ips200_show_string(2,286,"PRESS ANY BUTTON");
    else if(State==2)
        ips200_show_string(2,286,"PRESS AGAIN TO STOP");
    else
        ips200_show_string(2,286,"                    ");
    ips200_show_string(2,304,"WF:");
    ips200_show_uint(26,304,wifi_ready,1);
    ips200_show_string(50,304,"S:");
    ips200_show_uint(66,304,Wifi_Cam_GetStage(),1);
    ips200_show_string(90,304,"TX:");
    ips200_show_uint(114,304,wifi_send_cnt,5);
    ips200_show_string(170,304,"I:");
    ips200_show_uint(186,304,gpio_get_level(WIFI_SPI_INT_PIN),1);
}
