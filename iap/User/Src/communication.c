#include "communication.h"

static uint8_t comRxByte = 0;
static uint8_t comRxBuf[COM_RX_MAX_SIZE] = {0};
static uint32_t comRxCount = 0;

static uint8_t comHandleBuf[COM_RX_MAX_SIZE] = {0};
static uint32_t comHandleCount = 0;

static uint8_t com_txBuf[COM_TX_MAX_SIZE] = {0};
static uint32_t com_txBufLen = 0;

extern uint32_t FlashProtection;
extern pFunction JumpToApplication;
extern uint32_t JumpAddress;

enum COM_RX_STATE{
    COM_ADDR_S,
    COM_CMD_S,
    COM_REG_ADDRH_S,
    COM_REG_ADDRL_S,
    COM_REG_DATAH_S,
    COM_REG_DATAL_S,
    COM_CRC_L_S,
    COM_CRC_H_S,
    COM_EXTRA_DATA
};

enum COM_ERROR_TYPE{
    COM_FUN_ERR,
    COM_ADDR_ERR,
    COM_DATA_ERR,
    COM_EXE_ERR
};

enum EXE_ACTION_TYPE{
    EXE_NONE,
    EXE_JUMPTOAPP,
    EXE_TOGGLE_FLASHWRP,
};

    /**
  * @brief  calculate MODBUS CRC16.
  * @retval CRC16
  */
unsigned short getCRC16(volatile uint8_t *ptr ,uint8_t len)
{
    uint8_t  i;
    unsigned short crc = 0xFFFF;
    if(len==0)
    {
        len = 1;
    }
    while(len--)
    {
        crc ^= *ptr;
        for(i=0; i<8; i++)
        {
            if(crc&1)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
        ptr++;
    }
    return(crc);
}

static void comTransmitData(uint8_t * buf, uint32_t bufLen){
    /* transmit through interrupt */
    RS485_1_DE_EN;
    HAL_UART_Transmit_IT(&COM_UART_HANDLE, buf, bufLen);
}

static void COM_ErrorHandle(enum COM_ERROR_TYPE errType,\
    uint8_t addr, uint8_t func){
    unsigned short crcresult = 0;
    com_txBufLen = 0;
    com_txBuf[com_txBufLen++] = addr;
    com_txBuf[com_txBufLen++] = (0x80 | func);

    switch(errType){
    case COM_FUN_ERR:
        com_txBuf[com_txBufLen++] = 0x01;
        break;
    case COM_ADDR_ERR:
        com_txBuf[com_txBufLen++] = 0x02;
        break;
    case COM_DATA_ERR:
        com_txBuf[com_txBufLen++] = 0x03;
        break;
    case COM_EXE_ERR:
        com_txBuf[com_txBufLen++] = 0x04;
        break;
    default:
        break;
    }

    crcresult = getCRC16(com_txBuf, com_txBufLen);
    com_txBuf[com_txBufLen++] = LOW_BYTE(crcresult);
    com_txBuf[com_txBufLen++] = HIGH_BYTE(crcresult);
    comTransmitData(com_txBuf, com_txBufLen);
}

void communicationInit(void){
    RS485_1_RE_EN;
    comRxCount = 0;
    HAL_UART_Receive_IT(&COM_UART_HANDLE, &comRxByte, 1);
    __HAL_UART_ENABLE_IT(&COM_UART_HANDLE, UART_IT_IDLE);
}

void comPollingData(void){
    if(comHandleCount == 0){ return; }
    uint8_t handleIndex = 0;
    uint8_t state = COM_ADDR_S;
    uint8_t dataBuf[300] = {0};
    uint8_t dataLen = 0;
    uint8_t extraDataCount = 0;
    uint16_t crc16 = 0;
    while(handleIndex < comHandleCount){
        uint8_t data = comHandleBuf[handleIndex++];
        switch(state){
        case COM_ADDR_S:
            if((data == 0x01) || (data == 0xFF)){
                dataBuf[dataLen++] = data;
                ++state;
            }
            break;
        case COM_CMD_S:
            dataBuf[dataLen++] = data;
            ++state;
            break;
        case COM_REG_ADDRH_S:
            dataBuf[dataLen++] = data;
            ++state;
            break;
        case COM_REG_ADDRL_S:
            dataBuf[dataLen++] = data;
            ++state;
            break;
        case COM_REG_DATAH_S:
            dataBuf[dataLen++] = data;
            ++state;
            break;
        case COM_REG_DATAL_S:
            dataBuf[dataLen++] = data;
            ++state;
            break;
        case COM_CRC_L_S:
            if(data == 2 * (uint16_t)((dataBuf[4]<<8) | dataBuf[5])){
                dataBuf[dataLen++] = data;
                state = COM_EXTRA_DATA;
                extraDataCount = 0;
            }
            else{
                crc16 = data;
                ++state;
            }
            break;
        case COM_CRC_H_S:
            crc16 |= (data<<8);
            if(crc16 == getCRC16(dataBuf, dataLen)){
                comParseData(dataBuf, dataLen);
            }
            state = COM_ADDR_S;
            dataLen = 0;
            crc16 = 0;
            break;
        case COM_EXTRA_DATA:
            dataBuf[dataLen++] = data;
            extraDataCount++;
            if(extraDataCount >= dataBuf[6]){
                state = COM_CRC_L_S;
            }
            break;
        default:
            break;
        }
    }
    comHandleCount = 0;
}

/*
ret: 0,valid; 1: error
*/
static uint8_t isRegAddrValid(uint8_t func, uint16_t addrS){
    uint8_t result = 0;
    switch(func){
    case 3:
        result = (addrS == 0);
        break;
    case 6:
        result = (addrS == 1);
        break;
    case 0x10:
        result = 1;
        break;
    default:
        break;
    }
    return ((result)? 0 : 1);
}

/*
ret: 0,valid; 1: error
*/
static uint8_t isRegDataValid(uint8_t func, uint16_t addrS, uint16_t data){
    uint8_t result = 0;
    switch(func){
    case 3:
        result = ((addrS + data - 1) == 0);
        break;
    case 6:
        result = (data <= EXE_TOGGLE_FLASHWRP);
        break;
    case 0x10:
        result = (data <= IAP_PACKAGE_SIZE/2);
    default:
        break;
    }
    return ((result)? 0 : 1);
}

static void handleExeCmd(uint8_t addr, uint8_t cmd, uint16_t regAddr,\
    uint16_t data){
    uint16_t crc16 = 0;
    com_txBufLen = 0;
    com_txBuf[com_txBufLen++] = addr;
    com_txBuf[com_txBufLen++] = cmd;
    com_txBuf[com_txBufLen++] = HIGH_BYTE(regAddr);
    com_txBuf[com_txBufLen++] = LOW_BYTE(regAddr);
    com_txBuf[com_txBufLen++] = HIGH_BYTE(data);
    com_txBuf[com_txBufLen++] = LOW_BYTE(data);
    crc16 = getCRC16(com_txBuf, com_txBufLen);
    com_txBuf[com_txBufLen++] = LOW_BYTE(crc16);
    com_txBuf[com_txBufLen++] = HIGH_BYTE(crc16);

    switch(data){
    case EXE_NONE:
        comTransmitData(com_txBuf, com_txBufLen);
        break;
    case EXE_JUMPTOAPP:
        if (!(((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)){
            COM_ErrorHandle(COM_EXE_ERR, addr, cmd);
            break;
        }
        comTransmitData(com_txBuf, com_txBufLen);
        while(!IS_RS485_1_RE_EN);
        /* execute the new program */
        JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
        /* Jump to user application */
        JumpToApplication = (pFunction) JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
        JumpToApplication();
        break;
    case EXE_TOGGLE_FLASHWRP:
        if (FlashProtection != FLASHIF_PROTECTION_NONE)
        {
            /* Disable the write protection */
            if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_DISABLE) == FLASHIF_OK)
            {
                comTransmitData(com_txBuf, com_txBufLen);
                while(!IS_RS485_1_RE_EN);
                /* Launch the option byte loading */
                HAL_FLASH_OB_Launch();
            }
            else
            {
                COM_ErrorHandle(COM_EXE_ERR, addr, cmd);
            }
        }
        else
        {
            if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_ENABLE) == FLASHIF_OK)
            {
                comTransmitData(com_txBuf, com_txBufLen);
                while(!IS_RS485_1_RE_EN);
                /* Launch the option byte loading */
                HAL_FLASH_OB_Launch();
            }
            else
            {
                COM_ErrorHandle(COM_EXE_ERR, addr, cmd);
            }
        }
        break;
    }
}

void comParseData(uint8_t * buf, uint8_t len){
    uint8_t addr = buf[0];
    uint8_t cmd = buf[1];
    uint16_t regAddr = ((buf[2]<<8) | buf[3]);
    uint16_t data = ((buf[4]<<8) | buf[5]);

    if(isRegAddrValid(cmd, regAddr)){
        COM_ErrorHandle(COM_ADDR_ERR, addr, cmd);
    }
    else if(isRegDataValid(cmd, regAddr, data)){
        COM_ErrorHandle(COM_DATA_ERR, addr, cmd);
    }

    uint16_t crc16 = 0;
    int i,j = 0;
    switch(cmd){
    case 3:
        com_txBufLen = 0;
        com_txBuf[com_txBufLen++] = addr;
        com_txBuf[com_txBufLen++] = cmd;
        com_txBuf[com_txBufLen++] = data * 2;
        for(i=0,j=regAddr; i<data*2; i+=2,j++){
            com_txBuf[com_txBufLen + i] = HIGH_BYTE(FlashProtection);
            com_txBuf[com_txBufLen + i + 1] = LOW_BYTE(FlashProtection);
        }
        com_txBufLen += data * 2;
        crc16 = getCRC16(com_txBuf, com_txBufLen);
        com_txBuf[com_txBufLen++] = LOW_BYTE(crc16);
        com_txBuf[com_txBufLen++] = HIGH_BYTE(crc16);
        comTransmitData(com_txBuf, com_txBufLen);
        break;
    case 6:
        handleExeCmd(addr, cmd, regAddr, data);
        break;
    case 0x10:
        if(FLASHIF_OK == \
            FLASH_If_Write128Byte(regAddr, (buf + 7), *(buf + 6))){
            com_txBufLen = 0;
            com_txBuf[com_txBufLen++] = addr;
            com_txBuf[com_txBufLen++] = cmd;
            com_txBuf[com_txBufLen++] = HIGH_BYTE(regAddr);
            com_txBuf[com_txBufLen++] = LOW_BYTE(regAddr);
            com_txBuf[com_txBufLen++] = HIGH_BYTE(data);
            com_txBuf[com_txBufLen++] = LOW_BYTE(data);
            crc16 = getCRC16(com_txBuf, com_txBufLen);
            com_txBuf[com_txBufLen++] = LOW_BYTE(crc16);
            com_txBuf[com_txBufLen++] = HIGH_BYTE(crc16);
            comTransmitData(com_txBuf, com_txBufLen);
        }
        else{
            COM_ErrorHandle(COM_EXE_ERR, addr, cmd);
        }
        break;
    default:
        COM_ErrorHandle(COM_FUN_ERR, addr, cmd);
        break;
    }
}

void comUartIdleHandle(void){
    if(__HAL_UART_GET_FLAG(&COM_UART_HANDLE, UART_FLAG_IDLE)){
        __HAL_UART_CLEAR_IDLEFLAG(&COM_UART_HANDLE);
        if(comRxCount > 0){
            comHandleCount = comRxCount;
            memcpy(comHandleBuf, comRxBuf, comRxCount);
            comRxCount = 0;
        }
    }
}

void comRxHandle(void){
    comRxBuf[comRxCount++] = comRxByte;
    HAL_UART_Receive_IT(&COM_UART_HANDLE, &comRxByte, 1);
}

void comTxHandle(void){
    RS485_1_RE_EN;
}
