#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

#define DEBUG 1   //enable/disable debug msg.
#define EN_USART1_RX 			1
#define EN_USART3_RX 			1
#define EN_USART6_RX 			1

//调试完毕后务必关闭调试宏，避免调试串口因打印信息影响系统性能
#if DEBUG                   
#define DEBUG_MSG(x) x
#else 
#define DEBUG_MSG(x)
#endif

//串口宏定义
#define RFID_UART   USART3  //rfid 模块串口
#define DEBUG_UART  USART2  //调试串口
#define TFT_UART    USART1  //屏幕通信串口
#define ARM_UART    USART6  //与ARM通信串口

void vUart1_init(u32 bound);
void vUart3_init(u32 bound);
void vUart6_init(u32 bound);
void vGet_img(int64_t);
void send_to_arm(unsigned char *data);

#endif


