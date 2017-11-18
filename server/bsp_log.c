/*
*********************************************************************************************************
* @file    	bsp_log.c
* @author  	SY
* @version 	V1.0.0
* @date    	2017-10-24 19:12:27
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	日志源文件
*********************************************************************************************************
* @attention
* =======================================================================================================
*	版本 		时间					作者					说明
* -------------------------------------------------------------------------------------------------------
*	Ver1.0.0 	2017-10-24 19:13:03 	SY			创建源文件
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "bsp_server.h"
#include "SeqList.h"



/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct LOG_TypeDef {
	uint8_t enable;
	SEQ_LIST_TypeDef seqList;
	uint8_t buff[256];
	struct SERVER_TypeDef *server;
	struct SOCKET_TypeDef remoteSocket;
};

#pragma pack(1)

struct CmdLogRx {
	uint16_t cmd;
	uint16_t passwd;
	uint8_t code;
	uint16_t dataPort;
};

struct CmdLogTx {
	uint16_t cmd;
	uint8_t status;
	uint8_t code;
};

#pragma pack()

enum eLogRxCode {
	eRxCodeEnable = 0,
	eRxCodeDisable,
};
enum eLogTxCode {
	eTxCodeOk = 0,
	eTxCodeBusy,
};

/*
*********************************************************************************************************
*                              				Private constants
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private macro
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct LOG_TypeDef g_LogDev;



/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : ParseLogCmd
* Description	: 解析日志命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ParseLogCmd(const uint8_t *rxBody, uint16_t rxLength, \
		uint8_t *txBody, uint16_t *txLength)
{
	if (sizeof(struct CmdLogRx) != rxLength)
	{
		return FALSE;
	}
	
	struct LOG_TypeDef *this = &g_LogDev;
	this->remoteSocket = this->server->remoteSocket;
	struct CmdLogRx *rxHandle = (struct CmdLogRx *)rxBody;
	if (rxHandle->passwd != SERVER_PASSWD)
	{
		return FALSE;
	}
	if (rxHandle->code == eRxCodeEnable)
	{
		this->enable = TRUE;
	}
	else
	{
		this->enable = FALSE;
	}
	this->remoteSocket.port = rxHandle->dataPort;
	
	struct CmdLogTx *txHandle = (struct CmdLogTx *)txBody;
	txHandle->cmd = rxHandle->cmd;
	txHandle->status = STATUS_OK;
	txHandle->code = eTxCodeOk;
	*txLength = sizeof(struct CmdLogTx);	
	
	return TRUE;
}

/*
*********************************************************************************************************
* Function Name : LogOutput
* Description	: 日志输出
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void LogOutput(struct LOG_TypeDef *this, uint8_t ch)
{
	if (this->enable) {		
		if (AppendSeqList(&this->seqList, &ch,\
				PushSeqListU8_CallBack,\
				CopySeqListU8_CallBack) != STATUS_DATA_STRUCT_TRUE) {
			ClearSeqList(&this->seqList);
			return;
		}
		
		if (ch == '\n') {
			if (this->server->ops.send) {
				this->server->ops.send(this->server, \
					this->seqList.basePtr, GetSeqListLenth(&this->seqList), &this->remoteSocket);
			}									
			ClearSeqList(&this->seqList);
		}
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_InitLog
* Description	: 初始化日志
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitLog(void *para)
{
	g_LogDev.server = para;
	CreateSeqList(&g_LogDev.seqList, g_LogDev.buff, ARRAY_SIZE(g_LogDev.buff));
}

/*
*********************************************************************************************************
* Function Name : fputc
* Description	: 重定义putc函数，这样可以使用printf函数从串口1打印输出
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
int fputc(int ch, FILE *f)
{
	#if 1	/* 将需要printf的字符通过串口中断FIFO发送出去，printf函数会立即返回 */
		//ComSend(DEBUG_COM, (uint8_t *)&ch, 1);
	
		/* 调试输出 */
		LogOutput(&g_LogDev, ch);
	
		return ch;
	#else	/* 采用阻塞方式发送每个字符,等待数据发送完毕 */
		/* 写一个字节到USART1 */
		USART_SendData(USART1, (uint8_t) ch);
		/* 等待发送结束 */
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
		{
			;
		}
		return ch;
	#endif
}

#if 0
/*
*********************************************************************************************************
*	函 数 名: fgetc
*	功能说明: 重定义getc函数，这样可以使用getchar函数从串口1输入数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int fgetc(FILE *f)
{
#if 1	/* 从串口接收FIFO中取1个数据, 只有取到数据才返回 */
	uint8_t ucData;

	while(ComGet(DEBUG_COM, &ucData) == 0)
	{
		BSP_OS_TimeDlyMs(100);
	}

	return ucData;
#else
	/* 等待串口1输入数据 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

	return (int)USART_ReceiveData(USART1);
#endif
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
