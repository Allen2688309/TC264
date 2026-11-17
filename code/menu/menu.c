#include "menu.h"
#include "ROUND.h"

/***********************************************
 * @brief : 限制显示位数后绘制有符号整数
 * @param : X 显示横坐标
 * @param : Y 显示纵坐标
 * @param : Value 待显示整数
 * @param : Digits 显示数字位数
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void My_Show_Int(uint16 X,uint16 Y,int32 Value,uint8 Digits)
{
    if(Digits<1) Digits=1;
    if(Digits>10) Digits=10;
    ips200_show_int(X,Y,Value,Digits);
}

uint8 menu_key_event=menu_release;
uint8 menu_update_flag=0;
uint8 menu_editing=0;
uint8 menu_line=0;
uint8 menu_redraw=0;
MenuPage_Linked_List*menu_top=NULL;

/* 页面函数原型 */
void Ahead_Page_Init(void);
void Elem_Page_Init(void);
void Status_Page_Init(void);
void Steer_Page_Init(void);
void Servo_Page_Init(void);
void Servo_Page_ShowNow(void);
void Speed_Page_Init(void);
void Status_Page_Show(void);
void Car_Mode_Toggle(void);
void Round_Left_Time_Page_Init(void);

/***********************************************
 * @brief : 显示视觉状态和图像处理耗时
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Status_Page_Show(void)
{
    ips200_set_color(RGB565_WHITE,RGB565_BLACK);
    ips200_show_string(2,36,"ERR   ");My_Show_Int(120,36,(int32)vision_error,6);
    ips200_show_string(2,54,"RAW   ");My_Show_Int(120,54,(int32)vision_error_raw,6);
    ips200_show_string(2,72,"VALID ");My_Show_Int(120,72,(int32)vision_valid,3);
    ips200_show_string(2,90,"OUT   ");My_Show_Int(120,90,(int32)vision_lost,3);
    ips200_show_string(2,108,"ROAD  ");My_Show_Int(120,108,(int32)vision_road_type,3);
    ips200_show_string(2,126,"RND   ");My_Show_Int(120,126,(int32)vision_round_state,3);
    ips200_show_string(2,144,"DIR   ");My_Show_Int(120,144,(int32)vision_round_dir,3);
    ips200_show_string(2,162,"LLOST ");My_Show_Int(120,162,(int32)vision_left_lost,6);
    ips200_show_string(2,180,"RLOST ");My_Show_Int(120,180,(int32)vision_right_lost,6);
    ips200_show_string(2,198,"OTSU ");My_Show_Int(52,198,(int32)T_OTSU,5);
    ips200_show_string(2,216,"TRACE");My_Show_Int(52,216,(int32)T_TRACE,5);
    ips200_show_string(124,216,"FILL ");My_Show_Int(172,216,(int32)T_FILL,5);
    ips200_show_string(2,234,"ELEM ");My_Show_Int(52,234,(int32)T_ELEM,5);
    ips200_show_string(124,234,"SHOW ");My_Show_Int(172,234,(int32)T_SHOW,5);
    ips200_show_string(2,252,"FPS ");My_Show_Int(42,252,(int32)vision_fps,5);
    ips200_show_string(124,252,"CTO ");My_Show_Int(164,252,(int32)vision_camera_timeouts,5);
}

void Status_Page_Init(void)
{
    static MenuLine LineList[]={
            MENU_ITEM_STATIC_FUNC("(live)  ",Status_Page_Show),
            {".",}
    };
    static MenuPage Page={"STATUS",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

#define SERVO_CAL_MIN (3300)
#define SERVO_CAL_MAX (5400)

/***********************************************
 * @brief : 用旋转编码器调整舵机参数并在结束后回中
 * @param : val 待调整的舵机占空比参数
 * @param : title 校准页面标题
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void servo_calib_live(uint16*val,const char*title)
{
    uint8 stop=0;
    ips200_clear();
    ips200_set_color(RGB565_WHITE,RGB565_BLACK);
    ips200_show_string_centered(0,title);
    ips200_show_string(2,44,"VALUE   ");
    ips200_show_string(2,80,"TURN = +/-");
    ips200_show_string(2,98,"PRESS = OK");
    Servo_SetDuty(*val);
    My_Show_Int(120,44,(int32)(*val),6);
    while(!stop)
    {
        MyKey_Scan();
        if(If_Switch_Encoder_Change())
        {
            int32 v=(int32)(*val)+(int32)switch_encoder_change_num;
            if(v<SERVO_CAL_MIN) v=SERVO_CAL_MIN;
            if(v>SERVO_CAL_MAX) v=SERVO_CAL_MAX;
            *val=(uint16)v;
            Servo_SetDuty(*val);
            My_Show_Int(120,44,(int32)(*val),6);
        }
        if((my_key_get_state(MY_KEY_1)!=MY_KEY_RELEASE)||
            (my_key_get_state(MY_KEY_2)!=MY_KEY_RELEASE)||
            (my_key_get_state(MY_KEY_3)!=MY_KEY_RELEASE)||
            (my_key_get_state(MY_KEY_4)!=MY_KEY_RELEASE))
        {
            stop=1;
        }
        system_delay_ms(15);
    }
    Servo_Center();
    MyKey_Scan();
    menu_key_event=menu_release;
    menu_update_flag=1;/* 重画菜单 */
    ips200_clear();
    Beep_ShortRing();
}

void Servo_Calib_Center(void) { servo_calib_live(&SERVO_CENTER_DUTY,"CAL CENTER");}
void Servo_Calib_Min(void) { servo_calib_live(&SERVO_MIN_DUTY,"CAL MIN   ");}
void Servo_Calib_Max(void) { servo_calib_live(&SERVO_MAX_DUTY,"CAL MAX   ");}

/***********************************************
 * @brief : 显示舵机限位参数和最近输出
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Servo_Page_ShowNow(void)
{
    ips200_set_color(RGB565_WHITE,RGB565_BLACK);
    ips200_show_string(2,90,"CENTER");
    ips200_show_string(120,90,"      ");
    My_Show_Int(120,90,(int32)SERVO_CENTER_DUTY,6);
    ips200_show_string(2,108,"MIN   ");
    ips200_show_string(120,108,"      ");
    My_Show_Int(120,108,(int32)SERVO_MIN_DUTY,6);
    ips200_show_string(2,126,"MAX   ");
    ips200_show_string(120,126,"      ");
    My_Show_Int(120,126,(int32)SERVO_MAX_DUTY,6);
    ips200_show_string(2,144,"NOW   ");
    ips200_show_string(120,144,"      ");
    My_Show_Int(120,144,(int32)servo_duty,6);
}

void Servo_Page_Init(void)
{
    static MenuLine LineList[]={
            MENU_ITEM_ENTER_FUNC ("CAL CENTER",Servo_Calib_Center),
            MENU_ITEM_ENTER_FUNC ("CAL MIN   ",Servo_Calib_Min),
            MENU_ITEM_ENTER_FUNC ("CAL MAX   ",Servo_Calib_Max),
            MENU_ITEM_STATIC_FUNC("VALUES    ",Servo_Page_ShowNow),
            {".",}
    };
    static MenuPage Page={"SERVO",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

/***********************************************
 * @brief : 显示左右轮目标速度和实际速度及输出
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Speed_Page_Show(void)
{
    ips200_set_color(RGB565_WHITE,RGB565_BLACK);
    ips200_show_string(0,198,"MODE");
    ips200_show_string(56,198,"                ");
    ips200_show_string(56,198,CAR_MODE==CAR_MODE_SPEED_TUNE?"SPEED HOLD":"DRIVE");
    ips200_show_string(0,216,"L_T");ips200_show_string(56,216,"     ");My_Show_Int(56,216,(int32)spd_tgt_l,5);
    ips200_show_string(120,216,"L_N");ips200_show_string(176,216,"     ");My_Show_Int(176,216,(int32)enc_speed_l,5);
    ips200_show_string(0,234,"R_T");ips200_show_string(56,234,"     ");My_Show_Int(56,234,(int32)spd_tgt_r,5);
    ips200_show_string(120,234,"R_N");ips200_show_string(176,234,"     ");My_Show_Int(176,234,(int32)enc_speed_r,5);
    ips200_show_string(0,252,"L_O");ips200_show_string(56,252,"     ");My_Show_Int(56,252,(int32)spd_out_l,5);
    ips200_show_string(120,252,"R_O");ips200_show_string(176,252,"     ");My_Show_Int(176,252,(int32)spd_out_r,5);
    ips200_show_string(0,270,"DIF");ips200_show_string(56,270,"     ");My_Show_Int(56,270,(int32)spd_diff,5);
    ips200_show_string(120,270,"DPK");ips200_show_string(176,270,"     ");My_Show_Int(176,270,(int32)spd_diff_peak,5);
}

/***********************************************
 * @brief : 打开共用速度环参数及运行数据页面
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Speed_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_ENTER_FUNC("MODE TOGGLE",Car_Mode_Toggle),
        MENU_ITEM_INT_EDIT("PWM_FF",&CAR_BASE_SPEED,10,1),
        MENU_ITEM_INT_EDIT("PI_LIMIT",&SPD_TRIM,10,1),
        MENU_ITEM_FLOAT_EDIT("TUNE_KP",&SPD_TUNE_KP,0.1f),
        MENU_ITEM_FLOAT_EDIT("TUNE_KI",&SPD_TUNE_KI,0.1f),
        MENU_ITEM_STATIC_FUNC("(live)",Speed_Page_Show),
        {".",}
    };
    static MenuPage Page={"SPEED PI",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

void Steer_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_FLOAT_EDIT("KP",&STEER_KP,10.0f),
        MENU_ITEM_FLOAT_EDIT("KD",&STEER_KD,10.0f),
        MENU_ITEM_INT_EDIT("TURN_MAX",&STEER_OFFSET_LIMIT,1,1),
        MENU_ITEM_PAGE_JUMP("SERVO >",Servo_Page_Init),
        {".",}
    };
    static MenuPage Page={"STEER",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

/***********************************************
 * @brief : 打开前瞻范围参数页面
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Ahead_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_INT_EDIT("ERR_TOP",&ERR_TOP,1,1),
        MENU_ITEM_INT_EDIT("ERR_BOTTOM",&ERR_BOTTOM,1,1),
        {".",}
    };
    static MenuPage Page={"IMAGE",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

/* 左环阶段时间 */
void Round_Left_Time_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_INT_EDIT("R3_MIN_MS",&ROUND_LEFT_INTO_MIN_MS,100,1),
        MENU_ITEM_INT_EDIT("R3_MAX_MS",&ROUND_LEFT_INTO_MAX_MS,100,1),
        MENU_ITEM_INT_EDIT("R4_MIN_MS",&ROUND_LEFT_RND4_MIN_MS,100,1),
        MENU_ITEM_INT_EDIT("R4_MAX_MS",&ROUND_LEFT_RND4_MAX_MS,100,1),
        MENU_ITEM_INT_EDIT("R5_MAX_MS",&ROUND_LEFT_RND5_MAX_MS,100,1),
        MENU_ITEM_INT_EDIT("R6_MAX_MS",&ROUND_LEFT_ALIGN_MS,100,1),
        MENU_ITEM_INT_EDIT("R7_MAX_MS",&ROUND_LEFT_LEAVE_MS,100,1),
        {".",}
    };
    static MenuPage Page={"LEFT TIME",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

/***********************************************
 * @brief : 打开圆环识别参数和左右环阶段时间设置
 * @param : 无
 * @return: 无
 * @date  : 2026-11-08
 * @author: L
 ************************************************/
void Elem_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_INT_EDIT("ROUND_LOST",&ROUND_LOST_MIN,1,1),
        MENU_ITEM_INT_EDIT("LEFT_DELAY_S",&ROUND_LEFT_START_DELAY_S,1,1),
        MENU_ITEM_INT_EDIT("RIGHT_DELAY_S",&ROUND_RIGHT_START_DELAY_S,1,1),
        MENU_ITEM_INT_EDIT("R_R2_MIN_MS",&Right_rnd2_min_ms,100,1),
        MENU_ITEM_INT_EDIT("R_R3_MIN_MS",&Right_rnd3_min_ms,100,1),
        MENU_ITEM_PAGE_JUMP("LEFT TIME >",Round_Left_Time_Page_Init),
        {".",}
    };
    static MenuPage Page={"ROUND",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

/***********************************************
 * @brief : 打开无起点保护的连续帧数设置页面
 * @param : 无
 * @return: 无
 ************************************************/
void Protect_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_INT_EDIT("NO_START_FR",&LOST_NOSTART_FRAMES,1,1),
        {".",}
    };
    static MenuPage Page={"PROTECT",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

/***********************************************
 * @brief : 建立主菜单并绑定运行参数及三档参数恢复入口
 * @param : 无
 * @return: 无
 ************************************************/
void Main_Page_Init(void)
{
    static MenuLine LineList[]={
        MENU_ITEM_INT_EDIT("STRAIGHT",&SPD_BASE,1,1),
        MENU_ITEM_INT_EDIT("LEFT_RND",&LEFT_ROUND_SPEED,1,1),
        MENU_ITEM_INT_EDIT("RIGHT_RND",&RIGHT_ROUND_SPEED,1,1),
        MENU_ITEM_INT_EDIT("SPDEND",&SPDEND,1,1),
        MENU_ITEM_FLOAT_EDIT("DIFF_GAIN",&DIFF_GAIN,0.01f),
        MENU_ITEM_INT_EDIT("RUN_S",&CAR_RUN_S,1,1),
        MENU_ITEM_ENTER_FUNC("RESET SLOW",Car_ResetSlow),
        MENU_ITEM_ENTER_FUNC("RESET MID",Car_ResetParams),
        MENU_ITEM_ENTER_FUNC("RESET HIGH",Car_ResetHigh),
        MENU_ITEM_PAGE_JUMP("STEER >",Steer_Page_Init),
        MENU_ITEM_PAGE_JUMP("SPEED PI >",Speed_Page_Init),
        MENU_ITEM_PAGE_JUMP("IMAGE >",Ahead_Page_Init),
        MENU_ITEM_PAGE_JUMP("ROUND >",Elem_Page_Init),
        MENU_ITEM_PAGE_JUMP("PROTECT >",Protect_Page_Init),
        MENU_ITEM_PAGE_JUMP("STATUS >",Status_Page_Init),
        {".",}
    };
    static MenuPage Page={"MAIN",.line=LineList,.open_status=0};
    Menu_Push_Node(&Page);
}

void Menu_Init(void)
{
    Beep_Init();
    Main_Page_Init();
}

/***********************************************
 * @brief : 处理菜单动作并刷新菜单及实时数据
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void MENU_RUN(void)
{
    MenuPage*page;
    MenuLine*line;
    uint8 i,redraw;
    uint16 y,color;
    float value;
    my_assert(menu_top!=NULL);
    Menu_Key_Process();
    page=menu_top->page;
    if(!page->open_status)
    {
        Menu_CountLines(page);
        menu_line=page->line_num;
        ips200_show_string_centered(0,page->page_name);
    }
    redraw=menu_redraw||menu_key_event!=menu_release
           ||!page->open_status||menu_update_flag;
    if(redraw)
    {
        menu_key_event=menu_release;
        menu_redraw=0;
        page->open_status=1;
        menu_update_flag=0;
        for(i=0;i<page->line_num_max;i++)
        {
            line=&page->line[i];
            y=(i+1)*18;
            color=i+1==menu_line?SELECTED_BRUSH:DEFAULT_BRUSH;
            if(line->line_name) ips200_show_string_color(0,y,line->line_name,color);
            if(line->line_type==FLOAT_VALUE_EDIT_TYPE)
            {
                value=line->data.float_edit.value?*line->data.float_edit.value:0;
                value+=value>0?0.000001:-0.000001;
                if(!line->data.float_edit.value) value=0.000001;
                ips200_show_float_color(120,y,value,6,4,
                                   line->data.float_edit.value?color:DEFAULT_BRUSH);
            }
            else if(line->line_type==INT_VALUE_EDIT_TYPE)
            {
                if(line->data.int_edit.value)
                    ips200_show_int_color(120,y,*line->data.int_edit.value,10,color);
                else ips200_show_int_color(120,y,0,0,DEFAULT_BRUSH);
            }
        }
    }
    for(i=0;i<page->line_num_max;i++)
    {
        line=&page->line[i];
        if(line->line_type!=STATIC_FUNC_RUN_TYPE) continue;
        my_assert(line->action.run!=NULL);
        line->action.run();
    }
}

/* 读取菜单按键 */
void Menu_Event_Flush(void)
{
    if(If_Switch_Encoder_Change()==1)
    {
        if(menu_editing==0)
        {
            menu_line+=switch_encoder_change_num;
            Menu_LimitLine(&menu_line,menu_top->page->line_num_max);
            menu_redraw=1;
        }
    }
    if(my_key_get_state(MENU_KEY)==MY_KEY_SHORT_PRESS)
    {
        menu_key_event=menu_yes;
    }
    else if(my_key_get_state(MENU_KEY)==MY_KEY_LONG_PRESS)
    {
        menu_key_event=menu_back;
    }

}
/* 执行菜单按键动作 */
void Menu_Key_Process(void)
{
    MenuLine*line;
    my_assert(menu_top!=NULL);
    if(menu_key_event==menu_back)
    {
        Menu_Pop_Node();
        return;
    }
    if(menu_key_event!=menu_yes) return;
    if(menu_line<1||menu_line>menu_top->page->line_num_max)
        return;
    line=&menu_top->page->line[menu_line-1];
    switch(line->line_type)
    {
    case ENTER_FUNC_RUN_TYPE:
    case PAGE_JUMP_TYPE:
        my_assert(line->action.run!=NULL);
        line->action.run();
        break;
    case FLOAT_VALUE_EDIT_TYPE:
        my_assert(line->action.edit_float!=NULL);
        line->action.edit_float(line->data.float_edit.value,line->data.float_edit.step);
        break;
    case INT_VALUE_EDIT_TYPE:
        my_assert(line->action.edit_int!=NULL);
        line->action.edit_int(line->data.int_edit.value,line->data.int_edit.step,
                              line->data.int_edit.row);
        break;
    default:break;
    }
}

/***********************************************
 * @brief : 保存当前选择并打开新的菜单页面
 * @param : new_page 待打开的菜单页面
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Menu_Push_Node(MenuPage*new_page)
{
    ips200_clear();
    my_assert(new_page!=NULL);
    MenuPage_Linked_List*q=(MenuPage_Linked_List*)malloc(sizeof(MenuPage_Linked_List));
    my_assert(q!=NULL);
    q->page=new_page;
    q->next=NULL;
    if(menu_top==NULL)
    {
        menu_top=q;
    }
    else
    {
        menu_top->page->line_num=menu_line;
        q->next=menu_top;
        menu_top=q;
    }
}

/***********************************************
 * @brief : 关闭当前页面并恢复上一级菜单
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Menu_Pop_Node(void)
{
    MenuPage_Linked_List*p;
    my_assert(menu_top!=NULL);
    if(menu_top->next==NULL)
        return;
    ips200_clear();
    menu_top->page->open_status=0;
    menu_top->page->line_num=menu_line;
    p=menu_top->next;
    free(menu_top);
    menu_top=p;
    menu_top->page->open_status=0;
}

/* 获取页面行数 */
uint8 Menu_CountLines(MenuPage*Page)
{
    uint8 i=0;
    if(Page->line_num_max!=0)
    {
        return Page->line_num_max;
    }
    else
    {
        Page->line_num=1;
        for(;Page->line[i].line_name[0]!='.';i++);
        Page->line_num_max=i;
        return Page->line_num_max;
    }
}

/* 限制菜单行号 */
void Menu_LimitLine(uint8*line,uint8 line_max)
{
    if(*line>line_max)
    {
        *line=1;
    }
    else if(*line<1)
    {
        *line=line_max;
    }
}

/* 参数编辑内部函数 */
void menu_clear_edit(uint8*start_y);
void menu_draw_float(float*value,uint16 start_y,bool editing);
void menu_draw_arrow(uint16 start_y,uint16 line_num);

#define VAL_VFG_SHOW_NUM_BIT 6
#define VAL_VFG_SHOW_POINT_BIT 5
#define VAL_VFG_SHOW_TOTAL_LINE 6
#define LINE_MAX VAL_VFG_SHOW_TOTAL_LINE-1
uint8 edit_above=0;
float basic_1,basic_10,basic_100;
uint8 edit_line;

void menu_read_key(void)
{
    uint8 e;
    MyKey_Scan();
    if(menu_key_event==menu_release)
    {
        e=my_key_get_state(MENU_KEY);
        if(e==MY_KEY_SHORT_PRESS) menu_key_event=menu_yes;
        else if(e==MY_KEY_LONG_PRESS) menu_key_event=menu_back;
    }
}

void menu_edit_float(float*value,float step)
{
    menu_top->page->line_num=menu_line;
    menu_editing=1;
    uint8 flush_flag;
    uint8 done=0;
    edit_line=1;
    bool editing=0;
    basic_1=step;
    basic_10=step/10;
    basic_100=step/100;
    uint8 start_y;
    start_y=menu_line*18;
    if(start_y>(319-(10+18+18*VAL_VFG_SHOW_TOTAL_LINE)))
    {
        edit_above=1;
    }
    else edit_above=0;
    menu_clear_edit(&start_y);
    ips200_set_color(RGB565_WHITE,RGB565_BLACK);
    menu_draw_float(value,start_y,editing);
    menu_key_event=menu_release;
    while(done==0)
    {
        menu_read_key();
        flush_flag=menu_key_event!=menu_release;
        if(menu_key_event==menu_back)
        {
            menu_key_event=menu_release;
            done=1;
            ips200_clear();
            menu_editing=0;
            menu_top->page->open_status=0;
        }
        if((flush_flag==1||switch_encoder_change_num!=0)&&done==0)
        {
            if(menu_key_event==menu_yes)
            {
                menu_key_event=menu_release;
                switch(edit_line)
                {
                    case 1:
                    case 2:
                    case 3:
                        editing=!editing;
                        break;
                    case 4:
                        *value=0;
                        Beep_ShortRing();
                        break;
                    case 5:
                        done=1;
                        menu_editing=0;
                        menu_top->page->open_status=0;
                        break;
                }
            }
            if(If_Switch_Encoder_Change()==1)
            {
                if(editing==0)
                {
                    edit_line+=switch_encoder_change_num;
                    Menu_LimitLine(&edit_line,LINE_MAX);
                }
                else
                {
                    switch(edit_line)
                   {
                       case 1:
                           *value+=(basic_1*switch_encoder_change_num);
                           break;
                       case 2:
                           *value+=basic_10*switch_encoder_change_num;
                           break;
                       case 3:
                           *value+=basic_100*switch_encoder_change_num;
                           break;
                   }
                }
            }
            menu_draw_float(value,start_y,editing);
        }
    }
    ips200_clear();
}

/* 清理参数编辑页 */
void menu_clear_edit(uint8*start_y)
{
    uint16 i=0;
    if(edit_above==0)
    {
        for(i=*start_y+18;i<*start_y+10+18+VAL_VFG_SHOW_TOTAL_LINE*18;i++)
        {
            ips200_draw_line(0,i,239,i,RGB565_BLACK);
        }
        *start_y=*start_y+5+18*1;
    }
    else if(edit_above==1)
    {
        for(i=*start_y;i>*start_y-10-18*VAL_VFG_SHOW_TOTAL_LINE;i--)
        {
            ips200_draw_line(0,i,239,i,RGB565_BLACK);
        }
        *start_y=*start_y-5-18*VAL_VFG_SHOW_TOTAL_LINE;
    }
}

/* 刷新浮点参数 */
void menu_draw_float(float*value,uint16 start_y,bool editing)
{
    float steps[3]={basic_1,basic_10,basic_100};
    uint8 i;
    uint16 color;
    ips200_show_string_color(0,2,editing?"EDIT  TURN=CHG PRS=OK":"SEL   TURN=MOVE PRS=GO",
                             editing?RGB565_RED:RGB565_GREEN);
    ips200_show_float(0,start_y,*value+(*value>0?0.000001:-0.000001),
                      VAL_VFG_SHOW_NUM_BIT,VAL_VFG_SHOW_POINT_BIT);
    menu_draw_arrow(start_y,edit_line);
    for(i=0;i<3;i++)
    {
        if(editing)
        {
            color=edit_line==i+1?RGB565_RED:RGB565_WHITE;
            ips200_show_float_color(18,start_y+18*(i+1),steps[i]+0.0000001,
                                    VAL_VFG_SHOW_NUM_BIT,VAL_VFG_SHOW_POINT_BIT,color);
        }
        else ips200_show_float(18,start_y+18*(i+1),steps[i]+0.0000001,
                               VAL_VFG_SHOW_NUM_BIT,VAL_VFG_SHOW_POINT_BIT);
    }
    if(editing)
    {
        ips200_show_string_color(18,start_y+72,"empty",
                                 edit_line==4?RGB565_RED:RGB565_WHITE);
        ips200_show_string_color(18,start_y+90,edit_line==5?"exit":"  ",
                                 edit_line==5?RGB565_RED:RGB565_WHITE);
    }
    else
    {
        ips200_show_string(18,start_y+90,"  ");
        ips200_show_string(18,start_y+72,"empty");
    }
    key_clear_all_state();
}

/* 显示参数编辑箭头 */
void menu_draw_arrow(uint16 start_y,uint16 line_num)
{
    for(uint16 i=1;i<=LINE_MAX;i++)
    {
        if(i==line_num)
        {
            ips200_show_string(0,start_y+i*18,"->");
        }
        else
        {
            /* 其他行显示空格 */
            ips200_show_string(0,start_y+i*18,"  ");
        }
    }
}

void menu_draw_int(int16*value,uint16 start_y,bool editing,
                            int16 basic_1,int16 basic_10,int16 basic_100)
{
    int16 steps[3]={basic_1,basic_10,basic_100};
    uint8 i;
    uint16 color;
    ips200_show_string_color(0,2,editing?"EDIT  TURN=CHG PRS=OK":"SEL   TURN=MOVE PRS=GO",
                             editing?RGB565_RED:RGB565_GREEN);
    ips200_show_int(0,start_y,*value,VAL_VFG_SHOW_NUM_BIT);
    menu_draw_arrow(start_y,edit_line);
    for(i=0;i<3;i++)
    {
        if(editing)
        {
            color=edit_line==i+1?RGB565_RED:RGB565_WHITE;
            ips200_show_int_color(18,start_y+18*(i+1),steps[i],VAL_VFG_SHOW_NUM_BIT,color);
        }
        else ips200_show_int(18,start_y+18*(i+1),steps[i],VAL_VFG_SHOW_NUM_BIT);
    }
    if(editing)
    {
        ips200_show_string_color(18,start_y+72,"empty",
                                 edit_line==4?RGB565_RED:RGB565_WHITE);
        ips200_show_string_color(18,start_y+90,edit_line==5?"exit":"  ",
                                 edit_line==5?RGB565_RED:RGB565_WHITE);
    }
    else
    {
        ips200_show_string(18,start_y+90,"  ");
        ips200_show_string(18,start_y+72,"empty");
    }
    key_clear_all_state();
}
/* 整型参数编辑 */
void menu_edit_int(int16*value,int16 step,int16 son_start_line)
{
    menu_top->page->line_num=menu_line;
    menu_editing=1;
    uint8 flush_flag;
    uint8 done=0;
    edit_line=(uint8)son_start_line;
    bool editing=0;
    int16 basic_1=step;
    int16 basic_10=step*10;
    int16 basic_100=step*100;
    uint8 start_y;
    start_y=menu_line*18;
    if(start_y>(319-(10+18+18*VAL_VFG_SHOW_TOTAL_LINE)))
    {
        edit_above=1;
    }
    else
        edit_above=0;
    menu_clear_edit(&start_y);
    ips200_set_color(RGB565_WHITE,RGB565_BLACK);
    menu_draw_int(value,start_y,editing,basic_1,basic_10,basic_100);
    menu_key_event=menu_release;
    while(done==0)
    {
        menu_read_key();
        flush_flag=menu_key_event!=menu_release;
        if(menu_key_event==menu_back)
        {
            menu_key_event=menu_release;
            done=1;
            ips200_clear();
            menu_editing=0;
            menu_top->page->open_status=0;
        }
        if((flush_flag==1||switch_encoder_change_num!=0)&&done==0)
        {
            if(menu_key_event==menu_yes)
            {
                menu_key_event=menu_release;
                switch(edit_line)
                {
                    case 1:
                    case 2:
                    case 3:
                        editing=!editing;
                        break;
                    case 4:
                        *value=0;
                        Beep_ShortRing();
                        break;
                    case 5:
                        done=1;
                        menu_editing=0;
                        menu_top->page->open_status=0;
                        break;
                }
            }
            if(If_Switch_Encoder_Change()==1)
            {
                if(editing==0)
                {
                    edit_line+=switch_encoder_change_num;
                    Menu_LimitLine(&edit_line,LINE_MAX);
                }
                else
                {
                    switch(edit_line)
                    {
                        case 1:
                            *value+=basic_1*switch_encoder_change_num;
                            break;
                        case 2:
                            *value+=basic_10*switch_encoder_change_num;
                            break;
                        case 3:
                            *value+=basic_100*switch_encoder_change_num;
                            break;
                    }
                }
            }
            menu_draw_int(value,start_y,editing,basic_1,basic_10,basic_100);
        }
    }
    ips200_clear();
}
