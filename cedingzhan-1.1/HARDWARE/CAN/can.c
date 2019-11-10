#include "can.h"
#include "delay.h"
#include "usart.h"
#include "user_task.h"


//CAN初始化
//tsjw:重新同步跳跃时间单元.范围:CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:时间段2的时间单元.   范围:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:时间段1的时间单元.   范围:CAN_BS1_1tq ~CAN_BS1_16tq
//brp :波特率分频器.范围:1~1024; tq=(brp)*tpclk1

//波特率=Fpclk1/((tbs1+1+tbs2+1+1)*brp);
//mode:CAN_Mode_Normal,普通模式;CAN_Mode_LoopBack,回环模式;
//Fpclk1的时钟在初始化的时候设置为42M,如果设置CAN1_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//则波特率为:42M/((6+7+1)*12)=250Kbps
//返回值:0,初始化OK;
//    其他,初始化失败;


void vCan1_mode_init(u8 tsjw, u8 tbs2, u8 tbs1, u16 brp, u8 mode, u16 ID)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    CAN_InitTypeDef        CAN_InitStructure;
    CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN1_RX0_INT_ENABLE
    NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //使能相关时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能PORTA时钟

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟

    //初始化GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化PA11,PA12

    //引脚复用映射配置
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_CAN1); //GPIOA11复用为CAN1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_CAN1); //GPIOA12复用为CAN1

    //CAN单元设置
    CAN_InitStructure.CAN_TTCM = DISABLE; //非时间触发通信模式
    CAN_InitStructure.CAN_ABOM = DISABLE; //软件自动离线管理
    CAN_InitStructure.CAN_AWUM = DISABLE; //睡眠模式通过软件唤醒(清除CAN->MCR的SLEEP位)
    CAN_InitStructure.CAN_NART = ENABLE; //禁止报文自动传送
    CAN_InitStructure.CAN_RFLM = DISABLE; //报文不锁定,新的覆盖旧的
    CAN_InitStructure.CAN_TXFP = ENABLE;

    CAN_InitStructure.CAN_Mode = mode;   //模式设置
    CAN_InitStructure.CAN_SJW = tsjw; //重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位 CAN_SJW_1tq~CAN_SJW_4tq
    CAN_InitStructure.CAN_BS1 = tbs1; //Tbs1范围CAN_BS1_1tq ~CAN_BS1_16tq
    CAN_InitStructure.CAN_BS2 = tbs2; //Tbs2范围CAN_BS2_1tq ~    CAN_BS2_8tq
    CAN_InitStructure.CAN_Prescaler = brp; //分频系数(Fdiv)为brp+1
    CAN_Init(CAN1, &CAN_InitStructure);   // 初始化CAN1

    //配置过滤器
    CAN_FilterInitStructure.CAN_FilterNumber = 0; //过滤器0
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit; //32位

    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdHigh = ID << 5; //32位ID
    
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0005; //ID 不能大于127，在UI界面进行防呆
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0fe0; //mask:0000 1111 1110 0000 0000 0000 0000 0101



    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0; //过滤器0关联到FIFO0
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE; //激活过滤器0
    CAN_FilterInit(&CAN_FilterInitStructure);//滤波器初始化

#if CAN1_RX0_INT_ENABLE
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE); //FIFO0消息挂号中断允许.
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;     // 主优先级为6 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

#if CAN1_RX0_INT_ENABLE //使能RX0中断
//中断服务函数
void CAN1_RX0_IRQHandler(void)
{
    BaseType_t pxHigherPriorityTaskWoken;
    CanRxMsg RxMessage;

    if (NULL != canRecQueue_Handler) {
        /*获取帧数据*/
        CAN_Receive(CAN1, 0, &RxMessage);
        /*帧数据发送至CAN总线接收消息队列（canRecQueue）*/
        xQueueSendToBackFromISR(canRecQueue_Handler, &RxMessage, &pxHigherPriorityTaskWoken);
        /*是否需要任务切换*/
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
    }
}
#endif

//can发送一组数据(固定格式:ID为0X12,标准帧,数据帧)
//len:数据长度(最大为8)
//msg:数据指针,最大为8个字节.
//返回值:0,成功;
//       其他,失败;
u8 CAN1_Send_Msg(u8 *msg, u8 len, u32 CAN_ID, enum _type type)
{
    CanTxMsg TxMessage;
    TxMessage.StdId = CAN_ID;   // 标准标识符为0
    TxMessage.IDE = CAN_Id_Standard;   // 使用标识符

    if (type == eDATA) {
        TxMessage.RTR = CAN_RTR_Data;        // 消息类型为数据帧，一帧8位

        TxMessage.DLC = len;

        for (int i = 0; i < len; i++) {
            TxMessage.Data[i] = msg[i];
        }
    }

    else if (type == eREMOTE) {
        TxMessage.RTR = CAN_RTR_Remote;        // 消息类型为远程帧
        /* 对错误代码进行限制;将四个气缸的错误代码归为一类 */
        if(len == ERR_CYLINDER_1 || len == ERR_CYLINDER_2 \
			|| len == ERR_CYLINDER_3 || len == ERR_CYLINDER_4){
             len = ERR_CYLINDER;
		}
        TxMessage.DLC = len;//len表示错误代码
    }

    /*发送至canSendQueue消息队列*/
    if (pdTRUE == xQueueSend(canSendQueue_Handler, &TxMessage, 5)) {
        return 0;
    }

    return 1;
}









