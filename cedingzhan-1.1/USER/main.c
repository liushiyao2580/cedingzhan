/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: main.c
Description: main func -> use to init dev.
Author: caofengli
Version: v1.0
Date: 2018.10.25
History: modified history.

2019.5.7 Modify the language to English
*****************************************************************************/
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "user_task.h"
#include "FreeRTOS.h"
#include "task.h"

#include "ff.h"         /* Declarations of FatFs API */
#include "diskio.h"     /* Declarations of device I/O functions */
#include "sdio_sdcard.h"

#include "hmi_driver.h"
#include "hmi_user_uart.h"
#include "cmd_queue.h"
#include "cmd_process.h"
#include "can.h"
#include "24cxx.h"
#include "iwdg.h"
#include "uart_rfid.h"


/*Note: the system USES CCM memory CCM-64KB can only be accessed by the kernel*/
/*Note: the FreeRTOS heap memory space USES internal sram-128kb for all AHB master buses to access*/
/*Note: all FreeRTOS tasks are implemented in the user_task.c file*/

/*
*Main function: used to initialize some hardware interfaces and start task scheduling
*/
int main(void)
{  
    /*Define heap memory: internal SRAM, starting address 0X20000000, size 128KB*/
    HeapRegion_t xHeapRegions[] = {
        { ( uint8_t * ) 0X20000000UL, 0x20000 },
        { NULL, 0 } //End of array
    };
    /*Set system interrupt priority group 4, 0-15 interrupt priority*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /*Initializes the clock and serial port*/
    delay_init(168);
	vUart1_init(230400);//Screen serial port
    vUart2_init(115200);//debug serial port
    vUart3_init(115200);//RFID serial port
    vUart6_init(115200);//ARM serial port
    /*Clear the screen serial receive buffer*/
    queue_reset();

    /*Initializes the heap memory*/
    vPortDefineHeapRegions((const HeapRegion_t *)xHeapRegions);
    DEBUG_MSG(printf("\r\n-> FreeHeapSize =  %.3f Kb <-\r\n\r\n", xPortGetFreeHeapSize() / 1024.0));

    /*Initializes EEPROM */
    AT24CXX_Init();

	
    if (AT24CXX_Check()) {
       DEBUG_MSG(printf("-> EEPROM ERROR! <-\r\n"));
		eDevice_Status1=ERR_EEPROM;
   }

    /*Gets the data saved by EEPROM */
    scPull_all_eedata(pEdata);
	vPush_one_eedata(eRESET_COUNT_ADDR, pEdata->reset_count + 1); /*update EEPROM*/

    /*Initializes SD card*/
    if (disk_initialize(0)){
      DEBUG_MSG(printf("-> SD CARD ERROR! <-\r\n"));
	  eDevice_Status = ERR_SDCARD;
    }
    /*Initialize the file system*/
    vFile_ver_malloc();
  
    /*Initialize the status light and alarm light ports*/
    vLed_init();

    /*Initialize CAN bus baud rate at 125Kbps*/
    vCan1_mode_init(CAN_SJW_1tq, CAN_BS2_6tq, CAN_BS1_7tq, 24, CAN_Mode_Normal, pEdata->can_id);

    /*Initialize the weighing system*/
    vLoad_sys_init();

    /*Initial control panel photoelectric, electromagnetic, solenoid valve, motor and other pins*/
    vHard_io_init();
	
    vClose_rfid();
    /*Delay waiting for the completion of serial screen initialization, must wait for 300ms*/
    delay_xms(300);

	/*IWDG init*/
    IWDG_Init(6, 3000);
	
    /*Enter the login interface*/ 
    SetScreen(ID_SCREEN_MAIN_PAGE);
    current_screen_id = ID_SCREEN_MAIN_PAGE;

    /*Creat the start Task*/
	/*Note: please see the specific content of the start task in user_task.c*/
    xTaskCreate((TaskFunction_t )vStart_task,
                (const char *    )"vStart_task",
                (uint16_t       )START_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )START_TASK_PRIO,
                (TaskHandle_t *  )&StartTask_Handler);
  
    /*Turn on the task scheduler*/
    vTaskStartScheduler();

    /*Note: as long as the scheduler is not turned off, the program will not execute here*/
}

