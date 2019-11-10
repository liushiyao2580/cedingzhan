#ifndef __24CXX_H
#define __24CXX_H
#include "myiic.h"

#define AT24C02     255

/* EEPROM型号 */
#define EE_TYPE AT24C02

/* EEPROM中数据的地址 统一以 e 开头*/
enum data_addr {
    eK1_ADDR = 0,
    eK2_ADDR = 4,
    eK3_ADDR = 8,
    eK4_ADDR = 12,
    eK5_ADDR = 16,
    eKW_ADDR = 20,
    eKF_ADDR = 24,

    eM1_ADDR = 28,
    eM2_ADDR = 32,

    eTEMP1_ADDR = 36,
    eTEMP2_ADDR = 40,
    eTEMP3_ADDR = 44,
    eTEMP4_ADDR = 48,
    eTEMP5_ADDR = 52,

    eMODE_ADDR = 56,
    eCANID_ADDR = 60,
    eAVGWEIGHT_ADDR = 64,
    eFODDERWEIGHT_ADDR = 68,
    eK_I_ADDR = 72,
    eK_O_ADDR = 76,
    eWORK_STATUS_ADDR = 80,
    eRESET_COUNT_ADDR = 84,
    eMIN_FODDERWEIGHT_ADDR = 88,
    eADC_OVERTIME_COUNT_ADDR = 92,
    eLANGUAGE_ADDR = 96,
    eEND_ADDR = 96
};

/* 掉电保存数据结构体 */
typedef struct {
    int k1;
    int k2;
    int k3;
    int k4;
    int k5;
    int kw;
    int kf;
    int m1;
    int m2;

    int temp1;
    int temp2;
    int temp3;
    int temp4;
    int temp5;

    int mode;
    int can_id;

    int avgweight;
    int fodderweight;

    int k_i;
    int k_o;
    int work_status;
    int reset_count;
	int min_fodderweight;//饲料质量下限
	int adc_overtime_count; //模数转换器无响应计数
	int language;
} EEPROM_DATA, *pEEPROM_DATA;

extern pEEPROM_DATA pEdata;

u8 AT24CXX_Check(void);  //检查器件
void AT24CXX_Init(void); //初始化IIC

/*
*brief 获取 EEPROM 所有已保存数据
*data 结构体指针
*return 0成功  其他 失败
*/
int8_t scPull_all_eedata(pEEPROM_DATA data);

/*
*更新 EEPROM 所有已保存数据
*/
uint8_t ucPush_all_eedata(pEEPROM_DATA data);

/*
*获取 EEPROM 某一个已保存数据
*/
uint32_t ulPull_one_eedata(enum data_addr _num);
/*
*更新 EEPROM 某一个已保存数据
*/
void vPush_one_eedata(enum data_addr _num, uint32_t data);







#endif














