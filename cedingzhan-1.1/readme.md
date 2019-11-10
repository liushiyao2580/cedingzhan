# 测定站软件系统开发手册v1.0
## 1.说明

### 1.1代码获取

- **生成公钥**  
```bash
ssh-keygen -t rsa -C "your name"
```
- **开通权限**
把 ~/.ssh/id_rsa.pub(默认路径) 下内容发给代码管理者，在代码仓库中添加ssh公钥之后就有权限下载代码。
- **下载代码**
注意：使用ssh路径下载，使用 **-b 分支名** 只下载某一分支
```bash
git clone git@github.com:FengliCao/Cedingzhan.git -b develop
```
### 1.2代码开发
在**develop**分支进行开发  
log规范：提交使用 `git commit`，在编辑器中添加**log**信息：
```
[关键字][姓名首字母简写] 英文简单描述提交的内容  
--空一行--   
[note] 填写本次提交的中文描述，what why how等。  
```
### 1.3代码风格规范
>K&R缩进风格  
>缩进4个空格，不适用tab缩进  
>（*、&）符号与变量名结合 示例：int *b = &a;  
>操作符两端插入空格  
>if for while等结构不管语句多少均使用花括号,且前后各空一行  

示例code：  
```c
  /*!  
 1.  \brief  显示屏交互任务函数  
 2.  \param  任务传入参数  
  */  
  void dctft_task(void *pvParameters)  
  {  
      int size;  
      for (;;) {  
          size = queue_find_cmd(cmd_buffer, CMD_MAX_SIZE);  
          if (size) {  
              if (cmd_buffer[1] != 0x07) {                     //接收到指令 ，及判断是否为开机提示
                  ProcessMessage((PCTRL_MSG)cmd_buffer, size); //指令处理
              }

              else if (cmd_buffer[1] == 0x07) {                //如果为指令0x07就软重置STM32
                  __disable_fault_irq();
                  NVIC_SystemReset();
              }
          }

          //xTaskNotifyGive(CANTask_Handler);
          vTaskDelay(1000);
      }
  }
```

## 代码结构

## FreeRTOS任务说明

## 文档历史
**注：** 每次更新文档需要填写文档修改历史。

|版本| 修改日期 | 作者 | 说明 |
|--|--|--|--|
| v1.1 | 2019.6.1 | FengliCao  | ---- |
| v1.0 | 2019.6.1 | FengliCao  | 创建文档 |


