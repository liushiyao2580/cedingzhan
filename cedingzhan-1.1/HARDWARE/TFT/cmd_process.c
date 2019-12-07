#include "cmd_process.h"
#include "user_task.h"
#include "uart_rfid.h"
#include "myString.h"
#define TIME_100MS 10                                                                //100毫秒(10个单位)
volatile uint32  timer_tick_count = 0;                                               //定时器节拍

uint8_t  cmd_buffer[CMD_MAX_SIZE];                                                     //指令缓存
volatile uint16_t current_screen_id = 0;                                               //当前画面ID
static int k = 0;//记录压脚标定次数

void UpdateUI(void);                                                                 //更新UI数据
static int str2int(char *str, int len);
/*!
*  \brief  消息处理流程
*  \param msg 待处理消息
*  \param size 消息长度
*/

void ProcessMessage( PCTRL_MSG msg, uint16 size )
{
    uint8 cmd_type = msg->cmd_type;                                                  //指令类型
    uint8 ctrl_msg = msg->ctrl_msg;                                                  //消息的类型
    uint8 control_type = msg->control_type;                                          //控件类型
    uint16 screen_id = PTR2U16(&msg->screen_id);                                     //画面ID
    uint16 control_id = PTR2U16(&msg->control_id);                                   //控件ID
    uint32 value = PTR2U32(msg->param);                                              //数值


    switch (cmd_type) {
    case NOTIFY_TOUCH_PRESS:                                                        //触摸屏按下
    case NOTIFY_TOUCH_RELEASE:                                                      //触摸屏松开
        NotifyTouchXY(cmd_buffer[1], PTR2U16(cmd_buffer + 2), PTR2U16(cmd_buffer + 4));
        break;

    case NOTIFY_WRITE_FLASH_OK:                                                     //写FLASH成功
        NotifyWriteFlash(1);
        break;

    case NOTIFY_WRITE_FLASH_FAILD:                                                  //写FLASH失败
        NotifyWriteFlash(0);
        break;

    case NOTIFY_READ_FLASH_OK:                                                      //读取FLASH成功
        NotifyReadFlash(1, cmd_buffer + 2, size - 6);                               //去除帧头帧尾
        break;

    case NOTIFY_READ_FLASH_FAILD:                                                   //读取FLASH失败
        NotifyReadFlash(0, 0, 0);
        break;

    case NOTIFY_READ_RTC:                                                            //读取RTC时间
        NotifyReadRTC(cmd_buffer[2], cmd_buffer[3], cmd_buffer[4], cmd_buffer[5], cmd_buffer[6], cmd_buffer[7], cmd_buffer[8]);
        break;

    case NOTIFY_CONTROL: {
        if (ctrl_msg == MSG_GET_CURRENT_SCREEN) {                               //画面ID变化通知
            NotifyScreen(screen_id);                                            //画面切换调动的函数
        }

        else {
            switch (control_type) {
            case kCtrlButton:                                                   //按钮控件
                NotifyButton(screen_id, control_id, msg->param[1]);
                break;

            case kCtrlText:                                                     //文本控件
                NotifyText(screen_id, control_id, msg->param);
                break;

            case kCtrlProgress:                                                 //进度条控件
                NotifyProgress(screen_id, control_id, value);
                break;

            case kCtrlSlider:                                                   //滑动条控件
                NotifySlider(screen_id, control_id, value);
                break;

            case kCtrlMeter:                                                    //仪表控件
                NotifyMeter(screen_id, control_id, value);
                break;

            case kCtrlMenu:                                                     //菜单控件
                NotifyMenu(screen_id, control_id, msg->param[0], msg->param[1]);
                break;

            case kCtrlSelector:                                                 //选择控件
                NotifySelector(screen_id, control_id, msg->param[0]);
                break;

            case kCtrlRTC:                                                      //倒计时控件
                NotifyTimer(screen_id, control_id);
                break;

            default:
                break;
            }
        }

        break;
    }

    case NOTIFY_HandShake:                                                          //握手通知
        NOTIFYHandShake();
        break;

    default:
        break;
    }
}
/*!
*  \brief  握手通知
*/
void NOTIFYHandShake()
{

}

/*!
*  \brief  画面切换通知
*  \details  当前画面改变时(或调用GetScreen)，执行此函数
*  \param screen_id 当前画面ID
*/
extern volatile int M_ALL;//M_ALL用于整体标定
void NotifyScreen(uint16 screen_id)
{
    char str[10];
    //TODO: 添加用户代码
    current_screen_id = screen_id;

    if (current_screen_id == ID_SCREEN_MAIN_PAGE) {//主页

		AnimationPlayFrame(ID_SCREEN_MAIN_PAGE, 25, pEdata->work_status);
		if(0 == pEdata->work_status){
           SetTextValue(ID_SCREEN_MAIN_PAGE, 6, GetString(StrIndex_power_off));
		}
		
    }


    if (current_screen_id == ID_SCREEN_FUNC_SWITCH_PAGE) {
        if (pEdata->mode == 0) {
            SetButtonValue(ID_SCREEN_FUNC_SWITCH_PAGE, 6, 1);
            SetButtonValue(ID_SCREEN_FUNC_SWITCH_PAGE, 7, 0);
        }

        else {
            SetButtonValue(ID_SCREEN_FUNC_SWITCH_PAGE, 6, 0);
            SetButtonValue(ID_SCREEN_FUNC_SWITCH_PAGE, 7, 1);
        }
    }

    if (current_screen_id == ID_SCREEN_FUNC_TEST_PAGE) {
	    char str[15];
	    sprintf(str,"%d / %d",pEdata->reset_count, pEdata->adc_overtime_count);
	    SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, str);
        //SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_display_area));
    }

    if (current_screen_id == ID_SCREEN_CALIBRATION_PAGE) {
        /* 进入标定页面关机 */
        vPower_off();
    }
	
	if (current_screen_id == ID_SCREEN_FODDER_CALIBRATION_PAGE) {
		char str[10];
	    sprintf(str,"%d",pEdata->m2);
	    CONTROL_liaodou_OUT(CTL_ON);
		SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 7, str);
		SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_process));
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, GetString(StrIndex_process_real));
    }
    if (current_screen_id == ID_SCREEN_CALIBRATION_PAGE) {
        k=0;
	}
	
	if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
		char str[10];
		M_ALL = 0;
	 //   k = 0;
	    sprintf(str,"%d",pEdata->m1);
		SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 12, str);
		SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 1, GetString(StrIndex_0));
		SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_process));
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, GetString(StrIndex_process_real));
    }
	

    if (current_screen_id == ID_SCREEN_INQUIRE_PAGE) {
        scan_files("0:/Inquire");
    }

    if (current_screen_id == ID_SCREEN_SYS_SETUP_PAGE) {
        char str[10];
        sprintf(str, "%d", pEdata->fodderweight);
        SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 10, str);

        sprintf(str, "%.1f", 0.1 * pEdata->k_i);
        SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 11, str);

        sprintf(str, "%.1f", 0.1 *  pEdata->k_o);
        SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 12, str);

        sprintf(str, "%.1f", 0.1 *  pEdata->avgweight);
        SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 9, str);

        sprintf(str, "%3d", pEdata->min_fodderweight);
        SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 26, str);

		SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 22, GetString(StrIndex_language));
        CAN1_Send_Msg(NULL, 2, time_stamp_request | pEdata->can_id, eREMOTE); //CAN心跳
    }

}

/*!
*  \brief  触摸坐标事件响应
*  \param press 1按下触摸屏，3松开触摸屏
*  \param x x坐标
*  \param y y坐标
*/
void NotifyTouchXY(uint8 press, uint16 x, uint16 y)
{
    //TODO: 添加用户代码
}

/*!
*  \brief  更新数据
*/
void UpdateUI()
{

}

/*!
*  \brief  按钮控件通知
*  \details  当按钮状态改变(或调用GetControlValue)时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param state 按钮状态：0弹起，1按下
*/
void NotifyButton(uint16 screen_id, uint16 control_id, uint8  state)
{
    DEBUG_MSG(printf("%d,%d\r\n", screen_id, control_id));
    char str[20] = {'\0'};

    if (screen_id == ID_SCREEN_MAIN_PAGE) {
        if (control_id = 10 && state == 0) {

		  if(pEdata->work_status){
			  vPower_off();   //如果是开机状态就关机，否则就开机
		  }
		  else{
			  vPower_on();
		  }

        }
    }

    if (screen_id == ID_SCREEN_WARNING_PAGE) {
        if (control_id = 4 && state == 0) {
            SetScreen(current_screen_id);
            Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_RFID, 0);
            Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_MOTOR_SENSOR, 0);
            Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_SDCARD, 0);
            Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_ADC, 0);
			Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_EEPROM, 0);
			Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_1, 0);
			Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_2, 0);
			Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_3, 0);
			Record_ResetEvent(ID_SCREEN_WARNING_PAGE, 3, ERR_CYLINDER_4, 0);
			eDevice_Status = pEdata->work_status ? WORK_STATUS_ON : WORK_STATUS_OFF;
			eDevice_Status1 = pEdata->work_status ? WORK_STATUS_ON : WORK_STATUS_OFF;
        }
    }

    /* 测定--训练模式切换 */
    if (screen_id == ID_SCREEN_FUNC_SWITCH_PAGE) {
        if (control_id == 6 && state == 1) {
            vPower_off();//切换模式前先停止工作
            pEdata->mode = 0;/* 测定 */
            vPush_one_eedata(eMODE_ADDR, pEdata->mode);
            DEBUG_MSG(vUsart_report_msg("pEdata->mode0"));
        }

        if (control_id == 7 && state == 1) {
            vPower_off();//切换模式前先停止工作
            pEdata->mode = 1;/* 训练 */
            vPush_one_eedata(eMODE_ADDR, pEdata->mode);
            DEBUG_MSG(vUsart_report_msg("pEdata->mode1"));
        }
    }

    if (screen_id == ID_SCREEN_FUNC_TEST_PAGE) {
        switch (control_id) {
        case 13:/*SD卡读写*/
            if (state == 0) {
                FIL file;
                UINT br = 0;

                if (FR_OK == f_open(&file, "0:/SDtest.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ)) {
                    f_printf (&file, "%s\n","SD OK!");
                    f_lseek (&file, 0);
                    f_read(&file, str, (UINT)7, &br);
                    f_close(&file);
                } /* 创建测试文件 */

                if (br == 7) {
                    SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_sd_card_ok));
                }

                else {
                    SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_sd_card_error));
					eDevice_Status = ERR_SDCARD;
                }

            } break;

        case 6:/*RFID检测*/
            if (state == 0) {
				unsigned long data = 0;
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_rfid_checking));
				vOpen_rfid();
				if(vGet_rfid_num(&data)){
					char s[12];
					sprintf(s,"%ld",data);
					SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, s);
				}
				else{
					SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_timeout));
					eDevice_Status = ERR_RFID;
				}
            } break;

        case 7:/*EEPROM*/
            if (state == 0) {
                if (AT24CXX_Check()) {
                    SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_eeprom_error));
					eDevice_Status=ERR_EEPROM;
                }

                else {
                    SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_eeprom_ok));
                }
            } break;

        case 8:/*称重系统*/
            if (state == 0) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_weighing_system));
            } break;

        case 11:/*电机复位*/
            if (state == 0) {
                vMotor_run(eRESET);
                vMotor_stop(eRESET);
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_motor_reset));
            } break;

        case 14:/*电机单周运转*/
            if (state == 0) {
                vMotor_run(eONCE);
                vMotor_stop(eONCE);
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_single_week_operation));
            } break;

        case 15:/*电机连续运转*/
            if (state == 1) {
                vMotor_run(eNORMAL);
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_continuous_running));
            }

            if (state == 0) {
                vMotor_stop(eNORMAL);
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_stop_running));
            } break;

        case 40:/*缸1开关*/
            if (state == 0) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_left_door_open));
                LEFT_DOOR_OPEN;

            }

            if (state == 1) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_left_door_close));
                LEFT_DOOR_CLOSE;
            } break;

        case 41:/*缸2开关*/
            if (state == 0) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_right_door_open));
                RIGHT_DOOR_OPEN;
            }

            if (state == 1) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_right_door_close));
                RIGHT_DOOR_CLOSE;
            } break;

        case 42:/*缸3开关*/
            if (state == 0) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_gate_opening));
                CONTROL_6_OUT(CTL_ON);
            }

            if (state == 1) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_gate_closing));
                CONTROL_6_OUT(CTL_OFF);
            } break;

        case 43:/*缸4开关*/
            if (state == 0) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_open));
                CONTROL_liaodou_OUT(CTL_ON);
            }

            if (state == 1) {
                SetTextValue(ID_SCREEN_FUNC_TEST_PAGE, 5, GetString(StrIndex_close));
                CONTROL_liaodou_OUT(CTL_OFF);
            } break;

        default : break;
        }

    }

    if (screen_id == ID_SCREEN_SYS_SETUP_PAGE && state == 0) {
        if (control_id == 23) {
            delete_files("0:/Inquire");
        }
		if (control_id == 21) {
            pEdata->language = (pEdata->language + 1) % LANGUAGES;
            vPush_one_eedata(eLANGUAGE_ADDR, pEdata->language);/* 更新EEPROM */
            printf("lang %d\r\n", pEdata->language);
			SetTextValue(ID_SCREEN_SYS_SETUP_PAGE, 22, GetString(StrIndex_language));
        }
    }
	
    if (screen_id == ID_SCREEN_FODDER_CALIBRATION_PAGE) {
		
        switch (control_id) {
        case 19: //料秤去皮
            SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_clear));
		    SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, " ");
            vLoad_fodder_clear();
            break;
        case 27://料秤标定
            SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_calibrate));
			SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, " ");
            vLoad_fodder_correction();
            break;
        case 29://料秤测试
            SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_test));
			SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, " ");
            dFodder_data_get();
            break;
        default:
            break;
        }
    }
	
    if (screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
        switch (control_id) {

        case 21: //体秤去皮
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_clear));
		    SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, " ");
            vLoad_weight_clear();
            break;

        case 28://压角标定
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_calibrate));
			SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, " ");
            vLoad_weight_correction(k++);//K=0 1 2 3 压角标定
            k%=4;
            break;
		case 6://整体标定
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_calibrate));
			SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, " ");
            vLoad_weight_correction(4);//k=4 整体标定
            break;


        case 30://体秤测试
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_test));
			SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, " ");
            dWeight_data_get(2);
            break;

		case 4://退出判断
		    if(k!=0){
           SetScreen(ID_SCREEN_TIP_PAGE);//未完成标定回到提示页面
			}
			else{
		   SetScreen(ID_SCREEN_CALIBRATION_PAGE);//完成标定则跳转上一页
				}
            break;
			
        default:
            break;
        }
    }
	  if (screen_id == ID_SCREEN_WEIGHT_TEST_PAGE){
	   switch (control_id){
		   case 5://体称清零
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_clear));
		    SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, " ");
            vLoad_weight_clear();   
		   break;
		   case 6://料称清零
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_clear));
		    SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, " ");
            vLoad_fodder_clear();   
		   break;
		   case 10://体称测试
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test));
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, " ");
            dWeight_data_get(2);
            break;
           case 11://料称测试
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test));
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, " ");
            dFodder_data_get();
            break;
        default:
            break;  
		   
	   }
	  }
	 //  if (screen_id == ID_SCREEN_TIP_PAGE) {
	//	if (control_id = 4 && state == 0){
	//		 SetScreen(ID_SCREEN_WEIGHT_CALIBRATION_PAGE);//取消则回到原页面
	//	}
   // }
}

/*!
*  \brief  文本控件通知
*  \details  当文本通过键盘更新(或调用GetControlValue)时，执行此函数
*  \details  文本控件的内容以字符串形式下发到MCU，如果文本控件内容是浮点值，
*  \details  则需要在此函数中将下发字符串重新转回浮点值。
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param str 文本控件内容
*/
extern volatile int weight_count;//Weight data count
void NotifyText(uint16 screen_id, uint16 control_id, uint8 *str)
{
	char s[50];
    if (screen_id == ID_SCREEN_MAIN_PAGE && control_id == 13) {                   //画面ID0：CAN ID设置
        int32 value = 0;
        sscanf(str, "%d", &value);                             //把字符串转换为整数
        pEdata->can_id = value;
        vPush_one_eedata(eCANID_ADDR, pEdata->can_id);/* 更新EEPROM */

        vCan1_mode_init(CAN_SJW_1tq, CAN_BS2_6tq, CAN_BS1_7tq, 24, CAN_Mode_Normal, pEdata->can_id);
        DEBUG_MSG(vUsart_report_msg("setID:%ld", pEdata->can_id));
    }

    if (screen_id == ID_SCREEN_FODDER_CALIBRATION_PAGE && control_id == 7) {
        int32 value = 0;
        sscanf(str, "%d", &value);                             //把字符串转换为整数
        pEdata->m2 = value;
        vPush_one_eedata(eM2_ADDR, pEdata->m2);/* 更新EEPROM */
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_set));
		sprintf(s,"weight = %dg",value);
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, s);
    }

    if (screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE && control_id == 12) {
        int32 value = 0;
        sscanf(str, "%d", &value);                             //把字符串转换为整数
        pEdata->m1 = value;
        vPush_one_eedata(eM2_ADDR, pEdata->m1);/* 更新EEPROM */
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_set));
		sprintf(s,"weight = %dKg",value);
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, s);
    }

    if (screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE && control_id == 1) {
        int32 value = 0;
        sscanf(str, "%d", &value);                             //把字符串转换为整数
        M_ALL = value;
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_set));
		sprintf(s,"weight = %dKg",value);
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, s);
    }
	
    if (screen_id == ID_SCREEN_INQUIRE_PAGE) {/* 数据查询功能 */

        uint8 Data[28];
        char s[50];
        int32_t ofs;//偏移指针
        int64_t value = 0;
        sscanf(str, "%ld", &value);     //把字符串转换为整数
        sprintf(s, "0:/Inquire/%ld.txt", value);
        Record_Clear(ID_SCREEN_INFO_DISPLAY_PAGE, 1);
        vTaskDelay(100);

        SetTextValue(ID_SCREEN_INFO_DISPLAY_PAGE, 12, str);//显示耳标

        if (FR_OK == f_open(file, s, FA_OPEN_EXISTING | FA_WRITE | FA_READ)) { /* 创建测试文件 */
            int yesterday = 0, today = 0, days = 0, yest_fodder_sum = 0, yest_fodder_times = 0;
            today = sRtc_time.days;
            SetScreen(ID_SCREEN_INFO_DISPLAY_PAGE);

            for (int i = 1; ; i++) {
                int tmp = 0;
                ofs = f_size(file) - i * 29;

                if (ofs < 0) {
                    break;
                }

                f_lseek(file, ofs);
                f_gets(Data, 28, file);
                days = str2int(&Data[3], 2);

                if (days == today || days == yesterday  || yesterday == 0) {
                    Record_Add(ID_SCREEN_INFO_DISPLAY_PAGE, 1, Data);

                    if (days != today && yesterday == 0) {
                        yesterday = days;
                    }

                    if (yesterday == days) {
                        yest_fodder_sum += str2int(&Data[15], 4);
                        yest_fodder_times++;
                    }
                }

                else {
                    break;
                }
            }

            sprintf(s, "%d times %d g", yest_fodder_times, yest_fodder_sum);
            SetTextValue(ID_SCREEN_INFO_DISPLAY_PAGE, 11, s);
            f_close(file);
        }

        else {
          DEBUG_MSG(printf("file open faild\r\n"));
        }

    }

    if (screen_id == ID_SCREEN_SYS_SETUP_PAGE) {
        if (control_id == 9) {
            float value = 0;
            sscanf(str, "%f", &value);
            pEdata->avgweight = value * 10;
            vPush_one_eedata(eAVGWEIGHT_ADDR, pEdata->avgweight);/* 更新EEPROM */
			weight_count = 0;//Weight data count
        }

        if (control_id == 10) {
            float value = 0;
            sscanf(str, "%f", &value);
            pEdata->fodderweight = value;
            vPush_one_eedata(eFODDERWEIGHT_ADDR, pEdata->fodderweight);/* 更新EEPROM */
        }

        if (control_id == 11) {
            float value = 0;
            sscanf(str, "%f", &value);
            pEdata->k_i = value * 10;
            vPush_one_eedata(eK_I_ADDR, pEdata->k_i);/* 更新EEPROM */
        }

        if (control_id == 12) {
            float value = 0;
            sscanf(str, "%f", &value);
            pEdata->k_o = value * 10;
            vPush_one_eedata(eK_O_ADDR, pEdata->k_o);/* 更新EEPROM */
        }
		
		if (control_id == 26) {
            float value = 0;
            sscanf(str, "%f", &value);
            pEdata->min_fodderweight = value;
            vPush_one_eedata(eMIN_FODDERWEIGHT_ADDR, pEdata->min_fodderweight);/* 更新EEPROM */
        }
    }

}

/*!
*  \brief  进度条控件通知
*  \details  调用GetControlValue时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
void NotifyProgress(uint16 screen_id, uint16 control_id, uint32 value)
{

}

/*!
*  \brief  滑动条控件通知
*  \details  当滑动条改变(或调用GetControlValue)时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
void NotifySlider(uint16 screen_id, uint16 control_id, uint32 value)
{

}

/*!
*  \brief  仪表控件通知
*  \details  调用GetControlValue时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param value 值
*/
void NotifyMeter(uint16 screen_id, uint16 control_id, uint32 value)
{
    //TODO: 添加用户代码
}

/*!
*  \brief  菜单控件通知
*  \details  当菜单项按下或松开时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param item 菜单项索引
*  \param state 按钮状态：0松开，1按下
*/
void NotifyMenu(uint16 screen_id, uint16 control_id, uint8 item, uint8 state)
{
    if (screen_id == 2 && control_id == 21 && state == 0) {
        switch (item) {
        case 0:
            ReadTextValue(2, 2);
            break;

        case 1:
            ReadTextValue(2, 3);
            break;

        case 2:
            ReadTextValue(2, 4);
            break;

        case 3:
            ReadTextValue(2, 5);
            break;

        case 4:
            ReadTextValue(2, 6);
            break;

        case 5:
            ReadTextValue(2, 7);
            break;

        default:
            break;
        }
    }

    if (screen_id == 2 && control_id == 22 && state == 0) {
        switch (item) {
        case 0:
            ReadTextValue(2, 8);
            break;

        case 1:
            ReadTextValue(2, 9);
            break;

        case 2:
            ReadTextValue(2, 10);
            break;

        case 3:
            ReadTextValue(2, 11);
            break;

        case 4:
            ReadTextValue(2, 12);
            break;

        case 5:
            ReadTextValue(2, 13);
            break;

        default:
            break;
        }
    }

    if (screen_id == 2 && control_id == 23 && state == 0) {
        switch (item) {
        case 0:
            ReadTextValue(2, 14);
            break;

        case 1:
            ReadTextValue(2, 15);
            break;

        case 2:
            ReadTextValue(2, 16);
            break;

        case 3:
            ReadTextValue(2, 17);
            break;

        case 4:
            ReadTextValue(2, 18);
            break;

        default:
            break;
        }
    }
}

/*!
*  \brief  选择控件通知
*  \details  当选择控件变化时，执行此函数
*  \param screen_id 画面ID
*  \param control_id 控件ID
*  \param item 当前选项
*/
void NotifySelector(uint16 screen_id, uint16 control_id, uint8  item)
{

}

/*!
*  \brief  定时器超时通知处理
*  \param screen_id 画面ID
*  \param control_id 控件ID
*/
void NotifyTimer(uint16 screen_id, uint16 control_id)
{

}

/*!
*  \brief  读取用户FLASH状态返回
*  \param status 0失败，1成功
*  \param _data 返回数据
*  \param length 数据长度
*/
void NotifyReadFlash(uint8 status, uint8 *_data, uint16 length)
{
    //TODO: 添加用户代码
}

/*!
*  \brief  写用户FLASH状态返回
*  \param status 0失败，1成功
*/
void NotifyWriteFlash(uint8 status)
{
    //TODO: 添加用户代码
}

/*!
*  \brief  读取RTC时间，注意返回的是BCD码
*  \param year 年（BCD）
*  \param month 月（BCD）
*  \param week 星期（BCD）
*  \param day 日（BCD）
*  \param hour 时（BCD）
*  \param minute 分（BCD）
*  \param second 秒（BCD）
*/
RTC_TIME sRtc_time, _time[2];

void NotifyReadRTC(uint8 year, uint8 month, uint8 week, uint8 day, uint8 hour, uint8 minute, uint8 second)
{
    vTaskSuspendAll();
     if(_time[0].lock == 0){
	    _time[0].sec     = (0xff & (second >> 4)) * 10 + (0xf & second);                              //BCD to dec
	    _time[0].years   = (0xff & (year >> 4)) * 10 + (0xf & year);
	    _time[0].months  = (0xff & (month >> 4)) * 10 + (0xf & month);
	    _time[0].weeks   = (0xff & (week >> 4)) * 10 + (0xf & week);
	    _time[0].days    = (0xff & (day >> 4)) * 10 + (0xf & day);
	    _time[0].hours   = (0xff & (hour >> 4)) * 10 + (0xf & hour);
	    _time[0].minutes = (0xff & (minute >> 4)) * 10 + (0xf & minute);
	    _time[0].lock = 1;
		xEventGroupSetBits(EventGroupHandler, RTCUPDATE);
    }
    else if(_time[1].lock == 0){
	    _time[1].sec     = (0xff & (second >> 4)) * 10 + (0xf & second);                              //BCD to dec
	    _time[1].years   = (0xff & (year >> 4)) * 10 + (0xf & year);
	    _time[1].months  = (0xff & (month >> 4)) * 10 + (0xf & month);
	    _time[1].weeks   = (0xff & (week >> 4)) * 10 + (0xf & week);
	    _time[1].days    = (0xff & (day >> 4)) * 10 + (0xf & day);
	    _time[1].hours   = (0xff & (hour >> 4)) * 10 + (0xf & hour);
	    _time[1].minutes = (0xff & (minute >> 4)) * 10 + (0xf & minute);
	    _time[1].lock = 1;
		xEventGroupSetBits(EventGroupHandler, RTCUPDATE);
    }
    if(_time[0].lock == 1 && _time[1].lock==1) {
        if(_time[0].years==_time[1].years&&_time[0].months==_time[1].months&&_time[0].days==_time[1].days&&_time[0].hours==_time[1].hours){
            sRtc_time = _time[0];
        }
        _time[0].lock = 0;
        _time[1].lock = 0;
    }
    xTaskResumeAll();
    DEBUG_MSG(vUsart_report_msg("DATE: %02d-%02d-%02d %02d:%02d",sRtc_time.years,sRtc_time.months,sRtc_time.days,sRtc_time.hours,sRtc_time.minutes));
}

static int str2int(char *str, int len)
{
    char *pstr = NULL;
    int temp = 0;

    if (str == NULL) {
        return 0;
    }

    pstr = str;

    while (len--) {

        if (*pstr >= '0' && *pstr <= '9') {
            temp = 10 * temp + (*pstr - '0');
        }

        pstr++;
    }

    return temp;
}
