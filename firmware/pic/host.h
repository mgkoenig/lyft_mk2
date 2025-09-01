/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef HOST_H
#define	HOST_H
#include "mcc_generated_files/system/system.h"


#define HOST_STX            0x4C    
#define DATA_BUFFER_SIZE    8

#define PROTOCOL_VERSION    1


struct host_data_packet {
    uint8_t                 command;
    uint8_t                 length;
    uint8_t                 data[DATA_BUFFER_SIZE];
    uint8_t                 checksum;
};

enum host_transmission {
    READY,
    RECEIVING,
    TRANSMITTING,
    FINISHED,
};

enum host_commands {
    
    GET_DESK_STATE              = 0x10,
    GET_DESK_DRIFT              = 0x11,
    GET_DESK_POSITION           = 0x12,
    GET_DESK_UPPER_LIMIT        = 0x13,
    GET_DESK_LOWER_LIMIT        = 0x14,
    
    GET_MOTOR_LEFT_STATE        = 0x20,
    GET_MOTOR_LEFT_POSITION     = 0x21,
    GET_MOTOR_LEFT_UPPER_LIMIT  = 0x22,
    GET_MOTOR_LEFT_LOWER_LIMIT  = 0x23,
    GET_MOTOR_LEFT_NODE_ID      = 0x24,
    GET_MOTOR_LEFT_SCAN_ID      = 0x25,
    GET_MOTOR_LEFT_PROPERTY     = 0x26,
    
    GET_MOTOR_RIGHT_STATE       = 0x30,
    GET_MOTOR_RIGHT_POSITION    = 0x31,
    GET_MOTOR_RIGHT_UPPER_LIMIT = 0x32,
    GET_MOTOR_RIGHT_LOWER_LIMIT = 0x33,
    GET_MOTOR_RIGHT_NODE_ID     = 0x34,
    GET_MOTOR_RIGHT_SCAN_ID     = 0x35,
    GET_MOTOR_RIGHT_PROPERTY    = 0x36,    
    
    CALL_DESK_INIT              = 0x40,
    CALL_DESK_DEINIT            = 0x41,
    CALL_DESK_CALIBRATION       = 0x42,
    CALL_SOMETHING              = 0x43,
    
    SET_DESK_POSITION           = 0x50,
    SET_DESK_HALT               = 0x51,
    
    GET_PROTOCOL_VERSION        = 0x70,
    GET_FIRMWARE_VERSION        = 0x71,
    GET_BOARD_REVISION          = 0x72, 
    
    CALL_WATCHDOG_ENABLE        = 0x73,
    CALL_WATCHDOG_DISABLE       = 0x74
};

enum host_errorCodes {
    E_OK,
    E_TIMEOUT,
    E_INVALID_DATA,
    E_INVALID_COMMAND,
    E_INVALID_CHECKSUM,
    
    E_DESK_BUSY,
    E_DESK_NOT_IDLE,
    E_DESK_NOT_READY,

    E_DESK_MIN_DISTANCE,
    E_DESK_UPPER_LIMIT_REACHED,
    E_DESK_LOWER_LIMIT_REACHED
};


void host_init();
bool host_newDataAvailable();
void host_read(struct host_data_packet *packet);
void host_write(struct host_data_packet packet);

void host_calcChecksum(struct host_data_packet *packet);
bool host_verifyChecksum(struct host_data_packet packet);

static void cb_host_rx(void);
static void cb_host_tx(void);
static void cb_tmr2();


#endif	/* HOST_H */

