/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: error_code.h
Description: 定义种猪测定站系统各类错误代码
Author: 曹唪粒（CFL）
Version: 版本 v1.0
Date: 2018.11.22
History: 修改历史记录列表，每条修改记录应包括修改日期、修改者及修改内容简述。
*****************************************************************************/

#ifndef ERROR_CODE_H
#define ERROR_CODE_H

typedef enum _error_code {
/*注意：需要上传的错误代码范围必须为 0-8 */
    /* 关机 */
    WORK_STATUS_OFF = 1,
    /* 开机 */
    WORK_STATUS_ON = 2,
    /*通信类*/
    ERR_RFID = 3,            /* RFID故障 */
    /*传感器故障类*/
    ERR_MOTOR_SENSOR   = 4, /* 下料电机故障 */
    /*电路板外设故障类*/
    ERR_SDCARD         = 5, /* SD卡故障*/
    ERR_ADC            = 6, /* 称重传感器故障 */
    /*机械设备故障类*/
    ERR_CYLINDER       = 7, /* 气缸故障 */
	ERR_EEPROM         = 8,  /* EEPROM故障 */
	
/*注意：大于等于9的错误码仅限本地使用*/

	ERR_CYLINDER_1    = 9, /* 气缸1故障 */
	ERR_CYLINDER_2       , /* 气缸2故障 */
	ERR_CYLINDER_3       , /* 气缸3故障 */
	ERR_CYLINDER_4       , /* 气缸4故障 */
} ERROR_CODE;

typedef enum _feature_code {
    heart_beat =            (0x0f & 0) << 7,
    data_object_request =   (0x0f & 1) << 7,
    time_stamp_request =    (0x0f & 2) << 7,
    open_device =           (0x0f & 3) << 7,
    close_device =          (0x0f & 4) << 7,
    recv_complete =         (0x0f & 5) << 7,
    train_mode =            (0x0f & 6) << 7,
    measure_mode =          (0x0f & 7) << 7
} FEATURE_CODE;

#endif
