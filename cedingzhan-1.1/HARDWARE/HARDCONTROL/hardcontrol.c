/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: hardcontrol.c
Description: 硬件端口控制：气缸、接近开关、下料电机等输入输出信号及其控制
Author: 曹唪粒（CFL）
Version: 版本 v1.0
Date: 2018.11.28
History: 修改历史记录列表，每条修改记录应包括修改日期、修改者及修改内容简述。
*****************************************************************************/
#include "hardcontrol.h"
#include "sys.h"
#include "delay.h"
#include "led.h"
#include "user_task.h"
static volatile enum {
    ON = 0,
    OFF = 1
} e_motor_flag; //电机运行标志

/*
*初始化控制板光电、电磁、电磁阀、电机等引脚
*/
void vHard_io_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;
    EXTI_InitTypeDef   EXTI_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);   //使能GPIOA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);   //使能GPIOC时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);   //使能GPIOD时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);   //使能GPIOB时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);   //使能GPIOE时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);   //使能GPIOF时钟

    /*PA4,5,6,7端口配置  SENSOR_1-4_IN*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;            //输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;        //浮空
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*PC4,5端口配置  SENSOR_5-6_IN*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;            //输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;        //浮空
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /*PD10 端口配置  PESW*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;           //推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //浮空
    GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_10);

    /*PB0,1端口配置  SENSOR_7-8_IN*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;            //输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;        //浮空
    GPIO_Init(GPIOB, &GPIO_InitStructure);


    /*PE2,3,4,5端口配置  CONTROL_1-4_OUT*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;           //推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;       //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_SetBits(GPIOE, GPIO_Pin_2);
	GPIO_SetBits(GPIOE, GPIO_Pin_3);

    /*PE2,3,4,5端口配置  CONTROL_1-4_OUT*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;           //推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;       //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /*PF6,7,8,9端口配置  CONTROL_5-8_OUT*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;           //推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;       //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_SetBits(GPIOF, GPIO_Pin_9);

    /*PA15 端口配置 MOTOR CONTROL*/
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;            //输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;           //推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;       //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*PB4端口配置  Hall_SENSOR*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;            //输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;        //浮空
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//使能SYSCFG时钟
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource4);   //PB4 连接到中断线4

    EXTI_InitStructure.EXTI_Line = EXTI_Line4;                      //LINE4
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;             //中断事件
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;          //上升沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);                                 //配置

    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;                //外部中断4
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x07;    //抢占优先级7
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 //使能外部中断通道
    NVIC_Init(&NVIC_InitStructure);//配置
    MOTOR_CONTROL_OUT(CTL_OFF);
}

static void motor_on(void)
{
    int i = 1500;

    while (i--) {
        delay_xns(i);
        MOTOR_CONTROL_OUT(CTL_ON);
        delay_xns(1500 - i);
        MOTOR_CONTROL_OUT(CTL_OFF);
    }
}

/*
*电机开始运行
*/
void vMotor_run(enum _mode mode)
{
	STATUS_G = 0;
	e_motor_flag = ON;
    switch (mode) {
    case eNORMAL:/* 正常下料 */
		motor_on();
        MOTOR_CONTROL_OUT(CTL_ON);
        e_motor_flag = ON;
        break;
    case eRESET:
    case eONCE:/* 单周测试/复位 */
        motor_on();
        MOTOR_CONTROL_OUT(CTL_ON);
        e_motor_flag = OFF;
	//MOTOR_CONTROL_OUT(CTL_OFF);
        break;
	case eCYCLE:/* 循环测试 */
		motor_on();
		MOTOR_CONTROL_OUT(CTL_ON);
		e_motor_flag = ON;
		break;

    default :
        break;
    }

}

/*
*电机停止运行
*/
void vMotor_stop(enum _mode mode)
{
	STATUS_G = 1;
    switch (mode) {
    case eNORMAL:/* 正常下料 */
		e_motor_flag = OFF;
	//MOTOR_CONTROL_OUT(CTL_OFF);
        break;
    case eRESET:
    case eONCE:/* 单周测试/复位 */
        e_motor_flag = OFF;
        break;

    case eCYCLE:/* 循环测试 */
        MOTOR_CONTROL_OUT(CTL_OFF);
        break;

    default :
        break;
    }
}

/*
*外部中断4服务程序
*/
void EXTI4_IRQHandler(void)
{
	BaseType_t pxHigherPriorityTaskWoken;

    if (EXTI_GetITStatus(EXTI_Line4) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line4);
		if(PBin(4) == 1);{
			delay_xms(30);
			if(PBin(4) == 1){
				STATUS_G = !STATUS_G;
				if (e_motor_flag == OFF) {
					MOTOR_CONTROL_OUT(CTL_OFF);
				}
                if(EventGroupHandler != NULL){
					xEventGroupSetBitsFromISR(EventGroupHandler,XAILIAO,&pxHigherPriorityTaskWoken);
	                /*是否需要任务切换*/
	                portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
				}
			}
		}
    }
}
