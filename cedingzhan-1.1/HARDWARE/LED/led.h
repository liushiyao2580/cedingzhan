/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: led.h
Description: LED指示灯：主板指示灯和警报灯 宏定义
Author: 曹唪粒（CFL）
Version: 版本 v1.0
Date: 2018.11.25
History: 修改历史记录列表，每条修改记录应包括修改日期、修改者及修改内容简述。

*****************************************************************************/

#ifndef __LED_H
#define __LED_H

/*LED端口宏定义*/
/*主板状态灯*/
#define STATUS_R PEout(6)
#define STATUS_G PCout(13)

void vLed_init(void); /*初始化*/
#endif
