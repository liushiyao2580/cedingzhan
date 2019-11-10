/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: 24cxx.c
Description: EEPROM相关操作函数定义
Author: 曹唪粒-修改正点原子例程
Version: 版本 v1.0
Date: 2018.12.4
History:
修改：
    1：u8 AT24CXX_Check(void) 函数
    
        修改检查方式，使用伪随机数进行读写，确保可读/可写双向正常。
        添加首次开机检测，并写入默认值。
添加函数：
    1：int8_t scPull_all_eedata(pEEPROM_DATA data); //获取 EEPROM 所有已保存数据
    2：uint8_t ucPush_all_eedata(pEEPROM_DATA data);//更新 EEPROM 所有已保存数据
    3：uint32_t ulPull_one_eedata(enum data_addr _num);//获取 EEPROM 某一个已保存数据
    4：void vPush_one_eedata(enum data_addr _num, uint32_t data);//更新 EEPROM 某一个已保存数据
其他：
    定义EEPROM相关存储数据结构体，使用枚举常量列出数据在EEPROM中的地址
*****************************************************************************/
#include "24cxx.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
/*初始化IIC接口*/
void AT24CXX_Init(void)
{
    IIC_Init();//IIC初始化
}

/*
*在AT24CXX指定地址读出一个数据
*ReadAddr:开始读数的地址
*返回值  :读到的数据
*/
static u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{
    u8 temp = 0;

    IIC_Start();
    IIC_Send_Byte(0XA0 + ((ReadAddr / 256) << 1));    //发送器件地址0XA0,写数据
    IIC_Wait_Ack();

    IIC_Send_Byte(ReadAddr % 256); //发送低地址
    IIC_Wait_Ack();

    IIC_Start();
    IIC_Send_Byte(0XA1);           //进入接收模式
    IIC_Wait_Ack();
    temp = IIC_Read_Byte(0);
    IIC_Stop();//产生一个停止条件
    return temp;
}
//在AT24CXX指定地址写入一个数据
//WriteAddr  :写入数据的目的地址
//DataToWrite:要写入的数据
static void AT24CXX_WriteOneByte(u16 WriteAddr, u8 DataToWrite)
{
    IIC_Start();
    IIC_Send_Byte(0XA0 + ((WriteAddr / 256) << 1));    //发送器件地址0XA0,写数据
    IIC_Wait_Ack();

    IIC_Send_Byte(WriteAddr % 256); //发送低地址
    IIC_Wait_Ack();

    IIC_Send_Byte(DataToWrite);     //发送字节
    IIC_Wait_Ack();

    IIC_Stop();//产生一个停止条件
    delay_xms(10);
}
//在AT24CXX里面的指定地址开始写入长度为Len的数据
//该函数用于写入16bit或者32bit的数据.
//WriteAddr  :开始写入的地址
//DataToWrite:数据数组首地址
//Len        :要写入数据的长度2,4
static void AT24CXX_WriteLenByte(u16 WriteAddr, u32 DataToWrite, u8 Len)
{
    u8 t;

    for (t = 0; t < Len; t++) {
        AT24CXX_WriteOneByte(WriteAddr + t, (DataToWrite >> (8 * t)) & 0xff);
    }
}

//在AT24CXX里面的指定地址开始读出长度为Len的数据
//该函数用于读出16bit或者32bit的数据.
//ReadAddr   :开始读出的地址
//返回值     :数据
//Len        :要读出数据的长度2,4
static u32 AT24CXX_ReadLenByte(u16 ReadAddr, u8 Len)
{
    u8 t;
    u32 temp = 0;

    for (t = 0; t < Len; t++) {
        temp <<= 8;
        temp += AT24CXX_ReadOneByte(ReadAddr + Len - t - 1);
    }

    return temp;
}
//检查AT24CXX是否正常
//这里用了24XX的最后一个地址(255)来存储标志字.
//如果用其他24C系列,这个地址要修改
//返回1:检测失败
//返回0:检测成功
u8 AT24CXX_Check(void)
{
    u8 temp = 0, num = 0;
    num = rand() % 255;
	DEBUG_MSG(printf("num=%d\r\n",num));
    AT24CXX_WriteOneByte(255, num);
    temp = AT24CXX_ReadOneByte(255);
    DEBUG_MSG(printf("%d\r\n",temp));
    if (temp == num) {
        temp = AT24CXX_ReadOneByte(254); /* 判断是否首次开机 */

        if (temp == 0x55) {
            return 0;
        }

        else {
            ucPush_all_eedata(pEdata); /* 写入默认数据 */
            AT24CXX_WriteOneByte(254, 0x55);
            return 0;
        }

    }

    return 1;
}

/* 全局变量用于存储EEPROM中变量结构体，使用范围 -全局*/
EEPROM_DATA Edata = {
    .k1 = 0,
    .k2 = 0,
    .k3 = 0,
    .k4 = 0,
    .k5 = 0,
    .kw = 0,
    .kf = 0,
    .m1 = 25,
    .m2 = 500,
    .temp1 = 0,
    .temp2 = 0,
    .temp3 = 0,
    .temp4 = 0,
    .temp5 = 0,
    .mode = 0,
    .can_id = 10,
    .avgweight = 0,
    .fodderweight = 1200,
    .reset_count = 0,
    .min_fodderweight = 100,
    .adc_overtime_count = 0,
    .language = 1
};
pEEPROM_DATA pEdata = &Edata; /* 定义EEPROM存储的数据结构体指针 */

/*
*brief 获取 EEPROM 所有已保存数据
*data 结构体指针
*return 0成功  其他 失败
*/
int8_t scPull_all_eedata(pEEPROM_DATA data)
{
    unsigned int *tmp = NULL;
    tmp = (unsigned int *) data; /* 强制转换int指针，用于结构体数据循环访问 */

    if (data == NULL) {
        return 1;
    }

    if (AT24CXX_Check()) {/*24cxx error*/
        return 2;
    }

   DEBUG_MSG(printf("****** EEPROM Data ******\r\n"));

    for (int addr = 0; addr <= eEND_ADDR; addr += 4) {
        *(tmp + addr / 4) = (unsigned int)AT24CXX_ReadLenByte(addr, 4);
        DEBUG_MSG(printf("%d is %d \r\n", addr, *(tmp + addr / 4)));
    }

    return 0;
}

/*
*更新 EEPROM 所有已保存数据
*/
uint8_t ucPush_all_eedata(pEEPROM_DATA data)
{
    int *tmp = NULL;
    tmp = (int *) data;

    if (data == NULL) {
        return 1;
    }

    for (int addr = 0; addr <= eEND_ADDR; addr += 4) {
        AT24CXX_WriteLenByte(addr, *(tmp + addr / 4), 4);
    }

    return 0;
}

/*
*获取 EEPROM 某一个已保存数据
*/
uint32_t ulPull_one_eedata(enum data_addr _num)
{
    return  AT24CXX_ReadLenByte(_num, 4);
}
/*
*更新 EEPROM 某一个已保存数据
*/
void vPush_one_eedata(enum data_addr _num, uint32_t data)
{
    vTaskSuspendAll();
    AT24CXX_WriteLenByte(_num, data, 4);
    xTaskResumeAll();

}






