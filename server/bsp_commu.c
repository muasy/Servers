/*
*********************************************************************************************************
* @file    	bsp_commu.c
* @author  	SY
* @version 	V1.0.0
* @date    	2017-11-18 10:18:43
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	通讯源文件
*********************************************************************************************************
* @attention
* =======================================================================================================
*	版本 		时间					作者					说明
* -------------------------------------------------------------------------------------------------------
*	Ver1.0.0 	2017-11-18 10:18:53 	SY			创建源文件
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "CONFIG.H"
#pragma pack()
#include "bsp_server.h"
#include "bsp_commu.h"
#include "SeqList.h"
#include "bsp_rtc.h"



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

enum eLogRxCode {
	eRxCodeEnable = 0,
	eRxCodeDisable,
};
enum eLogTxCode {
	eTxCodeOk = 0,
};


struct UPGRADE_TypeDef {
	struct SERVER_TypeDef *server;
};

enum eUpgradeRxCode {
	eRxCodeAskUpgrade = 0,
	eRxCodeUpgrade,
};
enum eUpgradeTxCode {
	eTxCodeBusy = 0,
	eTxCodeNotAllow,
	eTxCodeAllow,
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
int32_t get_ctrl_degree(void);

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct LOG_TypeDef g_LogDev;
static struct UPGRADE_TypeDef g_UpgradeDev;


/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                              				通用
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : ParseVoidCmd
* Description	: 解析空命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ParseVoidCmd(const uint8_t *rxBody, uint16_t rxLength, \
		uint8_t *txBody, uint16_t *txLength)
{
	if (sizeof(struct CmdVoidTx) != rxLength)
	{
		return FALSE;
	}
	
	struct CmdVoidRx1 *rxHandle = (struct CmdVoidRx1 *)rxBody;	
	struct CmdVoidTx1 *txHandle = (struct CmdVoidTx1 *)txBody;
	txHandle->cmd = rxHandle->cmd;
	txHandle->status = STATUS_OK;
	*txLength = sizeof(struct CmdVoidTx1);	
	
	return TRUE;
}

/*
*********************************************************************************************************
*                              				日 志
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

/*
*********************************************************************************************************
*                              				固件更新
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : SetAutoUpdatePassword
* Description	: 设置自动升级密码
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static int8_t SetAutoUpdatePassword(void)
{
	int8_t ret = 0;
	uint32_t RTC_BKP_DATA;
	
	bsp_InitRTC();	
	WriteToRTC_BKP_DR(0, SERVER_PASSWD);	
	RTC_BKP_DATA = ReadRTC_BKP_DR(0);	
	if (RTC_BKP_DATA != SERVER_PASSWD)
	{
		ret = -1;
	}
	
	return ret;
}

/*
*********************************************************************************************************
* Function Name : FirmwareUpdate_Handler
* Description	: 固件升级处理
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void FirmwareUpdate_Handler(struct UPGRADE_TypeDef *this)
{	
	if (!SetAutoUpdatePassword())
	{
		extern struct netif netif;		
		uint32_t ip = netif.ip_addr.addr;
		WriteToRTC_BKP_DR(1, (ip>>0)&0xff);
		WriteToRTC_BKP_DR(2, (ip>>8)&0xff);
		WriteToRTC_BKP_DR(3, (ip>>16)&0xff);
		//WriteToRTC_BKP_DR(4, (ip>>24)&0xff);
		WriteToRTC_BKP_DR(4, 111);
		bsp_DeInitRTC();
		
		this->server->reboot = TRUE;		
	}
}

/*
*********************************************************************************************************
* Function Name : ParseUpgradeCmd
* Description	: 解析升级命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ParseUpgradeCmd(const uint8_t *rxBody, uint16_t rxLength, \
		uint8_t *txBody, uint16_t *txLength)
{
	if (sizeof(struct CmdUpgradeRx) != rxLength)
	{
		return FALSE;
	}
	
	struct UPGRADE_TypeDef *this = &g_UpgradeDev;
	struct CmdUpgradeRx *rxHandle = (struct CmdUpgradeRx *)rxBody;
	if (rxHandle->passwd != SERVER_PASSWD)
	{
		return FALSE;
	}
	
	struct CmdUpgradeTx *txHandle = (struct CmdUpgradeTx *)txBody;
	txHandle->cmd = rxHandle->cmd;
	txHandle->status = STATUS_OK;
	txHandle->code = eTxCodeAllow;
	if (get_ctrl_degree())
	{
		txHandle->code = eTxCodeBusy;
	}
	else
	{
		if (rxHandle->code == eRxCodeUpgrade)
		{
			FirmwareUpdate_Handler(this);
		}
	}
	*txLength = sizeof(struct CmdUpgradeTx);	
	
	return TRUE;
}

/*
*********************************************************************************************************
* Function Name : bsp_InitUpgrade
* Description	: 初始化固件更新
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitUpgrade(void *para)
{
	g_UpgradeDev.server = para;
}

/*
*********************************************************************************************************
*                              				设备型号
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : ParseDeviceModelCmd
* Description	: 解析设备型号命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ParseDeviceModelCmd(const uint8_t *rxBody, uint16_t rxLength, \
		uint8_t *txBody, uint16_t *txLength)
{
	if (sizeof(struct CmdVoidRx1) != rxLength)
	{
		return FALSE;
	}

	struct CmdVoidRx1 *rxHandle = (struct CmdVoidRx1 *)rxBody;
	struct CmdDeviceModelTx *txHandle = (struct CmdDeviceModelTx *)txBody;
	memset(txHandle, NULL, sizeof(*txHandle));
	txHandle->cmd = rxHandle->cmd;
	txHandle->status = STATUS_OK;
	strcpy(txHandle->deviceModel, CTRL_MODEL);	
	strcpy(txHandle->hwModel, PCB_MODEL);
	txHandle->firmwareVersion = FW_VERSION;	
	*txLength = sizeof(struct CmdDeviceModelTx);	
	
	return TRUE;
}


/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
