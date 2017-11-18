/*
*********************************************************************************************************
*
*	ģ������ : RTC
*	�ļ����� : bsp_rtc.c
*	��    �� : V1.0
*	˵    �� : RTC�ײ�����
*	�޸ļ�¼ :
*		�汾��   ����        ����     ˵��
*		V1.0    2013-12-12  armfly   ��ʽ����
*
*	Copyright (C), 2013-2014, ���������� www.armfly.com
*
*********************************************************************************************************
*/

#include "stm32f10x.h"
#include "bsp_rtc.h"

static void RTC_Config(void);


/* 20��RTC�������ݼĴ��� */
#define RTC_BKP_DR_NUMBER              20  

/* ѡ��ʱ��Դ */
/* #define RTC_CLOCK_SOURCE_LSE */       
#define RTC_CLOCK_SOURCE_LSI     

/* RTC �������ݼĴ��� */
uint32_t aRTC_BKP_DR[RTC_BKP_DR_NUMBER] =
{
	BKP_DR1, BKP_DR2, BKP_DR3, BKP_DR4, BKP_DR5,
	BKP_DR6, BKP_DR7, BKP_DR8, BKP_DR9, BKP_DR10,
	BKP_DR11, BKP_DR12, BKP_DR13, BKP_DR14, BKP_DR15,
	BKP_DR16, BKP_DR17, BKP_DR18, BKP_DR19, BKP_DR20,
};

/*
*********************************************************************************************************
*	�� �� ��: bsp_InitRTC
*	����˵��: ��ʼ��RTC
*	��    �Σ���
*	�� �� ֵ: ��		        
*********************************************************************************************************
*/
void bsp_InitRTC(void)
{
	RTC_Config();
}
	
/*
*********************************************************************************************************
*	�� �� ��: bsp_DeInitRTC
*	����˵��: �ָ�Ĭ��RTC
*	��    �Σ���
*	�� �� ֵ: ��		        
*********************************************************************************************************
*/
void bsp_DeInitRTC(void)
{
	/* ʹ��PWRʱ�� */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, DISABLE);

	/* �������RTC */
	PWR_BackupAccessCmd(DISABLE);
	
	/* Enable LSE */		
	RCC_LSEConfig(RCC_LSE_OFF);
}
	
/*
*********************************************************************************************************
*	�� �� ��: RTC_Config
*	����˵��: ����RTC�����ܱ�
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void RTC_Config(void)
{
	/* ʹ��PWRʱ�� */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* �������RTC */
	PWR_BackupAccessCmd(ENABLE);
	
	/* Enable LSE */		
	RCC_LSEConfig(RCC_LSE_ON);
	
}

/*
*********************************************************************************************************
*	�� �� ��: WriteToRTC_BKP_DR
*	����˵��: д���ݵ�RTC�ı������ݼĴ���
*	��    �Σ���FirstRTCBackupData д�������
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void WriteToRTC_BKP_DR(uint8_t RTCBackupIndex, uint16_t RTCBackupData)
{
	if (RTCBackupIndex < RTC_BKP_DR_NUMBER)
	{
		/* д���ݱ������ݼĴ��� */
		BKP_WriteBackupRegister(aRTC_BKP_DR[RTCBackupIndex], RTCBackupData);
	}
}

/*
*********************************************************************************************************
*	�� �� ��: ReadRTC_BKP_DR
*	����˵��: ��ȡ����������
*	��    �Σ�RTCBackupIndex �Ĵ�������ֵ
*	�� �� ֵ: 
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


/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
