/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: user_task.h
Description: 包含FreeRTOS所有用户任务堆栈、优先级定义及任务句柄、任务函数声明
Author: 曹唪粒（CFL）
Version: 版本 v1.0
Date: 2018.10.20
History: 修改历史记录列表，每条修改记录应包括修改日期、修改者及修改内容简述。
*****************************************************************************/

#ifndef USER_TASK_H
#define USER_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "can.h"
#include "hardcontrol.h"
#include "24cxx.h"
#include "error_code.h"
#include "Load_Sensor.h"
#include "exfuns.h"
#include "timers.h"
#include "cmd_process.h"
#include "event_groups.h"
#include "myString.h"


enum _event {
    DATA_REQUEST = 0x00000001,
    RECV_COMPLETE = 0x00000002,
    BIT3 = 0x00000004,
    BIT4 = 0x00000008,
    BIT5 = 0x00000010,
    BIT6 = 0x00000020,
    BIT7 = 0x00000040,
    BIT8 = 0x00000080,
    BIT9 = 0x00000100,
    POWEROFF = 0x00000200,
    RTCUPDATE = 0x00000400,

    XAILIAO = 0x00000800,//下料
    CHENGZHONG = 0x00001000,//称重

    TUOJIA_OFF = 0x00002000,
    DOOR1_OFF = 0x00004000,
    DOOR2_OFF = 0x00008000,
    HONGWAI_OFF = 0x00010000,
    CAISHI_OFF = 0x00020000,

    TUOJIA_ON = 0x00040000,//托架
    DOOR1_ON = 0x00080000,//门1
    DOOR2_ON = 0x00100000,//门2
    HONGWAI_ON = 0x00200000,//红外
    CAISHI_ON = 0x00400000,//采食门
};

/* 上传数据结构体 */
typedef struct _upload_data {
    uint8_t earid[13];
    float food_intake;
    float weight;
    float body_long;
    float body_width;
    float body_height;
    float body_temp;
    float env_temp;
    float env_humi;
    uint8_t start_time[13];
    uint8_t end_time[13];
} sData, *psDATA;

extern volatile ERROR_CODE eDevice_Status;
extern volatile ERROR_CODE eDevice_Status1;
extern EventGroupHandle_t EventGroupHandler; //事件标志组句柄
/*
*消息队列长度定义
*/
#define canRecQueueLength (100)
#define canSendQueueLength (100)
#define uartDebugQueueLength (100)
/*
*消息队列句柄外部声明
*/

extern QueueHandle_t canRecQueue_Handler;/*CAN 总线接收消息队列*/
extern QueueHandle_t canSendQueue_Handler;/*CAN 总线发送消息队列*/
#define ReportMsgBufLen (40)  /*UART调试发送消息缓冲区长度*/
extern QueueHandle_t uartDebugQueue_Handler;/*UART调试发送消息队列*/

extern volatile unsigned char uc_sensor_status;/* 保存8路接近开关状态 */
void StatusUpdatesCallback(TimerHandle_t xTimer); //周期定时器回调函数
extern TimerHandle_t StatusUpdates_Timer_Handle;/* 更新8路接近开关状态,定时器句柄 */


/*!
*  \brief 串口数据格式化发送函数
*  \param 同printf函数  vUsart_report_msg("test: %d %.2f",120, 35.51);
*  \retval 无
*  \brief 数据发送至消息发送队列（需要定义好发送队列：uartDebugQueue）
*          通过串口发送任务进行发送
*/
void vUsart_report_msg(const char *fmt, ...);

/*
**起始任务
*/
//任务优先级
#define START_TASK_PRIO     1
//任务堆栈大小
#define START_STK_SIZE      64
//任务句柄
extern TaskHandle_t StartTask_Handler;
//任务函数
/*!
*  \brief  起始任务函数
*  \param  任务传入参数
*/
void vStart_task(void *pvParameters);

/*
**串口调试任务
*/
//任务优先级
#define UART_DEBUG_TASK_PRIO        1
//任务堆栈大小
#define UART_DEBUG_STK_SIZE         64
//任务句柄
extern TaskHandle_t UartDebugTask_Handler; /*调试串口发送任务*/
//任务函数
/*!
*  \brief  串口调试任务函数
*  \param  任务传入参数
*   用法：
*   需要发送的信息 -> 串口发送队列
*   xQueueSend( uartDebugQueue_Handler,(void *) "SendMsg",0);
*/
void vUart_debug_task(void *pvParameters);

/*
**LED0任务
*/
//任务优先级
#define LED0_TASK_PRIO      2
//任务堆栈大小
#define LED0_STK_SIZE       128
//任务句柄
extern TaskHandle_t LED0Task_Handler;
//任务函数
/*!
*  \brief  LED0任务函数
*  \param  任务传入参数
*/
void vLed0_task(void *pvParameters);


/*
**显示屏交互任务
*/
//任务优先级
#define DCTFT_TASK_PRIO     2
//任务堆栈大小
#define DCTFT_STK_SIZE      1024
//任务句柄
extern TaskHandle_t DctftTask_Handler;
//任务函数
/*!
*  \brief  显示屏交互任务函数
*  \param  任务传入参数
*/
void vDctft_task(void *pvParameters);


/*
**工作模式任务
*/
//任务优先级
#define WORK_TASK_PRIO      4
//任务堆栈大小
#define WORK_STK_SIZE       2048
//任务句柄
extern TaskHandle_t WorkTask_Handler;
//任务函数
/*!
*  \brief  工作模式任务函数
*  \param  任务传入参数
*/
void vWork_task(void *pvParameters);


/*
**CAN接收任务
*/
//任务优先级
#define CANREC_TASK_PRIO        3
//任务堆栈大小
#define CANREC_STK_SIZE         128
//任务句柄
extern TaskHandle_t CANREC_Task_Handler;
//任务函数
/*!
*  \brief CAN接收任务函数
*  \param 任务传入参数
*/
void vCan_rec_task(void *pvParameters);

/*
**CAN发送任务
*/
//任务优先级
#define CANSEND_TASK_PRIO       3
//任务堆栈大小
#define CANSEND_STK_SIZE        128
//任务句柄
extern TaskHandle_t CANSEND_Task_Handler;
//任务函数
/*!
*  \brief CAN发送任务函数
*  \param 任务传入参数
*/
void vCan_send_task(void *pvParameters);

/*
**数据保存任务
*/
//任务优先级
#define S_DATA_TASK_PRIO        1
//任务堆栈大小
#define S_DATA_STK_SIZE         1024
//任务句柄
extern TaskHandle_t S_DATATask_Handler;
//任务函数
/*!
*  \brief 数据保存任务函数
*  \param 任务传入参数
*/
void vSave_data_task(void *pvParameters);


void vPower_on(void);//开始工作模式
void vPower_off(void);//结束工作模式
void vReport_error(ERROR_CODE);

FRESULT scan_files ( char *path );/* Start node to be scanned (***also used as work area***) */
FRESULT delete_files ( char *path );

#endif

