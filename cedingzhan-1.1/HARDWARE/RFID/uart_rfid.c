/*
模块数据帧解释：

模块上电工作后，每 200mS 会自动上传模块状态数据：
状态数据(字符串)：$T + 2B(FDX_RSSI) + 2B（HDX_RSSI）+1B(同步状态)+ 2BRCC（不包
括帧头）+ #
例如：$T028F128#

打开天线 $CA#
关闭天线 $CB#
天线能量调节 $P + 2B(能量值)+ #

模块支持 HDX 和 FDX-B 两种格式电子标签，信号输出数据(ASCLL)格式如下：
1、FDX-B 后加数据(字符串)：$R1+1B（填补位，无意义）+3B 国家代码（高字节在前）+ 10B
十六进制卡号（高位在前）+ 34B 的 16 进制字节数据 + 2B RCC（不包括帧头） + #
2、FDX-B 无加数据(字符串)：$R2+1B（填补位，无意义）+3B 国家代码（高字节在前）+ 10B
十六进制卡号（高位在前）+ 2B RCC（不包括帧头） + #
3、HDX 无加数据(字符串)：$R3+1B（填补位，无意义）+3B 国家代码（高字节在前）+ 10B
十六进制卡号（高位在前）+ 2B RCC（不包括帧头） + #
以上数据是读到的数据，读到数据后就往下发送（115200

例如： $R383E7006581B7C424#

R3 ： HDX  格式标签 ；
8 ： 无意义 ； 
国家代号 （ 3E7 ） ： 999 （ 十进制 ） ； 
卡号 （ 006581B7C4） ） ：001703000004( 十进制) )  ； 
RCC  校验（ 24 ）： 36 （十进制)
*/
#include <string.h>
#include "usart.h"
#include "user_task.h"
#include "uart_rfid.h"

//max length of rfid data queue
#define MAX_RFID_QUEUE_SIZE             500
#define MAX_RFID_DATA_LEN               40

#define DATA_HEAD '$'                                                  //帧头
#define DATA_TAIL '#'                                                  //帧尾

typedef struct _QUEUE_RFID {
    Qsize_t _head;                                                       //队列头
    Qsize_t _tail;                                                       //队列尾
    Qdata_t _data[MAX_RFID_QUEUE_SIZE];                                  //队列数据缓存区
} QUEUE_RFID;

static QUEUE_RFID que = {0, 0, 0};                                          //指令队列
static uint32 cmd_state = 0;                                           //队列帧尾检测状态
static Qsize_t cmd_pos = 0;                                              //当前指令指针位置

static inline void rfid_sent_byte(u8 byte)                           //rfid发送数据
{
    while ((RFID_UART->SR & 0X40) == 0);
    RFID_UART->DR = (u8)byte;
}

static void rfid_queue_pop(Qdata_t *_data)          //将队列暂存数据取出            
{
    if (que._tail != que._head) {
        *_data = que._data[que._tail];
        que._tail = (que._tail + 1) % MAX_RFID_QUEUE_SIZE;
    }
}

static Qsize_t rfid_queue_size()                 //判断当前指针是否指向帧尾
{
    return ((que._head + MAX_RFID_QUEUE_SIZE - que._tail) % MAX_RFID_QUEUE_SIZE);
}

static void rfid_open()                       //打开天线
{
	char *OPEN_CMD = "$CA#", *p_Str = NULL;
	p_Str = OPEN_CMD;
	USART_ITConfig(RFID_UART, USART_IT_RXNE, ENABLE);
	USART_ClearITPendingBit(RFID_UART, USART_IT_RXNE);
	while(*p_Str){
		rfid_sent_byte(*p_Str);
		p_Str++;
	}
}

static void rfid_close()                     //关闭天线
{
	char *CLOSE_CMD = "$CB#", *p_Str = NULL;
	p_Str = CLOSE_CMD;
	USART_ITConfig(RFID_UART, USART_IT_RXNE, DISABLE);
	while(*p_Str){
		rfid_sent_byte(*p_Str);
		p_Str++;
	}
}

static void rfid_queue_reset()                //指针位置复位
{
    que._head = que._tail = 0;
    cmd_pos = cmd_state = 0;
}

void rfid_queue_push(Qdata_t _data)             //将数据写入队列数据缓存中
{
    Qsize_t pos = (que._head + 1) % MAX_RFID_QUEUE_SIZE;

    if (pos != que._tail) {
        que._data[que._head] = _data;
        que._head = pos;
    }
}

static Qsize_t rfid_queue_find_data(unsigned long *data)   //取出队列中的完整数据
{
    Qsize_t cmd_size = 0;
    Qdata_t _data = 0, check_sum = 0;
	Qdata_t _data_buf[MAX_RFID_DATA_LEN] = {0};
	*data = 0;
	
	while(rfid_queue_size() > 0){
		rfid_queue_pop(&_data);

		if(0 == cmd_pos && DATA_HEAD != _data){// find DATA_HEAD
			continue;
		}
		if(_data == DATA_HEAD){ //排除数据不完整导致双帧头的情况
			cmd_pos = 0;
			check_sum = 0;
			_data_buf[cmd_pos++] = _data;
			continue;
		}		
		if(cmd_pos > 0 && cmd_pos < 17){
			check_sum ^= _data;
		}
		if(cmd_pos < MAX_RFID_DATA_LEN){
			_data_buf[cmd_pos++] = _data;
		}

		cmd_state = _data;

		if(DATA_TAIL == cmd_state){    //如果指针到了帧尾，则指针复位
			cmd_size = cmd_pos;
			cmd_state = 0;
			cmd_pos = 0;
				switch(_data_buf[1]){
							case 'T':{   //状态数据
								_data_buf[MAX_RFID_DATA_LEN -1] = '\0';
								DEBUG_MSG(vUsart_report_msg("rfid status msg %s\r\n",_data_buf));
								return 0;
							}
							case 'R':{  //格式标签
								int i_count = 0;
								Qdata_t data_check_sum = 0; //RCC校验标志位
								if(_data_buf[17] >= '0' && _data_buf[17] <= '9'){
									data_check_sum = _data_buf[17]-'0';
								}else if(_data_buf[17] >= 'A' && _data_buf[17] <= 'F'){
									data_check_sum = _data_buf[17]-'7';
								}else{
									DEBUG_MSG(vUsart_report_msg("check_sum error\r\n"));
									return 0;
								}
								if(_data_buf[18] >= '0' && _data_buf[18] <= '9'){
									data_check_sum <<= 4;
									data_check_sum += _data_buf[18]-'0';
								}else if(_data_buf[18] >= 'A' && _data_buf[18] <= 'F'){
									data_check_sum <<= 4;
									data_check_sum += _data_buf[18]-'7';
								}else{
									DEBUG_MSG(vUsart_report_msg("check_sum error\r\n"));
									return 0;
								}
								
								if(check_sum != data_check_sum){
									DEBUG_MSG(vUsart_report_msg("check_sum error %x is not %x\r\n", check_sum, data_check_sum));
									return 0;
								}
								
								while(i_count < 11){
									switch(_data_buf[6+i_count]){
										case 'A':
										case 'B':
										case 'C':
										case 'D':
										case 'E':
										case 'F':
											_data_buf[6+i_count] = _data_buf[6+i_count] - '7';
											break;
										default:
											_data_buf[6+i_count] = _data_buf[6+i_count] - '0';
									}
									*data = 16 * (*data) + _data_buf[6+i_count];
									i_count++;
								}
								DEBUG_MSG(vUsart_report_msg("rfid get num ok..."));
								return cmd_size;
								break;
							}
							default:
							_data_buf[MAX_RFID_DATA_LEN -1] = '\0';
								DEBUG_MSG(vUsart_report_msg("error of rfid data! - %s- \r\n",_data_buf));
								break;
						}
		}
	}
    return 0;
}

int vGet_rfid_num(unsigned long *data)
{
	int count = 0;
	rfid_open();
	rfid_queue_reset();
	do{
		vTaskDelay(250);
		if(count++ > 10){
			DEBUG_MSG(vUsart_report_msg(">>>>>>>> rfid read timeout! <<<<<<<<<"));
			rfid_close();
			return 0;
		}	
	}while(0 == rfid_queue_find_data(data));

	DEBUG_MSG(vUsart_report_msg(">>>>>>>> rfid num = %ld <<<<<<<<<",*data));
	rfid_close();
	return 1;
}

void vClose_rfid()
{
	RFID_Control=0;
}
void vOpen_rfid()
{
 RFID_Control=1;
}