/**
  ******************************************************************************
  * @file    IAP_Main/Inc/flash_if.h 
  * @author  MCD Application Team
  * @brief   This file provides all the headers of the flash_if functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_IF_H
#define __FLASH_IF_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Base address of the Flash sectors */
#define ADDR_FLASH_START      ((uint32_t)0x08000000)
#define ADDR_FLASH_PAGE(n)    ((uint32_t)(ADDR_FLASH_START + n*FLASH_PAGE_SIZE))

/* Error code */
enum 
{
  FLASHIF_OK = 0,
  FLASHIF_ERASEKO,
  FLASHIF_WRITINGCTRL_ERROR,
  FLASHIF_WRITING_ERROR,
  FLASHIF_PROTECTION_ERRROR
};

/* protection type */  
enum{
  FLASHIF_PROTECTION_NONE         = 0,
  FLASHIF_PROTECTION_PCROPENABLED = 0x1,
  FLASHIF_PROTECTION_WRPENABLED   = 0x2,
  FLASHIF_PROTECTION_RDPENABLED   = 0x4,
};

/* protection update */
enum {
    FLASHIF_WRP_ENABLE,
    FLASHIF_WRP_DISABLE
};

/* Define the address from where user application will be loaded.
   Note: this area is reserved for the IAP code                  */
#define IAP_PACKAGE_SIZE        128
#define APPLICATION_ADDRESS     ADDR_FLASH_PAGE(12)
#define USER_FLASH_END_ADDRESS  ADDR_FLASH_PAGE(63)

/* Define bitmap representing user flash area that could be write protected (check restricted to pages 16-31). */
#define FLASH_PAGE_TO_BE_PROTECTED (OB_WRP_PAGES12TO15 | OB_WRP_PAGES16TO19 | \
                                    OB_WRP_PAGES20TO23 | OB_WRP_PAGES24TO27 | \
                                    OB_WRP_PAGES28TO31 | OB_WRP_PAGES32TO35 | \
                                    OB_WRP_PAGES36TO39 | OB_WRP_PAGES40TO43 | \
                                    OB_WRP_PAGES44TO47 | OB_WRP_PAGES48TO51 | \
                                    OB_WRP_PAGES52TO55 | OB_WRP_PAGES56TO59 | \
                                    OB_WRP_PAGES60TO63)


/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void FLASH_If_Init(void);
uint32_t FLASH_If_Erase(uint32_t start, uint32_t end);
uint32_t FLASH_If_GetWriteProtectionStatus(void);
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length);
uint32_t FLASH_If_WriteProtectionConfig(uint32_t protectionstate);
uint32_t FLASH_If_Write128Byte(uint16_t fileIndex, uint8_t *dataBuf, uint8_t dataLen);

#endif  /* __FLASH_IF_H */
