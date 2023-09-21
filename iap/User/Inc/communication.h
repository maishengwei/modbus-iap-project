#ifndef __COMMUNICATION_H
#define __COMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flash_if.h"

extern UART_HandleTypeDef huart3;
#define COM_UART_HANDLE     huart3

#define RS485_1_RE_EN       {RS485_1_RE_OFF; RS485_1_DE_OFF;}
#define RS485_1_DE_EN       {RS485_1_RE_ON; RS485_1_DE_ON;}
#define IS_RS485_1_RE_EN    (IS_RS485_1_DE_OFF && IS_RS485_1_RE_OFF)

#define COM_RX_MAX_SIZE     1500
#define COM_TX_MAX_SIZE     64

void communicationInit(void);
void comPollingData(void);
void comParseData(uint8_t * buf, uint8_t len);
void comRxHandle(void);
void comUartIdleHandle(void);
void comTxHandle(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMMUNICATION_H */
