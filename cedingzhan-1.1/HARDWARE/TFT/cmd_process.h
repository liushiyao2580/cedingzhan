/*!
*  \file hmi_driver.h
*  \brief 指令分类处理
*  \version 1.0
*  \date 2012-2018
*  \copyright 广州大彩光电科技有限公司
*/


#ifndef _CMD_PROCESS_H
#define _CMD_PROCESS_H
#include "hmi_driver.h"
#include "stm32f4xx.h"

//画面ID
#define ID_SCREEN_MAIN_PAGE               (0) //主页
#define ID_SCREEN_WARNING_PAGE            (1) //告警信息页
#define ID_SCREEN_INQUIRE_PAGE            (2) //浏览模式页
#define ID_SCREEN_MENU_PAGE               (3) //菜单页
#define ID_SCREEN_FUNC_SWITCH_PAGE        (4) //模式切换页
#define ID_SCREEN_FUNC_TEST_PAGE          (5) //功能测试页
#define ID_SCREEN_CALIBRATION_PAGE    	  (6) //标定页面
#define ID_SCREEN_SYS_SETUP_PAGE      	  (7) //系统设定页 or 参数设定页
#define ID_SCREEN_INFO_DISPLAY_PAGE   	  (8) //测定信息显示页
#define ID_SCREEN_FODDER_CALIBRATION_PAGE (9) //饲料秤标定页
#define ID_SCREEN_WEIGHT_CALIBRATION_PAGE (10)  //体重秤标定页
#define ID_SCREEN_WEIGHT_TEST_PAGE        (11)  //称重测试页


//通知类型
#define NOTIFY_TOUCH_PRESS         0X01  //触摸屏按下通知
#define NOTIFY_TOUCH_RELEASE       0X03  //触摸屏松开通知
#define NOTIFY_WRITE_FLASH_OK      0X0C  //写FLASH成功
#define NOTIFY_WRITE_FLASH_FAILD   0X0D  //写FLASH失败
#define NOTIFY_READ_FLASH_OK       0X0B  //读FLASH成功
#define NOTIFY_READ_FLASH_FAILD    0X0F  //读FLASH失败
#define NOTIFY_MENU                0X14  //菜单事件通知
#define NOTIFY_TIMER               0X43  //定时器超时通知
#define NOTIFY_CONTROL             0XB1  //控件更新通知
#define NOTIFY_READ_RTC            0XF7  //读取RTC时间
#define MSG_GET_CURRENT_SCREEN     0X01  //画面ID变化通知
#define MSG_GET_DATA               0X11  //控件数据通知
#define NOTIFY_HandShake           0X55  //握手通知

#define PTR2U16(PTR) ((((uint8 *)(PTR))[0]<<8)|((uint8 *)(PTR))[1])  //从缓冲区取16位数据
#define PTR2U32(PTR) ((((uint8 *)(PTR))[0]<<24)|(((uint8 *)(PTR))[1]<<16)|(((uint8 *)(PTR))[2]<<8)|((uint8 *)(PTR))[3])  //从缓冲区取32位数据

#define _INT2STR(x)  (#x)
#define INT2STR(x)  _INT2STR(x) 
extern u8  cmd_buffer[CMD_MAX_SIZE];       //指令缓存
extern volatile uint16 current_screen_id;

typedef struct{
    int sec;
    int years;
    int months;
    int weeks;
    int days;
    int hours;
    int minutes;
    int lock;
}RTC_TIME;
extern RTC_TIME sRtc_time;//RTC 时间缓存区

enum CtrlType {
    kCtrlUnknown = 0x0,
    kCtrlButton = 0x10,                   //按钮
    kCtrlText,                            //文本
    kCtrlProgress,                        //进度条
    kCtrlSlider,                          //滑动条
    kCtrlMeter,                            //仪表
    kCtrlDropList,                        //下拉列表
    kCtrlAnimation,                       //动画
    kCtrlRTC,                             //时间显示
    kCtrlGraph,                           //曲线图控件
    kCtrlTable,                           //表格控件
    kCtrlMenu,                            //菜单控件
    kCtrlSelector,                        //选择控件
    kCtrlQRCode,                          //二维码
};

#pragma pack(push)
#pragma pack(1)                           //按字节对齐

typedef struct {
    uint8    cmd_head;                    //帧头
    uint8    cmd_type;                    //命令类型(UPDATE_CONTROL)
    uint8    ctrl_msg;                    //CtrlMsgType-指示消息的类型
    uint16   screen_id;                   //产生消息的画面ID
    uint16   control_id;                  //产生消息的控件ID
    uint8    control_type;                //控件类型

    uint8    param[256];                  //可变长度参数，最多256个字节

    uint8  cmd_tail[4];                   //帧尾
} CTRL_MSG, *PCTRL_MSG;

#pragma pack(pop)

/*!
*  \brief  握手通知
*/
void NOTIFYHandShake(void);

/*!
*  \brief  消息处理流程
*  \param msg 待处理消息
*  \param size 消息长度
*/
void ProcessMessage( PCTRL_MSG msg, uint16 size );
/*!
*  \brief  画面切换通知
*  \details  当前画面改变时(或调用GetScreen)，执行此函数
*  \param screen_id 当前画面ID
*/
void NotifyScreen(uint16 screen_id);
/*!
*  \brief  触摸坐标事件响应
*  \param press 1按下触摸屏，3松开触摸屏
*  \param x x坐标
*  \param y y坐标
*/
void NotifyTouchXY(uint8 press, uint16 x, uint16 y);

/*!
*  \brief  按钮控件通知
*  \details  当按钮状态改变(或调用GetControlValue)时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param state 按钮状态：0弹起，1按下
*/
void NotifyButton(uint16 screen_id, uint16 control_id, uint8 state);
/*!
*  \brief  文本控件通知
*  \details  当文本通过键盘更新(或调用GetControlValue)时，执行此函数
*  \details  文本控件的内容以字符串形式下发到MCU，如果文本控件内容是浮点值，
*  \details  则需要在此函数中将下发字符串重新转回浮点值。
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param str 文本控件内容
*/
void NotifyText(uint16 screen_id, uint16 control_id, uint8 *str);
/*!
*  \brief  进度条控件通知
*  \details  调用GetControlValue时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
void NotifyProgress(uint16 screen_id, uint16 control_id, uint32 value);
/*!
*  \brief  滑动条控件通知
*  \details  当滑动条改变(或调用GetControlValue)时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
void NotifySlider(uint16 screen_id, uint16 control_id, uint32 value);
/*!
*  \brief  仪表控件通知
*  \details  调用GetControlValue时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
void NotifyMeter(uint16 screen_id, uint16 control_id, uint32 value);
/*!
*  \brief  菜单控件通知
*  \details  当菜单项按下或松开时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param item 菜单项索引
*  \param state 按钮状态：0松开，1按下
*/
void NotifyMenu(uint16 screen_id, uint16 control_id, uint8  item, uint8  state);

/*!
*  \brief  选择控件通知
*  \details  当选择控件变化时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param item 当前选项
*/
void NotifySelector(uint16 screen_id, uint16 control_id, uint8  item);
/*!
*  \brief  定时器超时通知处理
*  \param screen_id 画面ID
*  \param control_id 控件ID
*/
void NotifyTimer(uint16 screen_id, uint16 control_id);
/*!
*  \brief  读取用户FLASH状态返回
*  \param status 0失败，1成功
*  \param _data 返回数据
*  \param length 数据长度
*/
void NotifyReadFlash(uint8 status, uint8 *_data, uint16 length);

/*!
*  \brief  写用户FLASH状态返回
*  \param status 0失败，1成功
*/
void NotifyWriteFlash(uint8 status);
/*!
*  \brief  读取RTC时间，注意返回的是BCD码
*  \param year 年（BCD）
*  \param month 月（BCD）
*  \param week 星期（BCD）
*  \param day 日（BCD）
*  \param hour 时（BCD）
*  \param minute 分（BCD）
*  \param second 秒（BCD）
*/
void NotifyReadRTC(uint8 year, uint8 month, uint8 week, uint8 day, uint8 hour, uint8 minute, uint8 second);

#endif
