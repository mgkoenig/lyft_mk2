/**
 * EUSART2 Generated Driver API Header File
 * 
 * @file eusart2.h
 * 
 * @defgroup eusart2 EUSART2
 * 
 * @brief This file contains API prototypes and other datatypes for the Enhanced Universal Synchronous and Asynchronous Receiver Transceiver (EUSART) module.
 *
 * @version EUSART2 Driver Version 3.0.2
*/
/*
© [2025] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/

#ifndef EUSART2_H
#define EUSART2_H


#include <stdbool.h>
#include <stdint.h>
#include "../system/system.h"
#include "uart_drv_interface.h"

#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif


#define UART2_interface UART2


#define UART2_Initialize     EUSART2_Initialize
#define UART2_Deinitialize   EUSART2_Deinitialize
#define UART2_Write          EUSART2_Write
#define UART2_Read           EUSART2_Read
#define UART2__IsRxReady     EUSART2_IsRxReady
#define UART2_IsTxReady      EUSART2_IsTxReady
#define UART2_IsTxDone       EUSART2_IsTxDone

#define UART2_TransmitEnable       EUSART2_TransmitEnable
#define UART2_TransmitDisable      EUSART2_TransmitDisable
#define UART2_AutoBaudSet          EUSART2_AutoBaudSet
#define UART2_AutoBaudQuery        EUSART2_AutoBaudQuery
#define UART2_BRGCountSet                (NULL)
#define UART2_BRGCountGet                (NULL)
#define UART2_BaudRateSet                (NULL)
#define UART2_BaudRateGet                (NULL)
#define UART2__AutoBaudEventEnableGet    (NULL)
#define UART2_ErrorGet             EUSART2_ErrorGet

#define UART2_TxCompleteCallbackRegister     EUSART2_TxCompleteCallbackRegister
#define UART2_RxCompleteCallbackRegister      EUSART2_RxCompleteCallbackRegister
#define UART2_TxCollisionCallbackRegister  (NULL)
#define UART2_FramingErrorCallbackRegister EUSART2_FramingErrorCallbackRegister
#define UART2_OverrunErrorCallbackRegister EUSART2_OverrunErrorCallbackRegister
#define UART2_ParityErrorCallbackRegister  (NULL)
#define UART2_EventCallbackRegister        (NULL)


/**
 @ingroup eusart2
 @struct eusart2_status_t
 @brief This is a structure defined for errors in reception of data.
 */
 /**
 * @misradeviation{@advisory,19.2}
 * The UART error status necessitates checking the bitfield and accessing the status within the group byte therefore the use of a union is essential.
 */
 /* cppcheck-suppress misra-c2012-19.2 */
typedef union {
    struct {
        uint8_t perr : 1;     /**<This is a bit field for Parity Error status*/
        uint8_t ferr : 1;     /**<This is a bit field for Framing Error status*/
        uint8_t oerr : 1;     /**<This is a bit field for Overfrun Error status*/
        uint8_t reserved : 5; /**<Reserved*/
    };
    size_t status;            /**<Group byte for status errors*/
}eusart2_status_t;





/**
 * @ingroup eusart2
 * @brief External object for eusart2_interface.
 */
extern const uart_drv_interface_t UART2;

/**
 * @ingroup eusart2
 * @brief Initializes the EUSART2 module. This routine is called
 *        only once during system initialization, before calling other APIs.
 * @param None.
 * @return None.
 */
void EUSART2_Initialize(void);

/**
 * @ingroup eusart2
 * @brief Deinitializes and disables the EUSART2 module.
 * @param None.
 * @return None.
 */
void EUSART2_Deinitialize(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 module.     
 * @param None.
 * @return None.
 */
void EUSART2_Enable(void);

/**
 * @ingroup eusart2
 * @brief This API disables the EUSART2 module.
 * @param None.
 * @return None.
 */
void EUSART2_Disable(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 transmitter.
 *        The EUSART2 must be enabled to send the bytes over to the TX pin.
 * @param None.
 * @return None.
 */
void EUSART2_TransmitEnable(void);

/**
 * @ingroup eusart2
 * @brief This API disables the EUSART2 transmitter.
 * @param None.
 * @return None.
 */
void EUSART2_TransmitDisable(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 receiver.
 *        The EUSART2 must be enabled to receive the bytes sent by the RX pin.
 * @param None.
 * @return None.
 */
void EUSART2_ReceiveEnable(void);

/**
 * @ingroup eusart2
 * @brief This API disables the EUSART2 receiver.
 * @param None.
 * @return None.
 */
void EUSART2_ReceiveDisable(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 transmitter interrupt.
 * @param None.
 * @return None.
 */
void EUSART2_TransmitInterruptEnable(void);

/**
 * @ingroup eusart2
 * @brief This API disables the EUSART2 transmitter interrupt.
 * @param None.
 * @return None.
 */
void EUSART2_TransmitInterruptDisable(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 receiver interrupt.
 * @param None.
 * @return None.
 */
void EUSART2_ReceiveInterruptEnable(void);

/**
 * @ingroup eusart2
 * @brief This API disables the EUSART2 receiver interrupt.
 * @param None.
 * @return None.
 */
void EUSART2_ReceiveInterruptDisable(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 to send a break control. 
 * @param None.
 * @return None.
 */
void EUSART2_SendBreakControlEnable(void);

/**
 * @ingroup eusart2
 * @brief This API disables the EUSART2 send break control.
 * @param None.
 * @return None.
 */
void EUSART2_SendBreakControlDisable(void);

/**
 * @ingroup eusart2
 * @brief This API enables the EUSART2 AutoBaud Detection (ABR). 
 * @param bool enable.
 * @return None.
 */
void EUSART2_AutoBaudSet(bool enable);

/**
 * @ingroup eusart2
 * @brief This API reads the EUSART2 ABR Complete bit.
 * @param None.
 * @return bool.
 */
bool EUSART2_AutoBaudQuery(void);

/**
 * @ingroup eusart2
 * @brief This API reads the EUSART2 ABR Overflow bit.
 * @param None.
 * @return None.
 */
bool EUSART2_IsAutoBaudDetectOverflow(void);

/**
 * @ingroup eusart2
 * @brief This API resets the EUSART2 ABR Overflow bit.
 * @param None.
 * @return None.
 */
void EUSART2_AutoBaudDetectOverflowReset(void);

/**
 * @ingroup eusart2
 * @brief This API checks if the EUSART2 has received available data.
 * @param None.
 * @retval true if EUSART2 receiver FIFO has a data
 * @retval false EUSART2 receiver FIFO is empty
 */
bool EUSART2_IsRxReady(void);

/**
 * @ingroup eusart2
 * @brief This function checks if the EUSART2 transmitter is ready to accept a data byte.
 * @param None.
 * @retval true if EUSART2 transmitter FIFO has atleast 1 byte space
 * @retval false if EUSART2 transmitter FIFO is full
 */
bool EUSART2_IsTxReady(void);

/**
 * @ingroup eusart2
 * @brief This function returns the status of Transmit Shift Register (TSR).
 * @param None.
 * @retval true if Data completely shifted out from the TSR
 * @retval false if Data is present in Transmit FIFO and/or in TSR
 */
bool EUSART2_IsTxDone(void);

/**
 * @ingroup eusart2
 * @brief This function receives the error status of the last read byte.
 * @param None.
 * @return Status of the last read byte. See eusart2_status_t struct for more details.
 */
size_t EUSART2_ErrorGet(void);

/**
 * @ingroup eusart2
 * @brief This function reads the 8 bits from the FIFO register receiver.
 * @pre The transfer status must be checked to see if the receiver is not empty
 *      before calling this function. Verify the EUSART2_IsRxReady(), before calling this API.
 * @param None.
 * @return 8-bit data from RX FIFO register.
 */
uint8_t EUSART2_Read(void);

/**
 * @ingroup eusart2
 * @brief This function writes a byte of data to the transmitter FIFO register.
 * @pre The transfer status must be checked to see if the transmitter is ready to accept a byte
 *      before calling this function. Verify the EUSART2_IsTxReady() before calling this API.
 * @param txData  - Data byte to write to the TX FIFO.
 * @return None.
 */
void EUSART2_Write(uint8_t txData);

/**
 * @ingroup eusart2
 * @brief This API registers the function to be called upon framing error.
 * @param callbackHandler - a function pointer which will be called upon framing error condition.
 * @return None.
 */
void EUSART2_FramingErrorCallbackRegister(void (* callbackHandler)(void));

/**
 * @ingroup eusart2
 * @brief This API registers the function to be called upon overrun error.
 * @param callbackHandler - a function pointer which will be called upon overrun error condition.
 * @return None.
 */
void EUSART2_OverrunErrorCallbackRegister(void (* callbackHandler)(void));

/**
 * @ingroup eusart2
 * @brief This indicates the function that will be called upon transmit interrupt.
 * @pre Initialize the EUSART2 module with transmit interrupt enabled
 * @param None.
 * @return None.
 */
extern void (*EUSART2_TxInterruptHandler)(void);

/**
 * @ingroup eusart2
 * @brief This API registers the function to be called upon after data has been successfully transmitted.
 * @param callbackHandler - a function pointer which will be called upon transmitter interrupt condition.
 * @return None.
 */
void EUSART2_TxCompleteCallbackRegister(void (* callbackHandler)(void));

/**
 * @ingroup eusart2
 * @brief This ISR function is called upon transmitter interrupt.
 * @param void.
 * @return None.
 */
void EUSART2_TransmitISR(void);

/**
 * @ingroup eusart2
 * @brief This indicates the function that will be called upon receive interrupt.
 * @pre Initialize the EUSART2 module with receive interrupt enabled
 * @param None.
 * @return None.
 */
extern void (*EUSART2_RxInterruptHandler)(void);
/**
 * @ingroup eusart2
 * @brief This API registers the function to be called upon after data has been successfully received.
 * @param callbackHandler - a function pointer which will be called upon receiver interrupt condition.
 * @return None.
 */
void EUSART2_RxCompleteCallbackRegister(void (* callbackHandler)(void));

/**
 * @ingroup eusart2
 * @brief This ISR function is called upon receiver interrupt.
 * @param void.
 * @return None.
 */
void EUSART2_ReceiveISR(void);

#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif

#endif  // EUSART2_H
