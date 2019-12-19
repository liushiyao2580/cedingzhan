#include "sys.h"
#include "usart.h"

#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h" /* FreeRTOS */
#endif

#if 1
#pragma import(__use_no_semihosting)

struct __FILE {
    int handle;
};

FILE __stdout;
int _sys_exit(int x)
{
    x = x;
}
int _ttywrch(int ch)
{
    ch = ch;
}

int fputc(int ch, FILE *f)
{
    while ((DEBUG_UART->SR & 0X40) == 0);

    DEBUG_UART->DR = (u8) ch;
    return ch;
}
#endif


void vUart1_init(u32 bound)
{
    //GPIO
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);

#if EN_USART1_RX
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    //Usart1 NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

#endif
}

void vUart3_init(u32 BaudRate)
{
//GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); //使能GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); //使能USART2时钟

    //串口2对应引脚复用映射
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3); //GPIOA2复用为USART2
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3); //GPIOA3复用为USART2

    //USART2端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; //GPIOA2与GPIOA3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
    GPIO_Init(GPIOD, &GPIO_InitStructure); //初始化PA2，PA3

    //USART2 初始化设置
    USART_InitStructure.USART_BaudRate = BaudRate;//波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发模式
    USART_Init(USART3, &USART_InitStructure); //初始化串口2

    USART_Cmd(USART3, ENABLE);  //使能串口2
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断

    //Usart2 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//串口2中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure); //根据指定的参数初始化VIC寄存器、
}


void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        USART_ReceiveData(USART2);
    }
}

void vUart6_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART6, &USART_InitStructure);

    USART_Cmd(USART6, ENABLE);

#if EN_USART6_RX
    USART_ITConfig(USART6, USART_IT_RXNE, DISABLE);

    //Usart1 NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

#endif

}

void USART3_IRQHandler(void)
{
	u8 Res = 0;
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        Res = USART_ReceiveData(USART3);
		rfid_queue_push(Res);
    }
}

void USART6_IRQHandler(void)
{
	u8 Res = 0;
    if (USART_GetITStatus(USART6, USART_IT_RXNE) != RESET) {
        Res = USART_ReceiveData(USART6);
    }
}

void vGet_img(int64_t num)
{
    u8 *ps;
    u8 num_buf[15];
    u8 sent_buf[20];
    u8 sum = 0;
    ps = num_buf;

    sprintf(num_buf, "%ld", num);

    while (*ps) {
        sum += *ps++;
    }

    sprintf(sent_buf, "%c%s\n", sum & 0xff, num_buf);

    ps = sent_buf;

    while (*ps) {
        while ((ARM_UART->SR & 0X40) == 0);

        ARM_UART->DR = (u8) * ps++;
    }
}

void send_to_arm(unsigned char *data)
{
    u8 i=0;
	while(data[i]!='\0')
	{
	USART_SendData(USART6,data[i]);
		while( USART_GetFlagStatus(USART6,USART_FLAG_TC)!= SET);
		i++;
	}
	
}


