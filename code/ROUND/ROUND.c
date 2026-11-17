#include "ROUND.h"
#include "../MyEncoder/MyEncoder.h"
#include "../car/car.h"
#include<string.h>

uint8 round_dir;
uint8 round_state=ROUND_NONE;
uint8 round_ok_frames,round_far_seen;

uint32 round_state_us;
uint8 entry_frames[5],entry_dir;
uint8 round_left_done;
volatile uint8 round_right_done; /* 本轮右环已完成 */
int16 ROUND_LEFT_START_DELAY_S=ROUND_LEFT_START_DELAY_DEFAULT_S;
int16 ROUND_RIGHT_START_DELAY_S=ROUND_RIGHT_START_DELAY_DEFAULT_S;
int16 ROUND_LEFT_INTO_MIN_MS=ROUND_LEFT_INTO_MIN_DEFAULT_MS;
int16 ROUND_LEFT_INTO_MAX_MS=ROUND_LEFT_INTO_MAX_DEFAULT_MS;
int16 ROUND_LEFT_RND4_MIN_MS=ROUND_LEFT_RND4_MIN_DEFAULT_MS;
int16 ROUND_LEFT_RND4_MAX_MS=ROUND_LEFT_RND4_MAX_DEFAULT_MS;
int16 ROUND_LEFT_RND5_MAX_MS=ROUND_LEFT_RND5_MAX_DEFAULT_MS;
int16 ROUND_LEFT_ALIGN_MS=ROUND_LEFT_ALIGN_DEFAULT_MS;
int16 ROUND_LEFT_LEAVE_MS=ROUND_LEFT_LEAVE_DEFAULT_MS;

/***********************************************
 * @brief : 清除左右圆环的入口命中记录
 * @param : 无
 * @return: 无
 * @date  : 2026-11-08
 * @author: L
 ************************************************/
void ROUND_ClearEntry(void)
{
    memset(entry_frames,0,sizeof(entry_frames));
    entry_dir=0;
}

void round_clear_points(void)
{
    memset(round_points,0,sizeof(round_points));
    L_corner_flag=L_corner_row=L_corner_col=0;
    R_corner_flag=R_corner_row=R_corner_col=0;
    L_mid_flag=L_mid_row=L_mid_col=0;
    R_mid_flag=R_mid_row=R_mid_col=0;
    L_far_flag=L_far_row=L_far_col=0;
    R_far_flag=R_far_row=R_far_col=0;
}

/***********************************************
 * @brief : 进入圆环状态并重置阶段计时和所需点位缓存
 * @param : state 即将进入的圆环状态
 * @return: 无
 ************************************************/
void ROUND_StateEnter(uint8 state)
{
    if(state==ROUND_FAR) ROUND_RepairReset();
    if(state==ROUND_IN)
    {
        if(round_dir==ROUND_RIGHT_DIR) ROUND_RightStartLine();
        round_clear_points();
    }
    if((round_dir==ROUND_LEFT_DIR||round_dir==ROUND_RIGHT_DIR)&&state==ROUND_OUT)
    {
        round_exit_point=(RoundPoint){0};
        round_points[round_dir-1][ROUND_FAR_POINT]=(RoundPoint){0};
        if(round_dir==ROUND_LEFT_DIR) L_far_flag=L_far_col=L_far_row=0;
        else R_far_flag=R_far_col=R_far_row=0;
    }
    if((round_dir==ROUND_LEFT_DIR||round_dir==ROUND_RIGHT_DIR)&&state==ROUND_ALIGN)
    {
        /* 保留出环远点 */
        RoundPoint far_point=round_points[round_dir-1][ROUND_FAR_POINT];
        round_clear_points();
        round_points[round_dir-1][ROUND_FAR_POINT]=far_point;
        if(round_dir==ROUND_LEFT_DIR)
        { L_far_flag=far_point.valid;L_far_col=far_point.x;L_far_row=far_point.y;}
        else
        { R_far_flag=far_point.valid;R_far_col=far_point.x;R_far_row=far_point.y;}
        ROUND_RepairReset();
    }
    if(state==ROUND_NONE)
    {
        round_dir=0;
        round_clear_points();
        ROUND_RepairReset();
        ROUND_ClearEntry();
    }
    round_state=state;
    round_state_us=system_getval_us();
    round_ok_frames=0;
    round_far_seen=0;
    if((round_dir==ROUND_LEFT_DIR||round_dir==ROUND_RIGHT_DIR)&&state==ROUND_INTO)
    {
        RoundPoint*far_point=&round_points[round_dir-1][ROUND_FAR_POINT];
        if(round_dir==ROUND_RIGHT_DIR)
        {
            /* 进入右环状态三时清理范围外旧点，避免继续用旧点补线。 */
            if(far_point->valid
               &&(far_point->y<ROUND_RIGHT_INTO_FAR_MIN_ROW
                  ||far_point->y>ROUND_RIGHT_INTO_FAR_MAX_ROW))
            {
                *far_point=(RoundPoint){0};
                R_far_flag=R_far_col=R_far_row=0;
            }
            else if(far_point->hits
                    &&(far_point->py<ROUND_RIGHT_INTO_FAR_MIN_ROW
                       ||far_point->py>ROUND_RIGHT_INTO_FAR_MAX_ROW))
                far_point->hits=0;
        }
        if(far_point->valid&&far_point->misses==0) round_far_seen=1;
    }
    Encoder_ClearCount();
}

/***********************************************
 * @brief : 清除圆环状态、点位缓存和本次运行的完成记录
 * @param : 无
 * @return: 无
 ************************************************/
void ROUND_Init(void)
{
    round_state=ROUND_NONE;
    round_dir=0;
    round_clear_points();
    round_state_us=0;
    ROUND_ClearEntry();
    round_left_done=round_right_done=0;
    round_ok_frames=round_far_seen=0;
    ROUND_RepairReset();
    vision_round_dist=0;
}

/***********************************************
 * @brief : 仅在车辆运行时按发车延时、入口命中及阶段条件推进圆环
 * @param : det 本帧道路识别结果
 * @param : l_lost 左侧丢线行数
 * @param : r_lost 右侧丢线行数
 * @return: 无
 ************************************************/
void ROUND_Process(uint8 det,uint8 l_lost,uint8 r_lost)
{
    uint32 now_us,ms;
    uint32 delay_s,run_s;
    uint8 active_dir,active_state,i,hits,Entry_rows;
    float dist;

    /* 待机、倒计时和调试模式均不能积累入口命中或推进状态。 */
    if(car_state!=CAR_RUNNING||CAR_TEST_MODE)
    {
        ROUND_ClearEntry();
        return;
    }
    __dsync();
    now_us=system_getval_us();
    ms=(now_us-round_state_us)/1000U;
    dist=Encoder_GetDistanceMM();
    if(dist<0) dist=0;
    vision_round_dist=(int16)(dist>9999?9999:dist);
    if(round_state==ROUND_NONE)
    {
        if((det==ROAD_ROUND_LEFT&&round_left_done)
           ||(det==ROAD_ROUND_RIGHT&&round_right_done))
        {
            ROUND_ClearEntry();
            return;
        }
        if(det==ROAD_ROUND_LEFT)
            delay_s=ROUND_LEFT_START_DELAY_S<0?0U:(uint32)ROUND_LEFT_START_DELAY_S;
        else
            delay_s=ROUND_RIGHT_START_DELAY_S<0?0U:(uint32)ROUND_RIGHT_START_DELAY_S;
        run_s=(uint32)(now_us-Car_state_us)/1000000U;
        if((det==ROAD_ROUND_LEFT||det==ROAD_ROUND_RIGHT)
           &&run_s<delay_s)
        {
            ROUND_ClearEntry();
            return;
        }
        if((det==ROAD_ROUND_LEFT||det==ROAD_ROUND_RIGHT)&&entry_dir!=det)
        {
            ROUND_ClearEntry();
            entry_dir=det;
        }
        Entry_rows=3;
        if(entry_dir==ROAD_ROUND_LEFT) Entry_rows=5;
        if(det==ROAD_ROUND_LEFT||det==ROAD_ROUND_RIGHT)
        {
            for(i=Entry_rows-1;i>0;i--) entry_frames[i]=entry_frames[i-1];
            entry_frames[0]=1;
            hits=0;
            for(i=0;i<Entry_rows;i++) hits+=entry_frames[i];
            if(hits>=1)
            {
                round_dir=det==ROAD_ROUND_LEFT?ROUND_LEFT_DIR:ROUND_RIGHT_DIR;
                ROUND_ClearEntry();
                ROUND_StateEnter(ROUND_FAR);
            }
        }
        else
        {
            for(i=Entry_rows-1;i>0;i--) entry_frames[i]=entry_frames[i-1];
            entry_frames[0]=0;
            hits=0;
            for(i=0;i<Entry_rows;i++) hits+=entry_frames[i];
            if(hits==0) entry_dir=0;
        }
        return;
    }
    active_dir=round_dir;
    active_state=round_state;
    if(active_dir==ROUND_LEFT_DIR) ROUND_LeftProcess(l_lost,r_lost,ms);
    else if(active_dir==ROUND_RIGHT_DIR) ROUND_RightProcess(l_lost,r_lost,ms);
    else
    {
        ROUND_StateEnter(ROUND_NONE);
    }
    if(active_state==ROUND_LEAVE&&round_state==ROUND_NONE)
    {
        if(active_dir==ROUND_LEFT_DIR) round_left_done=1;
        else if(active_dir==ROUND_RIGHT_DIR) round_right_done=1;
    }
}
