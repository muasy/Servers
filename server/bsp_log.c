/*
*********************************************************************************************************
* @file    	bsp_log.c
* @author  	SY
* @version 	V1.0.0
* @date    	2017-10-24 19:12:27
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	��־Դ�ļ�
*********************************************************************************************************
* @attention
* =======================================================================================================
*	�汾 		ʱ��					����					˵��
* -------------------------------------------------------------------------------------------------------
*	Ver1.0.0 	2017-10-24 19:13:03 	SY			����Դ�ļ�
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
* Description	: ������־����
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
* Description	: ��־���
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
* Description	: ��ʼ����־
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
* Description	: �ض���putc��������������ʹ��printf�����Ӵ���1��ӡ���
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
int fputc(int ch, FILE *f)
{
	#if 1	/* ����Ҫprintf���ַ�ͨ�������ж�FIFO���ͳ�ȥ��printf�������������� */
		//ComSend(DEBUG_COM, (uint8_t *)&ch, 1);
	
		/* ������� */
		LogOutput(&g_LogDev, ch);
	
		return ch;
	#else	/* ����������ʽ����ÿ���ַ�,�ȴ����ݷ������ */
		/* дһ���ֽڵ�USART1 */
		USART_SendData(USART1, (uint8_t) ch);
		/* �ȴ����ͽ��� */
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
*	�� �� ��: fgetc
*	����˵��: �ض���getc��������������ʹ��getchar�����Ӵ���1��������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
int fgetc(FILE *f)
{
#if 1	/* �Ӵ��ڽ���FIFO��ȡ1������, ֻ��ȡ�����ݲŷ��� */
	uint8_t ucData;

	while(ComGet(DEBUG_COM, &ucData) == 0)
	{
		BSP_OS_TimeDlyMs(100);
	}

	return ucData;
#else
	/* �ȴ�����1�������� */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

	return (int)USART_ReceiveData(USART1);
#endif
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
