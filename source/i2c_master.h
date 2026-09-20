/******************************************************************************
* File Name: i2c_master.h
* Version: 1.0
*
* Description: This file contains Constants and Prototypes.
*  
********************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#ifndef SOURCE_I2C_MASTER_H_
#define SOURCE_I2C_MASTER_H_

#include "cy_pdl.h"
#include "cycfg.h"

/***************************************
*        Function prototypes
****************************************/
uint8_t WritePacket(uint8_t* buffer, uint32_t bufferSize);
uint8_t ReadStatusPacket(void);
uint32_t initMaster(void);

/***************************************
*             Constants
****************************************/
#define TRANSFER_CMPLT      (0x00UL)
#define READ_CMPLT          (TRANSFER_CMPLT)

#define PACKET_SOP_POS      (0UL)
#define PACKET_CMD_POS      (1UL)
#define PACKET_EOP_POS      (2UL)
#define PACKET_STS_POS      (1UL)


/* Buffer and packet size */
#define TX_PACKET_SIZE      (3UL)

/* Start and end of packet markers */
#define PACKET_SOP          (0x01UL)
#define PACKET_EOP          (0x17UL)

#define I2C_SUCCESS         (0UL)
#define I2C_FAILURE         (1UL)

#endif /* SOURCE_I2C_MASTER_H_ */
