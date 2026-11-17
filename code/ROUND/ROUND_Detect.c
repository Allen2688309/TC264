#include "ROUND.h"
#include<math.h>
#include<string.h>

#ifndef POINT_CAP
#define POINT_CAP ROUND_POINT_CAP
#endif
#ifndef MID_PROMINENCE
#define MID_PROMINENCE 6
#endif
#ifndef ELEM_BOT_OK
#define ELEM_BOT_OK 6
#endif
#define ROUND_POINT_HOLD 8
#define ROUND_POINT_GATE 36
#define ROUND_MID_NEAR_GAP 6
#define ROUND_RIGHT_MID_GAP 6
#define ROUND_LEFT_MID_PROMINENCE 4
#define ROUND_LOST_MIN 30
#define ROUND_LOST_DIFF_MIN 20


RoundPoint round_points[2][3];
uint8 L_corner_flag,L_corner_row,L_corner_col;
uint8 R_corner_flag,R_corner_row,R_corner_col;
uint8 L_mid_flag,L_mid_row,L_mid_col;
uint8 R_mid_flag,R_mid_row,R_mid_col;
uint8 L_far_flag,L_far_row,L_far_col;
uint8 R_far_flag,R_far_row,R_far_col;

int32 round_abs(int32 x)
{
    return x<0?-x:x;
}

/***********************************************
 * @brief : 确认并跟踪特征点，按点位所在阶段保留短暂漏检的有效点
 * @param : p 待更新的特征点
 * @param : found 本次是否找到符合形状要求的候选点
 * @param : x 候选点横坐标
 * @param : y 候选点纵坐标
 * @return: 无
 ************************************************/
void round_update_point(RoundPoint*p,uint8 found,int32 x,int32 y)
{
    uint8 hold=ROUND_POINT_HOLD;
    if(round_dir==ROUND_LEFT_DIR&&round_state==ROUND_INTO
       &&p==&round_points[ROUND_LEFT][ROUND_FAR_POINT]) hold=ROUND_LEFT_INTO_FAR_HOLD;
    if(round_dir==ROUND_RIGHT_DIR&&round_state==ROUND_INTO
       &&p==&round_points[ROUND_RIGHT][ROUND_FAR_POINT])
        hold=ROUND_RIGHT_INTO_FAR_HOLD;
    else if(round_dir==ROUND_RIGHT_DIR&&round_state==ROUND_OUT
            &&p==&round_points[ROUND_RIGHT][ROUND_FAR_POINT])
        hold=ROUND_RIGHT_FAR_HOLD;
    if(found&&p->valid&&round_abs(x-p->x)<=ROUND_POINT_GATE
       &&round_abs(y-p->y)<=ROUND_POINT_GATE)
    {
        p->x=(uint8)((p->x+2*x+1)/3);
        p->y=(uint8)((p->y+2*y+1)/3);
        p->misses=0;
        p->hits=0;
        return;
    }
    if(found)
    {
        if(p->hits&&round_abs(x-p->px)<=ROUND_POINT_GATE
           &&round_abs(y-p->py)<=ROUND_POINT_GATE) p->hits++;
        else p->hits=1;
        p->px=(uint8)x;
        p->py=(uint8)y;
        if(p->hits>=2)
        {
            p->valid=1;
            p->x=(uint8)x;
            p->y=(uint8)y;
            p->hits=0;
            p->misses=0;
            return;
        }
    }
    else p->hits=0;
    if(p->valid&&++p->misses>hold) p->valid=0;
}

int32 round_avg_x(const uint16 pts[][2],int32 i)
{
    return ((int32)pts[i-4][0]+pts[i-2][0]+pts[i][0]
            +pts[i+2][0]+pts[i+4][0])/5;
}

void round_find_mid(uint8 side,const uint16 pts[][2],uint16 count)
{
    static uint16 smooth_x[POINT_CAP];
    RoundPoint*mid_point=&round_points[side][ROUND_MID];
    int32 i,sign=side==0?1:-1;
    int32 best=-9999,mx=0,my=0,mi=-1;
    int32 min_dx=(round_dir==ROUND_LEFT_DIR&&side==ROUND_LEFT)
                   ?ROUND_LEFT_MID_PROMINENCE:MID_PROMINENCE;
    if(count>32)
        for(i=4;i+4<count;i++) smooth_x[i]=(uint16)round_avg_x(pts,i);
    for(i=16;i+16<count;i++)
    {
        int32 x=smooth_x[i],y=pts[i][1];
        int32 left=sign*(x-smooth_x[i-12]);
        int32 right=sign*(x-smooth_x[i+12]);
        if(x<8||x>IMG_W-9||y<12||y>IMG_H-12) continue;
        if((left<min_dx||right<min_dx)
           &&i>=28&&i+28<count)
        {
            left=sign*(x-smooth_x[i-24]);
            right=sign*(x-smooth_x[i+24]);
        }
        if(left>=min_dx&&right>=min_dx&&sign*x>best)
        { best=sign*x;mx=x;my=y;mi=i;}
    }
    round_update_point(mid_point,mi>=0,mx,my);
}

/* 近点和出口点 */
void round_find_turn(uint8 side,const uint16 pts[][2],uint16 count,
                     uint8 near_on,uint8 exit_far)
{
    RoundPoint*near_point=&round_points[side][ROUND_NEAR];
    RoundPoint*far_point=&round_points[side][ROUND_FAR_POINT];
    int32 i,sign=side==0?1:-1;
    int32 nx=0,ny=0,fx=0,fy=0;
    float near_best=-2.0f,far_best=-2.0f;
    for(i=6;i+6<count;i++)
    {
        int32 x=pts[i][0],y=pts[i][1];
        float ax=(float)((int32)pts[i-6][0]-x);
        float ay=(float)((int32)pts[i-6][1]-y);
        float bx=(float)((int32)pts[i+6][0]-x);
        float by=(float)((int32)pts[i+6][1]-y);
        float aa=ax*ax+ay*ay,bb=bx*bx+by*by;
        float score;
        if(x<6||x>IMG_W-7||y<8||y>IMG_H-8||aa<16||bb<16) continue;
        score=(ax*bx+ay*by)/sqrtf(aa*bb);
        if(near_on&&score>=-0.574f&&score<=0.94f
           &&!(round_dir==ROUND_RIGHT_DIR&&round_state>=ROUND_IN
                &&side==ROUND_LEFT&&x>=IMG_W/2))
        {
            uint8 shape_ok=round_state>=ROUND_IN
                ||(ay>=3&&sign*ax<=-3&&sign*bx<=-3&&fabsf(by)<=fabsf(bx)*0.5f+1);
            uint8 better=score>near_best;
            if(round_state<=ROUND_OUT)
                better=near_best<-1.0f||y>ny||(y==ny&&score>near_best);
            if(shape_ok&&better) {near_best=score;nx=x;ny=y;}
        }
        if(!exit_far||score<-0.9063078f||score>0.8660254f) continue;
        if(round_dir==side+1)
        {
            /* 左右镜像 */
            if(sign*ax>-3||by>-3||sign*bx<-2
               ||fabsf(ay)>fabsf(ax)+2||sign*(ax*by-ay*bx)<=0) continue;
            if(far_best>=-1.0f&&(y<fy||(y==fy&&score<=far_best))) continue;
            far_best=score;fx=x;fy=y;
        }
        else if(score>far_best) {far_best=score;fx=x;fy=y;}
    }
    if(near_on) round_update_point(near_point,near_best>-2.0f,nx,ny);
    else*near_point=(RoundPoint){0};
    if(exit_far) round_update_point(far_point,far_best>-2.0f,fx,fy);
}

/***********************************************
 * @brief : 按轮廓转折找入环远点，右环状态二和三均要求中心点有效且远点至少高六行
 * @param : side 左侧或右侧轮廓编号
 * @param : pts 当前轮廓点数组
 * @param : count 当前轮廓点数量
 * @return: 无
 ************************************************/
void round_find_far(uint8 side,const uint16 pts[][2],uint16 count)
{
    static const int32 steps[]={6,12,20};
    RoundPoint*mid_point=&round_points[side][ROUND_MID];
    RoundPoint*far_point=&round_points[side][ROUND_FAR_POINT];
    int32 i,k,sign=side==0?1:-1;
    int32 fx=0,fy=0;
    float best=-2.0f;
    int32 track_x=0,track_y=0;
    float track_best=-2.0f;
    uint8 track_found=0;
    for(k=0;k<3;k++)
    for(i=steps[k];i+steps[k]<count;i++)
    {
        int32 step=steps[k],x=pts[i][0],y=pts[i][1];
        float ax=(float)((int32)pts[i-step][0]-x);
        float ay=(float)((int32)pts[i-step][1]-y);
        float bx=(float)((int32)pts[i+step][0]-x);
        float by=(float)((int32)pts[i+step][1]-y);
        float aa=ax*ax+ay*ay,bb=bx*bx+by*by;
        float dot=ax*bx+ay*by,score;
        if(x<6||x>IMG_W-7||y<8||y>IMG_H-8||!mid_point->valid
           ||(int32)mid_point->y-y<(side==ROUND_RIGHT?ROUND_RIGHT_MID_GAP:ROUND_MID_NEAR_GAP)
           ||aa<25||bb<25) continue;
        if(sign*ax>-4||by>-4||sign*bx<-2
           ||fabsf(ay)>fabsf(ax)*0.8f+2||sign*(ax*by-ay*bx)<=0) continue;
        if(dot<0&&dot*dot>0.883022f*aa*bb) continue;
        score=dot/sqrtf(aa*bb);
        if(score>0.8f) continue;
        if(side==ROUND_RIGHT&&far_point->valid
           &&round_abs(x-far_point->x)<=ROUND_POINT_GATE
           &&round_abs(y-far_point->y)<=ROUND_POINT_GATE)
        {
            if(score>track_best)
            {track_best=score;track_x=x;track_y=y;track_found=1;}
            continue;
        }
        if(score>best) {best=score;fx=x;fy=y;}
    }
    if(side==ROUND_RIGHT&&far_point->valid&&track_found)
    {best=track_best;fx=track_x;fy=track_y;}
    round_update_point(far_point,best>-2.0f,fx,fy);
}

/* 按阶段找点 */
void round_find_points(uint8 side,const uint16 pts[][2],uint16 count,
                       uint8 near_on,uint8 far_on)
{
    RoundPoint*near_point=&round_points[side][ROUND_NEAR];
    RoundPoint*mid_point=&round_points[side][ROUND_MID];
    RoundPoint*far_point=&round_points[side][ROUND_FAR_POINT];
    uint8 exit_far=far_on&&round_state>=ROUND_OUT&&round_state<=ROUND_LEAVE;
    if(round_state>=ROUND_IN) *mid_point=(RoundPoint){0};
    if(near_on||exit_far) round_find_turn(side,pts,count,near_on,exit_far);
    else*near_point=(RoundPoint){0};
    if(round_state<ROUND_IN)
    {
        if(round_state==ROUND_FAR&&!near_point->valid)
            *mid_point=(RoundPoint){0};
        else round_find_mid(side,pts,count);
    }
    if(!far_on) *far_point=(RoundPoint){0};
    else if(round_state==ROUND_STRAIGHT||round_state==ROUND_INTO)
        round_find_far(side,pts,count);
    else if(!exit_far) round_update_point(far_point,0,0,0);
}

void ROUND_GetPoints(void)
{
    uint8 side;
    for(side=0;side<2;side++)
    {
        uint8 own=round_dir==side+1;
        uint8 near_on,far_on;
        if(round_state>=ROUND_IN)
        {
            near_on=!own&&(round_state==ROUND_IN||round_state==ROUND_OUT);
            far_on=own&&round_state>=ROUND_OUT;
        }
        else
        {
            if(round_dir&&!own)
            {
                memset(round_points[side],0,sizeof(round_points[side]));
                continue;
            }
            near_on=round_state!=ROUND_INTO;
            far_on=round_state==ROUND_STRAIGHT||round_state==ROUND_INTO;
        }
        round_find_points(side,side==ROUND_LEFT?points_l:points_r,
                          side==ROUND_LEFT?count_l:count_r,near_on,far_on);
    }
    L_corner_flag=round_points[ROUND_LEFT][ROUND_NEAR].valid;
    L_corner_col=round_points[ROUND_LEFT][ROUND_NEAR].x;
    L_corner_row=round_points[ROUND_LEFT][ROUND_NEAR].y;
    L_mid_flag=round_points[ROUND_LEFT][ROUND_MID].valid;
    L_mid_col=round_points[ROUND_LEFT][ROUND_MID].x;
    L_mid_row=round_points[ROUND_LEFT][ROUND_MID].y;
    L_far_flag=round_points[ROUND_LEFT][ROUND_FAR_POINT].valid;
    L_far_col=round_points[ROUND_LEFT][ROUND_FAR_POINT].x;
    L_far_row=round_points[ROUND_LEFT][ROUND_FAR_POINT].y;
    R_corner_flag=round_points[ROUND_RIGHT][ROUND_NEAR].valid;
    R_corner_col=round_points[ROUND_RIGHT][ROUND_NEAR].x;
    R_corner_row=round_points[ROUND_RIGHT][ROUND_NEAR].y;
    R_mid_flag=round_points[ROUND_RIGHT][ROUND_MID].valid;
    R_mid_col=round_points[ROUND_RIGHT][ROUND_MID].x;
    R_mid_row=round_points[ROUND_RIGHT][ROUND_MID].y;
    R_far_flag=round_points[ROUND_RIGHT][ROUND_FAR_POINT].valid;
    R_far_col=round_points[ROUND_RIGHT][ROUND_FAR_POINT].x;
    R_far_row=round_points[ROUND_RIGHT][ROUND_FAR_POINT].y;
}

uint8 round_check_open(uint8 side)
{
    RoundPoint*near_point=&round_points[side][ROUND_NEAR];
    RoundPoint*mid_point=&round_points[side][ROUND_MID];
    const uint16 (*pts)[2]=side==ROUND_LEFT?points_l:points_r;
    int32 count=side==0?count_l:count_r;
    static const int32 left_spans[]={6,10,14};
    static const int32 right_spans[]={12,20,28};
    const int32*spans=side==0?left_spans:right_spans;
    int32 min_from_mid=side==0?4:5;
    int32 sign=side==0?1:-1,i,k;
    if(!near_point->valid||!mid_point->valid
       ||mid_point->misses>(side==ROUND_RIGHT?4:3)) return 0;
    if(sign*((int32)mid_point->x-near_point->x)<5) return 0;
    for(k=0;k<3;k++)
    for(i=0;i+spans[k]<count;i++)
    {
        int32 j=i+spans[k],dx,dy;
        dx=sign*((int32)pts[j][0]-pts[i][0]);
        dy=(int32)pts[j][1]-pts[i][1];
        if(sign*((int32)mid_point->x-pts[i][0])>=min_from_mid&&dx>0
           &&3*round_abs(dy)<=2*dx) return 1;
    }
    return 0;
}

uint8 count_valid(const uint8*valid,uint8 y0,uint8 y1)
{
    uint8 count=0;
    int32 y;
    for(y=y0;y<y1;y++) if(valid[y]==1) count++;
    return count;
}

/* 右岛形状 */
uint8 round_right_shape(void)
{
    int32 x,y,left,right,mid,top,bot,k;
    for(y=8;y<IMG_H-32;y+=2)
    for(x=IMG_W-4;x>=IMG_W/2-16;x--)
    {
        if(bin_image[y][x]!=IMG_BLACK) continue;
        right=x;
        while(x>=IMG_W/2-16&&bin_image[y][x]==IMG_BLACK) x--;
        left=x+1;
        if(x<IMG_W/2-16||right-left+1<12||right<=IMG_W/2) continue;
        mid=(left+right)/2;
        top=bot=y;
        while(top>3&&y-top<32&&bin_image[top-1][mid]==IMG_BLACK) top--;
        while(bot<IMG_H-4&&bot-y<32&&bin_image[bot+1][mid]==IMG_BLACK) bot++;
        if(bot-top+1<3||bot-top+1>32) continue;
        for(k=left+(right-left)/4;k<=right-(right-left)/4;k++)
        {
            if(bin_image[top-1][k]!=IMG_WHITE||bin_image[top-2][k]!=IMG_WHITE
               ||bin_image[bot+1][k]!=IMG_WHITE||bin_image[bot+2][k]!=IMG_WHITE)
                break;
        }
        if(k>right-(right-left)/4) return 1;
    }
    return 0;
}

/***********************************************
 * @brief : 根据两侧丢线差、特征点和轮廓形状识别圆环入口
 * @param : l_lost 左侧丢线行数
 * @param : r_lost 右侧丢线行数
 * @return: 左圆环、右圆环或普通赛道的类型编号
 ************************************************/
uint8 ROUND_Detect(uint8 l_lost,uint8 r_lost)
{
    RoundPoint*left_near=&round_points[ROUND_LEFT][ROUND_NEAR];
    RoundPoint*left_mid=&round_points[ROUND_LEFT][ROUND_MID];
    RoundPoint*right_near=&round_points[ROUND_RIGHT][ROUND_NEAR];
    RoundPoint*right_mid=&round_points[ROUND_RIGHT][ROUND_MID];
    uint8 l_point=left_near->valid&&left_mid->valid;
    uint8 r_near=R_corner_flag&&R_mid_flag;
    uint8 r_point=right_near->valid&&right_mid->valid;
    uint8 l_gap=l_lost>=ROUND_LOST_MIN
              &&l_lost>=r_lost+ROUND_LOST_DIFF_MIN;
    uint8 r_gap=r_lost>=ROUND_LOST_MIN
              &&r_lost>=l_lost+ROUND_LOST_DIFF_MIN;
    uint8 right_shape_gap=r_lost>=ROUND_LOST_DIFF_MIN
                       &&r_lost>=l_lost+ROUND_LOST_DIFF_MIN;
    if(l_gap&&l_point
       &&(int32)left_near->y-(int32)left_mid->y>=6)
        return ROAD_ROUND_LEFT;
    if(l_gap&&round_check_open(ROUND_LEFT)) return ROAD_ROUND_LEFT;
    if(r_gap&&r_point
       &&(int32)right_near->y-(int32)right_mid->y>=18)
        return ROAD_ROUND_RIGHT;
    if(right_shape_gap&&round_check_open(ROUND_RIGHT)) return ROAD_ROUND_RIGHT;
    if(round_state==ROUND_NONE&&round_right_shape()) return ROAD_ROUND_RIGHT;
    if(!l_gap&&!r_gap&&r_near&&round_check_open(ROUND_RIGHT))
        return ROAD_ROUND_RIGHT;
    return ROAD_NORMAL;
}
