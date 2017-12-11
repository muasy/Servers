/*
*********************************************************************************************************
* @file    	bsp_server.c
* @author  	SY
* @version 	V1.0.4
* @date    	2017-12-11 11:23:58
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	后台服务源文件
*********************************************************************************************************
* @attention
* =======================================================================================================
*
* ---------------------------------------------------------
* 版本：V1.0.0 	修改人：SY	修改日期：2017-10-24 19:13:03	
* 
* 1.创建源文件
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* 版本：V1.0.1 	修改人：SY	修改日期：2017-11-17 09:33:34	
* 
* 1.问题：严伟成使用 #include "bsp_server.h" 后，连接上位机报错
*	原因：该头文件中恢复默认8字节对齐，而下位机全部默认1字节对齐，因此产生冲突。
*	解决：创建头新文件"bsp_protocal.h"，将需要字节对齐的结构体放到该文件中。
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* 版本：V1.0.2 	修改人：SY	修改日期：2017-11-18 09:59:32	
* 
* 1.在实际使用过程中，由于局域网内存在多个主机，只使用套接字标识设备容易产生混乱，因此增加设备型号等命令
* -------------------------------------------------------------------------------------------------------
*	
* ---------------------------------------------------------
* 版本：V1.0.3 	修改人：SY	修改日期：2017-11-20 10:35:52
* 
* 1.问题：不能打印调试信息。
*	原因：由于引入了 #include "CONFIG.H"，该文件默认1字节对齐，导致结构体字节错位
* 	解决：在 #include "CONFIG.H" 后面添加 #pragma pack()恢复字节对齐
* -------------------------------------------------------------------------------------------------------
*										
* ---------------------------------------------------------
* 版本：V1.0.4 	修改人：SY	修改日期：2017-12-11 11:23:48
* 
* 1.问题：自动升级后，不能连接下位机。重启控制器后可以连接以太网连接正常。
*	原因：下位机以太网没有硬件复位
* 	解决：增加 udp_disconnect() 函数。修改函数 Ethernet_Configuration(); 增加以太网硬件复位。
* -------------------------------------------------------------------------------------------------------
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "bsp_server.h"
#include "bsp_protocal.h"
#include "crc.h"
#include "bsp_commu.h"



/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/
#define SERVER_PORT							(5555)
#define SERVER_RROADCAST_PORT				(5556)




/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/


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
void ms_delay(uint8_t md);

static err_t SocketSend(void *parent, const uint8_t *data, uint32_t lenth, struct SOCKET_TypeDef *socket);
static void SocketClose(void *parent);




/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct UDP_SERVER_TypeDef g_UDP_Server = {
	.server = {
		.socketType = UDP,
		.localSocket = {
			.port = SERVER_PORT,
		},
		.ops = {
			.send = SocketSend,
			.close = SocketClose,
		},
	},
};




/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : ETH_GetBodySize
* Description	: 获取数据包体大小
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint16_t ETH_GetBodySize(const uint8_t *buffer)
{
	struct frame_head_udp *udp_head = (struct frame_head_udp *)(void *)buffer;
	return udp_head->size;
}

/*
*********************************************************************************************************
* Function Name : ETH_GetIndex
* Description	: 获取数据包索引值
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t ETH_GetIndex(const uint8_t *buffer)
{
	struct frame_head_udp *udp_head = (struct frame_head_udp *)(void *)buffer;
	return udp_head->index;
}

/*
*********************************************************************************************************
* Function Name : ETH_GetPackageBody
* Description	: 获取数据包体
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t *ETH_GetPackageBody(const uint8_t *buffer)
{
	struct frame_head_udp *udp_head = (struct frame_head_udp *)(void *)buffer;	    
	return &(udp_head->data);
}

/*
*********************************************************************************************************
* Function Name : ETH_GetPackageTail
* Description	: 获取数据包尾
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
struct frame_tail_udp *ETH_GetPackageTail(const uint8_t *buffer)
{
	return (struct frame_tail_udp *)(ETH_GetPackageBody(buffer) + ETH_GetBodySize(buffer));
}

/*
*********************************************************************************************************
* Function Name : ETH_GetCmd
* Description	: 获取命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint16_t ETH_GetCmd(const uint8_t *buffer)
{
	uint8_t *body = ETH_GetPackageBody(buffer);
	return *(uint16_t *)body;
}

/*
*********************************************************************************************************
* Function Name : ETH_CheckPackage
* Description	: 校验数据包
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ETH_CheckPackage(const uint8_t *buffer)
{
	uint16_t bodySize = ETH_GetBodySize(buffer);
	struct frame_tail_udp *tail = ETH_GetPackageTail(buffer);
	uint16_t crc = Get_CRC16(0, buffer, sizeof(struct frame_head_udp) - 1 + bodySize);
	if (tail->check == crc)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
*********************************************************************************************************
* Function Name : SocketSend
* Description	: 发送数据包
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static err_t SocketSend(void *parent, const uint8_t *data, uint32_t lenth, struct SOCKET_TypeDef *socket)
{
	err_t err;
	struct pbuf *package = pbuf_alloc(PBUF_RAW, lenth, PBUF_ROM);				 				
	package->payload = (void *)data;
	
	struct SERVER_TypeDef *server = parent;
	if (server->socketType == UDP)
	{
		struct UDP_SERVER_TypeDef *udpServer = parent;
		err = udp_sendto(udpServer->PCB, package, &socket->ip, socket->port);
	}
	else if (server->socketType == TCP)
	{
		;//tcp send
	}
	pbuf_free(package);	
	
	return err;
}

/*
*********************************************************************************************************
* Function Name : SocketClose
* Description	: 关闭Socket
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SocketClose(void *parent)
{
	struct SERVER_TypeDef *server = parent;
	if (server->socketType == UDP)
	{
		struct UDP_SERVER_TypeDef *udpServer = parent;
		udp_remove(udpServer->PCB);
	}
	else if (server->socketType == TCP)
	{
		;//tcp close
	}
}
	
/*
*********************************************************************************************************
* Function Name : ETH_SendPackage
* Description	: 发送数据包
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ETH_SendPackage(void *parent, const uint8_t *msgBody, uint16_t msgSize, uint8_t addr, uint8_t index)
{	
	struct SERVER_TypeDef *server = parent;	
	struct frame_head_udp *udp_head=(struct frame_head_udp *)(void *)server->txBuff;
	udp_head->start = MESSAGE_START;
	udp_head->addr = addr;
	udp_head->index = index;
	udp_head->rsv1 = 0x5a;
	udp_head->size = msgSize;
	udp_head->rsv2 = 0xa5;
	udp_head->token = TOKEN;
	memcpy(&udp_head->data, msgBody, msgSize);	
	uint16_t length = sizeof(struct frame_head_udp) - 1 + msgSize;
	uint16_t crc16 = Get_CRC16(0, server->txBuff, length);
	struct frame_tail_udp *udp_tail = (struct frame_tail_udp *)(void *)(server->txBuff + length); 
	udp_tail->check = crc16;
	udp_tail->end = MESSAGE_END;
	length += sizeof(struct frame_tail_udp);
	SocketSend(parent, server->txBuff, length, &server->remoteSocket);
} 

/*
*********************************************************************************************************
* Function Name : ETH_GetIndex
* Description	: 获取索引值类型
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static UDP_INDEX_TypeDef ETH_GetIndexType(const uint8_t *buffer)
{
	UDP_INDEX_TypeDef index_type;
	struct frame_head_udp *udp_head = (struct frame_head_udp *)(void *)buffer;
	
	if ((udp_head->index >= 0x20) && (udp_head->index <= 0x7F))
	{
		index_type = UDP_INDEX_ACT_ASW_NOR;
	}
	else if ((udp_head->index >= 0x80) && (udp_head->index <= 0xBF))
	{
		index_type = UDP_INDEX_ACT_NASW_NOR;
	}
	else if ((udp_head->index >= 0xC0) && (udp_head->index <= 0xFF))
	{
		index_type = UDP_INDEX_PAS_ASW_NOR;
	}
	else
	{
		index_type = UDP_INDEX_IVLD;
	}
	
	return index_type;
}

/*
*********************************************************************************************************
* Function Name : ETH_NextIndex
* Description	: 获取下一个索引值
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t ETH_NextIndex(UDP_INDEX_TypeDef indexType, uint8_t lastIndex)
{
	lastIndex++;
	switch (indexType)
	{
		case UDP_INDEX_ACT_ASW_NOR:			
			if ((lastIndex < 0x20) || (lastIndex > 0x7F))
			{
				lastIndex = 0x20;
			}
			break;
		case UDP_INDEX_ACT_NASW_NOR:
			if ((lastIndex < 0x80) || (lastIndex > 0xBF))
			{
				lastIndex = 0x80;
			}
			break;
		case UDP_INDEX_PAS_ASW_NOR:
			if (lastIndex < 0xC0)
			{
				lastIndex = 0xC0;
			}
			break;
		default:			
			break;
	}
	
	return lastIndex;
}

/*
*********************************************************************************************************
* Function Name : SendBroadcastCmd
* Description	: 发送广播命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SendBroadcastCmd(struct SERVER_TypeDef *this, uint16_t cmd)
{
	uint8_t nextIndex = ETH_NextIndex(UDP_INDEX_ACT_NASW_NOR, 0);
	struct CmdVoidTx package;
	package.cmd = cmd;
	this->remoteSocket.ip = *IP_ADDR_BROADCAST;
	this->remoteSocket.port = SERVER_RROADCAST_PORT;
	
	ETH_SendPackage(this, (const uint8_t *)&package, sizeof(package), 0, nextIndex);
}

/*
*********************************************************************************************************
* Function Name : PushSeqQueueCmdVector_CallBack
* Description	: 顺序队列数据入队
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static __INLINE void PushSeqQueueCmdVector_CallBack( void *base, uint32_t offset, void *dataIn )
{
	struct CMD_VECTOR *dataPtr = dataIn;
	struct CMD_VECTOR *basePtr = base;
	
	basePtr[offset] = *dataPtr;
}

/*
*********************************************************************************************************
* Function Name : PopSeqQueueCmdVector_CallBack
* Description	: 顺序队列数据出队
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static __INLINE void PopSeqQueueCmdVector_CallBack( void *base, uint32_t offset, void *dataOut )
{
	struct CMD_VECTOR *dataPtr = dataOut;
	struct CMD_VECTOR *basePtr = base;
	
	*dataPtr = basePtr[offset];
}

/*
*********************************************************************************************************
* Function Name : AddCmd
* Description	: 添加命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void AddCmd(struct SERVER_TypeDef *this, uint16_t cmd, \
		void (*init)(void *para),\
		void *private_data,\
		bool (*parseCmd)(const uint8_t *rxBody, uint16_t rxLength, \
		uint8_t *txBody, uint16_t *txLength))
{
	struct CMD_VECTOR cmdVector;
	cmdVector.cmd = cmd;
	cmdVector.init = init;
	cmdVector.private_data = private_data;
	cmdVector.parseCmd = parseCmd;
	PushSeqQueue(&this->cmdQueue, &cmdVector, PushSeqQueueCmdVector_CallBack);
}

/*
*********************************************************************************************************
* Function Name : TraverseCmdInit
* Description	: 遍历命令初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void TraverseCmdInit(void *data, void *private)
{
	struct CMD_VECTOR *this = data;
	if (this->init)
	{
		this->init(this->private_data);
	}
}

/*
*********************************************************************************************************
* Function Name : ServerInitCmd
* Description	: 初始化命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ServerInitCmd(struct SERVER_TypeDef *this)
{
	CreateSeqQueue(&this->cmdQueue, this->cmdBuffer, ARRAY_SIZE(this->cmdBuffer));
	/* 添加命令 */
	AddCmd(this, CMD_PRINT_LOG, 	bsp_InitLog, 		this, ParseLogCmd);
	AddCmd(this, CMD_AUTO_UPGRADE, 	bsp_InitUpgrade, 	this, ParseUpgradeCmd);
	AddCmd(this, CMD_SHAKE_HANDS, 	NULL, 				this, ParseVoidCmd);
	AddCmd(this, CMD_DEVICE_MODEL, 	NULL, 				this, ParseDeviceModelCmd);
	
	struct CMD_VECTOR cmdVector;
	TraverseSeqQueue(&this->cmdQueue, &cmdVector, NULL, \
		PopSeqQueueCmdVector_CallBack, \
		TraverseCmdInit);
}

/*
*********************************************************************************************************
* Function Name : LwIP_PeriodicHandler
* Description	: LwIP周期性处理任务
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void LwIP_PeriodicHandler(void)
{
	extern __IO uint32_t LocalTime;
	void LwIP_Periodic_Handle(__IO uint32_t localtime);
	
	for (uint32_t i=0; i<50; ++i) {
		LwIP_Periodic_Handle(LocalTime);
		ms_delay(1);
	}
}

/*
*********************************************************************************************************
* Function Name : SystemRebootHandler
* Description	: 系统重启处理
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SystemRebootHandler(struct SERVER_TypeDef *this)
{
	if (this->reboot)
	{
		this->reboot = FALSE;
		
		SendBroadcastCmd(this, CMD_NOTIFY_DISCONNECT);
		ms_delay(50);
		
		extern struct udp_pcb *UdpPcb;
		udp_disconnect(UdpPcb);		
		extern struct netif netif;
		netif_set_down(&netif);
		ms_delay(50);	

		LwIP_PeriodicHandler();
		ms_delay(50);
		
		NVIC_SystemReset();
	}
}

/*
*********************************************************************************************************
* Function Name : ParseClientRequest
* Description	: 解析客户端请求
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ParseClientRequest(struct SERVER_TypeDef *this, uint32_t length, const struct ip_addr *addr, u16_t port)
{
	if (ETH_CheckPackage(this->rxBuff) == FALSE)
	{
		return;
	}
	UDP_INDEX_TypeDef indexType = ETH_GetIndexType(this->rxBuff);
	if (indexType != UDP_INDEX_ACT_ASW_NOR)
	{
		return;
	}
	
	uint8_t index = ETH_GetIndex(this->rxBuff);
	uint16_t cmd = ETH_GetCmd(this->rxBuff);
	uint16_t msgSize = 0;
	uint8_t msgBuffer[128];	
	bool ret = FALSE;
	
	SEQUEUE_TypeDef queue = this->cmdQueue;	
	struct CMD_VECTOR cmdVector;
	while (PopSeqQueue(&queue, &cmdVector, PopSeqQueueCmdVector_CallBack) == STATUS_DATA_STRUCT_TRUE)
	{
		if (cmdVector.cmd == cmd)
		{
			if (cmdVector.parseCmd)
			{
				ret = cmdVector.parseCmd(ETH_GetPackageBody(this->rxBuff),\
						ETH_GetBodySize(this->rxBuff),\
						msgBuffer, &msgSize);
				break;
			}
		}
	}
	
	if (ret == FALSE)
	{
		return;
	}
	/* 发送缓存溢出 */
	if (msgSize > sizeof(msgBuffer))
	{
		return;
	}
	ETH_SendPackage(this, msgBuffer, msgSize, 0, index);	
	SystemRebootHandler(this);
}

/*
*********************************************************************************************************
* Function Name : SocketServer_CallBack
* Description	: 套接字接收数据回调函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SocketServer_CallBack(void *arg, struct udp_pcb *upcb, struct pbuf *p, \
	struct ip_addr *addr, u16_t port)
{	
	struct SERVER_TypeDef *this = (struct SERVER_TypeDef *)arg;	
	this->remoteSocket.ip = *addr;
	this->remoteSocket.port = port;	
	if (p->len <= sizeof(this->rxBuff)) {
		memcpy((void *)this->rxBuff, p->payload, p->len);		
		ParseClientRequest(this, p->len, addr, port);
	}
	
	pbuf_free(p);
}

/*
*********************************************************************************************************
* Function Name : UDP_SocketInit
* Description	: 初始化UDP Socket
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void UDP_SocketInit(struct UDP_SERVER_TypeDef *this, \
			void (*recv)(void *arg, struct udp_pcb *upcb, struct pbuf *p, \
			struct ip_addr *addr, u16_t port))
{
	this->server.localSocket.ip = *IP_ADDR_ANY;
	this->PCB = udp_new(); 
	udp_bind(this->PCB, &this->server.localSocket.ip, this->server.localSocket.port);
	udp_recv(this->PCB, recv, this);
}

/*
*********************************************************************************************************
* Function Name : bsp_InitUDP_Server
* Description	: 初始化udp服务器
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_InitUDP_Server(struct UDP_SERVER_TypeDef *this)
{
	UDP_SocketInit(this, SocketServer_CallBack);	
	ServerInitCmd(&this->server);
	LwIP_PeriodicHandler();
	SendBroadcastCmd(&this->server, CMD_NOTIFY_CONNECT);
}

/*
*********************************************************************************************************
* Function Name : bsp_InitServer
* Description	: 初始化服务器
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitServer(void)
{
	bsp_InitUDP_Server(&g_UDP_Server);
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
