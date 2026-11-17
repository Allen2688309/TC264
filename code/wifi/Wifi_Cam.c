#include "zf_common_headfile.h"
#include "Camera.h"
#include "wifi/Wifi_Cam.h"

#define WIFI_CAM_SSID "testwifi"
#define WIFI_CAM_PASSWORD "abc12345"

#define WIFI_CAM_SEND_US 50000U
#define WIFI_BINARY_SIZE ((IMG_W*IMG_H)/8U)

volatile uint8 wifi_ready=0;
volatile uint8 wifi_err=0;
static volatile uint8 wifi_stage=0;
volatile uint16 wifi_send_cnt=0;
static uint32 wifi_last_us=0;
static uint32 wifi_last_frame=0;
static uint8 wifi_binary_image[WIFI_BINARY_SIZE];
static seekfree_assistant_camera_struct wifi_camera;
static seekfree_assistant_camera_boundary_struct wifi_boundary[3];
static seekfree_assistant_camera_boundary_uint8_struct wifi_line[3][IMG_H];

extern seekfree_assistant_transfer_callback_function seekfree_assistant_transfer_callback;

/***********************************************
 * @brief : 发送助手数据并记录发送失败
 * @param : data 待发送数据
 * @param : size 请求发送的字节数
 * @return: 尚未发送的字节数
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
uint32 wifi_send_data(const uint8*data,uint32 size)
{
    uint32 left;
    if(!wifi_ready) return size;
    left=wifi_spi_send_buffer(data,size);
    if(left)
    {
        wifi_ready=0;
        wifi_err=3;
        wifi_stage=9;
    }
    return left;
}

/***********************************************
 * @brief : 连接图传模块并配置图像及边线发送
 * @param : 无
 * @return: 零表示成功，一表示模块失败，二表示连接失败
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
uint8 Wifi_Cam_Init(void)
{
    wifi_ready=0;
    wifi_err=0;
    wifi_send_cnt=0;
    wifi_last_us=0;
    wifi_last_frame=vision_frame_id;
    wifi_stage=1;
    if(wifi_spi_init((char*)WIFI_CAM_SSID,(char*)WIFI_CAM_PASSWORD))
    {
        wifi_stage=wifi_spi_diag;
        wifi_err=1;
        return 1;
    }
    wifi_stage=6;
    if(wifi_spi_socket_connect((char*)"TCP",
                                  (char*)WIFI_SPI_TARGET_IP,
                                  (char*)WIFI_SPI_TARGET_PORT,
                                  (char*)WIFI_SPI_LOCAL_PORT))
    {
        wifi_err=2;
        wifi_stage=9;
        return 2;
    }
    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIFI_SPI);
    seekfree_assistant_transfer_callback=wifi_send_data;
    seekfree_assistant_camera_config(&wifi_camera,
                                     SEEKFREE_ASSISTANT_CAMERA_TYPE_BINARY,
                                     IMG_W,IMG_H,wifi_binary_image);
    seekfree_assistant_camera_boundary_config(&wifi_boundary[0],
                                              SEEKFREE_ASSISTANT_DATA_TYPE_UINT8,
                                              0xF800,IMG_H,wifi_line[0]);
    seekfree_assistant_camera_boundary_config(&wifi_boundary[1],
                                              SEEKFREE_ASSISTANT_DATA_TYPE_UINT8,
                                              0x07E0,IMG_H,wifi_line[1]);
    seekfree_assistant_camera_boundary_config(&wifi_boundary[2],
                                              SEEKFREE_ASSISTANT_DATA_TYPE_UINT8,
                                              0x001F,IMG_H,wifi_line[2]);
    wifi_stage=7;
    wifi_ready=1;
    return 0;
}

/***********************************************
 * @brief : 按发送间隔打包新图像并发送图像和边线
 * @param : 无
 * @return: 无
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
void Wifi_Cam_Send(void)
{
    uint32 now_us;
    uint16 y,x;
    uint32 bit_index;
    if(!wifi_ready||wifi_last_frame==vision_frame_id) return;
    now_us=system_getval_us();
    if((uint32)(now_us-wifi_last_us)<WIFI_CAM_SEND_US) return;
    if(!gpio_get_level(WIFI_SPI_INT_PIN)) return;
    wifi_last_us=now_us;
    wifi_last_frame=vision_frame_id;
    /* 二值打包 */
    for(bit_index=0;bit_index<WIFI_BINARY_SIZE;bit_index++)
        wifi_binary_image[bit_index]=0;
    for(y=0;y<IMG_H;y++)
    {
        wifi_line[0][y].x=l_border[y];
        wifi_line[0][y].y=(uint8)y;
        wifi_line[1][y].x=center_line[y];
        wifi_line[1][y].y=(uint8)y;
        wifi_line[2][y].x=r_border[y];
        wifi_line[2][y].y=(uint8)y;
        for(x=0;x<IMG_W;x++)
        {
            if(bin_image[y][x])
            {
                bit_index=(uint32)y*IMG_W+x;
                wifi_binary_image[bit_index>>3]|=(uint8)(0x80U>>(bit_index&7U));
            }
        }
    }
    seekfree_assistant_camera_send(&wifi_camera);
    seekfree_assistant_camera_boundary_send(&wifi_boundary[0]);
    seekfree_assistant_camera_boundary_send(&wifi_boundary[1]);
    seekfree_assistant_camera_boundary_send(&wifi_boundary[2]);
    if(wifi_ready&&wifi_send_cnt<65535) wifi_send_cnt++;
}

/***********************************************
 * @brief : 连接中读取驱动诊断阶段，其余读取图传阶段
 * @param : 无
 * @return: 当前图传或驱动诊断阶段
 * @date  : 2026-11-07
 * @author: L
 ************************************************/
uint8 Wifi_Cam_GetStage(void)
{
    if(wifi_stage==1) return wifi_spi_diag;
    return wifi_stage;
}
