/*****************************************************************************
Copyright: 2005-2019, HZAU Electronic design competition lab.
File name: user_task.c
Description: 
Author: caofengli
Version: v1.0
Date: 2018.10.25
History: modified history.

2019.5.7 Modify the language to English
*****************************************************************************/
#include <stdarg.h>
#include <string.h>
#include "user_task.h"
#include "led.h"
#include "cmd_process.h"
#include "cmd_queue.h"
#include "sdio_sdcard.h"
#include "math.h"
#include "iwdg.h"
#include "cJSON.h"

//test github

static double weight[100];//Save the last 100 times of weight data after power on
volatile int weight_count = 0;//Weight data count
static volatile int pig_eat_status = 0;//Pig feeding signs
volatile uint64_t store_era=0; 

static sData upload_data = {
    .earid = "999999999999",
    .food_intake = 100,
    .weight = 40.1,
    .body_long = 0,
    .body_width = 4,
    .body_height = 3,
    .body_temp = 32.5,
    .env_temp = 23.6,
    .env_humi = 43,
    .start_time = "190304050607",
    .end_time = "190404050607",
};

static float last_fodder_weight, now_fodder_weight; //History heavy, current heavy
volatile ERROR_CODE eDevice_Status = WORK_STATUS_OFF; //Device state
volatile ERROR_CODE eDevice_Status1 = WORK_STATUS_OFF; //Device state

/*
*Handle to the user task function
*/
TaskHandle_t StartTask_Handler;     /*Start task*/
TaskHandle_t DctftTask_Handler;     /*Display processing task*/
TaskHandle_t LED0Task_Handler;      /*The heartbeat task*/
TaskHandle_t WorkTask_Handler;      /*Work mode task*/
TaskHandle_t CANREC_Task_Handler;   /*CAN communication receiving task*/
TaskHandle_t CANSEND_Task_Handler;  /*CAN communication sending task*/
TaskHandle_t S_DATATask_Handler;    /*Data saving and uploading task*/
TaskHandle_t UartDebugTask_Handler; /*Debug serial port sending task*/

/*
*Software timer handle
*/
TimerHandle_t StatusUpdates_Timer_Handle;
volatile unsigned char uc_sensor_status; //Used to save the sensor input signal

/*
*Message queue handle
*/
QueueHandle_t canRecQueue_Handler;/*CAN receive msg. queue*/
QueueHandle_t canSendQueue_Handler;/*CAN send msg. queue*/
QueueHandle_t uartDebugQueue_Handler;/*UART debug msg. queue*/
/*
*Event flag group handle
*/
EventGroupHandle_t EventGroupHandler;

/*user task function*/
/*!
*  \brief  start task function 
*  \param  
*/
void vStart_task(void *pvParameters)
{   	
    /*Enter critical region*/
    taskENTER_CRITICAL();
    canRecQueue_Handler = xQueueCreate(canRecQueueLength, sizeof(CanRxMsg));
    configASSERT(canRecQueue_Handler);

    canSendQueue_Handler = xQueueCreate(canSendQueueLength, sizeof(CanTxMsg));
    configASSERT(canSendQueue_Handler);

    uartDebugQueue_Handler = xQueueCreate(uartDebugQueueLength, ReportMsgBufLen);
    configASSERT(uartDebugQueue_Handler);

    EventGroupHandler = xEventGroupCreate();
    configASSERT(EventGroupHandler);

    StatusUpdates_Timer_Handle = xTimerCreate((const char * )"StatusUpdates_Timer_Handle",
                                 (TickType_t )200,
                                 (UBaseType_t )pdTRUE,
                                 (void * )1,
                                 (TimerCallbackFunction_t)StatusUpdatesCallback);
    configASSERT(StatusUpdates_Timer_Handle);
    xTimerStart(StatusUpdates_Timer_Handle, 0);

    xTaskCreate((TaskFunction_t )vUart_debug_task,
                (const char *    )"vUart_debug_task",
                (uint16_t       )UART_DEBUG_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )UART_DEBUG_TASK_PRIO,
                (TaskHandle_t *  )&UartDebugTask_Handler);

    xTaskCreate((TaskFunction_t )vLed0_task,
                (const char *    )"vLed0_task",
                (uint16_t       )LED0_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )LED0_TASK_PRIO,
                (TaskHandle_t *  )&LED0Task_Handler);

    xTaskCreate((TaskFunction_t )vCan_rec_task,
                (const char *    )"vCan_rec_task",
                (uint16_t       )CANREC_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )CANREC_TASK_PRIO,
                (TaskHandle_t *  )&CANREC_Task_Handler);

    xTaskCreate((TaskFunction_t )vCan_send_task,
                (const char *    )"vCan_send_task",
                (uint16_t       )CANSEND_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )CANSEND_TASK_PRIO,
                (TaskHandle_t *  )&CANSEND_Task_Handler);

    xTaskCreate((TaskFunction_t )vSave_data_task,
                (const char *    )"vSave_data_task",
                (uint16_t       )S_DATA_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )S_DATA_TASK_PRIO,
                (TaskHandle_t *  )&S_DATATask_Handler);

    xTaskCreate((TaskFunction_t )vDctft_task,
                (const char *    )"vDctft_task",
                (uint16_t       )DCTFT_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )DCTFT_TASK_PRIO,
                (TaskHandle_t *  )&DctftTask_Handler);
 
    if (current_screen_id == ID_SCREEN_MAIN_PAGE) {
        char str[20];
        sprintf(str, "%d", pEdata->can_id);
        SetTextValue(ID_SCREEN_MAIN_PAGE, 13, str);
		/*If the work_status value in EEPROM is true, then power on directly*/
        if (pEdata->work_status) {
            vPower_on();
        }

        else {
            vPower_off();
        }
    }

    /*Exit critical region*/
    taskEXIT_CRITICAL();
    /*Delete start task*/
    vTaskDelete(StartTask_Handler);
}

/*
*Periodic timer callback function
*/
void StatusUpdatesCallback(TimerHandle_t xTimer)
{
    static unsigned char prv_old_status = 0;
    /*Get sensor status*/
    uc_sensor_status = SENSOR_8_IN << 7 | \
                       SENSOR_7_IN << 6 | \
                       SENSOR_6_IN << 5 | \
                       SENSOR_5_IN << 4 | \
                       SENSOR_4_IN << 3 | \
                       SENSOR_3_IN << 2 | \
                       SENSOR_2_IN << 1 | \
                       SENSOR_1_IN ;

    /*The sensor whose state changes is extracted and the new state is sent. */
    /*The sensor whose state does not change does not send information*/

    unsigned char new_change;/*Record the changed bits*/
    new_change = prv_old_status ^ uc_sensor_status;
    prv_old_status = uc_sensor_status;
  
    for (char i = 0; i < 8; i++) {
        if ( (new_change >> i) & 0x01 ) {
            if (current_screen_id == ID_SCREEN_FUNC_TEST_PAGE) {
                AnimationPlayFrame(ID_SCREEN_FUNC_TEST_PAGE, 18 + i, (uc_sensor_status >> i) & 0x01);
            }

       switch (i) {
            case 2:
                if (SENSOR_3_IN) {
                    xEventGroupClearBits(EventGroupHandler, DOOR1_OFF);
                    xEventGroupSetBits(EventGroupHandler, DOOR1_ON);
                }

                else {
                    xEventGroupClearBits(EventGroupHandler, DOOR1_ON);
                    xEventGroupSetBits(EventGroupHandler, DOOR1_OFF);
                }

                break;

            case 3:
                if (SENSOR_4_IN) {
                    xEventGroupClearBits(EventGroupHandler, DOOR2_OFF);
                    xEventGroupSetBits(EventGroupHandler, DOOR2_ON);
                }

                else {
                    xEventGroupClearBits(EventGroupHandler, DOOR2_ON);
                    xEventGroupSetBits(EventGroupHandler, DOOR2_OFF);
                }

                break;
			case 4:
				 if (SENSOR_5_IN) {
					 xEventGroupClearBits(EventGroupHandler, TUOJIA_OFF);
					 xEventGroupSetBits(EventGroupHandler, TUOJIA_ON);
				 }
			
				 else {
					 xEventGroupClearBits(EventGroupHandler, TUOJIA_ON);
					 xEventGroupSetBits(EventGroupHandler, TUOJIA_OFF);
				 }
			
				 break;
            case 5:
                if (SENSOR_6_IN) {
                    xEventGroupClearBits(EventGroupHandler, CAISHI_OFF);
                    xEventGroupSetBits(EventGroupHandler, CAISHI_ON);
                }

                else {
                    xEventGroupClearBits(EventGroupHandler, CAISHI_ON);
                    xEventGroupSetBits(EventGroupHandler, CAISHI_OFF);
                }

                break;

            default:
                break;
            }
        }
    }
}

/*!
*  \brief  Serial debugging task function
*  \param 
*       xQueueSend( uartDebugQueue_Handler,(void *) "SendMsg",0);
*/
void vUart_debug_task(void *pvParameters)
{
    char c_buf[ReportMsgBufLen];
    for (;;) {
        if (pdTRUE == xQueueReceive(uartDebugQueue_Handler, c_buf, portMAX_DELAY)) {
            char *pc_buf = c_buf;
            vTaskSuspendAll();

            while (*pc_buf != '\0') {
                while ((DEBUG_UART->SR & 0X40) == 0);

                DEBUG_UART->DR = (char) * pc_buf;
                pc_buf ++;
            }

            while ((DEBUG_UART->SR & 0X40) == 0);

            DEBUG_UART->DR = (char) '\r';

            while ((DEBUG_UART->SR & 0X40) == 0);

            DEBUG_UART->DR = (char) '\n';
		
            xTaskResumeAll();
        }
    }
}

/*!
*  \brief  Display processing task function
*  \param 
*/
void vDctft_task(void *pvParameters)
{
    int size;
    for (;;) {
        size = queue_find_cmd(cmd_buffer, CMD_MAX_SIZE);

        if (size == 0) {
            vTaskDelay(50);
            continue;
        }

        if (size) {
            if (cmd_buffer[1] != 0x07) {                     //If command is not 0x07, process it! 
                ProcessMessage((PCTRL_MSG)cmd_buffer, size);
            }

            else if (cmd_buffer[1] == 0x07) {                //Soft reset STM32 if command is 0x07
                delay_xms(400);//Wait for the screen to initialize
                __disable_fault_irq();
                NVIC_SystemReset();
            }
        }
    }
}

/*!
*  \brief  Heartbeat task function
*  \param  
*/
void vLed0_task(void *pvParameters)
{
    static int times = 0;
    static int fc = 0;
	ReadRTC();

    for (;;) {
        IWDG_Feed();  //feed dogs
		AnimationPlayFrame(ID_SCREEN_MAIN_PAGE, 25, pEdata->work_status);
        CAN1_Send_Msg(NULL, eDevice_Status, heart_beat | pEdata->can_id, eREMOTE); //CAN心跳

        /* TFT状态灯闪烁 */
        if (ulTaskNotifyTake(pdTRUE, 1000) == 0) {
            DEBUG_MSG(vUsart_report_msg("CAN Disconnect!"));

            if (current_screen_id == ID_SCREEN_MAIN_PAGE) {
                if (fc++ > 3) {
                    AnimationPlayFrame(ID_SCREEN_MAIN_PAGE, 20, 1);
                }

                AnimationPlayPrev(ID_SCREEN_MAIN_PAGE, 8);
            }
        }

        else {
            if (current_screen_id == ID_SCREEN_MAIN_PAGE) {
                fc = 0;
                AnimationPlayFrame(ID_SCREEN_MAIN_PAGE, 20, 0);
                AnimationPlayPrev(ID_SCREEN_MAIN_PAGE, 8);
            }
			vTaskDelay(1000);
        }

        switch (eDevice_Status) {

			case    ERR_RFID   : /*RFID failure*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_RFID, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
	        case    ERR_MOTOR_SENSOR   : /*motor failure*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_MOTOR_SENSOR, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
	        case    ERR_SDCARD         : /*Sd card failure*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_SDCARD, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
	        case    ERR_ADC            : /*Failure of weighing sensor*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_ADC, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
			case    ERR_EEPROM            : /*Failure of EEPROM*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_EEPROM, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
	        case    ERR_CYLINDER_1       : /*Cylinder failure*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_1, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
			case	ERR_CYLINDER_2		 : /*Cylinder failure*/
				Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_2, 0);
				SetScreen(ID_SCREEN_WARNING_PAGE);
				break;
			case	ERR_CYLINDER_3		 : /*Cylinder failure*/
				Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_3, 0);
				SetScreen(ID_SCREEN_WARNING_PAGE);
				break;
			case    ERR_CYLINDER_4       : /*Cylinder failure*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_4, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
	        default:
	            break;
        }
     switch (eDevice_Status1) {
		    case    ERR_EEPROM            : /*Failure of EEPROM*/
	            Record_SetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_EEPROM, 0);
	            SetScreen(ID_SCREEN_WARNING_PAGE);
	            break;
		  default:
	            break;
	 }
        if (0 == times) {
            EventBits_t res;
            /*Gets the current time from the RTC*/
            ReadRTC();
            /*Wait for the RTC clock to update successfully*/
            res = xEventGroupWaitBits(EventGroupHandler, RTCUPDATE, pdTRUE, pdTRUE, 500);
            if(res & RTCUPDATE){
                DEBUG_MSG(vUsart_report_msg("Reset count %d", pEdata->reset_count));
				DEBUG_MSG(vUsart_report_msg("Adc overtime count %d", pEdata->adc_overtime_count));
            }
        }

        times ++;
        times %= 15;//About 20 seconds to get the time from the RTC once
    }
}

/*!
*  \brief   Work mode task function
*  \param
*/
static void vWork_task_add_fodder(float *last_fodder_weight);
static void vWork_task_clear_weight(void);
static void vWork_task_wait_pig_enter(void);


void vWork_task(void *pvParameters)
{
    uint64_t EB_NUM = 0;             //ear tag
    uint64_t STORE_ER=0;             //store ear tag
    float T_WEIGHT = 0;              //weight(0.01kg)
    uint16_t S_WEIGHT = 0;           //Feed weight(g)
    uint8_t DATE_D[2] = {12, 12};    //month day
    uint8_t DATE_T[2] = {12, 12};    //hour minute
    uint8_t Fodder_time = 0;  
	uint8_t times=0;
	EventBits_t eventbit = 0;

    for (;;) {
     
        if (pEdata->mode == 0) {/*Measurement model*/
            char s[30],arm[50];
			 uint16_t ear=0;
			vTaskDelay(1000);
            /*Open the door and wait for the pigs*/
            LEFT_DOOR_CLOSE;
            RIGHT_DOOR_CLOSE;

            FEEDING_DOOR_CLOSE;//关闭采食门
            
			SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_clear_zero));
			vWork_task_clear_weight();
			
			SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_add_fodder));
			CONTROL_liaodou_OUT(CTL_ON);//放下料斗
	        vTaskDelay(4000);//
	        xEventGroupWaitBits(EventGroupHandler, TUOJIA_ON, pdTRUE, pdTRUE, 5000); //等待料斗放下
            vTaskDelay(4000);//料斗抖动4秒
			vWork_task_add_fodder(&last_fodder_weight);//下料

            while (1) {
				/* 耳标清零 */
				EB_NUM = 0;
//				SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_clear_zero));
//			    vWork_task_clear_weight();
                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_waiting_for_pigs_to_enter));
			    vWork_task_wait_pig_enter();
                if (dWeight_data_get(0) < pEdata->k_o * 0.1 * pEdata->avgweight * 0.1) {

                }

                else if ( dWeight_data_get(0) > 2 * pEdata->avgweight * 0.1) {
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_waiting_for_the_pig_to_quit));
                    LEFT_DOOR_OPEN;
                    RIGHT_DOOR_OPEN;

                    while (1) {
						vTaskDelay(1500);
                        if (dWeight_data_get(0) < pEdata->k_o * pEdata->avgweight * 0.01) {
                            break;
                        } 
                    }
                }

                else {
                    int count_errbiao = 20;//耳标检测次数

				    /*称重*/
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_weighing));
                    T_WEIGHT = dWeight_data_get(2); //use mode 2
					
					/* 称重次数记录，每100次更新一次均重 */
                    weight[weight_count++] = T_WEIGHT;

                    if (weight_count % 100 == 0) {
                        double min_w, max_w;
                        weight_count = 0;
                        max_w = min_w = weight[0];

                        for (int i = 1; i < 100; i++) {
                            max_w < weight[i] ? (max_w = weight[i]) : (max_w = max_w);
                            min_w > weight[i] ? (min_w = weight[i]) : (min_w = min_w);
                        }

                        pEdata->avgweight = (int)(max_w + min_w) / 2 * 10;
                        vPush_one_eedata(eAVGWEIGHT_ADDR, pEdata->avgweight);/* 更新EEPROM */
                    }

                    FEEDING_DOOR_OPEN; //打开采食门
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_electronic_ear_mark_test));
                    while (count_errbiao--) {/* 等待耳标 大概1分钟*/
                        EB_NUM = 9999000 + pEdata->can_id;
                        vOpen_rfid();
                        if (1 == vGet_rfid_num(&EB_NUM)) {
                            STORE_ER=EB_NUM;
							if(1==vGet_rfid_num(&EB_NUM)){
                             if(STORE_ER==EB_NUM){
							 	if(EB_NUM>9000000){
                                break;}
							 }
							}
                        }
                    }
					ear=EB_NUM-9000000;//耳标读取正确留下后三位
					if(0 == count_errbiao){
						EB_NUM = 9999000 + pEdata->can_id;
						ear= pEdata->can_id;
                        eDevice_Status = ERR_RFID;
					}
					pig_eat_status = 1;
					xEventGroupWaitBits(EventGroupHandler, CAISHI_ON, pdTRUE, pdTRUE, 8000); //等待采食门打开
                    //vGet_img(EB_NUM);//采集图像
                    //vTaskDelay(1000);
                    /* 记录开始采食的时间 */
                    DATE_D[0] = sRtc_time.months;
                    DATE_D[1] = sRtc_time.days;
                    DATE_T[0] = sRtc_time.hours;
                    DATE_T[1] = sRtc_time.minutes;
                    sprintf(upload_data.start_time, "%02d%02d%02d%02d%02d%02d", \
                            sRtc_time.years, sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes, sRtc_time.sec);

					sprintf(s, "%ld-%.2fKg ",EB_NUM, T_WEIGHT);
					SetTextValue(ID_SCREEN_MAIN_PAGE, 6, s);

					 sprintf(arm,"EE1801%02d%02d%02d%02d%02d-%03dFFFCFFFF", \
				                  	sRtc_time.years, sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes,ear);//通知arm拍照

						send_to_arm(arm);
                    /* 利用采食时间间隙 检测是否有需要重传的数据*/
                    xTaskNotify(S_DATATask_Handler, 0x02, eSetValueWithOverwrite); /* 通知数据保存任务 */
                    vTaskDelay(1000);
                    /*等待猪退出*/
                    LEFT_DRAIN;//左门中泄

					while(1){ //等待门被推开或收到关机信号
						 DEBUG_MSG(vUsart_report_msg("OK"));
                        if(DOOR2_ON & xEventGroupWaitBits(EventGroupHandler, DOOR2_ON, \
						  pdTRUE, pdTRUE, 1000)){
							break;
						}
                        if(POWEROFF & xEventGroupWaitBits(EventGroupHandler, POWEROFF, \
					  	pdTRUE, pdTRUE, 1000)){
                          break;
				      }
					}
					
                    LEFT_DOOR_OPEN;
                    RIGHT_DOOR_OPEN;

                if(pEdata->work_status == 1){
						while (1) {
							vTaskDelay(1500);
							if (dWeight_data_get(0) < pEdata->k_o * pEdata->avgweight * 0.01) {
								break;
							}
						}
					
						SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_quit_ok));
						sprintf(upload_data.end_time, "%02d%02d%02d%02d%02d%02d", \
								sRtc_time.years, sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes, sRtc_time.sec);
					
					SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_close_the_door));
					RIGHT_DOOR_CLOSE;
					xEventGroupWaitBits(EventGroupHandler, DOOR1_OFF, pdTRUE, pdTRUE, 2000); //等待门1关闭
					LEFT_DOOR_CLOSE;
					xEventGroupWaitBits(EventGroupHandler, DOOR2_OFF, pdTRUE, pdTRUE, 5000); //等待门2关闭
					
					{
						int count = 0;
					
						while (1 == SENSOR_4_IN) {
							if (count++ > 50) {
								eDevice_Status = ERR_CYLINDER;
								break;
							}
					
							LEFT_DOOR_OPEN;
					
							vTaskDelay(1000);
							LEFT_DOOR_CLOSE;
							xEventGroupWaitBits(EventGroupHandler, DOOR2_OFF, pdTRUE, pdTRUE, 2000); //等待门1关闭
						}
					}
					
					FEEDING_DOOR_CLOSE;//关闭采食门
					CONTROL_liaodou_OUT(CTL_ON);//放下料斗
					xEventGroupWaitBits(EventGroupHandler, CAISHI_OFF, pdTRUE, pdTRUE, 1000); //等待采食门打开
					xEventGroupWaitBits(EventGroupHandler, TUOJIA_OFF, pdTRUE, pdTRUE, 5000); //等待料斗放下
					
					SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_clear_zero));
					
					if (fabs(dWeight_data_get(0)) < 5.0) {
						while (1) {
							vLoad_weight_clear();//体秤清零
					
							if (fabs(dWeight_data_get(0)) < 0.02) {
								break;
							}
						}
					}
					vTaskDelay(4000);//清零过后等待料斗稳定2S
                	}
	   				else{
	   					sprintf(upload_data.end_time, "%02d%02d%02d%02d%02d%02d", \
	   												sRtc_time.years, sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes, sRtc_time.sec);
	   					FEEDING_DOOR_CLOSE;//关闭采食门
	   					CONTROL_liaodou_OUT(CTL_ON);//放下料斗
	                    vTaskDelay(3000);
	   				}

				}

                if (EB_NUM != 0) {/* 添加文件记录 */
                    char str[50];
					now_fodder_weight = dFodder_data_get();

	                /* 获取采食量 */
	                S_WEIGHT = last_fodder_weight - now_fodder_weight;

	                if (S_WEIGHT < 0 || S_WEIGHT > pEdata->fodderweight) {
	                    S_WEIGHT = 0;
	                }
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_data_save));
                    /* 记录采食时间 */
                    Fodder_time = sRtc_time.minutes >= DATE_T[1] ? \
                                  sRtc_time.minutes - DATE_T[1] : \
                                  sRtc_time.minutes + 60 - DATE_T[1];


                    sprintf(upload_data.earid, "%ld", EB_NUM);
                    upload_data.food_intake = S_WEIGHT;
                    upload_data.weight = T_WEIGHT;

                    sprintf(str, "0:/Inquire/%s.txt", upload_data.earid);

                    if (FR_OK == f_open(file, str, FA_OPEN_APPEND | FA_WRITE)) {
                        sprintf(str, "%2d.%-2d-%02d:%02d;%2d;%4d;%6.2f;\n", \
                                DATE_D[0], DATE_D[1], DATE_T[0], DATE_T[1], \
                                Fodder_time, S_WEIGHT, T_WEIGHT);
                        f_printf(file, "%s", str);
                        f_close(file);
                    }

                    sprintf(str, "%ld", EB_NUM);
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 9, str);
                    sprintf(str, "%ld", S_WEIGHT);
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 17, str);
                    sprintf(str, "%.1f", T_WEIGHT);
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 11, str);
                    xTaskNotify(S_DATATask_Handler, 0x01, eSetValueWithOverwrite); /* 通知数据保存任务 */
                    vTaskDelay(1000);
                }
				
				pig_eat_status = 0;
				if(0 == pEdata->work_status){//触发远程控制关机
					vPower_off();
				}

                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_add_fodder));
                vTaskDelay(500);
				vWork_task_add_fodder(&last_fodder_weight);//下料

            }
        }

        else if (pEdata->mode == 1) {/* 训练模式 */
            char s[30];
            /* 关门清零 */
            LEFT_DOOR_CLOSE;
            RIGHT_DOOR_CLOSE;
            vLoad_weight_clear();//体秤清零
            /* 开门等待猪进入 （全程开门） */
            LEFT_DOOR_OPEN;
            RIGHT_DOOR_OPEN;

            FEEDING_DOOR_CLOSE;//关闭采食门

            CONTROL_liaodou_OUT(CTL_ON);//放下料斗

            vTaskDelay(5000);

            last_fodder_weight = dFodder_data_get();

            DEBUG_MSG(vUsart_report_msg("fodder_weight %.1f", last_fodder_weight));

            if (last_fodder_weight < 300.0 && last_fodder_weight > -200.0) {
                DEBUG_MSG(vUsart_report_msg("dFodder to %d g", pEdata->fodderweight));
                vMotor_run(eNORMAL);

                while (dFodder_data_get() < pEdata->fodderweight) {
                    vTaskDelay(1000);
                }

                vMotor_stop(eNORMAL);
                last_fodder_weight = dFodder_data_get();
                DEBUG_MSG(vUsart_report_msg("dFodder ok %.1f", last_fodder_weight));
            }

            CONTROL_liaodou_OUT(CTL_OFF);//抬起料斗

            while (1) {//循环
                FEEDING_DOOR_OPEN;//打开采食门
                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_waiting_for_pigs_to_enter));

                while (1) {/* 等待检测到耳标 */
                    EB_NUM = 0;
                   vOpen_rfid();
                    if (1 == vGet_rfid_num(&EB_NUM)) {/**/

                        break;
                    }

                    vTaskDelay(1000);
                }
               store_era=EB_NUM;
                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_weighing));

                /* 称重 */
                T_WEIGHT = 100 * dWeight_data_get(1);

                FEEDING_DOOR_OPEN;//打开采食门

                DATE_D[0] = sRtc_time.months;

                DATE_D[1] = sRtc_time.days;

                DATE_T[0] = sRtc_time.hours;

                DATE_T[1] = sRtc_time.minutes;

                sprintf(s, "%d:%02d-%ld", sRtc_time.hours, sRtc_time.minutes, EB_NUM);

                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, s);

                while (1) {/* 等待猪退出 */
                    if (dWeight_data_get(0) < pEdata->k_o * pEdata->avgweight * 0.01) {
                        break;
                    }

                    vTaskDelay(500);
                }

                FEEDING_DOOR_CLOSE;//关闭采食门

                CONTROL_liaodou_OUT(CTL_ON);//放下料斗

                vTaskDelay(5000);

                now_fodder_weight = dFodder_data_get();

                /* 获取采食量 */
                S_WEIGHT = last_fodder_weight - now_fodder_weight;

                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_data_save));

                if (EB_NUM != 0) {/* 添加文件记录 */
                    char str[50];
                    Fodder_time = sRtc_time.minutes >= DATE_T[1] ? \
                                  sRtc_time.minutes - DATE_T[1] : \
                                  sRtc_time.minutes + 60 - DATE_T[1];

                    sprintf(str, "0:/Inquire/%ld.txt", EB_NUM);

                    if (FR_OK == f_open(file, str, FA_OPEN_APPEND | FA_WRITE)) {
                        sprintf(str, "%2d.%-2d-%02d:%02d;%2d;%4d;%6.2f;\n", \
                                DATE_D[0], DATE_D[1], DATE_T[0], DATE_T[1], \
                                Fodder_time, S_WEIGHT,T_WEIGHT);
                        f_printf(file, "%s", str);
                        f_close(file);
                    }

                    sprintf(str, "%ld", EB_NUM);
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 9, str);
                    sprintf(str, "%ld", S_WEIGHT);
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 17, str);
                    sprintf(str, "%.1f",T_WEIGHT);
                    SetTextValue(ID_SCREEN_MAIN_PAGE, 11, str);
                }

                SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_add_fodder));
                vTaskDelay(500);
                last_fodder_weight = dFodder_data_get();

                if (last_fodder_weight < 300.0) {
                    DEBUG_MSG(vUsart_report_msg("dFodder to %d g", pEdata->fodderweight));
                    vMotor_run(eNORMAL);

                    while (dFodder_data_get() < pEdata->fodderweight) {
						vTaskDelay(1000);
                        if(times++==200){
                         eDevice_Status=ERR_MOTOR_SENSOR;
						 vPower_off();
						 break;
						}						
                    }

                    vMotor_stop(eNORMAL);
                    last_fodder_weight = dFodder_data_get();
                    DEBUG_MSG(vUsart_report_msg("dFodder ok %.1f", last_fodder_weight));
                }

                CONTROL_liaodou_OUT(CTL_OFF);//抬起料斗

            }
        }
    }
}

/* 处理CAN总线接收到的数据函数声明 */
static void prv_process_can_msg(CanRxMsg *const px_rx_message);
/*!
*  \brief CAN总线接收任务函数
*  \param 任务传入参数
*/
void vCan_rec_task(void *pvParameters)
{
    CanRxMsg x_rx_message;

    for (;;) {
        /* 从消息队列中拉取一条消息，并处理*/
        /* 队列为空时此任务永久阻塞，直到队列不为空 */
        if (pdTRUE == xQueueReceive(canRecQueue_Handler, (CanRxMsg *)&x_rx_message, portMAX_DELAY)) {
            prv_process_can_msg(&x_rx_message);
        }
    }
}

/*!
*  \brief CAN总线发送任务函数
*  \param 任务传入参数
*/
void vCan_send_task(void *pvParameters)
{
    CanTxMsg x_tx_message;

    for (;;) {
        /* 从消息队列中拉取一条消息，并处理*/
        /* 队列为空时此任务永久阻塞，直到队列不为空 */
        if (pdTRUE == xQueueReceive(canSendQueue_Handler, (CanTxMsg *)&x_tx_message, portMAX_DELAY)) {
            /* 挂起调度器 */
            unsigned char uc_mbox, resent;
            vTaskSuspendAll();
		    u16 count = 0;
            u16 res = 0;
            resent = 0;
start: uc_mbox = CAN_Transmit(CAN1, &x_tx_message);

			count = 0;
            while ( ((res = (CAN_TransmitStatus(CAN1, uc_mbox))) == CAN_TxStatus_Pending)){
				count ++;
				delay_us(1);
				if(count > 10000){
					break;
				}
			};//等待发送结束

            if (res != CAN_TxStatus_Ok) {
                int err_num;
                /*发送出错*/
                /*获取错误原因*/
                err_num = CAN_GetLastErrorCode(CAN1);

                if (x_tx_message.RTR == CAN_RTR_Data) {
                     DEBUG_MSG(vUsart_report_msg("goto start"));
                    delay_us(200);

                    if (resent++ < 5) {
                        goto start;
                    }

                    DEBUG_MSG(vUsart_report_msg("Error: Transmit %d", err_num));
                }

            }

            xTaskResumeAll();
            delay_us(200);
//            {
//                int a = CAN_GetLSBTransmitErrorCounter(CAN1);
//                int b = CAN_GetLastErrorCode(CAN1);
//                int c = CAN_GetReceiveErrorCounter(CAN1);

//                if (a | b | c) {
//                    DEBUG_MSG(vUsart_report_msg("Error://count_%d - last_%d - re %d", a, b, c));
//                }
//            }
        }
    }

}

/*!
*  \brief 数据保存任务函数
*  \param 任务传入参数
*/
void vSave_data_task(void *pvParameters)
{
    uint32_t pulNotificationValue = 0;
    EventBits_t eventbit = 0;
    FIL f, *file;
    file = &f;

    for (;;) {
        pulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (pulNotificationValue == 0x01) {//数据打包上传
            char *Uploaded_Data = NULL;
            cJSON *root = NULL;

            root = cJSON_CreateObject();

            if (NULL == root) {
                configASSERT(root);
                continue;
            }

            cJSON_AddStringToObject(root, "earid", upload_data.earid);
            cJSON_AddNumberToObject(root, "food_intake", upload_data.food_intake);
            cJSON_AddNumberToObject(root, "weight", upload_data.weight);
            cJSON_AddNumberToObject(root, "body_long", upload_data.body_long);
            cJSON_AddNumberToObject(root, "body_width", upload_data.body_width);
            cJSON_AddNumberToObject(root, "body_height", upload_data.body_height);
            cJSON_AddNumberToObject(root, "body_temp", upload_data.body_temp);
            cJSON_AddNumberToObject(root, "env_temp", upload_data.env_temp);
            cJSON_AddNumberToObject(root, "env_humi", upload_data.env_humi);
            cJSON_AddStringToObject(root, "start_time", upload_data.start_time);
            cJSON_AddStringToObject(root, "end_time", upload_data.end_time);
            Uploaded_Data = cJSON_Print(root);

            if (NULL == Uploaded_Data) {
                configASSERT(Uploaded_Data);
                continue;
            }

            {
                //存入SD卡备份文件夹
                char filename[50];
                /*数据写入SD卡*/
                DEBUG_MSG(vUsart_report_msg("Data save.."));
                sprintf(filename, "0:/StationData/%02d%02d%02d%02d%02d.txt", \
                        sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes, sRtc_time.sec);

                if (FR_OK == f_open(file, filename, FA_CREATE_ALWAYS | FA_WRITE)) { /* 创建文件 */
                    f_printf (file, "%s", Uploaded_Data);
                    f_close(file);
                    DEBUG_MSG(vUsart_report_msg("Data has been saved"));
                }

            }

            CAN1_Send_Msg(NULL, eDevice_Status, data_object_request | pEdata->can_id, eREMOTE); //请求发送数据

            eventbit = xEventGroupWaitBits(EventGroupHandler, DATA_REQUEST, pdTRUE, pdTRUE, 2000);

            if (eventbit & DATA_REQUEST) {
                u8 all_count[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
                int frame_count, last_frame_dlc, data_len;
                data_len = strlen(Uploaded_Data);
                frame_count =  data_len / 8;
                last_frame_dlc = data_len % 8;

                /*数据发送到CAN总线*/
                for (uint8_t frameNUM = 1; frameNUM <= frame_count; frameNUM++) {

                    CAN1_Send_Msg((u8 *)(Uploaded_Data + 8 * (frameNUM - 1)), 8, 0x700 | pEdata->can_id, eDATA);
                }

                CAN1_Send_Msg((u8 *)(Uploaded_Data + 8 * (frame_count)), last_frame_dlc, 0x700 | pEdata->can_id, eDATA);
                all_count[0] = frame_count + 1;
                CAN1_Send_Msg(all_count, 8, 0 | pEdata->can_id, eDATA);//校验
                CAN1_Send_Msg("hello !!", 8, 0x300 | pEdata->can_id, eDATA);
                eventbit = xEventGroupWaitBits(EventGroupHandler, RECV_COMPLETE, pdTRUE, pdTRUE, 3000);

                if ( eventbit & RECV_COMPLETE) {
               DEBUG_MSG(vUsart_report_msg("Send ok"));
                }

                else {
                    char filename[50];
                    /*总线存在错误
                    数据写入SD卡待恢复文件夹*/
                    DEBUG_MSG(vUsart_report_msg("Send error"));
                    sprintf(filename, "0:/StationData/error/%02d%02d%02d%02d%02d.txt", \
                            sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes, sRtc_time.sec);

                    if (FR_OK == f_open(file, filename, FA_CREATE_ALWAYS | FA_WRITE)) { /* 创建文件 */
                        f_printf (file, "%s", Uploaded_Data);
                        f_close(file);
                        DEBUG_MSG(vUsart_report_msg("Error data sent has been saved"));
                    }

                }
            }

            else {
                char filename[50];
                /*总线存在错误
                数据写入SD卡待恢复文件夹*/
                DEBUG_MSG(vUsart_report_msg("Send error"));
                sprintf(filename, "0:/StationData/error/%02d%02d%02d%02d%02d.txt", \
                        sRtc_time.months, sRtc_time.days, sRtc_time.hours, sRtc_time.minutes, sRtc_time.sec);

                if (FR_OK == f_open(file, filename, FA_CREATE_ALWAYS | FA_WRITE)) { /* 创建文件 */
                    f_printf (file, "%s", Uploaded_Data);
                    f_close(file);
                    DEBUG_MSG(vUsart_report_msg("Error data sent has been saved"));
                }
            }

            cJSON_Delete(root);
            vPortFree(Uploaded_Data);
            Uploaded_Data = NULL;
        }

        if (pulNotificationValue == 0x02) {
            DEBUG_MSG(vUsart_report_msg("data reupload !!!"));
            char *ReUploaded_Data;
            char filename[50];
            /* 扫描SD保存需要重传数据的文件夹 */
            FRESULT res;
            DIR dir;
            FIL fd;
            static FILINFO fno;
            res = f_opendir(&dir, "0:/StationData/error");         /* Open the directory */

            if (res == FR_OK) {
                for (int times = 0; ; times++) {
                    res = f_readdir(&dir, &fno);                   /* Read a directory item */

                    if (res != FR_OK || fno.fname[0] == 0) {
                        DEBUG_MSG(vUsart_report_msg("no data need reupload"));
                        break;
                    }

                    else {
                        sprintf(filename, "0:/StationData/error/%s", fno.fname);

                        if (FR_OK == f_open(&fd, filename, FA_OPEN_EXISTING | FA_READ)) {
                            int size = 0;
                            UINT br;
                            size = f_size(&fd);
                            ReUploaded_Data = pvPortMalloc(size);

                            f_lseek(&fd, 0);
                            f_read(&fd, ReUploaded_Data, size, &br);

                            if (br == size) {
                                DEBUG_MSG(vUsart_report_msg("read %d bytes", br));
                            }

                            f_close(&fd);

                            CAN1_Send_Msg(NULL, eDevice_Status, data_object_request | pEdata->can_id, eREMOTE);    //请求发送数据

                            eventbit = xEventGroupWaitBits(EventGroupHandler, DATA_REQUEST, pdTRUE, pdTRUE, 2000);

                            if (eventbit & DATA_REQUEST) {
                                u8 all_count[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
                                int frame_count, last_frame_dlc, data_len;
                                data_len = size;
                                frame_count =  data_len / 8;
                                last_frame_dlc = data_len % 8;

                                /*数据发送到CAN总线*/
                                for (uint8_t frameNUM = 1; frameNUM <= frame_count; frameNUM++) {

                                    CAN1_Send_Msg((u8 *)(ReUploaded_Data + 8 * (frameNUM - 1)), 8, 0x700 | pEdata->can_id, eDATA);
                                }

                                CAN1_Send_Msg((u8 *)(ReUploaded_Data + 8 * (frame_count)), last_frame_dlc, 0x700 | pEdata->can_id, eDATA);
                                all_count[0] = frame_count + 1;
                                CAN1_Send_Msg(all_count, 8, 0 | pEdata->can_id, eDATA);
                                CAN1_Send_Msg("hello !!", 8, 0x300 | pEdata->can_id, eDATA);
                                //xTaskNotifyWait((uint32_t)(~0x00), (uint32_t)(~0x00), &pulNotificationValue, 200); /* 等待发送结果 */
                                eventbit = xEventGroupWaitBits(EventGroupHandler, RECV_COMPLETE, pdTRUE, pdTRUE, 3000);

                                if ( eventbit & RECV_COMPLETE) {
                                    DEBUG_MSG(vUsart_report_msg("Re Send ok"));
                                    /* 删除重传成功的文件 */
                                    f_rmdir(filename);
                                }

                                else {
                                    break;
                                }
                            }

                            else {
                                break;
                            }

                            vPortFree(ReUploaded_Data);
                        }
                    }
                }

                f_closedir(&dir);
            }
        }
    }

}
/*!
*  \brief 处理CAN总线接收到的数据
*  \retval 无
*  \brief CAN通信处理任务内部调用
*/
static void prv_process_can_msg(CanRxMsg *const px_rx_message)
{
    u8 c_buf[9];
    short s_id = 0, s_cmd = 0;

    if (px_rx_message->RTR == CAN_RTR_Remote) { /*远程帧*/

        switch (px_rx_message->StdId & 0x780) {
        case heart_beat:
            xTaskNotifyGive(LED0Task_Handler);/* 通知LED任务 CAN 连接正常 */
            break;

        case data_object_request:
            xEventGroupSetBits(EventGroupHandler, DATA_REQUEST);
            break;

        case recv_complete:
            xEventGroupSetBits(EventGroupHandler, RECV_COMPLETE);
            break;

        case close_device:
			if (eDevice_Status < 3) {
                 eDevice_Status = WORK_STATUS_OFF;
            }
            vPower_off();
            break;

        case open_device:
            vPower_on();
            break;

        case measure_mode:
            vPower_off();//切换模式前先停止工作
            pEdata->mode = 0;/* 测定 */
            vPush_one_eedata(eMODE_ADDR, pEdata->mode);
            DEBUG_MSG(vUsart_report_msg("pEdata->mode0"));
            break;

        case train_mode:
            vPower_off();//切换模式前先停止工作
            pEdata->mode = 1;/* 训练 */
            vPush_one_eedata(eMODE_ADDR, pEdata->mode);
            DEBUG_MSG(vUsart_report_msg("pEdata->mode1"));
            break;

        default : break;
        }
    }

    else if (px_rx_message->RTR == CAN_RTR_Data) { /*数据帧*/
        strncpy(c_buf, px_rx_message->Data, (unsigned int)px_rx_message->DLC);
        c_buf[px_rx_message->DLC] = '\0';

        switch (px_rx_message->StdId & 0x780) {
            uint16_t sum = 0;

        case time_stamp_request:
            sum = 0;

            for (int i = 0; i < 7; i++) {
                sum +=  px_rx_message->Data[i];
            }

            if ((0xff & sum) == px_rx_message->Data[7]) {
                DEBUG_MSG(vUsart_report_msg("online time update SUCCESS!"));
                SetRTC(px_rx_message->Data[0] % 10 + (px_rx_message->Data[0] / 10) * 16, \
                       px_rx_message->Data[1] % 10 + (px_rx_message->Data[1] / 10) * 16, \
                       px_rx_message->Data[6] % 10 + (px_rx_message->Data[6] / 10) * 16, \
                       px_rx_message->Data[2] % 10 + (px_rx_message->Data[2] / 10) * 16, \
                       px_rx_message->Data[3] % 10 + (px_rx_message->Data[3] / 10) * 16, \
                       px_rx_message->Data[4] % 10 + (px_rx_message->Data[4] / 10) * 16, \
                       px_rx_message->Data[5] % 10 + (px_rx_message->Data[5] / 10) * 16);
            }

            else {
                DEBUG_MSG(vUsart_report_msg("online time update ERROR!"));
            }

            break;

        default :
            break;
        }
    }

    memset(px_rx_message, 0, sizeof(CanRxMsg));
}

/*!
*  \brief 串口数据格式化发送函数
*  \param 同printf函数  vUsart_report_msg("test: %d %.2f",120, 35.51);
*  \retval 无
*  \brief 数据发送至消息发送队列（需要定义好发送队列：uartDebugQueue）
*          通过串口发送任务进行发送
*/
void vUsart_report_msg(const char *pc_fmt, ...)
{
    char c_msg_buf[ReportMsgBufLen];/*定义字符缓冲区*/
    va_list x_ap;

    va_start (x_ap, pc_fmt);
    vsnprintf( c_msg_buf, ReportMsgBufLen, pc_fmt, x_ap);
    va_end (x_ap);
    xQueueSend(uartDebugQueue_Handler, c_msg_buf, 5); /*发送至uartDebugQueue消息队列*/
}

void vPower_off(void)   //结束工作模式
{     
	    SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_shutting_down));
		vTaskDelay(1000);
		if (pig_eat_status) {
	        pEdata->work_status = 0;
			xEventGroupSetBits(EventGroupHandler, POWEROFF);
			return;
		}

	    vMotor_stop(eCYCLE);//关机时电机要停转
	    LEFT_DRAIN;
		RIGHT_DOOR_OPEN;
		FEEDING_DOOR_CLOSE;
	    CONTROL_liaodou_OUT(CTL_OFF);//抬起料斗
        SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_power_off));

        if (eDevice_Status < 3) {
            eDevice_Status = WORK_STATUS_OFF;
        }

        pEdata->work_status = 0;
		AnimationPlayFrame(ID_SCREEN_MAIN_PAGE, 25, pEdata->work_status);
        vPush_one_eedata(eWORK_STATUS_ADDR, pEdata->work_status);/* 更新EEPROM */
		xTaskNotify(S_DATATask_Handler, 0x02, eSetValueWithOverwrite); /* 通知数据保存任务 */

		if (WorkTask_Handler != NULL) {
			DEBUG_MSG(vUsart_report_msg("POWER OFF"));
			vTaskDelete(WorkTask_Handler);
			WorkTask_Handler = NULL;
		}
}


void vPower_on(void)   //开始工作模式
{  
	if (WorkTask_Handler != NULL && pEdata->work_status == 1) {
		return;
	}
    pEdata->work_status = 1;
	vPush_one_eedata(eWORK_STATUS_ADDR, pEdata->work_status);/* 更新EEPROM */
	AnimationPlayFrame(ID_SCREEN_MAIN_PAGE, 25, pEdata->work_status);
    if (pEdata->mode == 0) {
        SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_start_measurement)); 
    }

    else {
        SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_start_training));
    }
	
    if (eDevice_Status < 3) {
        eDevice_Status = WORK_STATUS_ON;
    }

    DEBUG_MSG(vUsart_report_msg("POWER ON"));
    xTaskCreate((TaskFunction_t )vWork_task,
                (const char *    )"vWork_task",
                (uint16_t       )WORK_STK_SIZE,
                (void *          )NULL,
                (UBaseType_t    )WORK_TASK_PRIO,
                (TaskHandle_t *  )&WorkTask_Handler);

}

FRESULT scan_files (
    char *path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    UINT j = 0;
    static FILINFO fno;

    res = f_opendir(&dir, path);                       /* Open the directory */

    if (res == FR_OK) {
        int64_t value = 0;

        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */

            if (res != FR_OK || fno.fname[0] == 0) {
                break;    /* Break on error or end of dir */
            }

            /* It is a file. */
            sscanf(fno.fname, "%ld", &value);
            sprintf(fno.fname, "%ld", value);
            SetTextValue(ID_SCREEN_INQUIRE_PAGE, j + 2, fno.fname);
            j++;
        }

        for (; j < 17; j++) {
            SetTextValue(ID_SCREEN_INQUIRE_PAGE, j + 2, "");
        }

        f_closedir(&dir);
    }

    return res;
}

FRESULT delete_files (
    char *path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */

    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */

            if (res != FR_OK || fno.fname[0] == 0) {
                break;    /* Break on error or end of dir */
            }

            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */

                if (res != FR_OK) {
                    break;
                }

                path[i] = 0;
            }

            else {                                         /* It is a file. */
                char str[50];
                sprintf(str, "%s/%s", path, fno.fname);
                f_unlink(str);
            }
        }

        f_closedir(&dir);
    }

    return res;
}

static void vWork_task_add_fodder(float *last_fodder_weight)
{
	 EventBits_t eventbit = 0;
//
	//CONTROL_liaodou_OUT(CTL_ON);//放下料斗
	//vTaskDelay(4000);//
	//xEventGroupWaitBits(EventGroupHandler, TUOJIA_ON, pdTRUE, pdTRUE, 5000); //等待料斗放下
   // vTaskDelay(4000);//料斗抖动4秒

	*last_fodder_weight = dFodder_data_get();
	
	if (*last_fodder_weight < pEdata->min_fodderweight) {
		int count = 0;
		vMotor_run(eNORMAL);
	
		while (*last_fodder_weight < pEdata->fodderweight) {
			vTaskDelay(1000);
			eventbit = xEventGroupWaitBits(EventGroupHandler, XAILIAO, pdTRUE, pdTRUE, 6000);
			if(eventbit & XAILIAO){
				*last_fodder_weight = dFodder_data_get();
			}

			else{
				eDevice_Status = ERR_MOTOR_SENSOR;//下料故障
				pig_eat_status = 0;
				vPower_off();//关机
			}
		}
	
		vMotor_stop(eNORMAL);
		vTaskDelay(4000);
		*last_fodder_weight = dFodder_data_get();
		DEBUG_MSG(vUsart_report_msg("dFodder ok %.1f", *last_fodder_weight));
	}
	CONTROL_liaodou_OUT(CTL_OFF);//抬起料斗

}

static void vWork_task_clear_weight(void)
{
	if (fabs(dWeight_data_get(0)) < 5.0) {
		while (1) {
			vLoad_weight_clear();//体秤清零

			if (fabs(dWeight_data_get(0)) < 0.02) {
				break;
			}
		}
	}
}

static void vWork_task_wait_pig_enter(void)
{
	EventBits_t eventbit = 0;
	/* 开门等待猪进入 */
	LEFT_DOOR_OPEN;
	RIGHT_DOOR_OPEN;
	
	while (1) { //体重大于设定值0.6 且检测到信号
		double temp;
		vTaskDelay(2000);
		temp = dWeight_data_get(0);
	
		if ((temp > pEdata->k_i * pEdata->avgweight * 0.01 )) {
			break;
		}

	}
	
	
	SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_close_the_door));
	LEFT_DOOR_CLOSE;
	xEventGroupWaitBits(EventGroupHandler, DOOR2_OFF, pdTRUE, pdTRUE, 2000); //等待门2关闭
	RIGHT_DOOR_CLOSE;
	xEventGroupWaitBits(EventGroupHandler, DOOR1_OFF, pdTRUE, pdTRUE, 2000); //等待门1关闭
	
	{
		int count = 0;
	
		while (1 == SENSOR_3_IN) {
			if (count++ > 10) {
				break;
			}

			RIGHT_DOOR_OPEN;
			vTaskDelay(2000);
			RIGHT_DOOR_CLOSE;
			xEventGroupWaitBits(EventGroupHandler, DOOR1_OFF, pdTRUE, pdTRUE, 2000); //等待门1关闭
		}
		if(SENSOR_3_IN == 1){
			eDevice_Status = ERR_CYLINDER_1;
		}
		if(SENSOR_4_IN == 1){
			eDevice_Status = ERR_CYLINDER_2;
		}
	}


}

