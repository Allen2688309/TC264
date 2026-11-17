#include "servo.h"
/* 限位 */
uint16 SERVO_CENTER_DUTY=4326;/* 中 */
uint16 SERVO_MIN_DUTY=3720;/* 右 */
uint16 SERVO_MAX_DUTY=4932;/* 左 */

volatile uint16 servo_duty=4326;/* 最近限幅后输出 */
/***********************************************
 * @brief : 初始化舵机输出通道并执行中位限幅
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Servo_Init(void)
{
    pwm_init(SERVO_PWM_PIN,SERVO_PWM_HZ,SERVO_CENTER_DUTY);
    Servo_Center();
}

/***********************************************
 * @brief : 按软硬限位输出舵机占空比并跳过重复请求
 * @param : Duty 请求的舵机占空比
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Servo_SetDuty(uint16 Duty)
{
    if(SERVO_MIN_DUTY<=SERVO_MAX_DUTY)
    {
        if(Duty<SERVO_MIN_DUTY) Duty=SERVO_MIN_DUTY;
        if(Duty>SERVO_MAX_DUTY) Duty=SERVO_MAX_DUTY;
    }
    else
    {
        /* 兼容限位填反 */
        if(Duty<SERVO_MAX_DUTY) Duty=SERVO_MAX_DUTY;
        if(Duty>SERVO_MIN_DUTY) Duty=SERVO_MIN_DUTY;
    }
    if(Duty<SERVO_HARD_MIN) Duty=SERVO_HARD_MIN;
    if(Duty>SERVO_HARD_MAX) Duty=SERVO_HARD_MAX;
    if(Duty==servo_duty) return;
    pwm_set_duty(SERVO_PWM_PIN,Duty);
    servo_duty=Duty;
}
/***********************************************
 * @brief : 按现有限位将舵机输出设为中位
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Servo_Center(void)
{
    Servo_SetDuty(SERVO_CENTER_DUTY);
}
