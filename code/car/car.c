#include "car.h"
#include "motor.h"
#include "servo.h"
#include "Camera.h"
#include "../ROUND/ROUND.h"
#include "MyEncoder/MyEncoder.h"
#include "MyKey/MyKey.h" /* 编码器 */
#include "menu/menu.h" /* 菜单 */
#include "ips-show/ips-show.h" /* 状态显示 */
#include "../Beep/Beep.h"

int16 CAR_BASE_SPEED=CAR_DEFAULT_PWM_FF;
int16 LEFT_ROUND_SPEED=CAR_DEFAULT_LEFT_ROUND;
int16 RIGHT_ROUND_SPEED=CAR_DEFAULT_RIGHT_ROUND;
int16 SPDEND=CAR_DEFAULT_SPDEND; /* 右环结束后目标速度 */

float STEER_KP=CAR_DEFAULT_STEER_KP;

float STEER_KD=CAR_DEFAULT_STEER_KD;
int16 STEER_OFFSET_LIMIT=CAR_DEFAULT_TURN_LIMIT;/* 舵机限幅 */
uint8 STEER_REVERSE=1;

int16 CAR_RUN_S=CAR_DEFAULT_RUN_S;

int16 CAR_TEST_MODE=0;

uint8 CAR_MODE=CAR_MODE_DRIVE;

static int16 Save_pwm;       /* 调试前的前馈 */
static int16 Save_speed;     /* 调试前的目标速度 */
static int16 Save_trim;      /* 调试前的修正量 */
static int16 Save_brake;     /* 调试前的反转限幅 */
static int16 Save_dead;      /* 调试前的低速帧数 */
static int16 Save_run_s;     /* 调试前的运行时长 */
static float Save_diff;      /* 调试前的差速系数 */

/***********************************************
 * @brief : 切换坡道调试模式并保存或恢复行驶参数
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Car_Mode_Toggle(void)
{
    if(CAR_MODE==CAR_MODE_DRIVE)
    {
        Save_pwm=CAR_BASE_SPEED;
        Save_speed=SPD_BASE;
        Save_trim=SPD_TRIM;
        Save_brake=SPD_BRAKE_TRIM;
        Save_dead=SPD_DEAD_FRAMES;
        Save_run_s=CAR_RUN_S;
        Save_diff=DIFF_GAIN;
        CAR_BASE_SPEED=0;
        SPD_BASE=0;
        SPD_TRIM=4000;
        SPD_BRAKE_TRIM=2000;
        SPD_DEAD_FRAMES=0;
        CAR_RUN_S=0;
        DIFF_GAIN=0.0f;
        CAR_TEST_MODE=1;
        CAR_MODE=CAR_MODE_SPEED_TUNE;
    }
    else
    {
        CAR_BASE_SPEED=Save_pwm;
        SPD_BASE=Save_speed;
        SPD_TRIM=Save_trim;
        SPD_BRAKE_TRIM=Save_brake;
        SPD_DEAD_FRAMES=Save_dead;
        CAR_RUN_S=Save_run_s;
        DIFF_GAIN=Save_diff;
        CAR_TEST_MODE=0;
        CAR_MODE=CAR_MODE_DRIVE;
    }
}

/***********************************************
 * @brief : 慢速档
 * @param : 无
 * @return: 无
 ************************************************/
void Car_ResetSlow(void)
{
    /* 先退出坡道调试，避免其旧缓存随后覆盖本次恢复结果。 */
    if(CAR_MODE==CAR_MODE_SPEED_TUNE) Car_Mode_Toggle();

    CAR_BASE_SPEED=CAR_SLOW_PWM_FF;
    SPD_BASE=CAR_SLOW_STRAIGHT;
    LEFT_ROUND_SPEED=CAR_SLOW_LEFT_ROUND;
    RIGHT_ROUND_SPEED=CAR_SLOW_RIGHT_ROUND;
    SPDEND=CAR_SLOW_SPDEND;
    DIFF_GAIN=CAR_SLOW_DIFF_GAIN;
    CAR_RUN_S=CAR_SLOW_RUN_S;
    STEER_KP=CAR_SLOW_STEER_KP;
    STEER_KD=CAR_SLOW_STEER_KD;
    STEER_OFFSET_LIMIT=CAR_SLOW_TURN_LIMIT;
    SPD_TUNE_KP=CAR_SLOW_SPEED_KP;
    SPD_TUNE_KI=CAR_SLOW_SPEED_KI;

    ROUND_LEFT_START_DELAY_S=ROUND_SLOW_LEFT_DELAY_S;
    ROUND_RIGHT_START_DELAY_S=ROUND_SLOW_RIGHT_DELAY_S;
    ROUND_LEFT_INTO_MIN_MS=ROUND_SLOW_LEFT_INTO_MIN_MS;
    ROUND_LEFT_INTO_MAX_MS=ROUND_SLOW_LEFT_INTO_MAX_MS;
    ROUND_LEFT_RND4_MIN_MS=ROUND_SLOW_LEFT_RND4_MIN_MS;
    ROUND_LEFT_RND4_MAX_MS=ROUND_SLOW_LEFT_RND4_MAX_MS;
    ROUND_LEFT_RND5_MAX_MS=ROUND_SLOW_LEFT_RND5_MAX_MS;
    ROUND_LEFT_ALIGN_MS=ROUND_SLOW_LEFT_ALIGN_MS;
    ROUND_LEFT_LEAVE_MS=ROUND_SLOW_LEFT_LEAVE_MS;
    Right_rnd2_min_ms=ROUND_SLOW_RIGHT_RND2_MIN_MS;
    Right_rnd3_min_ms=ROUND_SLOW_RIGHT_RND3_MIN_MS;
}

/***********************************************
 * @brief : 中速档
 * @param : 无
 * @return: 无
 ************************************************/
void Car_ResetParams(void)
{
    /* 先退出坡道调试，避免其旧缓存随后覆盖本次恢复结果。 */
    if(CAR_MODE==CAR_MODE_SPEED_TUNE) Car_Mode_Toggle();

    CAR_BASE_SPEED=CAR_MID_PWM_FF;
    SPD_BASE=CAR_MID_STRAIGHT;
    LEFT_ROUND_SPEED=CAR_MID_LEFT_ROUND;
    RIGHT_ROUND_SPEED=CAR_MID_RIGHT_ROUND;
    SPDEND=CAR_MID_SPDEND;
    DIFF_GAIN=CAR_MID_DIFF_GAIN;
    CAR_RUN_S=CAR_MID_RUN_S;
    STEER_KP=CAR_MID_STEER_KP;
    STEER_KD=CAR_MID_STEER_KD;
    STEER_OFFSET_LIMIT=CAR_MID_TURN_LIMIT;
    SPD_TUNE_KP=CAR_MID_SPEED_KP;
    SPD_TUNE_KI=CAR_MID_SPEED_KI;

    ROUND_LEFT_START_DELAY_S=ROUND_MID_LEFT_DELAY_S;
    ROUND_RIGHT_START_DELAY_S=ROUND_MID_RIGHT_DELAY_S;
    ROUND_LEFT_INTO_MIN_MS=ROUND_MID_LEFT_INTO_MIN_MS;
    ROUND_LEFT_INTO_MAX_MS=ROUND_MID_LEFT_INTO_MAX_MS;
    ROUND_LEFT_RND4_MIN_MS=ROUND_MID_LEFT_RND4_MIN_MS;
    ROUND_LEFT_RND4_MAX_MS=ROUND_MID_LEFT_RND4_MAX_MS;
    ROUND_LEFT_RND5_MAX_MS=ROUND_MID_LEFT_RND5_MAX_MS;
    ROUND_LEFT_ALIGN_MS=ROUND_MID_LEFT_ALIGN_MS;
    ROUND_LEFT_LEAVE_MS=ROUND_MID_LEFT_LEAVE_MS;
    Right_rnd2_min_ms=ROUND_MID_RIGHT_RND2_MIN_MS;
    Right_rnd3_min_ms=ROUND_MID_RIGHT_RND3_MIN_MS;
}

/***********************************************
 * @brief : 高速档
 * @param : 无
 * @return: 无
 ************************************************/
void Car_ResetHigh(void)
{
    /* 先退出坡道调试，避免其旧缓存随后覆盖本次恢复结果。 */
    if(CAR_MODE==CAR_MODE_SPEED_TUNE) Car_Mode_Toggle();

    CAR_BASE_SPEED=CAR_HIGH_PWM_FF;
    SPD_BASE=CAR_HIGH_STRAIGHT;
    LEFT_ROUND_SPEED=CAR_HIGH_LEFT_ROUND;
    RIGHT_ROUND_SPEED=CAR_HIGH_RIGHT_ROUND;
    SPDEND=CAR_HIGH_SPDEND;
    DIFF_GAIN=CAR_HIGH_DIFF_GAIN;
    CAR_RUN_S=CAR_HIGH_RUN_S;
    STEER_KP=CAR_HIGH_STEER_KP;
    STEER_KD=CAR_HIGH_STEER_KD;
    STEER_OFFSET_LIMIT=CAR_HIGH_TURN_LIMIT;
    SPD_TUNE_KP=CAR_HIGH_SPEED_KP;
    SPD_TUNE_KI=CAR_HIGH_SPEED_KI;

    ROUND_LEFT_START_DELAY_S=ROUND_HIGH_LEFT_DELAY_S;
    ROUND_RIGHT_START_DELAY_S=ROUND_HIGH_RIGHT_DELAY_S;
    ROUND_LEFT_INTO_MIN_MS=ROUND_HIGH_LEFT_INTO_MIN_MS;
    ROUND_LEFT_INTO_MAX_MS=ROUND_HIGH_LEFT_INTO_MAX_MS;
    ROUND_LEFT_RND4_MIN_MS=ROUND_HIGH_LEFT_RND4_MIN_MS;
    ROUND_LEFT_RND4_MAX_MS=ROUND_HIGH_LEFT_RND4_MAX_MS;
    ROUND_LEFT_RND5_MAX_MS=ROUND_HIGH_LEFT_RND5_MAX_MS;
    ROUND_LEFT_ALIGN_MS=ROUND_HIGH_LEFT_ALIGN_MS;
    ROUND_LEFT_LEAVE_MS=ROUND_HIGH_LEFT_LEAVE_MS;
    Right_rnd2_min_ms=ROUND_HIGH_RIGHT_RND2_MIN_MS;
    Right_rnd3_min_ms=ROUND_HIGH_RIGHT_RND3_MIN_MS;
}

int16 VISION_MOUNT_OFFSET=0;

int16 LOST_HOLD_MS=360; /* 普通道路误差无效时保持原转向的毫秒数 */
int16 LOST_HOLD_MS_ELEM=600;
volatile uint8 car_state=CAR_IDLE;
static volatile uint8 menu_open=0;
static volatile uint8 menu_servo_ready=0;
volatile uint32 Car_state_us=0; /* 当前车辆状态的起始时刻，双核共用 */
static uint32 text_last_us=0;
VisionData car_vision;

static float last_error=0.0f;
static uint32 steer_last_frame=0;
static uint8 last_exit_blend=0;
static uint16 lost_frames=0;/* 丢线帧数 */
static uint32 last_line_us=0;/* CPU1转向计时 */
static uint8 steer_line_lost=0;
static uint8 steer_first_frame=1;

static volatile int32 last_drive_offset=0;
static volatile uint8 steer_ready=0;
static uint8 drive_sent=0;

int16 round_turn_list[ROUND_TURN_SAMPLES];
uint8 round_turn_count,round_turn_pos,round_turn_dir,round_turn_hold;
int32 round_turn_mean;
uint32 round_turn_us,round_turn_last_us;

/***********************************************
 * @brief : 将转向偏移限制在当前允许范围内
 * @param : Offset 待限制的转向偏移
 * @return: 限幅后的转向偏移
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
int32 Car_LimitSteer(int32 Offset)
{
    if(Offset>STEER_OFFSET_LIMIT) Offset=STEER_OFFSET_LIMIT;
    if(Offset<-STEER_OFFSET_LIMIT) Offset=-STEER_OFFSET_LIMIT;
    return Offset;
}

/***********************************************
 * @brief : 校正安装偏置并更新图像使用的偏移量
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Car_UpdateOffset(void)
{
    if(VISION_MOUNT_OFFSET<-30) VISION_MOUNT_OFFSET=-30;
    if(VISION_MOUNT_OFFSET>30) VISION_MOUNT_OFFSET=30;
    vision_offset=VISION_MOUNT_OFFSET;
}

/***********************************************
 * @brief : 清除圆环续弯记录及出口交接标志
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void car_round_clear(void)
{
    round_turn_count=round_turn_pos=round_turn_dir=round_turn_hold=0;
    last_exit_blend=0;
}

/***********************************************
 * @brief : 圆环视觉失效时短暂保持近期转向均值
 * @param : Offset 本帧转向偏移
 * @param : Now_us 当前微秒时间
 * @param : Vision 本帧视觉快照
 * @return: 应执行的转向偏移
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
int32 car_round_turn(int32 Offset,uint32 Now_us,const VisionData*Vision)
{
    int32 Turn,Sum=0;
    uint8 I;
    Offset=Car_LimitSteer(Offset);
    if((Vision->round_dir!=ROUND_LEFT_DIR&&Vision->round_dir!=ROUND_RIGHT_DIR)
       ||(Vision->round_state!=ROUND_IN&&Vision->round_state!=ROUND_OUT))
    {
        car_round_clear();
        return Offset;
    }
    if(Vision->round_dir!=round_turn_dir)
    {
        car_round_clear();
        round_turn_dir=Vision->round_dir;
    }
    if(Vision->valid&&Vision->turn_ok)
    {
        if(round_turn_hold) round_turn_count=round_turn_pos=round_turn_hold=0;
        if(Vision->round_state==ROUND_IN)
        {
            Turn=Offset;
            if(!STEER_REVERSE) Turn=-Offset;
            if(Vision->round_dir==ROUND_RIGHT_DIR) Turn=-Turn;
            if(Turn<=0||(uint32)(Now_us-round_turn_last_us)>ROUND_TURN_FRESH_US)
                round_turn_count=round_turn_pos=0;
            if(Turn>0)
            {
                round_turn_list[round_turn_pos]=(int16)Offset;
                round_turn_pos=(round_turn_pos+1)%ROUND_TURN_SAMPLES;
                if(round_turn_count<ROUND_TURN_SAMPLES) round_turn_count++;
                round_turn_last_us=Now_us;
            }
        }
        return Offset;
    }
    if(!round_turn_hold)
    {
        if(round_turn_count<3
           ||(uint32)(Now_us-round_turn_last_us)>ROUND_TURN_FRESH_US) return Offset;
        for(I=0;I<round_turn_count;I++) Sum+=round_turn_list[I];
        round_turn_mean=Sum/round_turn_count;
        round_turn_us=Now_us;
        round_turn_hold=1;
    }
    if((uint32)(Now_us-round_turn_us)>=ROUND_TURN_HOLD_US)
    {
        car_round_clear();
        return Offset;
    }
    return round_turn_mean;
}
uint16 pwm=0;
/***********************************************
 * @brief : 在CPU1按新视觉帧计算转向并交接菜单舵机
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Car_SteerTask(void)
{
    uint32 Now_us,Lost_ms,Frame_step,Hold_ms;
    int32 Steer_offset;
    float Err_change;
    VisionData New_vision;

    /* 菜单接管舵机 */
    if(menu_open)
    {
        steer_ready=0;
        steer_first_frame=1;
        steer_line_lost=0;
        car_round_clear();
        if(menu_servo_ready) return;
        if(servo_duty!=SERVO_CENTER_DUTY) Servo_Center();
        __dsync();
        menu_servo_ready=1;
        return;
    }
    menu_servo_ready=0;

    if(car_state!=CAR_RUNNING||CAR_TEST_MODE)
    {
        if(Vision_Read(&New_vision)) steer_last_frame=New_vision.frame;
        car_round_clear();
        steer_ready=0;
        steer_first_frame=1;
        steer_line_lost=0;
        if(servo_duty!=SERVO_CENTER_DUTY) Servo_Center();
        return;
    }

    /* 新图像才计算 */
    if(!Vision_Read(&New_vision)) return;
    if(New_vision.frame==steer_last_frame) return;
    Frame_step=New_vision.frame-steer_last_frame;
    steer_last_frame=New_vision.frame;
    Now_us=system_getval_us();
    if(steer_first_frame)
    {
        last_error=New_vision.error;
        last_line_us=Now_us;
        steer_first_frame=0;
    }
    if(New_vision.valid)
    {
        Err_change=(New_vision.error-last_error)/(float)Frame_step;
        if(steer_line_lost||round_turn_hold
           ||((New_vision.round_state==ROUND_IN||New_vision.round_state==ROUND_OUT)
              &&(New_vision.exit_blend||last_exit_blend))) Err_change=0;
        last_exit_blend=(New_vision.round_state==ROUND_IN||New_vision.round_state==ROUND_OUT)
                        &&New_vision.exit_blend;
        last_error=New_vision.error;
        steer_line_lost=0;
        last_line_us=Now_us;
        Steer_offset=(int32)(STEER_KP*New_vision.error+STEER_KD*Err_change);
        if(STEER_REVERSE) Steer_offset=-Steer_offset;
        Steer_offset=car_round_turn(Steer_offset,Now_us,&New_vision);
        Steer_offset=Car_LimitSteer(Steer_offset);
        last_drive_offset=Steer_offset;
    }
    else
    {
        steer_line_lost=1;
        Lost_ms=(uint32)(Now_us-last_line_us)/1000U;
        /* 丢线时保持 */
        Steer_offset=car_round_turn(last_drive_offset,Now_us,&New_vision);
        if(round_turn_hold)
        {
            Steer_offset=Car_LimitSteer(Steer_offset);
            last_drive_offset=Steer_offset;
        }
        if(New_vision.road_type!=ROAD_NORMAL
           ||New_vision.round_state!=ROUND_NONE)
            Hold_ms=(uint32)LOST_HOLD_MS_ELEM;
        else
            Hold_ms=(uint32)LOST_HOLD_MS;

        if(Lost_ms>=Hold_ms)
        {
            Steer_offset=0;
            last_drive_offset=0;
        }
    }

    if(car_state!=CAR_RUNNING||CAR_TEST_MODE) return;
    Steer_offset=Car_LimitSteer(Steer_offset);
    pwm=(uint16)((int32)SERVO_CENTER_DUTY+Steer_offset);
    Servo_SetDuty(pwm);
    __dsync();
    steer_ready=1;
}

/***********************************************
 * @brief : 仅在请求变化时提交闭环目标或开环输出
 * @param : Dir_offset 舵机相对中位的偏移
 * @param : Speed_target 闭环目标速度
 * @param : Open_duty 开环电机占空比
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
static void car_drive(int32 Dir_offset,int16 Speed_target,int32 Open_duty)
{
    static int32 Sent_offset,Sent_duty;
    static int16 Sent_speed;
    static uint8 Sent_speed_en;
    static float Sent_diff_gain;
    if(drive_sent&&Sent_offset==Dir_offset&&Sent_speed==Speed_target
       &&Sent_duty==Open_duty&&Sent_speed_en==SPD_EN
       &&Sent_diff_gain==DIFF_GAIN) return;

    if(SPD_EN) Motor_Drive(Dir_offset,Speed_target);
    else Motor_SetSpeed(Open_duty,Open_duty);
    Sent_offset=Dir_offset;
    Sent_speed=Speed_target;
    Sent_duty=Open_duty;
    Sent_speed_en=SPD_EN;
    Sent_diff_gain=DIFF_GAIN;
    drive_sent=1;
}

/***********************************************
 * @brief : 连续确认启动按键并只报告一次按下
 * @param : 无
 * @return: 确认按下返回一，否则返回零
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
static uint8 button_scan(void)
{
    uint8 Raw_level;
    static uint8 Stable=1;
    static uint8 Count=0;
    if(gpio_get_level(BTN1_PIN)==BUTTON_PRESSED
       ||gpio_get_level(BTN2_PIN)==BUTTON_PRESSED
       ||gpio_get_level(BTN3_PIN)==BUTTON_PRESSED)
    {
        Raw_level=0;
    }
    else
    {
        Raw_level=1;
    }
    if(Raw_level==Stable)
    {
        Count=0;
        return 0;
    }
    Count++;
    if(Count<DEBOUNCE_FRAMES) return 0;
    Stable=Raw_level;
    Count=0;
    if(Stable==0) return 1;
    return 0;
}

/***********************************************
 * @brief : 初始化车辆硬件和视觉状态并保持停车
 * @param : 无
 * @return: 无
 ************************************************/
void Car_Init(void)
{
    gpio_init(BTN1_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(BTN2_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(BTN3_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    Motor_Init();
    Servo_Init();
    Encoder_Init();
    mt9v03x_init();
    /* 3ms中断 */
    pit_ms_init(CCU60_CH0,3);
    Vision_Reset();
    Car_UpdateOffset();
    last_error=0;
    last_drive_offset=0;
    text_last_us=system_getval_us();
    drive_sent=0;
    steer_ready=0;
    lost_frames=0;
    last_line_us=system_getval_us();
    car_state=CAR_IDLE;
    Motor_SpeedLoop_Reset();
}

/***********************************************
 * @brief : 处理发车、无起点及图像停更保护，并按圆环状态选择轮速
 * @param : 无
 * @return: 无
 ************************************************/
void Car_Task(void)
{
    static uint32 Last_frame=0;
    static uint32 Last_frame_us=0;
    static uint8 Round_beep_ready=1;
    uint8 Pressed;
    uint8 Vision_new;
    uint32 Now_us;
    uint8 Count_s=0;
    uint8 Run_s=0;
    VisionData Next;
    if(car_state!=CAR_RUNNING) drive_sent=0;
    Pressed=button_scan();
    Now_us=system_getval_us();
    Vision_new=Vision_Read(&Next)&&Next.frame!=Last_frame;
    if(Vision_new)
    {
        if(car_state==CAR_RUNNING&&!CAR_TEST_MODE
           &&Next.round_state==ROUND_FAR
           &&Round_beep_ready)
        {
            Beep_ShortRing();
            Round_beep_ready=0;
        }
        if(Next.round_state==ROUND_NONE) Round_beep_ready=1;
        car_vision=Next;
        Last_frame=Next.frame;
        Last_frame_us=Now_us;
    }
    switch(car_state)
    {
        case CAR_IDLE:
            Motor_Stop();
            if(Pressed)
            {
                car_state=CAR_COUNTDOWN;
                Car_state_us=Now_us;
            }
            break;
        case CAR_COUNTDOWN:
            Motor_Stop();
            {
                uint32 Elapsed=Now_us-Car_state_us;
                if(Elapsed>=(uint32)COUNTDOWN_MS*1000U)
                {
                    /* 发车时重新记时，圆环延时与车辆运行时长共用此起点。 */
                    Car_state_us=Now_us;
                    last_drive_offset=0;
                    steer_ready=0;
                    lost_frames=0;
                    Last_frame_us=Now_us;
                    Motor_SpeedLoop_Reset();
                    Vision_RequestReset();
                    __dsync();
                    car_state=CAR_RUNNING;
                }
                else
                {
                    Count_s=(uint8)((((uint32)COUNTDOWN_MS*1000U)-Elapsed+999999U)/1000000U);
                }
            }
            break;
        case CAR_RUNNING:
        {
            uint32 Run_us=Now_us-Car_state_us;
            int32 Open_duty=(int32)CAR_BASE_SPEED;
            Car_UpdateOffset();
            int16 Speed_target=SPD_BASE;
            if(Open_duty<0) Open_duty=0;
            if(Open_duty>(int32)MOTOR_DUTY_MAX) Open_duty=(int32)MOTOR_DUTY_MAX;
            if(Pressed||(CAR_RUN_S>0&&Run_us>=(uint32)CAR_RUN_S*1000000U))
            {
                car_state=CAR_IDLE;
                Motor_Stop();
                lost_frames=0;
                break;
            }
            if(CAR_RUN_S>0)
            {
                Run_s=(uint8)(((uint32)CAR_RUN_S*1000000U-Run_us+999999U)/1000000U);
            }
            else
            {
                uint32 Run_seconds=Run_us/1000000U;
                if(Run_seconds>99U) Run_seconds=99U;
                Run_s=(uint8)Run_seconds;
            }
            if(!CAR_TEST_MODE&&round_right_done) Speed_target=SPDEND;
            if(!CAR_TEST_MODE&&car_vision.round_state!=ROUND_NONE)
            {
                if(car_vision.round_dir==ROUND_LEFT_DIR)
                    Speed_target=LEFT_ROUND_SPEED;
                else if(car_vision.round_dir==ROUND_RIGHT_DIR)
                    Speed_target=RIGHT_ROUND_SPEED;
            }
            if(CAR_TEST_MODE)
            {
                car_drive(0,Speed_target,Open_duty);
                break;
            }
            if(!Vision_new)
            {
                if((uint32)(Now_us-Last_frame_us)>=VISION_FRAME_LOST_MS*1000U)
                {
                    car_state=CAR_IDLE;
                    Motor_Stop();
                    lost_frames=0;
                }
                else if(steer_ready) car_drive(last_drive_offset,Speed_target,Open_duty);
                break;
            }
            /* 无起点停车 */
            if(car_vision.out)
            {
                car_state=CAR_IDLE;
                Motor_Stop();
                lost_frames=0;
                break;
            }
            /* 中线无效帧数仅用于屏幕观察，不再触发停车。 */
            if(car_vision.valid)
            {
                lost_frames=0;
            }
            else if(lost_frames<65535U) lost_frames++;
            if(steer_ready) car_drive(last_drive_offset,Speed_target,Open_duty);
            break;
        }
        default:
            Motor_Stop();
            car_state=CAR_IDLE;
            break;
    }
    if(car_state==CAR_RUNNING) SHOW_DIV=RACE_SHOW_DIV;
    else SHOW_DIV=1;
    if(car_state!=CAR_RUNNING) drive_sent=0;
    if((uint32)(Now_us-text_last_us)>=100000U) /* 每100ms刷新 */
    {
        text_last_us=Now_us;
        Car_Show_Debug((uint8)car_state,Count_s,Run_s,lost_frames);
    }
}

/***********************************************
 * @brief : 停车并请求CPU1将舵机交给菜单控制
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
static void car_enter_menu(void)
{
    vision_show_enable=0;
    menu_servo_ready=0;
    __dsync();
    menu_open=1;
    car_state=CAR_IDLE;
    drive_sent=0;
    Motor_Stop();
    lost_frames=0;
    last_drive_offset=0;
    Motor_SpeedLoop_Reset();
    ips200_clear();
    Menu_Event_Flush();
    menu_key_event=menu_release;
}

/***********************************************
 * @brief : 扫描菜单输入并执行当前页面或车辆控制
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Car_Loop(void)
{
    MyKey_Scan();
    if(!menu_open)
    {
        vision_show_enable=1;
        if(my_key_get_state(MY_KEY_1)==MY_KEY_LONG_PRESS)
        {
            car_enter_menu();
            return;
        }
        Car_Task();
        Image_Display();
        return;
    }
    Motor_Stop();
    if(!menu_servo_ready) return;
    __dsync();
    Motor_UpdateDisplay(0,SPD_BASE);
    vision_show_enable=0;
    if(menu_top!=NULL
       &&menu_top->next==NULL
       &&my_key_get_state(MY_KEY_1)==MY_KEY_LONG_PRESS)
    {
        menu_open=0;
        if(vision_snapshot_release==2) vision_snapshot_release=1;
        __dsync();
        vision_show_enable=1;
        ips200_clear();/* 不需要互斥 */
        key_clear_all_state();
    }
    else
    {
        Menu_Event_Flush();
        MENU_RUN();/* 不需要互斥 */
    }
}
