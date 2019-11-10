/*****************************
*功能  ：ADS1263/1262 模拟SPI通信
* BY   : cfl
*平台  : stm32f407  及其他单片机
*TIME  :2017 - 5 - 21
******************************/
#include "ADS126x.h"    //包括126x寄存器宏定义等
#include "stm32f4xx.h"  // 用于 单片机 端口定义
#include "delay.h"
#include "math.h"
#include "Load_Sensor.h"
#include "24cxx.h"
#include "error_code.h"
#include "led.h"
extern volatile ERROR_CODE eDevice_Status; //Device state
extern volatile ERROR_CODE eDevice_Status1; //Device state
volatile int AD_DRDY = 1;

void vAdc_io_init(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;
    EXTI_InitTypeDef   EXTI_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //开GPIOB时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHZ
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化 //PB 14 15推挽输出

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;    // PB10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_IO_INIT(); //spi io 初始化
    set_adc_CS;
    reset_adc_START;
}

void EXTI15_10_IRQHandler(void)
 {
     if (EXTI_GetITStatus(EXTI_Line10) != RESET) {
         EXTI_ClearITPendingBit(EXTI_Line10);
         AD_DRDY = 0;
     }
}

static int WAIT_DRED(void)
{
    long count;
	AD_DRDY = 1; 
    count = 1000000;
    while ( 0 == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10)) {

		delay_us(1);
        if (0 == count--) {
			eDevice_Status = ERR_ADC;//ADC故障
			pEdata->adc_overtime_count = pEdata->adc_overtime_count + 1;
		    vPush_one_eedata(eADC_OVERTIME_COUNT_ADDR, pEdata->adc_overtime_count);
			//__disable_fault_irq();
            //NVIC_SystemReset();
            return 1;
        }
    }
	count = 1000000;
    while ( 1 == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10)) {

		delay_us(1);
        if (0 == count--) {
			eDevice_Status = ERR_ADC;//ADC故障
			pEdata->adc_overtime_count = pEdata->adc_overtime_count + 1;
		    vPush_one_eedata(eADC_OVERTIME_COUNT_ADDR, pEdata->adc_overtime_count); 
		    vAdc_reg_set(); //ADS1236 寄存器初始化
			//__disable_fault_irq();
            //NVIC_SystemReset();
            return 1;
        }
    }

    return 0;
}


static inline unsigned char ADS126xXferByte(unsigned char cData)         //spi 数据读 / 写   cDate写入的数据/函数返回值-读出的数据
{
    return SPI_SendByte(cData);
}

static int32_t ADS126xReadData(uint8_t NumBytes, uint8_t DataByteStartNum)
{
    uint8_t ADC_Bytes[6];                                                    //Holds
    int32_t ADC_Data_Only;

    reset_adc_CS;

    for (int32_t i = 0; i < NumBytes; ++i) {
        ADC_Bytes[i] = ADS126xXferByte(0);
    }

    ADC_Data_Only = ((int32_t)ADC_Bytes[DataByteStartNum + 0] << 24) |      //Data MSB
                    ((int32_t)ADC_Bytes[DataByteStartNum + 1] << 16) |
                    ((int32_t)ADC_Bytes[DataByteStartNum + 2] <<  8) |
                    ((int32_t)ADC_Bytes[DataByteStartNum + 3] <<  0);       //Data LSB
    set_adc_CS;

    return ADC_Data_Only;
}

static void ADS126xWriteRegister(int StartAddress, int NumRegs, unsigned char *pdata)
{
    reset_adc_CS;
    ADS126xXferByte(0x40 + StartAddress);
    ADS126xXferByte(NumRegs - 1);

    for (int32_t i = 0; i < NumRegs; i++) {
        ADS126xXferByte(pdata[i]);
    }

    set_adc_CS;


}

static void ADS126xReadRegister(int StartAddress, int NumRegs, unsigned char *pdata)
{
    reset_adc_CS;
    ADS126xXferByte(0x20 + StartAddress);
    ADS126xXferByte(NumRegs - 1);

    for (int32_t i = 0; i < NumRegs; i++) {

        pdata[i] = ADS126xXferByte(0x00);
    }

    set_adc_CS;

}

void ADS126xSendResetCommand(void)
{
    reset_adc_CS;
    ADS126xXferByte(RESET);
    set_adc_CS;
}

void ADS126xSendStartCommand(void)
{
    reset_adc_CS;
    ADS126xXferByte(START1);
    set_adc_CS;
}

void ADS126xSendStopCommand(void)
{
    reset_adc_CS;
    ADS126xXferByte(STOP1);
    set_adc_CS;
}

/*
*初始化所有寄存器 （根据需要修改）
*/
void vAdc_reg_set(void)
{
    uint8_t AdcRegData[ADS126x_NUM_REG];
    uint8_t WriteRegData[ADS126x_NUM_REG];
    ADS126xSendResetCommand();
    ADS126xReadRegister(ID, ADS126x_NUM_REG, AdcRegData);           //Read ALL registers

    /* Configure Register Settings */
    WriteRegData[ID] = AdcRegData[ID];                                  //ID
    WriteRegData[POWER] = INTREF;                                           //POWER (RESET = 0, INTREF = 1)
    WriteRegData[INTERFACE] = 0;                    //INTERFACE
    WriteRegData[MODE0] = DELAY_0us | RUNMODE;                              //MODE0 (Set DELAY)
    WriteRegData[MODE1] = (AdcRegData[MODE1] & FILTER_SINC1);       //MODE1
    WriteRegData[MODE2] =  BYPASS | DR_2400_SPS;    //MODE2 (BYPASS = 0, GAIN = 32 V/V)
    WriteRegData[INPMUX] = MUXP_AIN0 | MUXN_AIN1;                   //INPMUX (AINP = AIN1, AINN = AIN2)

    WriteRegData[OFCAL0] = 0x80;                    //OFCAL0 (reset to default) //2.5校正值
    WriteRegData[OFCAL1] = 0x06;                    //OFCAL1 (reset to default)
    WriteRegData[OFCAL2] = 0x00;                    //OFCAL2 (reset to default)

    WriteRegData[FSCAL0] = 0xa0;                    //FSCAL0 (reset to default)
    WriteRegData[FSCAL1] = 0x86;                    //FSCAL1 (reset to default)
    WriteRegData[FSCAL2] = 0x40;                    //FSCAL2 (reset to default)

    WriteRegData[IDACMUX] = MUX2_NO_CONM | MUX1_NO_CONM;            //IDACMUX (IDAC1MUX = AINCOM)
    WriteRegData[IDACMAG] = MAG2_OFF | MAG1_OFF;                    //IDACMAG (IDAC1MAG = 500 uA)
    WriteRegData[REFMUX] = RMUXP_AIN0 | RMUXN_AIN1;                 //REFMUX (REFP = RMUXP_INTP, REFN = RMUXP_INTN)
    WriteRegData[TDACP] = TDACP_DEFAULT_VALUE;                        //TDACP (reset to default)
    WriteRegData[TDACN] = TDACN_DEFAULT_VALUE;                        //TDACN (reset to default)
    WriteRegData[GPIOCON] = CON6_AIN09 | CON5_AIN08;                //GPIOCON (Enable GPIOs on AIN8 & AIN9)
    WriteRegData[GPIODIR] = GPIOCON_DEFAULT_VALUE;                  //GPIODIR (reset to default)
    WriteRegData[GPIODAT] = DAT5_AIN08;                                   //GPIODAT (Biases bridge with + polarity)
#ifdef ADS1263
    WriteRegData[ADC2CFG] = ADC2CFG_DEFAULT_VALUE;                  //ADC2CFG (reset to default)
    WriteRegData[ADC2MUX] = ADC2MUX_DEFAULT_VALUE;                  //ADC2MUX (reset to default)
    WriteRegData[ADC2OFC0] = ADC2OFC0_DEFAULT_VALUE;                //ADC2OFC0 (reset to default)
    WriteRegData[ADC2OFC1] = ADC2OFC1_DEFAULT_VALUE;                //ADC2OFC1 (reset to default)
    WriteRegData[ADC2FSC0] = ADC2FSC0_DEFAULT_VALUE;                //ADC2FSC0 (reset to default)
    WriteRegData[ADC2FSC1] = ADC2FSC1_DEFAULT_VALUE;                //ADC2FSC1 (reset to default)
#endif

    ADS126xWriteRegister(ID, ADS126x_NUM_REG, &WriteRegData[0]);    //Write ALL registers
}



double dAdc_chx_data( uint8_t IO )
{
    uint8_t WriteRegData;                 //Stores the register write values
    int32_t AdcOutput = 0;
    double DateReading;


    WriteRegData = 0;
    ADS126xWriteRegister(INTERFACE, 1, &WriteRegData);
    WriteRegData = DELAY_35us | RUNMODE;
    ADS126xWriteRegister(MODE0, 1, &WriteRegData);

    WriteRegData = RMUXP_AIN0 | RMUXN_AIN1;
    ADS126xWriteRegister(REFMUX, 1, &WriteRegData);

    //REFMUX
    //WriteRegData = RMUXP_INTP | RMUXN_INTN;
    //ADS126xWriteRegister(REFMUX, 1, &WriteRegData);
    //
    if (IO == 6) { //饲料称重通道 降低速率 提高精度
        WriteRegData = DR_10_SPS;
        ADS126xWriteRegister(MODE2, 1, &WriteRegData);
    }

    else {
        WriteRegData = DR_2400_SPS;
        ADS126xWriteRegister(MODE2, 1, &WriteRegData);
    }

    switch (IO) {
    case 0 : WriteRegData = MUXP_AIN0 | MUXN_AIN1; break;

    case 1 : WriteRegData = MUXP_AIN1 | MUXN_AIN1; break;

    case 2 : WriteRegData = MUXP_AIN2 | MUXN_AIN1; break;

    case 3 : WriteRegData = MUXP_AIN3 | MUXN_AIN1; break;

    case 4 : WriteRegData = MUXP_AIN4 | MUXN_AIN1; break;

    case 5 : WriteRegData = MUXP_AIN5 | MUXN_AIN1; break;

    case 6 : WriteRegData = MUXP_AIN6 | MUXN_AIN1; break;

    default : WriteRegData = MUXP_AINCOM | MUXN_AIN1;

    }

    ADS126xWriteRegister(INPMUX, 1, &WriteRegData);

    reset_adc_CS;
    set_adc_START;

    if (WAIT_DRED()) {
        return 0;
    }

    AdcOutput = ADS126xReadData(4, 0);

    reset_adc_START;
    DateReading = round(AdcOutput / 2147483647.0 * 4100.0); //将输出比例放大  100000  倍取整
    return DateReading;
}


/**
  * @brief  获取体重称重数据
  * @param  k1 - k4 四通道比例系数
  * @param  tmp1 - tmp4 四通道称重前数据
  * @retval 四通道数据和，即净体重数据
  */
float fAdc_get_weight( double k1, double k2, double k3, double k4, double tmp1, double tmp2, double tmp3, double tmp4)
{
    uint8_t WriteRegData;                   //Stores the register write values
    int32_t AdcOutput = 0;

    double Date[4] = {0.0};

    WriteRegData = 0;
    ADS126xWriteRegister(INTERFACE, 1, &WriteRegData);
    WriteRegData = DELAY_35us | RUNMODE;
    ADS126xWriteRegister(MODE0, 1, &WriteRegData);

    WriteRegData = RMUXP_AIN0 | RMUXN_AIN1;
    ADS126xWriteRegister(REFMUX, 1, &WriteRegData);

    //REFMUX
    //  WriteRegData = RMUXP_INTP | RMUXN_INTN;
    //  ADS126xWriteRegister(REFMUX, 1, &WriteRegData);

    WriteRegData = DR_1200_SPS;
    ADS126xWriteRegister(MODE2, 1, &WriteRegData);
    WriteRegData = MUXP_AIN2 | MUXN_AIN1;
    ADS126xWriteRegister(INPMUX, 1, &WriteRegData);
    //Read one conversion
    reset_adc_CS;
    set_adc_START;//开启第一次转换

    if (WAIT_DRED()) {
        return 0;
    }  //等待第一次转换完成

    reset_adc_START;

    WriteRegData = 0;
    ADS126xWriteRegister(INTERFACE, 1, &WriteRegData);
    WriteRegData = DELAY_35us | RUNMODE;
    ADS126xWriteRegister(MODE0, 1, &WriteRegData);
    WriteRegData = DR_1200_SPS;
    ADS126xWriteRegister(MODE2, 1, &WriteRegData);
    WriteRegData = MUXP_AIN3 | MUXN_AIN1;
    ADS126xWriteRegister(INPMUX, 1, &WriteRegData);

    //Read one conversion
    reset_adc_CS;
    set_adc_START;//开启第二次转换
    AdcOutput = ADS126xReadData(4, 0); //读取第一次转换结果
    Date[0] = (round(AdcOutput / 2147483647.0 * 4100.0) - tmp1);

    if (WAIT_DRED()) {
        return 0;
    }  //等待第二次转换完成

    reset_adc_START;


    WriteRegData = 0;
    ADS126xWriteRegister(INTERFACE, 1, &WriteRegData);
    WriteRegData = DELAY_35us | RUNMODE;
    ADS126xWriteRegister(MODE0, 1, &WriteRegData);
    WriteRegData = DR_1200_SPS;
    ADS126xWriteRegister(MODE2, 1, &WriteRegData);
    WriteRegData = MUXP_AIN4 | MUXN_AIN1;
    ADS126xWriteRegister(INPMUX, 1, &WriteRegData);
    //Read one conversion
    reset_adc_CS;
    set_adc_START;//开启第三次转换
    AdcOutput = ADS126xReadData(4, 0); //读取第二次转换结果
    Date[1] = (round(AdcOutput / 2147483647.0 * 4100.0) - tmp2);

    if (WAIT_DRED()) {
        return 0;
    }//等待第三次转换完成

    reset_adc_START;

    WriteRegData = 0;
    ADS126xWriteRegister(INTERFACE, 1, &WriteRegData);
    WriteRegData = DELAY_35us | RUNMODE;
    ADS126xWriteRegister(MODE0, 1, &WriteRegData);
    WriteRegData = DR_1200_SPS;
    ADS126xWriteRegister(MODE2, 1, &WriteRegData);
    WriteRegData = MUXP_AIN5 | MUXN_AIN1;
    ADS126xWriteRegister(INPMUX, 1, &WriteRegData);
    //Read one conversion
    reset_adc_CS;
    set_adc_START;//开启第四次转换
    AdcOutput = ADS126xReadData(4, 0); //读取第三次转换结果
    Date[2] = (round(AdcOutput / 2147483647.0 * 4100.0) - tmp3);

    if (WAIT_DRED()) {
        return 0;
    }//等待第四次转换完成

    reset_adc_START;
    AdcOutput = ADS126xReadData(4, 0); //读取第四次转换结果
    Date[3] = (round(AdcOutput / 2147483647.0 * 4100.0) - tmp4);

    return (float)(k1 * Date[0] + k2 * Date[1] + k3 * Date[2] + k4 * Date[3]);
}
