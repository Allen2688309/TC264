#include "motor.h"
#include "MyEncoder/MyEncoder.h"
#include "car.h"

float Pi_out[MOTOR_WHEEL_COUNT];       /* 各轮累计输出 */
float Pi_err_last[MOTOR_WHEEL_COUNT];  /* 各轮上次误差 */
uint8 Pi_started[MOTOR_WHEEL_COUNT];   /* 各轮初始化标志 */
uint16 Pi_dead[MOTOR_WHEEL_COUNT];     /* 各轮低速计数 */
uint8 Pi_moved[MOTOR_WHEEL_COUNT];     /* 各轮转动标志 */

volatile int16 spd_out_l=0;/* 左轮当前输出 */
volatile int16 spd_out_r=0;/* 右轮当前输出 */
volatile int16 spd_tgt_l=0;
volatile int16 spd_tgt_r=0;
int16 spd_diff=0;

int16 spd_diff_peak=0;
volatile uint8 motor_run_enable=0;

/***********************************************
 * @brief : 限制单轮输出并设置电机方向和占空比
 * @param : Pwm_pin 电机占空比通道
 * @param : Dir_pin 电机方向引脚
 * @param : Duty 有符号电机占空比
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void motor_set_one(uint32 Pwm_pin,uint32 Dir_pin,int32 Duty)
{
    /* 限幅 */
    if(Duty>(int32)MOTOR_DUTY_MAX) Duty=(int32)MOTOR_DUTY_MAX;
    if(Duty<-(int32)MOTOR_DUTY_MAX) Duty=-(int32)MOTOR_DUTY_MAX;
    /* 电机方向 */
    if(Duty>=0)
    {
        gpio_set_level(Dir_pin,GPIO_HIGH);/* 高电平=正转 */
        pwm_set_duty(Pwm_pin,(uint32)Duty);
    }
    else
    {
        gpio_set_level(Dir_pin,GPIO_LOW);/* 低电平=反转 */
        pwm_set_duty(Pwm_pin,(uint32)(-Duty));/* 取绝对值 */
    }
}

/***********************************************
 * @brief : 初始化两轮电机输出并保持停车
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_Init(void)
{
    /* PWM初始化 */
    pwm_init(MOTOR_L_PWM_PIN,MOTOR_PWM_HZ,0);
    pwm_init(MOTOR_R_PWM_PIN,MOTOR_PWM_HZ,0);
    gpio_init(MOTOR_L_DIR_PIN,GPO,GPIO_HIGH,GPO_PUSH_PULL);
    gpio_init(MOTOR_R_DIR_PIN,GPO,GPIO_HIGH,GPO_PUSH_PULL);
    Motor_Stop();
}
/***********************************************
 * @brief : 分别设置左右轮电机占空比
 * @param : Left_duty 左轮有符号占空比
 * @param : Right_duty 右轮有符号占空比
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_SetSpeed(int32 Left_duty,int32 Right_duty)
{
    motor_set_one(MOTOR_L_PWM_PIN,MOTOR_L_DIR_PIN,Left_duty);
    motor_set_one(MOTOR_R_PWM_PIN,MOTOR_R_DIR_PIN,Right_duty);
}

/***********************************************
 * @brief : 停用速度控制并立即关闭两轮输出
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_Stop(void)
{
    motor_run_enable=0;
    pwm_set_duty(MOTOR_L_PWM_PIN,0);
    pwm_set_duty(MOTOR_R_PWM_PIN,0);
    spd_out_l=0;
    spd_out_r=0;
}

/***********************************************
 * @brief : 根据编码器速度更新左右轮增量式速度环
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_SpeedTick(void)
{
    static uint8 Pwm_zero_latch=0;
    /* 开环直接控PWM */
    if(!SPD_EN) return;
    if(!motor_run_enable)
    {
        if(Pwm_zero_latch==0)
        {
            pwm_set_duty(MOTOR_L_PWM_PIN,0);
            pwm_set_duty(MOTOR_R_PWM_PIN,0);
            Pwm_zero_latch=1;
        }
        return;
    }
    Pwm_zero_latch=0;
    /* 两环各自闭环 */
    spd_out_l=(int16)spd_pid_run(MOTOR_LEFT,(float)spd_tgt_l,(float)enc_speed_l);
    spd_out_r=(int16)spd_pid_run(MOTOR_RIGHT,(float)spd_tgt_r,(float)enc_speed_r);
    Motor_SetSpeed((int32)spd_out_l,(int32)spd_out_r);
}

uint8 SPD_EN=1;
int16 SPD_BASE=CAR_DEFAULT_STRAIGHT;/* 普通赛道目标速度 */
int16 SPD_TRIM=2000;/* 前馈的最大修正量 */
int16 SPD_BRAKE_TRIM=0;
int16 SPD_DEAD_SPD=5;/* 速度阈值 */
int16 SPD_DEAD_FRAMES=100;/* 失效检测计数上限 */
float DIFF_GAIN=CAR_DEFAULT_DIFF_GAIN;/* 后轮差速系数 */
float SPD_TUNE_KP=CAR_DEFAULT_SPEED_KP;/* 共用速度比例 */
float SPD_TUNE_KI=CAR_DEFAULT_SPEED_KI;/* 共用速度积分 */

/***********************************************
 * @brief : 将速度环输出限制在本次计算的上下限内
 * @param : Output 待限制的累计输出
 * @param : Low 本次允许的输出下限
 * @param : High 本次允许的输出上限
 * @return: 限幅后的速度环输出
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
float Motor_LimitOut(float Output,float Low,float High)
{
    if(Output>High) Output=High;
    if(Output<Low) Output=Low;
    return Output;
}

/***********************************************
 * @brief : 用共用参数计算单轮增量式PI并限制输出
 * @param : Side 左轮或右轮编号
 * @param : Target 本次目标速度
 * @param : Actual 本次编码器速度
 * @return: 本轮电机输出占空比
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
int32 spd_pid_run(uint8 Side,float Target,float Actual)
{
    float Err,Delta,High,Low;
    High=(float)CAR_BASE_SPEED+(float)SPD_TRIM;
    Low=(float)CAR_BASE_SPEED-(float)SPD_TRIM;
    if(High>(float)MOTOR_DUTY_MAX) High=(float)MOTOR_DUTY_MAX;
    /* 输出下限 */
    if(Low<-(float)SPD_BRAKE_TRIM) Low=-(float)SPD_BRAKE_TRIM;
    if(Low>High) Low=High;
    Err=Target-Actual;
    if(!Pi_started[Side])
    {
        Pi_out[Side]=Motor_LimitOut((float)CAR_BASE_SPEED,Low,High);
        Pi_err_last[Side]=Err;
        Pi_started[Side]=1;
        Pi_dead[Side]=0;
        Pi_moved[Side]=0;
    }
    /* 增量式PI */
    Delta=SPD_TUNE_KP*(Err-Pi_err_last[Side])+SPD_TUNE_KI*Err;
    Pi_err_last[Side]=Err;
    /* 条件积分限幅 */
    if(!((Pi_out[Side]>=High&&Delta>0.0f)||(Pi_out[Side]<=Low&&Delta<0.0f)))
    {
        Pi_out[Side]+=Delta;
    }
    Pi_out[Side]=Motor_LimitOut(Pi_out[Side],Low,High);
    if(Actual>=(float)SPD_DEAD_SPD||Actual<=-(float)SPD_DEAD_SPD)
    {
        Pi_moved[Side]=1;
        Pi_dead[Side]=0;
    }
    else if(SPD_DEAD_FRAMES>0&&Pi_moved[Side]&&(Pi_out[Side]>(float)CAR_BASE_SPEED))
    {
        Pi_dead[Side]++;
        if(Pi_dead[Side]>(uint16)SPD_DEAD_FRAMES)
        {
            Pi_out[Side]=(float)CAR_BASE_SPEED;
            Pi_dead[Side]=(uint16)SPD_DEAD_FRAMES;
        }
    }
    else
    {
        Pi_dead[Side]=0;
    }
    return (int32)Pi_out[Side];
}

/***********************************************
 * @brief : 在速度中断暂停期间重置两轮控制状态
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_SpeedLoop_Reset(void)
{
    uint8 Side;
    uint32 Irq_state=interrupt_global_disable();
    motor_run_enable=0;
    for(Side=0;Side<MOTOR_WHEEL_COUNT;Side++)
    {
        Pi_out[Side]=(float)CAR_BASE_SPEED;
        Pi_err_last[Side]=0.0f;
        Pi_started[Side]=0;
        Pi_dead[Side]=0;
        Pi_moved[Side]=0;
    }
    spd_out_l=spd_out_r=0;
    spd_tgt_l=spd_tgt_r=0;
    spd_diff=0;
    spd_diff_peak=0;
    __dsync();
    interrupt_global_enable(Irq_state);
}

/***********************************************
 * @brief : 根据转向差速成对发布轮速目标并记录峰值
 * @param : Dir_offset 舵机相对中位的偏移
 * @param : Base_target 左右轮平均目标速度
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_UpdateDisplay(int32 Dir_offset,int16 Base_target)
{
    float Diff;
    int16 Target_l,Target_r;
    uint32 Irq_state;
    if(Base_target<0) Base_target=0;
    if(Base_target>600) Base_target=600;

    Diff=DIFF_GAIN*(float)Dir_offset;

    if(Diff>(float)Base_target*0.5f) Diff=(float)Base_target*0.5f;
    if(Diff<-(float)Base_target*0.5f) Diff=-(float)Base_target*0.5f;
    Target_l=(int16)((float)Base_target-Diff);
    Target_r=(int16)((float)Base_target+Diff);
    /* 同时发布目标 */
    Irq_state=interrupt_global_disable();
    spd_tgt_l=Target_l;
    spd_tgt_r=Target_r;
    __dsync();
    interrupt_global_enable(Irq_state);
    spd_diff=(int16)Diff;
    if(spd_diff>spd_diff_peak) spd_diff_peak=spd_diff;
    else if(-spd_diff>spd_diff_peak) spd_diff_peak=(int16)(-spd_diff);
}

/***********************************************
 * @brief : 发布左右轮目标速度并启用速度控制
 * @param : Dir_offset 舵机相对中位的偏移
 * @param : Base_target 左右轮平均目标速度
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Motor_Drive(int32 Dir_offset,int16 Base_target)
{
    Motor_UpdateDisplay(Dir_offset,Base_target);
    motor_run_enable=1;
}
