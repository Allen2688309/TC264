#include "MyEncoder.h"

int16 enc_last_count_l=0;
int16 enc_last_count_r=0;

volatile int16 enc_speed_l=0;
volatile int16 enc_speed_r=0;

volatile int32 enc_total_l=0;
volatile int32 enc_total_r=0;

int32 enc_dist_l=0;
int32 enc_dist_r=0;

uint32 enc_last_us=0;/* 上次采样的时刻 */

/***********************************************
 * @brief : 初始化编码器并清零速度和里程
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Encoder_Init(void)
{
    encoder_dir_init(ENCODER_L_TIMER,ENCODER_L_COUNT_PIN,ENCODER_L_DIR_PIN);
    encoder_dir_init(ENCODER_R_TIMER,ENCODER_R_COUNT_PIN,ENCODER_R_DIR_PIN);
    encoder_clear_count(ENCODER_L_TIMER);
    encoder_clear_count(ENCODER_R_TIMER);
    /* 清零 */
    enc_last_count_l=0;
    enc_last_count_r=0;
    enc_speed_l=0;
    enc_speed_r=0;
    enc_total_l=0;
    enc_total_r=0;
    enc_dist_l=0;
    enc_dist_r=0;
    enc_last_us=system_getval_us();
}

/***********************************************
 * @brief : 按采样间隔换算轮速并累计脉冲
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Encoder_Update(void)
{
    uint32 now_us;
    uint32 dt_us;
    int16 raw_l,raw_r;
    int16 delta_l,delta_r;
    now_us=system_getval_us();
    dt_us=now_us-enc_last_us;
    if(dt_us==0)
    {
        return;
    }
    enc_last_us=now_us;
    raw_l=encoder_get_count(ENCODER_L_TIMER);
    raw_r=encoder_get_count(ENCODER_R_TIMER);
    delta_l=(int16)(raw_l-enc_last_count_l);
    delta_r=(int16)(raw_r-enc_last_count_r);
    enc_last_count_l=raw_l;
    enc_last_count_r=raw_r;
#if ENCODER_L_REVERSE
    delta_l=-delta_l;
#endif
#if ENCODER_R_REVERSE
    delta_r=-delta_r;
#endif
    enc_total_l+=delta_l;
    enc_total_r+=delta_r;
    enc_dist_l+=delta_l;
    enc_dist_r+=delta_r;
    {
        int32 speed_l=(int32)delta_l*(int32)ENCODER_SPEED_UNIT_US/(int32)dt_us;
        int32 speed_r=(int32)delta_r*(int32)ENCODER_SPEED_UNIT_US/(int32)dt_us;
        enc_speed_l=(int16)(((int32)enc_speed_l*(10-ENCODER_FILTER_N)
                               +speed_l*ENCODER_FILTER_N)/10);
        enc_speed_r=(int16)(((int32)enc_speed_r*(10-ENCODER_FILTER_N)
                               +speed_r*ENCODER_FILTER_N)/10);
    }
}

/***********************************************
 * @brief : 将左右轮累计脉冲换算为平均行驶距离
 * @param : 无
 * @return: 自上次清零后的平均行驶距离，单位毫米
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
float Encoder_GetDistanceMM(void)
{
    return (float)enc_dist_l*(ENCODER_MM_PER_COUNT*0.5f)
         +(float)enc_dist_r*(ENCODER_MM_PER_COUNT*0.5f);
}

/***********************************************
 * @brief : 清零距离计算使用的左右轮累计脉冲
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Encoder_ClearCount(void)
{
    enc_dist_l=0;
    enc_dist_r=0;
}
