#include "Beep.h"

void Beep_Init(void)
{
    gpio_init(Beep,GPO,0,GPO_PUSH_PULL);
    /* 上电关闭蜂鸣器。 */
    Beep_Stop();
}

void Beep_Start(void)
{
    gpio_set_level(Beep,1);
}

void Beep_Stop(void)
{
    gpio_set_level(Beep,0);
}

void Beep_ShortRing(void)
{
    Beep_Start();
    system_delay_ms(SHORT_RING_TIME);
    Beep_Stop();
}

