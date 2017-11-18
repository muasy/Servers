/*
*********************************************************************************************************
* @file    	bsp_upgrade.c
* @author  	SY
* @version 	V1.0.0
* @date    	2017-10-26 09:02:55
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	升级源文件
*********************************************************************************************************
* @attention
* =======================================================================================================
*	版本 		时间					作者					说明
* -------------------------------------------------------------------------------------------------------
*	Ver1.0.0 	22017-10-26 09:03:05 	SY			创建源文件
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "bsp_server.h"
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
struct UPGRADE_TypeDef {
	struct SERVER_TypeDef *server;
};

#pragma pack(1)

struct CmdUpgradeRx {
	uint16_t cmd;
	uint16_t passwd;
	uint8_t code;
};

struct CmdUpgradeTx {
	uint16_t cmd;
	uint8_t status;
	uint8_t code;
};

#pragma pack()

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
static struct UPGRADE_TypeDef g_UpgradeDev;



/*
*********************************************************************************************************
*                              				Private functions
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

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
