#ifndef __EXFUNS_H
#define __EXFUNS_H
#include <stm32f4xx.h>
#include "ff.h"

extern FATFS *fs;
extern FIL *file;
extern UINT br, bw;
extern FILINFO fileinfo;
extern DIR dir;
extern uint8_t *fatbuf;//SD卡数据缓存区

void vFile_ver_malloc(void);        //申请内存

#endif
