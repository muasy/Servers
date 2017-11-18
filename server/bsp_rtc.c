/*
*********************************************************************************************************
*
*	模块名称 : RTC
*	文件名称 : bsp_rtc.c
*	版    本 : V1.0
*	说    明 : RTC底层驱动
*	修改记录 :
*		版本号   日期        作者     说明
*		V1.0    2013-12-12  armfly   正式发布
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "stm32f10x.h"
#include "bsp_rtc.h"

static void RTC_Config(void);


/* 20个RTC备份数据寄存器 */
#define RTC_BKP_DR_NUMBER              20  

/* 选择时钟源 */
/* #define RTC_CLOCK_SOURCE_LSE */       
#define RTC_CLOCK_SOURCE_LSI     

/* RTC 备份数据寄存器 */
uint32_t aRTC_BKP_DR[RTC_BKP_DR_NUMBER] =
{
	BKP_DR1, BKP_DR2, BKP_DR3, BKP_DR4, BKP_DR5,
	BKP_DR6, BKP_DR7, BKP_DR8, BKP_DR9, BKP_DR10,
	BKP_DR11, BKP_DR12, BKP_DR13, BKP_DR14, BKP_DR15,
	BKP_DR16, BKP_DR17, BKP_DR18, BKP_DR19, BKP_DR20,
};

/*
*********************************************************************************************************
*	函 数 名: bsp_InitRTC
*	功能说明: 初始化RTC
*	形    参：无
*	返 回 值: 无		        
*********************************************************************************************************
*/
void bsp_InitRTC(void)
{
	RTC_Config();
}
	
/*
*********************************************************************************************************
*	函 数 名: bsp_DeInitRTC
*	功能说明: 恢复默认RTC
*	形    参：无
*	返 回 值: 无		        
*********************************************************************************************************
*/
void bsp_DeInitRTC(void)
{
	/* 使能PWR时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, DISABLE);

	/* 允许访问RTC */
	PWR_BackupAccessCmd(DISABLE);
	
	/* Enable LSE */		
	RCC_LSEConfig(RCC_LSE_OFF);
}
	
/*
*********************************************************************************************************
*	函 数 名: RTC_Config
*	功能说明: 配置RTC用于跑表
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
static void RTC_Config(void)
{
	/* 使能PWR时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* 允许访问RTC */
	PWR_BackupAccessCmd(ENABLE);
	
	/* Enable LSE */		
	RCC_LSEConfig(RCC_LSE_ON);
	
}

/*
*********************************************************************************************************
*	函 数 名: WriteToRTC_BKP_DR
*	功能说明: 写数据到RTC的备份数据寄存器
*	形    参：无FirstRTCBackupData 写入的数据
*	返 回 值: 无
*********************************************************************************************************
*/
void WriteToRTC_BKP_DR(uint8_t RTCBackupIndex, uint16_t RTCBackupData)
{
	if (RTCBackupIndex < RTC_BKP_DR_NUMBER)
	{
		/* 写数据备份数据寄存器 */
		BKP_WriteBackupRegister(aRTC_BKP_DR[RTCBackupIndex], RTCBackupData);
	}
}

/*
*********************************************************************************************************
*	函 数 名: ReadRTC_BKP_DR
*	功能说明: 读取备份域数据
*	形    参：RTCBackupIndex 寄存器索引值
*	返 回 值: 
*********************************************************************************************************
*/
uint16_t ReadRTC_BKP_DR(uint8_t RTCBackupIndex)
{
	if (RTCBackupIndex < RTC_BKP_DR_NUMBER)
	{
		return BKP_ReadBackupRegister(aRTC_BKP_DR[RTCBackupIndex]);		
	}
	
	return 0xFFFF;
}


/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
