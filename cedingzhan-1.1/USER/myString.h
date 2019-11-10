/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: myString.h
Description: �ַ���ͳһ���������ԣ�
Author: caofengli
Version: v1.1
Date: 2019.6.19
Note: ���ļ������ʽ: Gb2312, �����޸ġ�

 ������������������utf8����������ڱ����UI����ʾ���룡����������������
 
History: modified history.
*****************************************************************************/
#ifndef MY_STRING_H
#define MY_STRING_H

#define LANGUAGES 2 //�������

#ifdef MY_STRING_C
const unsigned char *myString[][LANGUAGES] = {
       "English",                    "��������",
       "Display area",               "��ʾ��",
       "Process:",                   "���̣�",
       "Clear -> Set -> Calibrate",  "ȥƤ->��������->�궨",
       "0",                          "0",
       "Sd card OK!",                "SD��������",
       "SD card ERROR!",             "SD������",
       "RFID checking...",           "RFID�����",
       "Timeout...",                 "��ʱ",
       "EEPROM ERROR!",              "EEPROM ����",
       "EEPROM OK!",                 "EEPROM ������",
       "Weighing system",            "����ϵͳ",
       "Motor reset",                "�����λ",
       "Single week operation",      "��������",
       "Continuous running",         "��������",
       "Stop running",               "ֹͣ����",
       "Left door open",             "���ſ�",
       "Left door close",            "���Ź�",
       "Right door open",            "���ſ�",
       "Right door close",           "���Ź�",
       "Gate opening",               "��ʳ�ſ�",
       "Gate closing",               "��ʳ�Ź�",
       "open",                       "��",
       "close",                      "�ر�",
       "Clear:",                     "���㣺",
       "Calibrate:",                 "�궨��",
       "Test:",                      "���ԣ�",
       "Set:",                       "�趨��",
       "OK:",                        "�ɹ���",
       "Error:",                     "����",
       "Please set the weight",      "����������",
       "Result:",                    "�����",
       "Test OK:",                   "���Գɹ�",
       "Clear zero",                  "����",
       "Add fodder...",               "����...",
       "Waiting for pigs to enter",   "�ȴ������",
       "Waiting for the pig to quit", "�ȴ����˳�",
       "Weighing...",                  "������...",
       "Electronic ear mark test...", "�����Ӷ���",
       "Quit OK",                    "�˳����",
       "Close the door",              "����",
       "Data save...",                "���ݱ���",
       "Power Off",                   "�ѹػ�",
       "Start measurement",           "��ʼ�ⶨ",
       "Start training",              "��ʼѵ��",
       "Step",                        "����",
       "Eatting",                     "��ʳ��", 
       "Shutting down...",            "���ڹػ�...",
	   "clear ok",                     "����ɹ�"
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


