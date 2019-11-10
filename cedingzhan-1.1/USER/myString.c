/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: myString.h
Description: 字符串统一管理（多语言）
Author: caofengli
Version: v1.1
Date: 2019.6.19
Note: 本文件编码格式: Gb2312, 请勿修改。

 ！！！！！！！！！utf8编码的中文在编译后UI上显示乱码！！！！！！！！！
 
History: modified history.
*****************************************************************************/
#define MY_STRING_C

#include "myString.h"
#include "24cxx.h"

unsigned char * GetString(enum language_index index)
{
  if(pEdata->language >= LANGUAGES || pEdata->language < 0){
    return (unsigned char *)"no lang.";
  }
  if(index > STR_INDEX_MAX || index < 0){
	return (unsigned char *)"no str.";
  }
  return (unsigned char *)myString[index][pEdata->language];
}


