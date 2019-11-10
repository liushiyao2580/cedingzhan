#ifndef LOAD_SR_H
#define LOAD_SR_H

#define channel_1 2
#define channel_2 3
#define channel_3 4
#define channel_4 5
#define channel_5 6

typedef struct {
    double Ch1;
    double Ch2;
    double Ch3;
    double Ch4;
    double Sum_ch_all;
} INPUT_DATE;

/**
  * @brief  称重系统初始化
  */
void vLoad_sys_init(void);

/**
  * @brief  饲料称重传感器系数校正
  */
void vLoad_fodder_correction(void);

/**
  * @brief  体重传感器系数校正
  */
void vLoad_weight_correction(int);

/**
  * @brief  饲料称重传感器数据清零
  */
void vLoad_fodder_clear(void);

/**
  * @brief  饲料质量获取
  */
double dFodder_data_get(void);

double dWeight_data_get(int mode);

#endif
