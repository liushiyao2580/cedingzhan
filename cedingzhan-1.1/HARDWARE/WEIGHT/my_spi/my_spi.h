#ifndef _SPI_H_
#define _SPI_H_
#include "stm32f4xx.h"

#define MOSI_H GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define MOSI_L GPIO_ResetBits(GPIOB, GPIO_Pin_12)

#define SCLK_H GPIO_SetBits(GPIOB, GPIO_Pin_13)
#define SCLK_L GPIO_ResetBits(GPIOB, GPIO_Pin_13)

#define MISO GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)

unsigned char SPI_SendByte(unsigned char dt);
void SPI_IO_INIT(void);

#endif
