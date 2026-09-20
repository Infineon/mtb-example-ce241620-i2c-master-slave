/******************************************************************************
* File Name: i2c_master.c
* Version: 1.0
*
* Description: This file contains the functions and variables required for proper
*                operation of I2C master SCB
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

#include "i2c_master.h"
#include "stdio.h" 
/***************************************
*            Constants
****************************************/

/* Buffer and packet size */
#define RX_PACKET_SIZE      (3UL)

/* I2C slave address to communicate with */
#define I2C_SLAVE_ADDR      (0x24UL)

/* Command valid status */
#define STS_CMD_DONE        (0x00UL)
#define STS_CMD_FAIL        (0xFFUL)

/* Command valid status */
#define TRANSFER_ERROR      (0xFFUL)
#define READ_ERROR          (TRANSFER_ERROR)

/* Timeout */
#define LOOP_FOREVER        (0UL)

/* Packet positions */
#define RX_PACKET_SOP_POS   (0UL)
#define RX_PACKET_STS_POS   (1UL)
#define RX_PACKET_EOP_POS   (2UL)

/* Combine master error statuses in single mask  */
#define MASTER_ERROR_MASK   (CY_SCB_I2C_MASTER_DATA_NAK | CY_SCB_I2C_MASTER_ADDR_NAK    | \
                             CY_SCB_I2C_MASTER_ARB_LOST | CY_SCB_I2C_MASTER_ABORT_START | \
                             CY_SCB_I2C_MASTER_BUS_ERR)
/*******************************************************************************
* Global variables
*******************************************************************************/
/* Structure for master transfer configuration */

cy_stc_scb_i2c_master_xfer_config_t masterTransferCfg =
{
    .slaveAddress = I2C_SLAVE_ADDR,
    .buffer       = NULL,
    .bufferSize   = 0U,
    .xferPending  = false
};
cy_stc_scb_i2c_context_t mI2C_context;

/*******************************************************************************
* Forward declaration
*******************************************************************************/
void mI2C_Interrupt(void);

/*******************************************************************************
* Function Name: mI2C_Interrupt
****************************************************************************//**
*
* Invokes the Cy_SCB_I2C_Interrupt() PDL driver function.
*
*******************************************************************************/
void mI2C_Interrupt(void)
{
    Cy_SCB_I2C_Interrupt(mI2C_HW, &mI2C_context);
}

/*******************************************************************************
* Function Name: WritePacket
****************************************************************************//**
*
* Buffer is assigned with data to be sent to slave.
* high level APIs are used to control I2C SCB to send data.
* Errors are handled depend on the return value from the appropriate function.
*
* \param buffer
*
* \param bufferSize
*
* \return
* returns the status after command is written to slave.
* TRANSFER_ERROR is returned if any error occurs.
* TRANSFER_CMPLT is returned if write is successful.
* \ref uint32_t
*
*******************************************************************************/
uint8_t WritePacket(uint8_t* buffer, uint32_t bufferSize)
{
    uint8_t status = TRANSFER_ERROR;
    cy_en_scb_i2c_status_t  errorStatus;
    uint32_t masterStatus;
    /* Timeout 1 second (one unit is us) */
    int32_t timeout =  1000000UL;

    /* Setup transfer specific parameters. */
    masterTransferCfg.buffer     = buffer;
    masterTransferCfg.bufferSize = bufferSize;
    masterTransferCfg.slaveAddress = I2C_SLAVE_ADDR;
    
    /* Initiate write transaction. */
    errorStatus = Cy_SCB_I2C_MasterWrite(mI2C_HW, &masterTransferCfg, &mI2C_context);
    if(errorStatus == CY_SCB_I2C_SUCCESS)
    {
        /* Wait until master complete read transfer or time out has occurred */
        do
        {
            masterStatus  = Cy_SCB_I2C_MasterGetStatus(mI2C_HW, &mI2C_context);
            Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
            timeout--;
        } while ((0UL != (masterStatus & CY_SCB_I2C_MASTER_BUSY)) && (timeout > 0));

        if (timeout <= 0)
        {
            /* Timeout recovery */
            Cy_SCB_I2C_Disable(mI2C_HW, &mI2C_context);
            Cy_SCB_I2C_Enable (mI2C_HW);
        }
        else
        {
            if ((0u == (MASTER_ERROR_MASK & masterStatus)) &&
                (TX_PACKET_SIZE == Cy_SCB_I2C_MasterGetTransferCount(mI2C_HW, &mI2C_context)))
            {
                status = TRANSFER_CMPLT;
            }
        }
    }

    return (status);
}


/*******************************************************************************
* Function Name: ReadStatusPacket
****************************************************************************//**
*
*  Master initiates to read status packet from the slave.
*  The status of the transfer is returned.
*
* \return
* Checks the status packet and returns the status.
* ref uint32_t
*
* \note
* * If the status packet read is correct function returns TRANSFER_CMPLT anf
*   if status packet is incorrect function returns TRANSFER_ERROR.
*
*******************************************************************************/
uint8_t ReadStatusPacket(void)
{
    uint8_t status = TRANSFER_ERROR;
    cy_en_scb_i2c_status_t errorStatus;
    uint32_t masterStatus;
    /* Timeout 1 second (one unit is us) */
    uint32_t timeout = 1000000UL;
    uint8_t  rxBuffer[RX_PACKET_SIZE];

    /* Setup transfer specific parameters */
    masterTransferCfg.buffer     = rxBuffer;
    masterTransferCfg.bufferSize = RX_PACKET_SIZE;
    masterTransferCfg.slaveAddress = I2C_SLAVE_ADDR;

    /* Initiate read transaction */
    errorStatus = Cy_SCB_I2C_MasterRead(mI2C_HW, &masterTransferCfg, &mI2C_context);
    if(errorStatus == CY_SCB_I2C_SUCCESS)
    {
        /* Wait until master complete read transfer or time out has occurred */
        do
        {
            masterStatus  = Cy_SCB_I2C_MasterGetStatus(mI2C_HW, &mI2C_context);
            Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
            timeout--;

        } while ((0UL != (masterStatus & CY_SCB_I2C_MASTER_BUSY)) && (timeout > 0));
       if (timeout <= 0)
        {
            /* Timeout recovery */
            Cy_SCB_I2C_Disable(mI2C_HW, &mI2C_context);
            Cy_SCB_I2C_Enable (mI2C_HW);
        }
        else
        {
            /* Check transfer status */
            if (0u == (MASTER_ERROR_MASK & masterStatus))
            {
                /* Check packet structure and status */
                if((PACKET_SOP   == rxBuffer[RX_PACKET_SOP_POS]) &&
                    (PACKET_EOP   == rxBuffer[RX_PACKET_EOP_POS]) &&
                    (STS_CMD_DONE == (rxBuffer[RX_PACKET_STS_POS])))
                {
                    status = TRANSFER_CMPLT;
                }
            }
        }
    }
    return (status);
}
/*******************************************************************************
* Function Name: initMaster
********************************************************************************
*
* This function initiates and enables Master SCB
*
* \param None
*
* \return
* Status of initialization
*
*******************************************************************************/
uint32_t initMaster(void)
{
    cy_en_scb_i2c_status_t initStatus;
    cy_en_sysint_status_t sysStatus;

    cy_stc_sysint_t mI2C_SCB_IRQ_cfg =
    {
            .intrSrc = mI2C_IRQ,
            .intrPriority = 2UL
    };

    /* Initialize and enable the I2C in master mode. */
    initStatus = Cy_SCB_I2C_Init(mI2C_HW, &mI2C_config, &mI2C_context);
    if(initStatus != CY_SCB_I2C_SUCCESS)
    {
        return I2C_FAILURE;
    }

    /* Hook interrupt service routine. */
    sysStatus = Cy_SysInt_Init(&mI2C_SCB_IRQ_cfg, &mI2C_Interrupt);
    if(sysStatus != CY_SYSINT_SUCCESS)
    {
        return I2C_FAILURE;
    }

    /* Enable interrupt in NVIC. */
    NVIC_EnableIRQ((IRQn_Type) mI2C_SCB_IRQ_cfg.intrSrc);

    /* Enable I2C master hardware. */
    Cy_SCB_I2C_Enable(mI2C_HW);
    return I2C_SUCCESS;
}
