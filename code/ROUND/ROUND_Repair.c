#include "ROUND.h"
#include<string.h>

#define ROUND_LEFT_INTO_FAR_USE_MISS 12
#define ROUND_RIGHT_ENTRY_HOLD 20U
#define ROUND_EXIT_FAR_HOLD 2
#define ROUND_LINE_BOTTOM_OFF 78

RoundPoint round_exit_point;
RoundPoint round_right_far_cache;
uint8 round_right_entry_hold;
uint8 round_line_rows[IMG_H];

uint8 round_check_exit(const RoundPoint*corner)
{
    if(!corner->valid||corner->misses!=0) return 0;
    if(round_dir==ROUND_LEFT_DIR) return 1;
    return corner->y>ROUND_EXIT_NEAR_MIN_ROW&&corner->x<IMG_W/2;
}

void ROUND_RepairReset(void)
{
    round_exit_point=(RoundPoint){0};
    round_right_far_cache=(RoundPoint){0};
    round_right_entry_hold=0;
    vision_exit_blend=0;
    memset(round_line_rows,0,sizeof(round_line_rows));
}

void ROUND_RightStartLine(void)
{
    RoundPoint*far_point=&round_points[ROUND_RIGHT][ROUND_FAR_POINT];
    if(far_point->valid&&far_point->misses<=ROUND_RIGHT_INTO_FAR_HOLD)
        round_right_far_cache=*far_point;
    round_right_entry_hold=round_right_far_cache.valid?ROUND_RIGHT_ENTRY_HOLD:0;
}

/* 右环交接线 */
void round_draw_right_entry(void)
{
    int32 x,y;
    int32 bottom_x=IMG_W/2-ROUND_LINE_BOTTOM_OFF;
    int32 bottom_y=IMG_H-2;
    if(!round_right_far_cache.valid||round_right_far_cache.y>=bottom_y) return;
    for(y=1;y<=bottom_y;y++)
    {
        x=round_right_far_cache.x+(bottom_x-round_right_far_cache.x)
            *(y-round_right_far_cache.y)/(bottom_y-round_right_far_cache.y);
        if(x<1||x>IMG_W-2) continue;
        if(x>=r_border[y]-3) continue;
        l_border[y]=(uint8)x;
        l_valid[y]=2;
        round_line_rows[y]=1;
    }
}

void round_right_cache_line(void)
{
    if(!round_right_far_cache.valid) return;
    if(round_right_entry_hold>0)
    {
        round_right_entry_hold--;
        round_draw_right_entry();
    }
    else round_right_far_cache.valid=0;
}

void round_draw_line(uint8 side,int32 x1,int32 y1,
                             int32 x2,int32 y2,uint8 keep_real)
{
    int32 y,x,t;
    uint8*border=side==0?l_border:r_border;
    uint8*valid=side==0?l_valid:r_valid;
    if(y1>y2)
    {
        t=y1;y1=y2;y2=t;
        t=x1;x1=x2;x2=t;
    }
    if(y2==y1) return;
    for(y=y1;y<=y2&&y<IMG_H-1;y++)
    {
        if(y<1) continue;
        if(keep_real&&valid[y]==1) continue;
        x=x1+(x2-x1)*(y-y1)/(y2-y1);
        if(x<1||x>IMG_W-2) break;
        if((side==0&&x>=r_border[y]-3)
           ||(side==1&&x<=l_border[y]+3)) continue;
        border[y]=(uint8)x;
        valid[y]=2;
        round_line_rows[y]=1;
    }
}

/* 入环延长线 */
void round_draw_entry(uint8 side,const RoundPoint*far_point)
{
    int32 bottom_x=side==0?IMG_W/2+ROUND_LINE_BOTTOM_OFF
                         :IMG_W/2-ROUND_LINE_BOTTOM_OFF;
    int32 top_x,y=far_point->y;
    if(y>=IMG_H-2) return;
    top_x=far_point->x-(bottom_x-far_point->x)*y/(IMG_H-2-y);
    if(top_x>=1&&top_x<=IMG_W-2)
        round_draw_line(1-side,top_x,0,bottom_x,IMG_H-2,0);
    else
    {
        int32 edge=top_x<1?1:IMG_W-2;
        int32 start_y=y+(edge-far_point->x)*(IMG_H-2-y)
                            /(bottom_x-far_point->x);
        round_draw_line(1-side,edge,start_y,bottom_x,IMG_H-2,0);
    }
}

/***********************************************
 * @brief : 经过本帧出口近点生成边线并沿同一斜率延伸
 * @param : border_side 待替换的左边线或右边线
 * @param : top_x 图像顶部的补线横坐标
 * @param : corner 本帧有效且未漏检的出口近点
 * @return: 无
 * @date  : 2026-11-08
 * @author: L
 ************************************************/
void round_draw_exit_near(uint8 border_side,int32 top_x,const RoundPoint*corner)
{
    int32 bottom_y=IMG_H-2;
    int32 bottom_x;
    if(!corner->valid||corner->misses||corner->y==0) return;
    bottom_x=(int32)corner->x
        +((int32)corner->x-top_x)
         *(bottom_y-corner->y)/corner->y;
    round_draw_line(border_side,top_x,0,
                    corner->x,corner->y,0);
    round_draw_line(border_side,corner->x,corner->y,
                    bottom_x,bottom_y,0);
    if(border_side==ROUND_LEFT)
    {
        l_border[0]=(uint8)top_x;
        l_valid[0]=2;
    }
    else
    {
        r_border[0]=(uint8)top_x;
        r_valid[0]=2;
    }
    round_line_rows[0]=1;
}

/***********************************************
 * @brief : 按当前圆环状态补线并标记本帧补线行
 * @param : 无
 * @return: 无
 * @date  : 2026-11-08
 * @author: L
 ************************************************/
void ROUND_Repair(void)
{
    uint8 side;
    RoundPoint*near_point,*mid_point,*far_point;
    memset(round_line_rows,0,sizeof(round_line_rows));
    vision_exit_blend=0;
    if(!round_dir||round_state==ROUND_NONE) return;
    side=round_dir==ROUND_RIGHT_DIR?ROUND_RIGHT:ROUND_LEFT;
    near_point=&round_points[side][ROUND_NEAR];
    mid_point=&round_points[side][ROUND_MID];
    far_point=&round_points[side][ROUND_FAR_POINT];
    switch(round_state)
    {
    case ROUND_FAR:
    case ROUND_STRAIGHT:
        if(near_point->valid&&mid_point->valid)
            round_draw_line(side,mid_point->x,mid_point->y,
                             near_point->x,near_point->y,side==ROUND_RIGHT);
        break;
    case ROUND_INTO:
    {
        if(side==ROUND_RIGHT)
        {
            if(far_point->valid&&far_point->misses<=ROUND_RIGHT_INTO_FAR_HOLD)
            {
                round_right_far_cache=*far_point;
                round_draw_right_entry();
            }
        }
        else if(far_point->valid&&far_point->misses<ROUND_LEFT_INTO_FAR_USE_MISS)
            round_draw_entry(side,far_point);
        break;
    }
    case ROUND_IN:
        if(side==ROUND_RIGHT)
            round_draw_exit_near(ROUND_LEFT,ROUND_RIGHT_EXIT_TOP_X,
                                 &round_points[ROUND_LEFT][ROUND_NEAR]);
        else round_draw_exit_near(ROUND_RIGHT,ROUND_LEFT_EXIT_TOP_X,
                                  &round_points[ROUND_RIGHT][ROUND_NEAR]);
        break;
    case ROUND_OUT:
    {
        if(side==ROUND_RIGHT)
        {
            round_right_far_cache.valid=0;
            round_right_entry_hold=0;
            round_draw_line(ROUND_LEFT,ROUND_RIGHT_EXIT_TOP_X,0,
                            ROUND_RIGHT_EXIT_BOTTOM_X,IMG_H-2,0);
            l_border[0]=ROUND_RIGHT_EXIT_TOP_X;
            l_valid[0]=2;
            round_line_rows[0]=1;
        }
        else
        {
            round_draw_line(ROUND_RIGHT,ROUND_LEFT_RND5_TOP_X,0,
                            ROUND_LEFT_EXIT_BOTTOM_X,IMG_H-2,0);
            r_border[0]=ROUND_LEFT_RND5_TOP_X;
            r_valid[0]=2;
            round_line_rows[0]=1;
        }
        break;
    }
    case ROUND_ALIGN:
    case ROUND_LEAVE:
        if(far_point->valid&&far_point->misses<=ROUND_EXIT_FAR_HOLD)
            round_draw_line(side,far_point->x,far_point->y,
                side==0?IMG_W/2-ROUND_LINE_BOTTOM_OFF
                       :IMG_W/2+ROUND_LINE_BOTTOM_OFF,IMG_H-2,0);
        break;
    default:break;
    }
}
