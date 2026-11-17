#include "Camera.h"
#include "../ROUND/ROUND.h"
#include "../car/car.h"
#include "MyEncoder/MyEncoder.h"
#include<math.h>
#include<string.h>

/* 参数 */
int16 ERR_TOP=48;/* 前瞻上行 */
int16 ERR_BOTTOM=72;/* 前瞻下行 */

/* 十字 */
int16 CROSS_LOST_MIN=25;/* 十字左右各自的丢线行数下限 */

int16 ROUND_LOST_MIN=18;/* 单侧丢线下限 */

/* 边线 */
#define BORDER_MIN (1U)
#define BORDER_MAX (IMG_W-2U)

#define CROSS_FIT_ROWS (12) /* 拟合行数 */
#define CROSS_JUMP_MIN (14)
#define CROSS_SLOPE_MAX (3.0f) /* 最大斜率 */

#define CROSS_HOLD_FRAMES (6)

/* 元素区 */
#define ELEM_TOP (8)
#define ELEM_BOT (IMG_H-32)
#define CAM_WARMUP_FRAMES (5U) /* 开机丢帧 */

#define ROUND_LOST_DIFF_MIN (10)

/* 容量 */
#define POINT_CAP (IMG_H*3U)

/* 绝对值 */
int32 my_abs(int32 x)
{
    return (x>=0)?x:-x;
}

float my_fabs(float x)
{
    return(x>=0.0f)?x:-x;
}

uint8 camera_limit_x(int32 v)
{
    if(v<(int32)BORDER_MIN)v=(int32)BORDER_MIN;
    if(v>(int32)BORDER_MAX)v=(int32)BORDER_MAX;
    return (uint8)v;
}

/* 直方图 */
int32 hist[256];

/* 大津法 */
uint8 camera_get_threshold(void)
{
    int32 x,y;
    int32 amount=0;/* 像素数 */
    int32 pixel_back,pixel_fore;/* 点数 */
    int32 integral=0,integral_back=0;/* 灰度和 */
    int32 integral_fore;
    float omega_back,omega_fore,mean_back,mean_fore,sigma,sigma_max;
    uint8 threshold=0;
    uint8 min_v=0,max_v=255;
    for(x=0;x<256;x++)hist[x]=0;
    for(y=0;y<IMG_H;y+=2)
    {
        for(x=0;x<IMG_W;x+=2)
        {
            hist[mt9v03x_image[y][x]]++;
        }
    }
    for(min_v=0;min_v<255&&hist[min_v]==0;min_v++);
    for(max_v=255;max_v>min_v&&hist[max_v]==0;max_v--);
    if(max_v==min_v) return max_v;/* 单色图 */
    if(min_v+1==max_v) return min_v;/* 二值图 */
    for(y=min_v;y<=max_v;y++)
    {
        amount+=hist[y];
        integral+=hist[y]*(int32)y;
    }
    if(amount<=0) return 100;
    pixel_back=0;
    integral_back=0;
    sigma_max=-1.0f;
    for(y=min_v;y<max_v;y++)
    {
        pixel_back+=hist[y];
        integral_back+=hist[y]*(int32)y;
        pixel_fore=amount-pixel_back;
        if(pixel_back==0||pixel_fore==0) continue;
        omega_back=(float)pixel_back/(float)amount;/* 暗占比 */
        omega_fore=(float)pixel_fore/(float)amount;/* 亮占比 */
        integral_fore=integral-integral_back;
        mean_back=(float)integral_back/(float)pixel_back;/* 暗均值 */
        mean_fore=(float)integral_fore/(float)pixel_fore;/* 亮均值 */
        sigma=omega_back*omega_fore*(mean_back-mean_fore)*(mean_back-mean_fore);
        if(sigma>sigma_max)
        {
            sigma_max=sigma;
            threshold=(uint8)y;
        }
    }
    return threshold;
}

/* 二值化 */
uint8 bin_image[IMG_H][IMG_W];

void turn_to_bin(uint8 threshold)
{
    int32 x,y;
    for(y=0;y<IMG_H;y++)
    {
        for(x=0;x<IMG_W;x++)
        {
            if(mt9v03x_image[y][x]>threshold) bin_image[y][x]=IMG_WHITE;
            else bin_image[y][x]=IMG_BLACK;
        }
    }
}

void image_draw_rect(void)
{
    int32 x,y;
    for(y=0;y<IMG_H;y++)
    {
        bin_image[y][0]=IMG_BLACK;
        bin_image[y][1]=IMG_BLACK;
        bin_image[y][IMG_W-1]=IMG_BLACK;
        bin_image[y][IMG_W-2]=IMG_BLACK;
    }
    for(x=0;x<IMG_W;x++)
    {
        bin_image[0][x]=IMG_BLACK;
        bin_image[1][x]=IMG_BLACK;
    }
}

uint8 start_l[2];/* 左起点 */
uint8 start_r[2];/* 右起点 */

/***********************************************
 * @brief : 从底部向上寻找边线起点，允许合格单侧起点继续爬线
 * @param : row 起点搜索的最下行
 * @return: 找到合格双侧或单侧起点返回一，否则返回零；缺失侧坐标清零
 ************************************************/
uint8 get_start_point(uint8 row)
{
    int32 i,y,left_x=-1,left_y=-1,right_x=-1,right_y=-1;
    start_l[0]=start_l[1]=0;
    start_r[0]=start_r[1]=0;
    for(y=row;y>=80;y--)
    {
        uint8 l_found=0,r_found=0;
        for(i=IMG_W/2;i>(int32)BORDER_MIN;i--)
        {
            if(bin_image[y][i]==IMG_WHITE&&bin_image[y][i-1]==IMG_BLACK)
            {
                start_l[0]=(uint8)i;
                start_l[1]=y;
                l_found=1;
                break;
            }
        }
        for(i=IMG_W/2;i<(int32)BORDER_MAX;i++)
        {
            if(bin_image[y][i]==IMG_WHITE&&bin_image[y][i+1]==IMG_BLACK)
            {
                start_r[0]=(uint8)i;
                start_r[1]=y;
                r_found=1;
                break;
            }
        }
        if(l_found&&r_found)
        {
            int32 xx,black_in=0,tot_in=0;
            if(start_l[0]<=BORDER_MIN+1&&start_r[0]>=BORDER_MAX-1) continue;
            for(xx=(int32)start_l[0];xx<=(int32)start_r[0];xx++)
            {
                tot_in++;
                if(bin_image[y][xx]==IMG_BLACK) black_in++;
            }
            if(tot_in>0&&START_IN_BLACK_MAX>0
               &&(black_in*100/tot_in)>START_IN_BLACK_MAX) continue;
            return 1;
        }
        if(l_found&&start_l[0]>BORDER_MIN+1&&left_y<0)
        {
            int32 black_in=0,tot_in=0,xx;
            for(xx=(int32)start_l[0];xx<=IMG_W/2;xx++)
            {tot_in++;if(bin_image[y][xx]==IMG_BLACK) black_in++;}
            if(tot_in>0&&(START_IN_BLACK_MAX<=0
               ||black_in*100/tot_in<=START_IN_BLACK_MAX))
            {left_x=start_l[0];left_y=y;}
        }
        if(r_found&&start_r[0]<BORDER_MAX-1&&right_y<0)
        {
            int32 black_in=0,tot_in=0,xx;
            for(xx=IMG_W/2;xx<=(int32)start_r[0];xx++)
            {tot_in++;if(bin_image[y][xx]==IMG_BLACK) black_in++;}
            if(tot_in>0&&(START_IN_BLACK_MAX<=0
               ||black_in*100/tot_in<=START_IN_BLACK_MAX))
            {right_x=start_r[0];right_y=y;}
        }
    }
    if(left_y>=0) {start_l[0]=(uint8)left_x;start_l[1]=(uint8)left_y;}
    else start_l[0]=start_l[1]=0;
    if(right_y>=0) {start_r[0]=(uint8)right_x;start_r[1]=(uint8)right_y;}
    else start_r[0]=start_r[1]=0;
    /* 单侧候选已通过原有筛选；两侧都有候选时仍检查配对关系。 */
    if(left_y<0) return right_y>=0;
    if(right_y<0) return 1;
    if(right_x-left_x>20&&right_x-left_x<IMG_W-8
       &&my_abs(left_y-right_y)<=24)
    {
        return 1;
    }
    return 0;
}

/* 八邻域 */
uint16 points_l[POINT_CAP][2];
uint16 points_r[POINT_CAP][2];
uint16 count_l;
uint16 count_r;

/* 左向偏移 */
const int8 seeds_l[8][2]=
{
    { 0,1},{-1,1},{-1,0},{-1,-1},
    { 0,-1},{ 1,-1},{ 1,0},{ 1,1}
};

/* 右向偏移 */
const int8 seeds_r[8][2]=
{
    { 0,1},{ 1,1},{ 1,0},{ 1,-1},
    { 0,-1},{-1,-1},{-1,0},{-1,1}
};

/* 坐标 */
#define IN_IMAGE(x,y) ((x)>=0&&(x)<IMG_W&&(y)>=0&&(y)<IMG_H)

/* 下一步 */
uint8 camera_next_point(uint8 side,const uint16 points[][2],uint16 count,int16 next[2])
{
    const int8(*seeds)[2]=side==0?seeds_l:seeds_r;
    int32 i,x=points[count-1][0],y=points[count-1][1];
    int32 dx=0,dy=-1,best=-100,score;
    uint8 found=0;
    if(count>=2)
    {
        dx=x-points[count-2][0];
        dy=y-points[count-2][1];
    }
    for(i=0;i<8;i++)
    {
        int32 nx=x+seeds[i][0],ny=y+seeds[i][1];
        int32 wx=x+seeds[(i+1)&7][0],wy=y+seeds[(i+1)&7][1];
        if(!IN_IMAGE(nx,ny)||!IN_IMAGE(wx,wy)) continue;
        if(count>=2&&nx==points[count-2][0]&&ny==points[count-2][1]) continue;
        if(bin_image[ny][nx]!=IMG_BLACK||bin_image[wy][wx]!=IMG_WHITE) continue;
        score=4*((nx-x)*dx+(ny-y)*dy)-(ny-y);
        if(score<=best) continue;
        best=score;next[0]=(int16)nx;next[1]=(int16)ny;found=1;
    }
    return found;
}

/***********************************************
 * @brief : 从各自合格起点爬取左右轮廓，缺失侧不启动搜索
 * @param : Steps 最多执行的爬线步数
 * @param : Highest 左右轮廓相遇时输出的行号
 * @return: 无
 ************************************************/
void camera_find_border(uint16 Steps,uint8*Highest)
{
    int16 Next_l[2],Next_r[2];
    uint8 Left_stop=start_l[1]==0;
    uint8 Right_stop=start_r[1]==0;

    /* 缺失侧保持零个轮廓点，后续取线不会读到上帧轮廓或虚假的零坐标。 */
    count_l=count_r=0;
    if(!Left_stop)
    {
        points_l[0][0]=start_l[0];
        points_l[0][1]=start_l[1];
        count_l=1;
    }
    if(!Right_stop)
    {
        points_r[0][0]=start_r[0];
        points_r[0][1]=start_r[1];
        count_r=1;
    }
    while(Steps--)
    {
        if(!Left_stop)
        {
            if(count_l>=POINT_CAP||!camera_next_point(0,points_l,count_l,Next_l)) Left_stop=1;
            else
            {
                points_l[count_l][0]=(uint16)Next_l[0];
                points_l[count_l][1]=(uint16)Next_l[1];
                count_l++;
            }
        }
        if(!Right_stop)
        {
            if(count_r>=POINT_CAP||!camera_next_point(1,points_r,count_r,Next_r)) Right_stop=1;
            else
            {
                points_r[count_r][0]=(uint16)Next_r[0];
                points_r[count_r][1]=(uint16)Next_r[1];
                count_r++;
            }
        }
        if(Left_stop&&Right_stop) break;
        if(!Left_stop&&!Right_stop&&count_l>=3&&count_r>=3
           &&my_abs((int32)points_l[count_l-1][0]-points_r[count_r-1][0])<2
           &&my_abs((int32)points_l[count_l-1][1]-points_r[count_r-1][1])<2)
        {
            *Highest=(uint8)((points_l[count_l-1][1]+points_r[count_r-1][1])/2);
            break;
        }
    }
}

/* 按行取线 */
uint8 l_border[IMG_H];
uint8 r_border[IMG_H];
uint8 center_line[IMG_H];
uint8 l_valid[IMG_H];
uint8 r_valid[IMG_H];

/* 记录本帧轮廓是否接近对应画框，不参与中线有效性判断。 */
uint8 l_edge[IMG_H],r_edge[IMG_H];

/***********************************************
 * @brief : 提取左侧实测边线，并按本帧轮廓横坐标记录到达左画框的行
 * @param : 无
 * @return: 无
 ************************************************/
void get_left(void)
{
    int32 x,y,k;
    uint16 j;
    for(y=0;y<IMG_H;y++)
    {
        l_border[y]=BORDER_MIN;
        l_valid[y]=0;
        l_edge[y]=0;
    }
    for(j=0;j<count_l;j++)
    {
        y=points_l[j][1];x=points_l[j][0];
        if(y<2||y>start_l[1]) continue;
        if(x<4) l_edge[y]=1;
        if(l_valid[y]) continue;
        /* 只查邻点 */
        for(k=x-2;k<=x+2;k++)
        {
            if(k<=2||k>IMG_W-3) continue;
            if(bin_image[y][k]!=IMG_WHITE||bin_image[y][k-1]!=IMG_BLACK) continue;
            l_border[y]=(uint8)k;l_valid[y]=1;
            break;
        }
    }
}

/***********************************************
 * @brief : 提取右侧实测边线，并按本帧轮廓横坐标记录到达右画框的行
 * @param : 无
 * @return: 无
 ************************************************/
void get_right(void)
{
    int32 x,y,k;
    uint16 j;
    for(y=0;y<IMG_H;y++)
    {
        r_border[y]=BORDER_MAX;
        r_valid[y]=0;
        r_edge[y]=0;
    }
    for(j=0;j<count_r;j++)
    {
        y=points_r[j][1];x=points_r[j][0];
        if(y<2||y>start_r[1]) continue;
        if(x>=IMG_W-4) r_edge[y]=1;
        if(r_valid[y]) continue;
        for(k=x+2;k>=x-2;k--)
        {
            if(k<2||k>=IMG_W-3) continue;
            if(bin_image[y][k]!=IMG_WHITE||bin_image[y][k+1]!=IMG_BLACK) continue;
            r_border[y]=(uint8)k;r_valid[y]=1;
            break;
        }
    }
}

/* 短缺口插补 */
void border_fill(uint8*border,uint8*valid)
{
    int32 y,last=-1,k;
    for(y=1;y<IMG_H-1;y++)
    {
        if(valid[y]!=1) continue;
        if(last>=0&&y-last>1&&y-last<=4)
        {
            for(k=last+1;k<y;k++)
            {
                border[k]=(uint8)((int32)border[last]
                          +((int32)border[y]-border[last])*(k-last)/(y-last));
                valid[k]=2;
            }
        }
        last=y;
    }
}

float Slope_Calculate(uint8 begin,uint8 end,const uint8*border)
{
    float xsum=0,ysum=0,xysum=0,x2sum=0;
    int16 i;
    float result=0;
    float n;
    if(end<=begin) return 0.0f;
    n=(float)(end-begin);
    for(i=(int16)begin;i<(int16)end;i++)
    {
        xsum+=(float)i;
        ysum+=(float)border[i];
        xysum+=(float)i*(float)border[i];
        x2sum+=(float)i*(float)i;
    }
    if(n*x2sum-xsum*xsum!=0.0f) /* 防除零 */
    {
        result=(n*xysum-xsum*ysum)/(n*x2sum-xsum*xsum);
    }
    return result;
}

void Calculate_SI(uint8 begin,uint8 end,const uint8*border,float*slope,float*intercept)
{
    uint16 i,num=0;
    uint32 xsum=0,ysum=0;
    float x_avg=0,y_avg=0;
    if(end<=begin) {*slope=0;*intercept=0;return;}
    for(i=begin;i<end;i++)
    {
        xsum+=i;
        ysum+=border[i];
        num++;
    }
    if(num)
    {
        x_avg=(float)xsum/(float)num;
        y_avg=(float)ysum/(float)num;
    }
    *slope=Slope_Calculate(begin,end,border);
    *intercept=y_avg-(*slope)*x_avg;
}

uint8 find_break_row(const uint8*border,const uint8*valid)
{
    int32 y;
    for(y=IMG_H-3;y>6;y--)
    {
        if(valid[y]==1&&valid[y-1]==1
           &&my_abs((int32)border[y]-(int32)border[y-1])>CROSS_JUMP_MIN)
        {
            return (uint8)y;
        }
    }
    return 0;
}

uint8 line_border(float k,float b,int32 y)
{
    float v=k*(float)y+b;
    if(v<-100.0f) v=-100.0f;
    if(v>300.0f) v=300.0f;
    return camera_limit_x((int32)v);
}

/***********************************************
 * @brief : 按断点附近边线拟合十字延长线，并同步标记实际补线行
 * @param : Border 当前侧边线坐标数组
 * @param : Valid 当前侧有效标记数组，补线行写为二
 * @param : Brk 当前侧断点行，零表示未找到断点
 * @return: 实际补线返回一，未找到断点、斜率不合格或补线范围为空返回零
 ************************************************/
uint8 cross_fill_one(uint8*Border,uint8*Valid,uint8 Brk)
{
    float K=0,B=0;
    uint8 Use_down=0;
    int32 Y,Stop,Guard;
    if(Brk==0) return 0;
    if((int32)Brk+CROSS_FIT_ROWS+3<IMG_H)
    {
        Calculate_SI((uint8)(Brk+3),(uint8)(Brk+CROSS_FIT_ROWS+3),Border,&K,&B);
        if(my_fabs(K)<CROSS_SLOPE_MAX) Use_down=1;
    }
    if(Use_down)
    {
        for(Y=0;Y<(int32)Brk;Y++)
        {
            Border[Y]=line_border(K,B,Y);
            Valid[Y]=2;
        }
        return 1;
    }
    if(Brk<=(uint8)(CROSS_FIT_ROWS+2)) return 0;
    Calculate_SI((uint8)(Brk-CROSS_FIT_ROWS),(uint8)(Brk-1),Border,&K,&B);
    if(my_fabs(K)>=CROSS_SLOPE_MAX) return 0;

    Stop=(int32)Brk+40;
    Guard=ERR_TOP;
    if(Guard<2) Guard=2;
    if(Guard>IMG_H-2) Guard=IMG_H-2;
    if(Stop>Guard) Stop=Guard;
    if(Stop<=(int32)Brk-1) return 0;
    for(Y=(int32)Brk-1;Y<Stop;Y++)
    {
        Border[Y]=line_border(K,B,Y);
        Valid[Y]=2;
    }
    return 1;
}

uint8 count_true_valid(const uint8*valid,uint8 y0,uint8 y1)
{
    uint8 cnt=0;
    int32 y;
    for(y=(int32)y0;y<(int32)y1;y++)
    {
        if(valid[y]==1) cnt++;
    }
    return cnt;
}

uint8 cross_hold=0;/* 十字保持帧数 */

int32 cross_find_open_row(const uint16 pts[][2],uint16 count,int32 sign)
{
    int32 i;
    for(i=6;i+12<count;i++)
    {
        int32 y=pts[i][1];
        int32 dx=sign*((int32)pts[i+12][0]-pts[i][0]);
        int32 dy=(int32)pts[i+12][1]-y;
        if(y<12||y>IMG_H-12||pts[i+12][1]<12
           ||pts[i+12][1]>IMG_H-12||pts[i-6][1]<y+3) continue;
        if(dx>=8&&3*my_abs(dy)<=2*dx) return y;
    }
    return-1;
}

uint8 cross_check_open(void)
{
    int32 left=cross_find_open_row(points_l,count_l,-1);
    int32 right=cross_find_open_row(points_r,count_r,1);
    return left>=0&&right>=0&&my_abs(left-right)<=12;
}

/***********************************************
 * @brief : 结合双侧丢线、底部实测线和开口形状识别十字，否则继续判断圆环
 * @param : l_lost 第八至第八十七行的左侧丢线行数
 * @param : r_lost 第八至第八十七行的右侧丢线行数
 * @return: 本帧识别的道路类型
 ************************************************/
uint8 camera_find_road(uint8 l_lost,uint8 r_lost)
{
    uint8 Bot_l,Bot_r;
    if(l_lost<CROSS_LOST_MIN||r_lost<CROSS_LOST_MIN)
        return ROUND_Detect(l_lost,r_lost);

    /* 第九十至第一百一十八行，两侧各需至少八行实测线，补线不计入。 */
    Bot_l=count_true_valid(l_valid,90,IMG_H-1);
    Bot_r=count_true_valid(r_valid,90,IMG_H-1);
    if(Bot_l>=8&&Bot_r>=8&&cross_check_open())
        return ROAD_CROSS;
    return ROUND_Detect(l_lost,r_lost);
}
/* 算中线 */
volatile int16 vision_error=0;
volatile int16 vision_error_raw=0;
float error_now=0.0f;
volatile uint8 vision_valid=0;
volatile uint8 vision_turn_ok=0;
volatile uint8 vision_exit_blend=0;
volatile uint8 vision_lost=0;

volatile uint8 vision_out=0;

volatile int16 vision_no_start=0;
volatile uint8 vision_cross=0;
volatile uint8 vision_road_type=ROAD_NORMAL;
volatile uint8 vision_round_state=ROUND_NONE;
volatile uint8 vision_round_dir=0;

volatile int16 vision_round_dist=0;
volatile uint8 vision_threshold=100;

int16 START_IN_BLACK_MAX=60;
int16 LOST_NOSTART_FRAMES=LOST_NOSTART_FRAMES_DEFAULT;
volatile uint16 vision_fps=0;
volatile uint16 vision_camera_timeouts=0;

int16 SHOW_DIV=1;

volatile uint16 T_OTSU=0;
volatile uint16 T_TRACE=0;
volatile uint16 T_FILL=0;
volatile uint16 T_ELEM=0;
volatile uint16 T_SHOW=0;
uint16 show_tick=0;
volatile uint16 vision_proc_us=0;
volatile int16 vision_left_lost=0;
volatile int16 vision_right_lost=0;
volatile int16 vision_offset=0;

volatile uint8 vision_show_enable=1;
volatile uint32 vision_frame_id=0;
volatile uint8 vision_reset_request=0;
volatile uint8 vision_snapshot_release=1;/* 0写1空2待显 */
volatile VisionData vision_data;
volatile uint32 vision_data_count=0;

/***********************************************
 * @brief : 发布整帧视觉结果，复位后的首帧发布完毕再解除复位请求
 * @param : Reset_frame 本次发布前是否已执行请求的视觉复位
 * @return: 无
 ************************************************/
void camera_send_result(uint8 Reset_frame)
{
    vision_data_count++;
    __dsync();
    vision_data.frame=vision_frame_id+1U;
    vision_data.error=error_now;
    vision_data.no_start=vision_no_start;
    vision_data.valid=vision_valid;
    vision_data.out=vision_out;
    vision_data.turn_ok=vision_turn_ok;
    vision_data.exit_blend=vision_exit_blend;
    vision_data.road_type=vision_road_type;
    vision_data.round_state=vision_round_state;
    vision_data.round_dir=vision_round_dir;
    __dsync();
    vision_data_count++;
    vision_frame_id++;
    if(Reset_frame)
    {
        __dsync();
        vision_reset_request=0;
    }
}

/***********************************************
 * @brief : 读取一致的整帧结果，发车复位交接期间拒绝旧结果
 * @param : data 接收视觉结果的地址
 * @return: 读取成功返回一，复位中或快照变化时返回零
 ************************************************/
uint8 Vision_Read(VisionData *data)
{
    VisionData next;
    uint32 count=vision_data_count;
    if(vision_reset_request||(count&1U)) return 0;
    __dsync();
    next=vision_data;
    __dsync();
    if(vision_reset_request||count!=vision_data_count) return 0;
    *data=next;
    return 1;
}

typedef struct
{
    uint8 valid,x,y;
} CameraShowPoint;

typedef struct
{
    uint8 bin[IMG_H][IMG_W];
    uint16 points_l[POINT_CAP][2];
    uint16 points_r[POINT_CAP][2];
    uint8 center_line[IMG_H],l_border[IMG_H],r_border[IMG_H];
    uint8 l_valid[IMG_H],r_valid[IMG_H];
    uint16 count_l,count_r;
    uint8 round_dir;
    CameraShowPoint near_point[2],mid_point[2],far_point[2];
    CameraShowPoint candidate[2][3];
} CameraShowData;

CameraShowData show_data;

/* 发布快照 */
void camera_save_image(void)
{
    memcpy(show_data.bin,bin_image,sizeof(bin_image));
    memcpy(show_data.points_l,points_l,sizeof(points_l));
    memcpy(show_data.points_r,points_r,sizeof(points_r));
    memcpy(show_data.center_line,center_line,sizeof(center_line));
    memcpy(show_data.l_border,l_border,sizeof(l_border));
    memcpy(show_data.r_border,r_border,sizeof(r_border));
    memcpy(show_data.l_valid,l_valid,sizeof(l_valid));
    memcpy(show_data.r_valid,r_valid,sizeof(r_valid));
    show_data.count_l=count_l;
    show_data.count_r=count_r;
    show_data.round_dir=round_dir;
    show_data.near_point[ROUND_LEFT].valid=L_corner_flag;
    show_data.near_point[ROUND_LEFT].x=L_corner_col;
    show_data.near_point[ROUND_LEFT].y=L_corner_row;
    show_data.near_point[ROUND_RIGHT].valid=R_corner_flag;
    show_data.near_point[ROUND_RIGHT].x=R_corner_col;
    show_data.near_point[ROUND_RIGHT].y=R_corner_row;
    show_data.mid_point[ROUND_LEFT].valid=L_mid_flag;
    show_data.mid_point[ROUND_LEFT].x=L_mid_col;
    show_data.mid_point[ROUND_LEFT].y=L_mid_row;
    show_data.mid_point[ROUND_RIGHT].valid=R_mid_flag;
    show_data.mid_point[ROUND_RIGHT].x=R_mid_col;
    show_data.mid_point[ROUND_RIGHT].y=R_mid_row;
    show_data.far_point[ROUND_LEFT].valid=L_far_flag;
    show_data.far_point[ROUND_LEFT].x=L_far_col;
    show_data.far_point[ROUND_LEFT].y=L_far_row;
    show_data.far_point[ROUND_RIGHT].valid=R_far_flag;
    show_data.far_point[ROUND_RIGHT].x=R_far_col;
    show_data.far_point[ROUND_RIGHT].y=R_far_row;
    for(uint8 side=0;side<2;side++)
    for(uint8 point=0;point<3;point++)
    {
        RoundPoint*p=&round_points[side][point];
        show_data.candidate[side][point].valid=p->hits>0;
        show_data.candidate[side][point].x=p->px;
        show_data.candidate[side][point].y=p->py;
    }
}

/***********************************************
 * @brief : 根据有效边线计算中点，单侧有效时以缺失侧画面边缘估算
 * @param : y 当前图像行号
 * @return: 有效中点横坐标，无有效配对时返回二百五十五
 ************************************************/
uint8 camera_row_center(int32 y)
{
    int32 Left=l_border[y],Right=r_border[y];
    if(!l_valid[y])
    {
        if(!r_valid[y]) return 255;
        /* 十字补线可单侧使用；圆环仍要求本帧补线标记。 */
        if(r_valid[y]!=1&&r_valid[y]!=2) return 255;
        if(r_valid[y]==2&&vision_road_type!=ROAD_CROSS
           &&(round_state==ROUND_NONE||!round_line_rows[y])) return 255;
        Left=2;
    }
    else if(!r_valid[y])
    {
        if(l_valid[y]!=1&&l_valid[y]!=2) return 255;
        if(l_valid[y]==2&&vision_road_type!=ROAD_CROSS
           &&(round_state==ROUND_NONE||!round_line_rows[y])) return 255;
        Right=IMG_W-3;
    }
    else if(vision_road_type==ROAD_NORMAL&&l_valid[y]!=1&&r_valid[y]!=1) return 255;
    if(Left+3>=Right) return 255;
    return (uint8)((Left+Right)/2);
}

/* 修正孤点 */
void camera_fix_center(void)
{
    uint8 raw[IMG_H];
    int32 y,up,mid,down,low,high,fix;
    if(vision_road_type!=ROAD_NORMAL||round_state!=ROUND_NONE) return;
    memcpy(raw,center_line,sizeof(raw));
    for(y=2;y<IMG_H-2;y++)
    {
        if(l_valid[y-1]!=1||l_valid[y]!=1||l_valid[y+1]!=1
           ||r_valid[y-1]!=1||r_valid[y]!=1||r_valid[y+1]!=1) continue;
        if(round_line_rows[y-1]||round_line_rows[y]||round_line_rows[y+1]) continue;
        up=raw[y-1];mid=raw[y];down=raw[y+1];
        if(up==255||mid==255||down==255||my_abs(up-down)>6) continue;
        if(raw[y-2]==255||raw[y+2]==255) continue;
        if(my_abs(raw[y-2]-up)>6||my_abs(raw[y+2]-down)>6) continue;
        low=up<down?up:down;
        high=up>down?up:down;
        if(mid>=low-8&&mid<=high+8) continue;
        fix=(up+down)/2;
        if(fix<=l_border[y]||fix>=r_border[y]) continue;
        center_line[y]=(uint8)fix;
    }
}

/***********************************************
 * @brief : 根据前瞻范围加权计算误差，窗口内无效中点累计达到十四行时判失效
 * @param : 无
 * @return: 有可用中点且无效行少于十四行返回一，否则返回零并保留上次误差
 ************************************************/
uint8 camera_get_error(void)
{
    int32 y,w,sum=0,weight=0,real_rows=0,guide_rows=0;
    int32 Lost_rows=0;
    float error;
    for(y=0;y<IMG_H;y++) center_line[y]=camera_row_center(y);
    camera_fix_center();
    for(y=ERR_TOP;y<=ERR_BOTTOM;y++)
    {
        if(center_line[y]==255)
        {
            Lost_rows++;
            continue;
        }
        /* 近处权重大 */
        w=y-ERR_TOP+1;
        sum+=center_line[y]*w;
        weight+=w;
        if(round_line_rows[y]) guide_rows++;
        if(round_state==ROUND_IN||round_state==ROUND_OUT)
        {
            if(l_valid[y]!=2&&r_valid[y]!=2) real_rows++;
        }
        else if(round_dir==ROUND_LEFT_DIR&&l_valid[y]==1) real_rows++;
        else if(round_dir==ROUND_RIGHT_DIR&&r_valid[y]==1) real_rows++;
    }
    vision_turn_ok=real_rows*2>=ERR_BOTTOM-ERR_TOP+1
                   ||guide_rows*2>=ERR_BOTTOM-ERR_TOP+1;
    if(!weight||Lost_rows>=14)
    {
        vision_turn_ok=0;
        return 0;
    }
    vision_error_raw=(int16)(sum/weight-IMG_W/2-vision_offset);
    error=(float)sum/(float)weight-IMG_W/2-vision_offset;
    error_now=error;
    vision_error=(int16)(error+(error>=0.0f?0.5f:-0.5f));
    return 1;
}

/***********************************************
 * @brief : 复位圆环识别及视觉控制状态
 * @param : 无
 * @return: 无
 ************************************************/
void Vision_Reset(void)
{
    ROUND_Init();
    vision_turn_ok=0;
    vision_valid=0;
    vision_lost=1;
    vision_exit_blend=0;
    vision_error=0;
    vision_error_raw=0;
    error_now=0.0f;
    vision_out=0;
    vision_cross=0;
    vision_road_type=ROAD_NORMAL;
    vision_round_state=ROUND_NONE;
    vision_round_dir=0;
    cross_hold=0;
    vision_no_start=0;
    start_l[0]=start_l[1]=0;
    start_r[0]=start_r[1]=0;
    memset(l_edge,0,sizeof(l_edge));
    memset(r_edge,0,sizeof(r_edge));
}

void Vision_RequestReset(void)
{
    vision_reset_request=1;
}

/* 等待新帧 */
uint8 camera_wait_frame(void)
{
    uint32 wait_us;
    mt9v03x_finish_flag=0;
    wait_us=system_getval_us();
    while(mt9v03x_finish_flag==0)
    {
        if((uint32)(system_getval_us()-wait_us)>=100000U) return 0;
    }
    mt9v03x_finish_flag=0;
    return 1;
}

/***********************************************
 * @brief : 逐帧计算阈值并识别赛道及生成控制误差
 * @param : 无
 * @return: 本帧处理结束时的微秒计时值
 ************************************************/
uint32 camera_process(void)
{
    uint32 start_us,now_us,t_last,t_now,i;
    uint8 highest=0,l_lost,r_lost,det;
    start_us=system_getval_us();
    t_last=system_getval_us();
    vision_threshold=camera_get_threshold();
    turn_to_bin(vision_threshold);
    t_now=system_getval_us();T_OTSU=(uint16)(t_now-t_last);t_last=t_now;
    image_draw_rect();
    vision_valid=0;
    vision_turn_ok=0;
    vision_lost=0;
    vision_out=0;
    vision_cross=0;
    if(get_start_point(IMG_H-2))
    {
        vision_no_start=0;
        camera_find_border(POINT_CAP,&highest);
        get_left();
        get_right();
        t_now=system_getval_us();T_TRACE=(uint16)(t_now-t_last);t_last=t_now;
        l_lost=(uint8)((ELEM_BOT-ELEM_TOP)-count_true_valid(l_valid,ELEM_TOP,ELEM_BOT));
        r_lost=(uint8)((ELEM_BOT-ELEM_TOP)-count_true_valid(r_valid,ELEM_TOP,ELEM_BOT));
        vision_left_lost=(int16)l_lost;
        vision_right_lost=(int16)r_lost;
        ROUND_GetPoints();
        det=camera_find_road(l_lost,r_lost);
        if(det==ROAD_CROSS)
        {
            cross_hold=CROSS_HOLD_FRAMES;
        }
        else if(det==ROAD_ROUND_LEFT||det==ROAD_ROUND_RIGHT)
        {
            cross_hold=0;
        }
        border_fill(l_border,l_valid);
        border_fill(r_border,r_valid);
        t_now=system_getval_us();T_FILL=(uint16)(t_now-t_last);t_last=t_now;
        if(round_state!=ROUND_NONE)
        {
            if(round_dir==1) vision_road_type=ROAD_ROUND_LEFT;
            else vision_road_type=ROAD_ROUND_RIGHT;
            ROUND_Process(det,l_lost,r_lost);
        }
        else if(det==ROAD_CROSS||cross_hold>0)
        {
            uint8 brk_l=find_break_row(l_border,l_valid);
            uint8 brk_r=find_break_row(r_border,r_valid);
            vision_road_type=ROAD_CROSS;
            vision_cross=1;
            cross_fill_one(l_border,l_valid,brk_l);
            cross_fill_one(r_border,r_valid,brk_r);
            ROUND_ClearEntry();
            if(cross_hold>0) cross_hold--;
        }
        else
        {
            ROUND_Process(det,l_lost,r_lost);
            if(round_state!=ROUND_NONE)
            {
                if(round_dir==1) vision_road_type=ROAD_ROUND_LEFT;
                else vision_road_type=ROAD_ROUND_RIGHT;
            }
            else
            {
                vision_road_type=ROAD_NORMAL;
            }
        }
        ROUND_Repair();
        if(round_state==ROUND_NONE) vision_road_type=vision_cross?ROAD_CROSS:ROAD_NORMAL;
        vision_valid=camera_get_error();
        vision_lost=!vision_valid;
        t_now=system_getval_us();
        T_ELEM=(uint16)(t_now-t_last);
    }
    else
    {
        for(i=0;i<IMG_H;i++)
        {
            l_border[i]=BORDER_MIN;
            r_border[i]=BORDER_MAX;
            center_line[i]=255;
            l_valid[i]=0;
            r_valid[i]=0;
            l_edge[i]=r_edge[i]=0;
        }
        count_l=0;
        count_r=0;
        vision_exit_blend=0;
        ROUND_GetPoints();
        vision_lost=1;
        if(LOST_NOSTART_FRAMES<1) LOST_NOSTART_FRAMES=1;
        if(start_l[1]||start_r[1]) vision_no_start=0;
        else if(vision_no_start<LOST_NOSTART_FRAMES) vision_no_start++;
        if(vision_no_start>=LOST_NOSTART_FRAMES) vision_out=1;
    }
    vision_round_state=round_state;
    vision_round_dir=round_dir;
    now_us=system_getval_us();
    vision_proc_us=(uint16)(now_us-start_us);
    return now_us;
}

/* 画点限幅 */
void camera_draw_point(int32 x,int32 y,uint16 color)
{
    if(x<0) x=0;
    if(x>=IMG_W) x=IMG_W-1;
    if(y<0) y=0;
    if(y>=IMG_H) y=IMG_H-1;
    ips200_draw_point((uint16)x,(uint16)y,color);
}

/* 画拐点 */
void camera_draw_corner(CameraShowPoint point,uint16 color)
{
    int32 dx,dy;
    if(!point.valid) return;
    for(dy=-3;dy<=3;dy++)
        for(dx=-3;dx<=3;dx++)
            camera_draw_point((int32)point.x+dx,(int32)point.y+dy,color);
}

void camera_draw_candidate(CameraShowPoint point,uint16 color)
{
    if(!point.valid) return;
    camera_draw_point(point.x-2,point.y,color);
    camera_draw_point(point.x-1,point.y,color);
    camera_draw_point(point.x,point.y,color);
    camera_draw_point(point.x+1,point.y,color);
    camera_draw_point(point.x+2,point.y,color);
    camera_draw_point(point.x,point.y-2,color);
    camera_draw_point(point.x,point.y-1,color);
    camera_draw_point(point.x,point.y+1,color);
    camera_draw_point(point.x,point.y+2,color);
}

/* 显示图像 */
void camera_show_image(void)
{
    uint32 start_us;
    uint16 i;
    T_SHOW=0;
    if(SHOW_DIV<1) SHOW_DIV=1;
    show_tick++;
    if(show_tick<(uint16)SHOW_DIV) return;
    show_tick=0;
    if(!vision_show_enable) return;
    start_us=system_getval_us();
    ips200_show_gray_image(0,0,show_data.bin[0],IMG_W,IMG_H,IMG_W,IMG_H,0);
    for(i=0;i<show_data.count_l;i++)
        camera_draw_point(show_data.points_l[i][0],show_data.points_l[i][1],RGB565_BLUE);
    for(i=0;i<show_data.count_r;i++)
        camera_draw_point(show_data.points_r[i][0],show_data.points_r[i][1],RGB565_RED);
    for(i=0;i<IMG_H-1;i++)
    {
        if(show_data.center_line[i]<IMG_W)
            camera_draw_point(show_data.center_line[i],i,RGB565_GREEN);
        if(show_data.l_valid[i])
            camera_draw_point(show_data.l_border[i],i,RGB565_YELLOW);
        if(show_data.r_valid[i])
            camera_draw_point(show_data.r_border[i],i,RGB565_YELLOW);
    }
    if(show_data.round_dir)
    {
        camera_draw_corner(show_data.near_point[ROUND_LEFT],RGB565_RED);
        camera_draw_corner(show_data.near_point[ROUND_RIGHT],RGB565_RED);
        camera_draw_corner(show_data.mid_point[ROUND_LEFT],RGB565_YELLOW);
        camera_draw_corner(show_data.mid_point[ROUND_RIGHT],RGB565_YELLOW);
        camera_draw_corner(show_data.far_point[ROUND_LEFT],RGB565_GREEN);
        camera_draw_corner(show_data.far_point[ROUND_RIGHT],RGB565_GREEN);
    }
    for(i=0;i<2;i++)
    {
        camera_draw_candidate(show_data.candidate[i][ROUND_NEAR],RGB565_CYAN);
        camera_draw_candidate(show_data.candidate[i][ROUND_MID],RGB565_MAGENTA);
    }
    T_SHOW=(uint16)(system_getval_us()-start_us);
}

/***********************************************
 * @brief : 等待新图像，处理并发布视觉结果，按请求保存显示快照
 * @param : 无
 * @return: 完成本帧处理返回一，等待超时或开机丢帧返回零
 ************************************************/
uint8 Image_Show(void)
{
    static uint32 last_fps_us=0;
    static uint16 frame_cnt=0;
    static uint8 start_frames=0;
    uint8 Reset_frame=0;
    uint32 now_us;

    /* 停车后清掉上次圆环状态，即使这一帧没有爬线起点也能清理。 */
    if(car_state!=CAR_RUNNING&&round_state!=ROUND_NONE) Vision_Reset();

    /* 等运行状态发布后再复位，首帧结果发布前保留请求标记。 */
    if(vision_reset_request&&car_state==CAR_RUNNING)
    {
        Vision_Reset();
        Reset_frame=1;
    }
    if(!camera_wait_frame())
    {
        if(vision_camera_timeouts<65535U) vision_camera_timeouts++;
        vision_valid=0;
        vision_turn_ok=0;
        vision_exit_blend=0;
        vision_lost=1;
        vision_out=0;
        vision_no_start=0;
        vision_fps=0;
        frame_cnt=0;
        last_fps_us=system_getval_us();
        camera_send_result(Reset_frame);
        return 0;
    }
    if(start_frames<CAM_WARMUP_FRAMES)
    {
        start_frames++;
        vision_valid=0;
        vision_lost=1;
        if(start_frames<CAM_WARMUP_FRAMES)
        {
            camera_send_result(Reset_frame);
            return 0;
        }
    }
    now_us=camera_process();
    if(vision_show_enable&&vision_snapshot_release==1)
    {
        vision_snapshot_release=0;
        __dsync();
        camera_save_image();
        __dsync();
        vision_snapshot_release=2;
    }
    camera_send_result(Reset_frame);
    frame_cnt++;
    if(last_fps_us==0) last_fps_us=now_us;
    if(now_us-last_fps_us>=1000000U)
    {
        vision_fps=(uint16)((uint32)frame_cnt*1000000U/(now_us-last_fps_us));
        frame_cnt=0;
        last_fps_us=now_us;
    }
    return 1;
}

void Image_Display(void)
{
    if(vision_snapshot_release!=2) return;
    __dsync();
    camera_show_image();
    __dsync();
    vision_snapshot_release=1;
}
