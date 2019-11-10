/*
模拟spi
by ：cfl
*/
#include "my_spi.h"
/*****************************************************************
CPOL=0，表示当SCLK=0时处于空闲态，所以有效状态就是SCLK处于高电平时
CPHA=1，表示数据采样是在第2个边沿，数据发送在第1个边沿
*****************************************************************/

//CPOL=0  //CPHA=1
unsigned char SPI_SendByte(unsigned char dt)
{
    u8 i;
    u8 temp = 0;

    for (i = 0; i < 8; i++) {
        SCLK_H;
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();

        if (dt & 0x80) {
            MOSI_H;    //写数据
        }

        else {
            MOSI_L;
        }

        dt <<= 1;

        SCLK_L;
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        temp <<= 1;

        if (MISO) {
            temp |= 0x01;    //读数据
        }

    }

    return temp;
}

void SPI_IO_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //开GPIOB时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHZ
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化推挽输出

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;// MISO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHZ
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化
}
