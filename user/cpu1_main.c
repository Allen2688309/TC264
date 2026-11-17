#include "zf_common_headfile.h"
#include "Camera.h"
#include "car/car.h"
#include "wifi/Wifi_Cam.h"

#pragma section all "cpu1_dsram"

void core1_main(void)
{
    disable_Watchdog();
    interrupt_global_enable(0);
    cpu_wait_event_ready();
    while(TRUE)
    {
        /* 处理停车 */
        Car_SteerTask();
        Image_Show();
        /* 处理新图 */
        Car_SteerTask();
    }
}

#pragma section all restore
