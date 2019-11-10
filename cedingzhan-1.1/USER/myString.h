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
#ifndef MY_STRING_H
#define MY_STRING_H

#define LANGUAGES 2 //语言类别

#ifdef MY_STRING_C
const unsigned char *myString[][LANGUAGES] = {
       "English",                    "简体中文",
       "Display area",               "显示区",
       "Process:",                   "流程：",
       "Clear -> Set -> Calibrate",  "去皮->设置砝码->标定",
       "0",                          "0",
       "Sd card OK!",                "SD卡正常！",
       "SD card ERROR!",             "SD卡错误！",
       "RFID checking...",           "RFID检测中",
       "Timeout...",                 "超时",
       "EEPROM ERROR!",              "EEPROM 错误！",
       "EEPROM OK!",                 "EEPROM 正常！",
       "Weighing system",            "称重系统",
       "Motor reset",                "电机复位",
       "Single week operation",      "单周运行",
       "Continuous running",         "连续运行",
       "Stop running",               "停止运行",
       "Left door open",             "左门开",
       "Left door close",            "左门关",
       "Right door open",            "右门开",
       "Right door close",           "右门关",
       "Gate opening",               "采食门开",
       "Gate closing",               "采食门关",
       "open",                       "打开",
       "close",                      "关闭",
       "Clear:",                     "清零：",
       "Calibrate:",                 "标定：",
       "Test:",                      "测试：",
       "Set:",                       "设定：",
       "OK:",                        "成功：",
       "Error:",                     "错误：",
       "Please set the weight",      "请设置砝码",
       "Result:",                    "结果：",
       "Test OK:",                   "测试成功",
       "Clear zero",                  "清零",
       "Add fodder...",               "下料...",
       "Waiting for pigs to enter",   "等待猪进入",
       "Waiting for the pig to quit", "等待猪退出",
       "Weighing...",                  "称重中...",
       "Electronic ear mark test...", "检测电子耳标",
       "Quit OK",                    "退出完成",
       "Close the door",              "关门",
       "Data save...",                "数据保存",
       "Power Off",                   "已关机",
       "Start measurement",           "开始测定",
       "Start training",              "开始训练",
       "Step",                        "步骤",
       "Eatting",                     "采食中", 
       "Shutting down...",            "正在关机...",
	   "clear ok",                     "清零成功"
};
	   
#else
extern const unsigned char *myString[][LANGUAGES];
#endif

enum language_index
{
       StrIndex_language = 0,
       StrIndex_display_area,
       StrIndex_process,
       StrIndex_process_real,
       StrIndex_0,                     
       StrIndex_sd_card_ok,            
       StrIndex_sd_card_error,          
       StrIndex_rfid_checking,          
       StrIndex_timeout,               
       StrIndex_eeprom_error,            
       StrIndex_eeprom_ok,             
       StrIndex_weighing_system,         
       StrIndex_motor_reset,				
       StrIndex_single_week_operation,   
       StrIndex_continuous_running,         
       StrIndex_stop_running,               
       StrIndex_left_door_open,             
       StrIndex_left_door_close,           
       StrIndex_right_door_open,            
       StrIndex_right_door_close,          
       StrIndex_gate_opening,               
       StrIndex_gate_closing,               
       StrIndex_open,                       
       StrIndex_close,                     
       StrIndex_clear,                     
       StrIndex_calibrate,                 
       StrIndex_test,                     
       StrIndex_set,                       
       StrIndex_ok,                        
       StrIndex_error,                     
       StrIndex_please_set_the_weight,      
       StrIndex_result,                    
       StrIndex_test_ok,                  
       StrIndex_clear_zero,                  
       StrIndex_add_fodder,               
       StrIndex_waiting_for_pigs_to_enter,   
       StrIndex_waiting_for_the_pig_to_quit, 
       StrIndex_weighing,                  
       StrIndex_electronic_ear_mark_test, 
       StrIndex_quit_ok,                    
       StrIndex_close_the_door,                 
       StrIndex_data_save,                      
       StrIndex_power_off,                  
       StrIndex_start_measurement,             
       StrIndex_start_training,
       StrIndex_step,
       StrIndex_eatting,
	   StrIndex_shutting_down,
	   StrIndex_clear_ok,
       STR_INDEX_MAX
};


unsigned char * GetString(enum language_index);

#endif


