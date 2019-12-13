#include "Load_Sensor.h"
#include "ADS126x.h"
#include "usart.h"
#include "24cxx.h"
#include "cmd_process.h"
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "myString.h"

void vUsart_report_msg(const char *fmt, ...);
/* 定义静态全局变量，保存各传感器系数 */
static double temp1, temp2, temp3, temp4, temp5;//皮重
static double k1, k2, k3, k4, k5, kw; //系数
static int M1, M2;//标定砝码重量 
volatile int M_ALL;//M_ALL用于整体标定
extern volatile uint64_t store_era;
/**
  * @brief  称重系统初始化
  * @param
  * @retval
  */
void vLoad_sys_init(void)
{
    vAdc_io_init();//ADS1263  io初始化
    vAdc_reg_set(); //ADS1236 寄存器初始化

    /* 使用EEPROM中保存的系数 */
    temp1 = pEdata->temp1 / 100.0;
    temp2 = pEdata->temp2 / 100.0;
    temp3 = pEdata->temp3 / 100.0;
    temp4 = pEdata->temp4 / 100.0;
    temp5 = pEdata->temp5 / 100.0;

    k1 = pEdata->k1 / 10000.0;
    k2 = pEdata->k2 / 10000.0;
    k3 = pEdata->k3 / 10000.0;
    k4 = pEdata->k4 / 10000.0;
    k5 = pEdata->k5 / 10000.0;
    kw = pEdata->kw / 10000.0;

    M1 = pEdata->m1;
    M2 = pEdata->m2;
}

/**
  * @brief  通道多次数据求均值
  * @param  n 次数 channel
  * @retval 数据 均值
  */
static double prvLoad_avg_data(unsigned char channel, unsigned int n)
{
    double tmp = 0;

    for (unsigned char i = 0; i < n; i++) {
        switch (channel) {
        case channel_1 :    tmp += (dAdc_chx_data(channel_1) - temp1); break;

        case channel_2 :    tmp += (dAdc_chx_data(channel_2) - temp2); break;

        case channel_3 :    tmp += (dAdc_chx_data(channel_3) - temp3); break;

        case channel_4 :    tmp += (dAdc_chx_data(channel_4) - temp4); break;

        case channel_5 :    tmp += (dAdc_chx_data(channel_5) - temp5); break;

        default : break;
        }
    }

    return (tmp / n);
}

/**
  * @brief  饲料称重传感器系数校正
  */

void vLoad_fodder_correction(void)
{
    M2 = pEdata->m2;
    k5 = M2 / prvLoad_avg_data(channel_5, 50);

    pEdata->k5 = (int)(10000 * k5);
    vPush_one_eedata(eK5_ADDR, pEdata->k5);/* 更新EEPROM */

    if (current_screen_id == ID_SCREEN_FODDER_CALIBRATION_PAGE) {
        char str[20];
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_ok));
        sprintf(str, "%s %.5f", GetString(StrIndex_result), k5);
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, str);
    }
}

/**
  * @brief  体重传感器系数校正
  */

void vLoad_weight_correction(int k)
{
    static double a[4] = {0}, b[4] = {0}, c[4] = {0}, d[4] = {0}, tmp[4] = {0};
    double M = pEdata->m1;
    M1 = pEdata->m1;
    DEBUG_MSG(printf("\r\nCorrection : %d times\r\n", k + 1));

    tmp[0] = prvLoad_avg_data(channel_1, 200);
    tmp[1] = prvLoad_avg_data(channel_2, 200);
    tmp[2] = prvLoad_avg_data(channel_3, 200);
    tmp[3] = prvLoad_avg_data(channel_4, 200);
    DEBUG_MSG(printf("channel_1 :%.2lf\tchannel_2 :%.2lf\tchannel_3 :%.2lf\tchannel_4 :%.2lf\t \r\n", tmp[0], tmp[1], tmp[2], tmp[3]));
 if ( k != 4 ){
    switch (k) {
    case 0: a[0] = tmp[0]; a[1] = tmp[1]; a[2] = tmp[2]; a[3] = tmp[3]; break;

    case 1: b[0] = tmp[0]; b[1] = tmp[1]; b[2] = tmp[2]; b[3] = tmp[3]; break;

    case 2: c[0] = tmp[0]; c[1] = tmp[1]; c[2] = tmp[2]; c[3] = tmp[3]; break;

    case 3: d[0] = tmp[0]; d[1] = tmp[1]; d[2] = tmp[2]; d[3] = tmp[3]; break;

    default : break;
    }

    if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
		char s[10];
        char str[100];
		sprintf(s,"%s%d/4:",GetString(StrIndex_step) , k+1);
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, s);
        sprintf(str, "%4.1f %4.1f %4.1f %4.1f ", tmp[0], tmp[1], tmp[2], tmp[3]);
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, str);
    }

    if ( k == 3 ) {
        //系数计算
        k1 = -(M * a[1] * b[2] * c[3] - M * a[1] * b[3] * c[2] - M * a[2] * b[1] * c[3] + M * a[2] * b[3] * c[1] + M * a[3] * b[1] * c[2] - M * a[3] * b[2] * c[1] - M * a[1] * b[2] * d[3] + M * a[1] * b[3] * d[2] + M * a[2] * b[1] * d[3] - M * a[2] * b[3] * d[1] - M * a[3] * b[1] * d[2] + M * a[3] * b[2] * d[1] + M * a[1] * c[2] * d[3] - M * a[1] * c[3] * d[2] - M * a[2] * c[1] * d[3] + M * a[2] * c[3] * d[1] + M * a[3] * c[1] * d[2] - M * a[3] * c[2] * d[1] - M * b[1] * c[2] * d[3] + M * b[1] * c[3] * d[2] + M * b[2] * c[1] * d[3] - M * b[2] * c[3] * d[1] - M * b[3] * c[1] * d[2] + M * b[3] * c[2] * d[1]) / (a[0] * b[1] * c[2] * d[3] - a[0] * b[1] * c[3] * d[2] - a[0] * b[2] * c[1] * d[3] + a[0] * b[2] * c[3] * d[1] + a[0] * b[3] * c[1] * d[2] - a[0] * b[3] * c[2] * d[1] - a[1] * b[0] * c[2] * d[3] + a[1] * b[0] * c[3] * d[2] + a[1] * b[2] * c[0] * d[3] - a[1] * b[2] * c[3] * d[0] - a[1] * b[3] * c[0] * d[2] + a[1] * b[3] * c[2] * d[0] + a[2] * b[0] * c[1] * d[3] - a[2] * b[0] * c[3] * d[1] - a[2] * b[1] * c[0] * d[3] + a[2] * b[1] * c[3] * d[0] + a[2] * b[3] * c[0] * d[1] - a[2] * b[3] * c[1] * d[0] - a[3] * b[0] * c[1] * d[2] + a[3] * b[0] * c[2] * d[1] + a[3] * b[1] * c[0] * d[2] - a[3] * b[1] * c[2] * d[0] - a[3] * b[2] * c[0] * d[1] + a[3] * b[2] * c[1] * d[0]);
        k2 = (M * a[0] * b[2] * c[3] - M * a[0] * b[3] * c[2] - M * a[2] * b[0] * c[3] + M * a[2] * b[3] * c[0] + M * a[3] * b[0] * c[2] - M * a[3] * b[2] * c[0] - M * a[0] * b[2] * d[3] + M * a[0] * b[3] * d[2] + M * a[2] * b[0] * d[3] - M * a[2] * b[3] * d[0] - M * a[3] * b[0] * d[2] + M * a[3] * b[2] * d[0] + M * a[0] * c[2] * d[3] - M * a[0] * c[3] * d[2] - M * a[2] * c[0] * d[3] + M * a[2] * c[3] * d[0] + M * a[3] * c[0] * d[2] - M * a[3] * c[2] * d[0] - M * b[0] * c[2] * d[3] + M * b[0] * c[3] * d[2] + M * b[2] * c[0] * d[3] - M * b[2] * c[3] * d[0] - M * b[3] * c[0] * d[2] + M * b[3] * c[2] * d[0]) / (a[0] * b[1] * c[2] * d[3] - a[0] * b[1] * c[3] * d[2] - a[0] * b[2] * c[1] * d[3] + a[0] * b[2] * c[3] * d[1] + a[0] * b[3] * c[1] * d[2] - a[0] * b[3] * c[2] * d[1] - a[1] * b[0] * c[2] * d[3] + a[1] * b[0] * c[3] * d[2] + a[1] * b[2] * c[0] * d[3] - a[1] * b[2] * c[3] * d[0] - a[1] * b[3] * c[0] * d[2] + a[1] * b[3] * c[2] * d[0] + a[2] * b[0] * c[1] * d[3] - a[2] * b[0] * c[3] * d[1] - a[2] * b[1] * c[0] * d[3] + a[2] * b[1] * c[3] * d[0] + a[2] * b[3] * c[0] * d[1] - a[2] * b[3] * c[1] * d[0] - a[3] * b[0] * c[1] * d[2] + a[3] * b[0] * c[2] * d[1] + a[3] * b[1] * c[0] * d[2] - a[3] * b[1] * c[2] * d[0] - a[3] * b[2] * c[0] * d[1] + a[3] * b[2] * c[1] * d[0]);
        k3 = -(M * (a[0] * b[1] * c[3] - a[0] * b[3] * c[1] - a[1] * b[0] * c[3] + a[1] * b[3] * c[0] + a[3] * b[0] * c[1] - a[3] * b[1] * c[0] - a[0] * b[1] * d[3] + a[0] * b[3] * d[1] + a[1] * b[0] * d[3] - a[1] * b[3] * d[0] - a[3] * b[0] * d[1] + a[3] * b[1] * d[0] + a[0] * c[1] * d[3] - a[0] * c[3] * d[1] - a[1] * c[0] * d[3] + a[1] * c[3] * d[0] + a[3] * c[0] * d[1] - a[3] * c[1] * d[0] - b[0] * c[1] * d[3] + b[0] * c[3] * d[1] + b[1] * c[0] * d[3] - b[1] * c[3] * d[0] - b[3] * c[0] * d[1] + b[3] * c[1] * d[0])) / (a[0] * b[1] * c[2] * d[3] - a[0] * b[1] * c[3] * d[2] - a[0] * b[2] * c[1] * d[3] + a[0] * b[2] * c[3] * d[1] + a[0] * b[3] * c[1] * d[2] - a[0] * b[3] * c[2] * d[1] - a[1] * b[0] * c[2] * d[3] + a[1] * b[0] * c[3] * d[2] + a[1] * b[2] * c[0] * d[3] - a[1] * b[2] * c[3] * d[0] - a[1] * b[3] * c[0] * d[2] + a[1] * b[3] * c[2] * d[0] + a[2] * b[0] * c[1] * d[3] - a[2] * b[0] * c[3] * d[1] - a[2] * b[1] * c[0] * d[3] + a[2] * b[1] * c[3] * d[0] + a[2] * b[3] * c[0] * d[1] - a[2] * b[3] * c[1] * d[0] - a[3] * b[0] * c[1] * d[2] + a[3] * b[0] * c[2] * d[1] + a[3] * b[1] * c[0] * d[2] - a[3] * b[1] * c[2] * d[0] - a[3] * b[2] * c[0] * d[1] + a[3] * b[2] * c[1] * d[0]);
        k4 = (M * (a[0] * b[1] * c[2] - a[0] * b[2] * c[1] - a[1] * b[0] * c[2] + a[1] * b[2] * c[0] + a[2] * b[0] * c[1] - a[2] * b[1] * c[0] - a[0] * b[1] * d[2] + a[0] * b[2] * d[1] + a[1] * b[0] * d[2] - a[1] * b[2] * d[0] - a[2] * b[0] * d[1] + a[2] * b[1] * d[0] + a[0] * c[1] * d[2] - a[0] * c[2] * d[1] - a[1] * c[0] * d[2] + a[1] * c[2] * d[0] + a[2] * c[0] * d[1] - a[2] * c[1] * d[0] - b[0] * c[1] * d[2] + b[0] * c[2] * d[1] + b[1] * c[0] * d[2] - b[1] * c[2] * d[0] - b[2] * c[0] * d[1] + b[2] * c[1] * d[0])) / (a[0] * b[1] * c[2] * d[3] - a[0] * b[1] * c[3] * d[2] - a[0] * b[2] * c[1] * d[3] + a[0] * b[2] * c[3] * d[1] + a[0] * b[3] * c[1] * d[2] - a[0] * b[3] * c[2] * d[1] - a[1] * b[0] * c[2] * d[3] + a[1] * b[0] * c[3] * d[2] + a[1] * b[2] * c[0] * d[3] - a[1] * b[2] * c[3] * d[0] - a[1] * b[3] * c[0] * d[2] + a[1] * b[3] * c[2] * d[0] + a[2] * b[0] * c[1] * d[3] - a[2] * b[0] * c[3] * d[1] - a[2] * b[1] * c[0] * d[3] + a[2] * b[1] * c[3] * d[0] + a[2] * b[3] * c[0] * d[1] - a[2] * b[3] * c[1] * d[0] - a[3] * b[0] * c[1] * d[2] + a[3] * b[0] * c[2] * d[1] + a[3] * b[1] * c[0] * d[2] - a[3] * b[1] * c[2] * d[0] - a[3] * b[2] * c[0] * d[1] + a[3] * b[2] * c[1] * d[0]);

        if (0 == k1 || 0 == k2 || 0 == k3 || 0 == k4 ) {
             DEBUG_MSG(printf("\r\nSensor_correction Error !!\r\n"));
            return ;
        }

        if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
            char str[100];
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_ok));
            sprintf(str, "%4.3f %4.3f %4.3f %4.3f", 100*k1, 100*k2, 100*k3, 100*k4);
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, str);
        }

        pEdata->k1 = (int)(10000 * k1);
        pEdata->k2 = (int)(10000 * k2);
        pEdata->k3 = (int)(10000 * k3);
        pEdata->k4 = (int)(10000 * k4);
        vPush_one_eedata(eK1_ADDR, pEdata->k1);/* 更新EEPROM */
        vPush_one_eedata(eK2_ADDR, pEdata->k2);/* 更新EEPROM */
        vPush_one_eedata(eK3_ADDR, pEdata->k3);/* 更新EEPROM */
        vPush_one_eedata(eK4_ADDR, pEdata->k4);/* 更新EEPROM */
         DEBUG_MSG(printf("\r\nThe Sensor_correction result is :\r\nk1: %.6lf\r\nk2: %.6lf\r\nk3: %.6lf\r\nk4: %.6lf\r\n", k1, k2, k3, k4));

    }
 }
    if (k == 4) {
        double res;
        res = 0.0;

		if(0 == M_ALL){
	        if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
	            char str[100];
	            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_error));
	            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, GetString(StrIndex_please_set_the_weight));
	        }
			return;
		}

        for (int i = 0; i < 100; i++) {
            res += fAdc_get_weight(  k1,  k2,  k3,  k4,  temp1,  temp2,  temp3,  temp4);
        }

        res /= 100.0;
        kw = M_ALL / res;
        pEdata->kw = (int)(10000 * kw);
        vPush_one_eedata(eKW_ADDR, pEdata->kw);/* 更新EEPROM */

        if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
            char str[100];
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_ok));
            sprintf(str, "%s %6.5f", GetString(StrIndex_result), kw);
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, str);
        }
    }
}

/**
  * @brief  体重传感器数据清零
  */

void vLoad_weight_clear(void)
{
    int times = 200;
    temp1 = 0.0;
    temp2 = 0.0;
    temp3 = 0.0;
    temp4 = 0.0;

    vTaskSuspendAll();

    for (unsigned int i = 0; i < times; i++) {
        temp1 += dAdc_chx_data(channel_1);
        temp2 += dAdc_chx_data(channel_2);
        temp3 += dAdc_chx_data(channel_3);
        temp4 += dAdc_chx_data(channel_4);
    }

    temp1 /= times;
    temp2 /= times;
    temp3 /= times;
    temp4 /= times;

    pEdata->temp1 = (int)(100 * temp1);
    pEdata->temp2 = (int)(100 * temp2);
    pEdata->temp3 = (int)(100 * temp3);
    pEdata->temp4 = (int)(100 * temp4);
    vPush_one_eedata(eTEMP1_ADDR, pEdata->temp1);/* 更新EEPROM */
    vPush_one_eedata(eTEMP2_ADDR, pEdata->temp2);/* 更新EEPROM */
    vPush_one_eedata(eTEMP3_ADDR, pEdata->temp3);/* 更新EEPROM */
    vPush_one_eedata(eTEMP4_ADDR, pEdata->temp4);/* 更新EEPROM */

    xTaskResumeAll();

    if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {
        char str[100];
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_result));
        sprintf(str, "%4.0f %4.0f %4.0f %4.0f", temp1, temp2, temp3, temp4);
        SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, str);
    }
	if(current_screen_id == ID_SCREEN_WEIGHT_TEST_PAGE){
		SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test));
		SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12,GetString(StrIndex_clear_ok));
	}
}

/**
  * @brief  饲料称重传感器数据清零
  */
void vLoad_fodder_clear(void)
{
    int times = 50;
    temp5 = 0.0;
    vTaskSuspendAll();

    for (unsigned int i = 0; i < times; i++) {
        temp5 += dAdc_chx_data(6);
    }

    temp5 /= times;
    pEdata->temp5 = (int)(100 * temp5);
    vPush_one_eedata(eTEMP5_ADDR, pEdata->temp5);/* 更新EEPROM */

    xTaskResumeAll();

    if (current_screen_id == ID_SCREEN_FODDER_CALIBRATION_PAGE) {
        char str[20];
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_result));
        sprintf(str, "%4.0f ", temp5);
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, str);
    }
	if(current_screen_id == ID_SCREEN_WEIGHT_TEST_PAGE){
		SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test));
			SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12,GetString(StrIndex_clear_ok));
		}
}

/**
  * @brief  饲料质量获取
  * @param  比例系数
  * @retval 质量
  */
double dFodder_data_get(void)
{
    double res[30] = {0.0};
    double max, min, sum, re;

    vTaskSuspendAll();
    sum = max = min = res[0] =  (dAdc_chx_data(channel_5) - temp5) * k5;

    for (int i = 1; i < 30; i++) {
        res[i] =  (dAdc_chx_data(channel_5) - temp5) * k5;
        sum += res[i];

        if (max < res[i]) {
            max = res[i];
        }

        if (min > res[i]) {
            min = res[i];
        }
    }

    xTaskResumeAll();
    sum = sum - (max + min);
    re = sum / 28.0;

    if (current_screen_id == ID_SCREEN_FODDER_CALIBRATION_PAGE) {
        char str[20];
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 11, GetString(StrIndex_test_ok));
        sprintf(str, "%s%.1fg",GetString(StrIndex_result) , re);
        SetTextValue(ID_SCREEN_FODDER_CALIBRATION_PAGE, 33, str);
    }
if (current_screen_id == ID_SCREEN_WEIGHT_TEST_PAGE) {
        char str[20];
        SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test_ok));
        sprintf(str, "%s%.1fg",GetString(StrIndex_result) , re);
        SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, str);
    }
    if (current_screen_id == ID_SCREEN_MAIN_PAGE) {
        char str[20];
		sprintf(str, "%6.1fg", re);
		SetTextValue(ID_SCREEN_MAIN_PAGE, 24,str);
    }

    return re > 5 ? re : 0;
}

static float Low_pass_filter(float *data, int len);
static float cal_variance(float avg, float *data, int len);
static void add_data_to_buf(float *const buf, int bufLen, float data);
static float find_min_variance_result(float *const variance_buf, float *const avg_buf, int bufLen);
/**
  * @brief  体重获取
  * @param  比例系数
  * @retval 质量
  * mode 0 测试  1称量
  */
double dWeight_data_get(int mode)
{
    static double avg, avg1;
    char str[20];
    const char img[2] = {'-', ' '};
    const int times = 250;
    static char c = 0;
    avg = 0.0;

    if (0 == mode) {//快速称重
        c++;
        c %= 2;
        vTaskSuspendAll();

        for (int i = 0; i < 5; i++) {
            avg += kw * fAdc_get_weight(  k1,  k2,  k3,  k4,  temp1,  temp2,  temp3,  temp4);
        }

        xTaskResumeAll();
        avg /= 5;

		if (current_screen_id == ID_SCREEN_MAIN_PAGE) {
            if(pEdata->mode == 0){//测定模式显示当前体重
			sprintf(str, " %6.2f %c", avg, img[c]);
        	SetTextValue(ID_SCREEN_MAIN_PAGE, 6, str);
            	} 
				if(pEdata->mode == 1){//训练模式显示耳标号
			sprintf(str, "%ld is feeding-%6.2f %c", store_era,avg, img[c]);
        	SetTextValue(ID_SCREEN_MAIN_PAGE, 6, str);
            	}
        }

		
		return avg;
    }

    if (1 == mode) {//均值限制
       #define MODE1_TIMES_LEN  20
        char str[20];
        int index = 0, cal_ok = 0; //数据索引 
        float *temp = NULL;
		float variance = 0.0; //方差
        float avg_buf[MODE1_TIMES_LEN] = {0.0};
        FIL file_w;
        temp = pvPortMalloc(MODE1_TIMES_LEN * times * sizeof(float));

        if (NULL == temp) {
			DEBUG_MSG(vUsart_report_msg("Error:pvPortMalloc() when mode == 1"));
            return -1;
        }

        for (int j = 0; j < MODE1_TIMES_LEN; j++) {
            /* 获取数据 */
            vTaskSuspendAll();

            for (int i = 0; i < times; i++) {
                *(temp + index) = kw * fAdc_get_weight(  k1,  k2,  k3,  k4,  temp1,  temp2,  temp3,  temp4);
                index++;
            }

            xTaskResumeAll();
			
			Low_pass_filter(temp + index - times, times);
            
            avg = Low_pass_filter(temp + index - times, times);
			variance = cal_variance(avg, temp + index - times, times); //计算分段方差
            add_data_to_buf(avg_buf, MODE1_TIMES_LEN, avg);

            DEBUG_MSG(vUsart_report_msg("%d avg= %lf __ variance= %lf", j, avg, variance));
            vTaskDelay(100);
            /*if (j >= 2) {
                if (fabs(avg_buf[1] - avg_buf[2]) < 0.3 && fabs(avg_buf[0] - (avg_buf[1] + avg_buf[2]) / 2.0 ) < 0.3) {
                    avg1 = (avg_buf[0] + avg_buf[1] + avg_buf[2]) / 3.0;
                    cal_ok = 1;
                    DEBUG_MSG(vUsart_report_msg("Weight is %lf", avg1));
                    break;
                }
            }*/
            if(variance < 0.15){
				avg1 = avg_buf[0];
				cal_ok = 1;
                DEBUG_MSG(vUsart_report_msg("Weight is %lf", avg1));
                break;
			}
        }

        if (cal_ok == 0) {
            avg1 = 0;

            for (int i = 0; i < MODE1_TIMES_LEN; i++) {
                avg1 += avg_buf[i];
            }

            avg1 /= MODE1_TIMES_LEN;
            DEBUG_MSG(vUsart_report_msg("over time! Weight is %lf", avg1));
        }

        if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {

            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_test_ok));
            sprintf(str, "%s%.2fKg", GetString(StrIndex_result), avg1);
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, str);
        }
         if (current_screen_id == ID_SCREEN_WEIGHT_TEST_PAGE) {

            SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test_ok));
            sprintf(str, "%s%.2fKg", GetString(StrIndex_result), avg1);
            SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, str);
        }

        vPortFree(temp);
		return avg1;
    }
	
    if (2 == mode) {//均值方差双限制
		#define MODE2_TIMES_LEN  20
        char str[20];
        int index = 0, cal_ok = 0; //数据索引
        float *temp = NULL;
		float variance = 0.0; //方差
        float avg_buf[MODE2_TIMES_LEN] = {0.0};
		float variance_buf[MODE2_TIMES_LEN] = {0.0};
        FIL file_w;
        temp = pvPortMalloc(MODE2_TIMES_LEN * times * sizeof(float));

        if (NULL == temp) {
			DEBUG_MSG(vUsart_report_msg("Error:pvPortMalloc() when mode == 2"));
            return -1;
        }

        for (int j = 0; j < MODE2_TIMES_LEN; j++) {
            /* 获取数据 */
            vTaskSuspendAll();

            for (int i = 0; i < times; i++) {
                *(temp + index) = kw * fAdc_get_weight(  k1,  k2,  k3,  k4,  temp1,  temp2,  temp3,  temp4);
                index++;
            }

            xTaskResumeAll();
			//Low_pass_filter(temp + index - times, times);
            avg = Low_pass_filter(temp + index - times, times); //计算低通滤波后均值
			variance = cal_variance(avg, temp + index - times, times); //计算分段方差
            add_data_to_buf(avg_buf, MODE2_TIMES_LEN, avg);
			add_data_to_buf(variance_buf, MODE2_TIMES_LEN, variance);

            DEBUG_MSG(vUsart_report_msg("%d avg= %lf __ variance= %lf", j, avg, variance));
            vTaskDelay(100);
			
            if (j > 0) {
                if (fabs(avg_buf[0] - avg_buf[1]) < 0.3) {
					if( variance_buf[0] < 0.1 && variance_buf[1] < 0.1){
						
						avg1 = (avg_buf[0] + avg_buf[1]) / 2.0;
	                    cal_ok = 1;
	                    DEBUG_MSG(vUsart_report_msg("Weight is %lf", avg1));
	                    break;
					}
                }
            }
        }

        if (cal_ok == 0) {
            avg1 = find_min_variance_result(variance_buf, avg_buf, MODE2_TIMES_LEN);
			if(avg1 > 0.01){
				DEBUG_MSG(vUsart_report_msg("Weight is %lf", avg1));
			}
			else{
	            for (int i = 0; i < MODE2_TIMES_LEN; i++) {
	                avg1 += avg_buf[i];
	            }
	            avg1 /= MODE2_TIMES_LEN;
	            DEBUG_MSG(vUsart_report_msg("over time! Weight is %lf", avg1));
			}
        }

        sprintf(str, "0:/WeightData/%f.txt", avg1);

        if (FR_OK == f_open(&file_w, str, FA_CREATE_ALWAYS | FA_WRITE) ) {

            for (int i = 0; i < index; i++) {
                sprintf(str, "%.2f", *(temp + i));
                f_printf(&file_w, "%s\n", str);
            }

            f_close(&file_w);
        }

		if (current_screen_id == ID_SCREEN_WEIGHT_CALIBRATION_PAGE) {

            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 11, GetString(StrIndex_test_ok));
            sprintf(str, "%s%.2fKg",GetString(StrIndex_result), avg1);
            SetTextValue(ID_SCREEN_WEIGHT_CALIBRATION_PAGE, 33, str);
        }
		  if (current_screen_id == ID_SCREEN_WEIGHT_TEST_PAGE) {

            SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 4, GetString(StrIndex_test_ok));
            sprintf(str, "%s%.2fKg", GetString(StrIndex_result), avg1);
            SetTextValue(ID_SCREEN_WEIGHT_TEST_PAGE, 12, str);
        }
        vPortFree(temp);
		return avg1;
    }
    return 0;
}

static float Low_pass_filter(float *data, int len)
{
    float K = 0.016, avg = 0.0;

    for (int i = 5; i < len ; i++) {
        *(data + i) = *(data + i) * K + *(data + i - 1) * (1 - K);
        avg += *(data + i);
    }

    return avg / (len - 5);
}

static void add_data_to_buf(float *const buf, int bufLen, float data)
{
    for (int i = bufLen - 1; i > 0; i--) {
        buf[i] = buf[i - 1];
    }

    buf[0] = data;
}

static float cal_variance(float avg, float *data, int len)
{
	float variance = 0.0, sum = 0.0, tmp;

	for(int i = 0; i < len; i++){
		tmp = *(data + i) - avg;
		sum += tmp * tmp;
	}
	variance = sum / len;

	return variance;
}

static float find_min_variance_result(float *const variance_buf, float *const avg_buf, int bufLen)
{
	float min_variance = 0.1, result = 0.0;//设置方差限制
	int count = 0;
	for(int i = 0; i < bufLen; i++){
		if(*(variance_buf + i) < min_variance){
			count++;
			result += avg_buf[i];
			DEBUG_MSG(vUsart_report_msg("Count %d variance is %f",count, *(variance_buf + i)));
		}
	}
	if(count > 1){
		return result / count;
	}
	else{
		DEBUG_MSG(vUsart_report_msg("No data matching!"));
		return 0.0;
	}
}