/*******************************************************************************
* File Name:   main.c
*
* Description: This code example demonstrates the use of the I2C resource in
* PDL master  and slave modes both on the same device. This is an integrated
* I2C master and I2C slave code where in the master is configured to send
* command packets to control a user LED on the slave.
* 
* Related Document: See README.md
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

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "i2c_master.h"
#include "i2c_slave.h"
#include "cy_pdl.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define OFF                 (1UL)
#define ON                  (0UL)
#define CMD_TO_CMD_DELAY    (1000UL)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* For the Retarget -IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    DEBUG_UART_context;           /** UART context */
static mtb_hal_uart_t               DEBUG_UART_hal_obj;

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function.
* The I2C master and slave interface are initialized using personalities.
* Interrupt priority is set for the receive event of the I2C slave.
*   1. I2C Master sends command packet to the slave
*   2. I2C Master reads the response packet to generate the next command
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/

int main(void)
{
    cy_rslt_t result;
    cy_en_scb_uart_status_t init_status;
    uint8_t cmd = ON;
    uint32_t status;
    uint8_t  buffer[TX_PACKET_SIZE];

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Start UART operation */
    init_status = Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);
    if (init_status!=CY_SCB_UART_SUCCESS)
    {
         CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(DEBUG_UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj, &DEBUG_UART_hal_config, &DEBUG_UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(&DEBUG_UART_hal_obj);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("************************************************************\r\n");
    printf("PSOC Control C3M/P8: I2C master-slave\r\n");
    printf("************************************************************\r\n\n");

    /* Initialize I2C Slave */
    printf(">> Initializing I2C Slave..... ");

    status = initSlave();
    if( I2C_SUCCESS != status)
    {
        CY_ASSERT(0);
    }
    printf("Done\r\n");

    /* Initialize I2C Master */
    printf(">> Initializing I2C master..... ");
    status = initMaster();
    if(status == I2C_FAILURE)
    {
        CY_ASSERT(0);
    }
    printf("Done\r\n");

    /* Enable interrupts */
    __enable_irq();

    for(;;)
    {
        /* create packet to be sent to slave.  */
        buffer[PACKET_SOP_POS] = PACKET_SOP;
        buffer[PACKET_EOP_POS] = PACKET_EOP;
        buffer[PACKET_CMD_POS] = cmd;

        uint8_t res = WritePacket(buffer, TX_PACKET_SIZE);

        /* Send packet with command to the slave. */
       if (TRANSFER_CMPLT == res)
       {
           printf("I2C: Write complete\r\n");

           /* Read response packet from the slave. */
           if (TRANSFER_CMPLT == ReadStatusPacket())
           {
               printf("I2C:  Read Complete\r\n");
               /* Next command to be written. */
               cmd = (cmd == ON) ? OFF : ON;
           }
           /* Give 1 Second delay between commands. */
           Cy_SysLib_Delay(CMD_TO_CMD_DELAY);
        }
        else
        {
            printf("Write failed with return val: %d\r\n", res);
            Cy_SysLib_Delay(1000);
        }
    }
}
