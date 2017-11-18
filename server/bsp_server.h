/*
*********************************************************************************************************
* @file    	bsp_server.h
* @author  	SY
* @version 	V1.0.0
* @date    	2017-10-24 19:54:57
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	后台服务头文件
*********************************************************************************************************
* @attention
*
* 
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Define to prevent recursive inclusion
*********************************************************************************************************
*/
#ifndef __BSP_SERVER_H
#define __BSP_SERVER_H

/*
*********************************************************************************************************
*                              				Exported Includes
*********************************************************************************************************
*/
#include "utils.h"
#include "command.h"
#include "SeqQueue.h"

/*
*********************************************************************************************************
*                              				Exported define
*********************************************************************************************************
*/
#define SERVER_PASSWD						(0x5AA5)

/*
*********************************************************************************************************
*                              				Exported types
*********************************************************************************************************
*/
struct SOCKET_TypeDef {
	struct ip_addr ip;
	uint16_t port;
};

enum SOCKET_TYPE {
	UDP = 0,
	TCP,	
};

struct SOCKET_OPS_TypeDef {
	err_t (*send)(void *parent, const uint8_t *data, uint32_t lenth, struct SOCKET_TypeDef *socket);
	void (*close)(void *parent);
};

struct CMD_VECTOR
{
	uint16_t cmd;
	void (*init)(void *para);
	bool (*parseCmd)(const uint8_t *rxBody, uint16_t rxLength, \
		uint8_t *txBody, uint16_t *txLength);
};

struct SERVER_TypeDef {
	bool reboot;
	bool enforceStop;
	enum SOCKET_TYPE socketType;
	struct SOCKET_TypeDef localSocket;
	struct SOCKET_TypeDef remoteSocket;
	uint8_t rxBuff[128];
	uint8_t txBuff[128];
	struct CMD_VECTOR cmdBuffer[5];
	SEQUEUE_TypeDef cmdQueue;
	struct SOCKET_OPS_TypeDef ops;
};

struct UDP_SERVER_TypeDef {
	struct SERVER_TypeDef server;
	struct udp_pcb *PCB;
};



/*
*********************************************************************************************************
*                              				Exported constants
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Exported variables
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                              				Exported macro
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Exported functions
*********************************************************************************************************
*/
void bsp_InitServer(void);


#endif
/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
