/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: hardcontrol.h
Description: 硬件端口控制：气缸、接近开关、下料电机等输入输出信号及其控制
Author: 曹唪粒（CFL）
Version: 版本 v1.0
Date: 2018.11.28
History: 修改历史记录列表，每条修改记录应包括修改日期、修改者及修改内容简述。

*****************************************************************************/

#ifndef _HARDCONTROL_H
#define _HARDCONTROL_H

#define CTL_ON (0)
#define CTL_OFF (1)

enum _mode {
    eNORMAL = 0,
    eRESET= 1,
    eONCE = 2,
    eCYCLE = 3
};

/*****************与硬件IO相对应*******************/

/* 光电、电磁等传感器信号输入脚 */
#define SENSOR_1_IN  PAin(4)
#define SENSOR_2_IN  PAin(5)
#define SENSOR_3_IN  PAin(6)
#define SENSOR_4_IN  PAin(7)
#define SENSOR_5_IN  PCin(4)
#define SENSOR_6_IN  PCin(5)
#define SENSOR_7_IN  PBin(0)
#define SENSOR_8_IN  PBin(1)

/* 气缸电磁阀控制引脚 */
#define CONTROL_1_OUT(x) PEout(2) = (x)
#define CONTROL_2_OUT(x) PEout(3) = (x)
#define CONTROL_3_OUT(x) PEout(4) = (x)
#define CONTROL_4_OUT(x) PEout(5) = (x)
#define CONTROL_5_OUT(x) PFout(6) = (x)
#define CONTROL_6_OUT(x) PFout(7) = (x)
#define CONTROL_7_OUT(x) PFout(8) = (x)
#define CONTROL_8_OUT(x) PFout(9) = (x)
/*****************************************/


/*电机控制引脚*/
#define MOTOR_CONTROL_OUT(x) PAout(15) = (x)

/* 开关门控制 */
#define LEFT_DOOR_CLOSE CONTROL_4_OUT(CTL_ON);\
                        CONTROL_3_OUT(CTL_OFF)
#define RIGHT_DOOR_CLOSE CONTROL_5_OUT(CTL_ON)

#define LEFT_DOOR_OPEN CONTROL_4_OUT(CTL_OFF);\
                    CONTROL_3_OUT(CTL_ON)
#define RIGHT_DOOR_OPEN CONTROL_5_OUT(CTL_OFF)

#define LEFT_DRAIN CONTROL_4_OUT(CTL_OFF);\
                    CONTROL_3_OUT(CTL_OFF)   /* 左门中泄，使门处于可推开状态 */


#define FEEDING_DOOR_CLOSE CONTROL_6_OUT(CTL_OFF) 
#define FEEDING_DOOR_OPEN CONTROL_6_OUT(CTL_ON)
#define CONTROL_liaodou_OUT(x) CONTROL_7_OUT(x)

/* 传感器状态 */
#define LEFT_DOOR_SENSOR ()
#define RIGHT_DOOR_SENSOR ()
#define BODY_SENSOR ()


void vMotor_run(enum _mode mode);
void vMotor_stop(enum _mode mode);

void vHard_io_init(void);

#endif