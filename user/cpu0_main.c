#include "zf_common_headfile.h"
#include "MyHeadFile.h"
#include "menu/menu.h"
#include "cpu0_main.h"
#include "wifi/Wifi_Cam.h"

#pragma section all "cpu0_dsram"

int core0_main(void)
{
    disable_Watchdog();
    clock_init();
    system_start();
    debug_init();
    ips200_init(IPS200_TYPE_SPI);
    Car_Init();
    MyKey_Init();
    Menu_Init();
    cpu_wait_event_ready();

    while(TRUE)
    {
        Car_Loop();
    }
}

#pragma section all restore
