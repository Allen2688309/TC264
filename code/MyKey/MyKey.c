#include "MyKey.h"

/* 内部变量 */
int16 switch_encoder_change_num=0;

gpio_pin_enum key_pin[MY_KEY_NUM]=
{
    SWITCH_ENCODER_KEY_PIN,/* MY_KEY_1 */
    MY_BTN_SW5_PIN,/* MY_KEY_2 */
    MY_BTN_SW1_PIN,/* MY_KEY_3 */
    MY_BTN_SW2_PIN /* MY_KEY_4 */
};

uint8 key_stable[MY_KEY_NUM];
uint8 key_cnt[MY_KEY_NUM];
uint32 key_press_us[MY_KEY_NUM];/* 按下去的时刻 */
uint8 key_long_fired[MY_KEY_NUM];
uint8 key_event[MY_KEY_NUM];/* 待读取的动作 */

uint8 enc_last_state=0;
int16 enc_edge_accum=0;
uint8 enc_change_flag=0;

const int8 enc_rot_table[16]=
{
     0,-1,1,0,
     1,0,0,-1,
    -1,0,0,1,
     0,1,-1,0
};

void MyKey_Init(void)
{
    uint8 i;
    uint8 a,b;
    gpio_init(SWITCH_ENCODER_A_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(SWITCH_ENCODER_B_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(SWITCH_ENCODER_KEY_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(MY_BTN_SW5_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(MY_BTN_SW1_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    gpio_init(MY_BTN_SW2_PIN,GPI,GPIO_HIGH,GPI_PULL_UP);
    for(i=0;i<MY_KEY_NUM;i++)
    {
        key_stable[i]=1;
        key_cnt[i]=0;
        key_press_us[i]=0;
        key_long_fired[i]=0;
        key_event[i]=MY_KEY_RELEASE;
    }
    a=gpio_get_level(SWITCH_ENCODER_A_PIN)?1:0;
    b=gpio_get_level(SWITCH_ENCODER_B_PIN)?1:0;
    enc_last_state=(uint8)((a<<1)|b);
    enc_edge_accum=0;
    enc_change_flag=0;
    switch_encoder_change_num=0;
}

void MyKey_Scan(void)
{
    static uint32 last_scan_us=0;
    uint32 now_us=system_getval_us();
    uint8 i;
    uint8 a,b,st;
    int8 d;
    switch_encoder_change_num=0;
    for(i=0;i<MY_KEY_NUM;i++) key_event[i]=MY_KEY_RELEASE;
    if(last_scan_us!=0&&(uint32)(now_us-last_scan_us)<MY_KEY_MIN_GAP_US)
    {
        return;
    }
    last_scan_us=now_us;
    a=gpio_get_level(SWITCH_ENCODER_A_PIN)?1:0;
    b=gpio_get_level(SWITCH_ENCODER_B_PIN)?1:0;
    st=(uint8)((a<<1)|b);
    d=enc_rot_table[(uint8)((enc_last_state<<2)|st)];
    enc_last_state=st;
    if(d!=0)
    {
        enc_edge_accum+=d;
        if(enc_edge_accum>=ENC_EDGES_PER_STEP)
        {
            switch_encoder_change_num+=1;
            enc_edge_accum=0;
            enc_change_flag=1;
        }
        else if(enc_edge_accum<=-ENC_EDGES_PER_STEP)
        {
            switch_encoder_change_num-=1;
            enc_edge_accum=0;
            enc_change_flag=1;
        }
    }
    for(i=0;i<MY_KEY_NUM;i++)
    {
        uint8 raw_level=(gpio_get_level(key_pin[i])==0)?0:1;/* 低电平=按下 */
        if(raw_level!=key_stable[i])
        {
            key_cnt[i]++;
            if(key_cnt[i]>=MY_KEY_DEBOUNCE)
            {
                key_stable[i]=raw_level;
                key_cnt[i]=0;
                if(key_stable[i]==0)
                {
                    key_press_us[i]=now_us;
                    key_long_fired[i]=0;
                }
                else
                {
                    if(key_long_fired[i]==0)
                    {
                        key_event[i]=MY_KEY_SHORT_PRESS;
                    }
                }
            }
        }
        else
        {
            key_cnt[i]=0;
        }
        if(key_stable[i]==0&&key_long_fired[i]==0)
        {
            if((now_us-key_press_us[i])>=(uint32)MY_KEY_LONG_MS*1000U)
            {
                key_event[i]=MY_KEY_LONG_PRESS;
                key_long_fired[i]=1;
            }
        }
    }
}

/* 读取本次动作 */
uint8 my_key_get_state(uint8 key)
{
    if(key>=MY_KEY_NUM) return MY_KEY_RELEASE;
    return key_event[key];
}

uint8 If_Switch_Encoder_Change(void)
{
    uint8 r=enc_change_flag;
    enc_change_flag=0;
    return r;
}
