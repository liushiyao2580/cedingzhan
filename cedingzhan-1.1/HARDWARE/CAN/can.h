#ifndef _CAN_H
#define _CAN_H
#include "stm32f4xx.h"
#include "stm32f4xx_can.h"

//CAN1接收RX0中断使能
#define CAN1_RX0_INT_ENABLE 1       //0,不使能;1,使能.                                 

enum _type {
eREMOTE = 0,
eDATA = 1
};

void vCan1_mode_init(u8 tsjw, u8 tbs2, u8 tbs1, u16 brp, u8 mode, u16 ID); //CAN初始化

u8 CAN1_Send_Msg(u8 *msg, u8 len, u32 CAN_ID, enum _type type);         //发送数据

#endif

















