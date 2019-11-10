#include "ff.h"
#include "FreeRTOS.h"
#include "exfuns.h"

FATFS  *fs;//逻辑磁盘工作区.
FIL fl, *file;         //文件1

UINT br, bw;        //读写变量
FILINFO fileinfo;   //文件信息
DIR dir;            //目录
uint8_t *fatbuf;         //SD卡数据缓存区

void vFile_ver_malloc(void)
{
    fs = (FATFS *)pvPortMalloc(sizeof(FATFS)); //为磁盘工作区申请内存
    file = (FIL *)pvPortMalloc(sizeof(FIL)); //为file申请内存

    f_mount(fs, "0:", 1);

    f_mkdir("0:/Inquire");
	f_mkdir("0:/WeightData");
    f_mkdir("0:/StationData");
    f_mkdir("0:/StationData/error");
    
    configASSERT(fs);
    configASSERT(file);
}

//f_open(file, "0:/SDtest.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ); /* 创建测试文件 */
// f_printf (file, "%s\n", "SD OK！");
// f_lseek (file, 0);
// f_read(file, str, (UINT)7, &br);
// f_close(file);
