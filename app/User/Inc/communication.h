#ifndef __COMMUNICATION_H
#define __COMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <string.h>

#define RS485_1_RE_EN   {RS485_1_RE_OFF; RS485_1_DE_OFF;}
#define RS485_1_DE_EN   {RS485_1_RE_ON; RS485_1_DE_ON;}

#define HIGH_BYTE(VALUE)        (((VALUE)>>8)&(0xFF))
#define LOW_BYTE(VALUE)         ((VALUE)&(0xFF))

#define COM_RX_MAX_SIZE     256
#define COM_TX_MAX_SIZE     256

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
