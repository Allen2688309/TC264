#include "ROUND.h"

#ifndef ROUND_LOST_AGAIN
#define ROUND_LOST_AGAIN 8
#endif
#define ROUND_FAR_CONFIRM 4U
#ifndef ELEM_DEBOUNCE
#define ELEM_DEBOUNCE 3
#endif

void ROUND_LeftProcess(uint8 l_lost,uint8 r_lost,uint32 ms)
{
    uint8 ok=0;
    RoundPoint*near_point=&round_points[ROUND_LEFT][ROUND_NEAR];
    RoundPoint*exit_point=&round_points[ROUND_RIGHT][ROUND_NEAR];
    RoundPoint*far_point=&round_points[ROUND_LEFT][ROUND_FAR_POINT];
    switch(round_state)
    {
    case ROUND_FAR:
        if(!near_point->valid||near_point->misses>=4) ROUND_StateEnter(ROUND_STRAIGHT);
        break;
    case ROUND_STRAIGHT:
        if(far_point->valid&&far_point->misses<=4) ROUND_StateEnter(ROUND_INTO);
        break;
    case ROUND_INTO:
        if(far_point->valid&&far_point->misses==0) round_far_seen=1;
        if(ms>=ROUND_TIME_MS(ROUND_LEFT_INTO_MAX_MS)
           ||(round_far_seen&&far_point->misses>=ROUND_LEFT_INTO_FAR_MISS
              &&ms>=ROUND_TIME_MS(ROUND_LEFT_INTO_MIN_MS)))
            ROUND_StateEnter(ROUND_IN);
        break;
    case ROUND_IN:
        ok=round_check_exit(exit_point);
        if(!ok) round_ok_frames=0;
        else if(round_ok_frames<ROUND_EXIT_CONFIRM) round_ok_frames++;
        if((round_ok_frames>=ROUND_EXIT_CONFIRM
            &&ms>=ROUND_TIME_MS(ROUND_LEFT_RND4_MIN_MS))
           ||ms>=ROUND_TIME_MS(ROUND_LEFT_RND4_MAX_MS))
            ROUND_StateEnter(ROUND_OUT);
        break;
    case ROUND_OUT:
        ok=far_point->valid&&far_point->misses==0;
        if(ok) round_ok_frames++;
        else round_ok_frames=0;
        if(round_ok_frames>=ROUND_FAR_CONFIRM
           ||ms>=ROUND_TIME_MS(ROUND_LEFT_RND5_MAX_MS))
            ROUND_StateEnter(ROUND_ALIGN);
        break;
    case ROUND_ALIGN:
        ok=l_lost<=ROUND_LOST_AGAIN&&r_lost<=ROUND_LOST_AGAIN;
        if(ok) round_ok_frames++;else round_ok_frames=0;
        if(round_ok_frames>=ELEM_DEBOUNCE||ms>=ROUND_TIME_MS(ROUND_LEFT_ALIGN_MS))
            ROUND_StateEnter(ROUND_LEAVE);
        break;
    case ROUND_LEAVE:
        ok=l_lost<=ROUND_LOST_AGAIN&&r_lost<=ROUND_LOST_AGAIN;
        if(ok) round_ok_frames++;else round_ok_frames=0;
        if(round_ok_frames>=ELEM_DEBOUNCE
           ||ms>=ROUND_TIME_MS(ROUND_LEFT_LEAVE_MS))
            ROUND_StateEnter(ROUND_NONE);
        break;
    default:ROUND_StateEnter(ROUND_NONE);break;
    }
}
