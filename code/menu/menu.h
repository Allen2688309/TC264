#ifndef CODE_MENU_MENU_H_
#define CODE_MENU_MENU_H_

#include "MyHeadFile.h"

#define DEFAULT_BRUSH RGB565_WHITE
#define SELECTED_BRUSH RGB565_RED
#define MENU_KEY MY_KEY_1
#define my_assert(x) zf_assert(x)

#define MENU_ITEM_ENTER_FUNC(name,func) \
    {name,ENTER_FUNC_RUN_TYPE,.action={.run=func}}
#define MENU_ITEM_STATIC_FUNC(name,func) \
    {name,STATIC_FUNC_RUN_TYPE,.action={.run=func}}
#define MENU_ITEM_PAGE_JUMP(name,func) \
    {name,PAGE_JUMP_TYPE,.action={.run=func}}
#define MENU_ITEM_FLOAT_EDIT(name,value,step) \
    {name,FLOAT_VALUE_EDIT_TYPE,.action={.edit_float=menu_edit_float},\
     .data={.float_edit={value,step}}}
#define MENU_ITEM_INT_EDIT(name,value,step,row) \
    {name,INT_VALUE_EDIT_TYPE,.action={.edit_int=menu_edit_int},\
     .data={.int_edit={value,step,row}}}

typedef enum {
    menu_release,
    menu_yes,
    menu_back
} Menu_Event;
extern uint8 menu_key_event;
extern uint8 menu_update_flag;

enum LineExtendsType {
    STATIC_FUNC_RUN_TYPE,
    ENTER_FUNC_RUN_TYPE,
    PAGE_JUMP_TYPE,
    FLOAT_VALUE_EDIT_TYPE,
    INT_VALUE_EDIT_TYPE
};

typedef union {
    void (*run)(void);
    void (*edit_float)(float*,float);
    void (*edit_int)(int16*,int16,int16);
} ActionFunction;

typedef union {
    struct {float*value;float step;} float_edit;
    struct {int16*value;int16 step,row;} int_edit;
} LineExtends;

typedef struct {
    const char*line_name;
    enum LineExtendsType line_type;
    ActionFunction action;
    LineExtends data;
} MenuLine;

typedef struct{
    const char*page_name;/* 行名称 */
    uint8 line_num;
    uint8 line_num_max;
    uint8 open_status;
    MenuLine*line;
}MenuPage;

/* 菜单页面结构体 */
typedef struct MenuPage_Linked_List{
    MenuPage*page;
    struct MenuPage_Linked_List*next;
} MenuPage_Linked_List;
extern MenuPage_Linked_List*menu_top;

/*==================================================== 菜单控制 ====================================================*/
void    Menu_Push_Node   (MenuPage*new_page);
void    Menu_Pop_Node    (void);
void    Menu_Event_Flush (void);
uint8   Menu_CountLines  (MenuPage*Page);
void    MENU_RUN         (void);
void    Menu_Init        (void);
void    Menu_LimitLine   (uint8*line,uint8 line_max);
void    Menu_Key_Process (void);
/*==================================================== 菜单控制 ====================================================*/

/*==================================================== 参数编辑 ====================================================*/
void    menu_edit_float  (float*value,float step);
void    menu_edit_int    (int16*value,int16 step,int16 son_start_line);
void    My_Show_Int      (uint16 X,uint16 Y,int32 Value,uint8 Digits);
/*==================================================== 参数编辑 ====================================================*/

#endif
