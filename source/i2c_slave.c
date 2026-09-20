/******************************************************************************
* File Name: i2c_slave.c
* Version: 1.0
*
* Description: This file contains all the functions and variables required for proper
*                operation of I2C slave SCB.
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

#include "i2c_slave.h"
#include "stdio.h"

/*******************************************************************************
* Global constants
*******************************************************************************/
/* Buffer and packet size */
#define PACKET_SIZE          (3UL)

/* Buffer and packet size in the slave*/
#define SL_TX_BUFFER_SIZE    (03UL)
#define SL_RX_BUFFER_SIZE    (PACKET_SIZE)

/* Start and end of packet markers */
#define PACKET_SOP           (0x01UL)
#define PACKET_EOP           (0x17UL)

/* Command valid status */
#define STS_CMD_DONE         (0x00UL)
#define STS_CMD_FAIL         (0xFFUL)

/* Packet positions */
#define PACKET_SOP_POS       (0UL)
#define PACKET_STS_POS       (1UL)
#define PACKET_CMD_POS       (1UL)
#define PACKET_EOP_POS       (2UL)

#define ZERO                 (0UL)

/*******************************************************************************
* Global variables
*******************************************************************************/
/* I2C slave read and write buffers. */
uint8_t i2cTxBuffer [SL_TX_BUFFER_SIZE] = {PACKET_SOP, STS_CMD_FAIL, PACKET_EOP};
uint8_t i2cRxBuffer[SL_RX_BUFFER_SIZE] ;
cy_stc_scb_i2c_context_t sI2C_context;

/*******************************************************************************
* Forward declaration
*******************************************************************************/
void SlaveExecuteCommand(void);
void HandleEventsSlave(uint32 event);
void sI2C_Interrupt(void);

/*******************************************************************************
* Function Name: HandleEventsSlave
****************************************************************************//**
*
* Handles slave events write and read completion events.
*
* \param event
* reports slave events.
* ref uint32_t
*
* \return
*  None
*
*******************************************************************************/
void HandleEventsSlave(uint32_t event)
{
    /* Check write complete event. */
    if (0UL != (CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT & event))
    {

        /* Check for errors */
        if (0UL == (CY_SCB_I2C_SLAVE_ERR_EVENT & event))
        {
            /* Check packet length */
            if (PACKET_SIZE ==  Cy_SCB_I2C_SlaveGetWriteTransferCount(sI2C_HW, &sI2C_context))
            {
                /* Check start and end of packet markers. */
                if ((i2cRxBuffer[PACKET_SOP_POS] == PACKET_SOP) &&
                    (i2cRxBuffer[PACKET_EOP_POS] == PACKET_EOP))
                {
                    SlaveExecuteCommand();
                }
            }
        }

        /* Update status of received commend. */
        i2cTxBuffer[PACKET_STS_POS] = STS_CMD_DONE;

        /* Configure write buffer for the next write. */
        i2cRxBuffer[PACKET_SOP_POS] = ZERO;
        i2cRxBuffer[PACKET_EOP_POS] = ZERO;
        Cy_SCB_I2C_SlaveConfigWriteBuf(sI2C_HW, i2cRxBuffer, SL_RX_BUFFER_SIZE, &sI2C_context);
    }

    /* Check write complete event. */
    if (0UL != (CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT & event))
    {
        /* Configure read buffer for the next read. */
        i2cTxBuffer[PACKET_STS_POS] = STS_CMD_FAIL;
        Cy_SCB_I2C_SlaveConfigReadBuf(sI2C_HW, i2cTxBuffer, SL_TX_BUFFER_SIZE, &sI2C_context);
    }
}


/*******************************************************************************
* Function Name: SlaveExecuteCommand
****************************************************************************//**
*
*  Executes received command to control the LED state.
*
* \param None
*
* \return
*  None
*
*******************************************************************************/
void SlaveExecuteCommand( void )
{
    Cy_GPIO_Write(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN, i2cRxBuffer[PACKET_CMD_POS]);
}

/*******************************************************************************
* Function Name: sI2C_Interrupt
****************************************************************************//**
*
* Invokes the Cy_SCB_I2C_Interrupt() PDL driver function.
*
*******************************************************************************/
inline void sI2C_Interrupt(void)
{
    Cy_SCB_I2C_Interrupt(sI2C_HW, &sI2C_context);
}

/*******************************************************************************
* Function Name: initSlave
********************************************************************************
*
* This function initiates and enables slave SCB
*
* \param None
*
* \return
* Status of initialization
*
*******************************************************************************/
uint32_t initSlave(void)
{
    cy_en_scb_i2c_status_t initI2Cstatus;
    cy_en_sysint_status_t sysI2Cstatus;

    cy_stc_sysint_t sI2C_SCB_IRQ_cfg =
    {
            /*.intrSrc =*/ sI2C_IRQ,
            /*.intrPriority =*/ 2UL
    };

    /* Initialize the reply status packet. */
    i2cTxBuffer[PACKET_STS_POS] = STS_CMD_FAIL;

    /* Initialize and enable I2C component in slave mode. */
    initI2Cstatus = Cy_SCB_I2C_Init(sI2C_HW, &sI2C_config, &sI2C_context);
    if(initI2Cstatus != CY_SCB_I2C_SUCCESS)
    {
        return I2C_FAILURE;
    }
    sysI2Cstatus = Cy_SysInt_Init(&sI2C_SCB_IRQ_cfg, &sI2C_Interrupt);

    if(sysI2Cstatus != CY_SYSINT_SUCCESS)
    {
        return I2C_FAILURE;
    }
    Cy_SCB_I2C_SlaveConfigReadBuf(sI2C_HW, i2cTxBuffer, SL_TX_BUFFER_SIZE, &sI2C_context);
    Cy_SCB_I2C_SlaveConfigWriteBuf(sI2C_HW, i2cRxBuffer, SL_RX_BUFFER_SIZE, &sI2C_context);
    Cy_SCB_I2C_RegisterEventCallback(sI2C_HW, (cy_cb_scb_i2c_handle_events_t) HandleEventsSlave, &sI2C_context);

    NVIC_EnableIRQ((IRQn_Type) sI2C_SCB_IRQ_cfg.intrSrc);

    Cy_SCB_I2C_Enable(sI2C_HW);
    return I2C_SUCCESS;
}

