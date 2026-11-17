#include "ROUND.h"

int16 Right_rnd2_min_ms=ROUND_RIGHT_RND2_MIN_DEFAULT_MS; /* 右环状态二最短毫秒 */
int16 Right_rnd3_min_ms=ROUND_RIGHT_RND3_MIN_DEFAULT_MS; /* 右环状态三最短毫秒 */

#ifndef ROUND_LOST_AGAIN
#define ROUND_LOST_AGAIN 8
#endif
#define ROUND_FAR_CONFIRM 3U
#ifndef ELEM_DEBOUNCE
#define ELEM_DEBOUNCE 3
#endif

/***********************************************
 * @brief : 按近点、远点、阶段最短时间及边线恢复条件推进右圆环状态
 * @param : l_lost 左侧丢线行数
 * @param : r_lost 右侧丢线行数
 * @param : ms 当前状态已持续的毫秒数
 * @return: 无
 ************************************************/
void ROUND_RightProcess(uint8 l_lost,uint8 r_lost,uint32 ms)
{
    uint8 ok=0;
    RoundPoint*near_point=&round_points[ROUND_RIGHT][ROUND_NEAR];
    RoundPoint*exit_point=&round_points[ROUND_LEFT][ROUND_NEAR];
    RoundPoint*far_point=&round_points[ROUND_RIGHT][ROUND_FAR_POINT];
    switch(round_state)
    {
    case ROUND_FAR:
        if(!near_point->valid||near_point->misses>=4) ROUND_StateEnter(ROUND_STRAIGHT);
        break;
    case ROUND_STRAIGHT:
        if(ms>=ROUND_TIME_MS(Right_rnd2_min_ms)
           &&far_point->valid&&far_point->misses<=2)
            ROUND_StateEnter(ROUND_INTO);
        break;
    case ROUND_INTO:
        if(far_point->valid&&far_point->misses==0) round_far_seen=1;
        if(ms>=ROUND_TIME_MS(Right_rnd3_min_ms)
           &&round_far_seen&&far_point->misses>=ROUND_RIGHT_INTO_FAR_HOLD)
            ROUND_StateEnter(ROUND_IN);
        break;
    case ROUND_IN:
        ok=round_check_exit(exit_point);
        if(ok) round_ok_frames++;else round_ok_frames=0;
        if(round_ok_frames>=ROUND_RIGHT_EXIT_CONFIRM)
            ROUND_StateEnter(ROUND_OUT);
        break;
    case ROUND_OUT:
        ok=far_point->valid&&far_point->misses==0;
        if(ok) round_ok_frames++;else round_ok_frames=0;
        if(round_ok_frames>=ROUND_FAR_CONFIRM)
            ROUND_StateEnter(ROUND_ALIGN);
        break;
    case ROUND_ALIGN:
        ok=l_lost<=ROUND_LOST_AGAIN&&r_lost<=ROUND_LOST_AGAIN;
        if(ok) round_ok_frames++;else round_ok_frames=0;
        if(round_ok_frames>=ELEM_DEBOUNCE) ROUND_StateEnter(ROUND_LEAVE);
        break;
    case ROUND_LEAVE:
        ok=l_lost<=ROUND_LOST_AGAIN&&r_lost<=ROUND_LOST_AGAIN;
        if(ok) round_ok_frames++;else round_ok_frames=0;
        if(round_ok_frames>=2)
            ROUND_StateEnter(ROUND_NONE);
        break;
    default:ROUND_StateEnter(ROUND_NONE);break;
    }
}
