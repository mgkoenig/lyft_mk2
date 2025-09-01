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
#ifndef BEKANT_H
#define	BEKANT_H
#include "mcc_generated_files/system/system.h"


#define STARTUP_CMD_01              0x01
#define STARTUP_CMD_SCAN            0x02
#define STARTUP_CMD_WRITE           0x04
#define STARTUP_CMD_READ            0x06
#define STARTUP_CMD_07              0x07

#define STARTUP_REG_09              0x09
#define STARTUP_REG_UPPER_LIMIT_HI  0x0C        /**< node register holding the value for the upper_limit_high_byte */
#define STARTUP_REG_UPPER_LIMIT_LO  0x0D
#define STARTUP_REG_LOWER_LIMIT_HI  0x0A
#define STARTUP_REG_LOWER_LIMIT_LO  0x0B

#define DESK_ADDR_ONE               1			/**< LIN node ID 1 */
#define DESK_ADDR_MOTOR_LEFT        8			/**< LIN node ID 8 (Motor Left) */
#define DESK_ADDR_MOTOR_RIGHT       9			/**< LIN node ID 9 (Motor Right) */
#define DESK_ADDR_SIXTEEN           16			/**< LIN node ID 16 */
#define DESK_ADDR_SEVENTEEN         17			/**< LIN node ID 17 */
#define DESK_ADDR_MASTER            18			/**< LIN node ID 18 (Master) */

#define STARTUP_READ_RETRY          2
#define DESK_PACKET_LEN             3           /**< every data packet in operation mode has a net data length of 3 bytes (without checksum) */
#define DESK_STOPPING_DISTANCE      140

#define MOTOR_CMD_IDLE				0xFC		/**< Motor command idle */
#define MOTOR_CMD_ANNOUNCEMENT		0xC4		/**< Motor command to announce a moving command */
#define MOTOR_CMD_CALIBRATION       0xBD		/**< Motor command for calibration */
#define MOTOR_CMD_CALIBRATION_STOP  0xBC        /**< Motor command to finish calibration mode */
#define MOTOR_CMD_MOVE_UP			0x86		/**< Motor command move up */
#define MOTOR_CMD_MOVE_DOWN			0x85		/**< Motor command move down */
#define MOTOR_CMD_MOVE_SLOW			0x87		/**< Motor command move slow */
#define MOTOR_CMD_MOVE_STOP			0x84		/**< Motor command stop */
#define MOTOR_CMD_LIMIT_START		0x80		/**< Motor command when trying to go above/beneath the limit */
#define MOTOR_CMD_LIMIT_HIGH		0x82		/**< Motor command when upper limit is reached */
#define MOTOR_CMD_LIMIT_LOW			0x81		/**< Motor command when lower limit is reached */

#define MOTOR_STATE_IDLE			0x00		/**< Motor state idle */
#define MOTOR_STATE_CALIBRATED		0x01		/**< Motor state calibrated (finished) */
#define MOTOR_STATE_MOVING			0x02		/**< Motor state moving */
#define MOTOR_STATE_MOVING_SLOW		0x03		/**< Motor state moving slow */
#define MOTOR_STATE_CALIBRATING     0x04		/**< Motor state calibrating (ongoing) */
#define MOTOR_STATE_LIMIT_A			0xA5		/**< Motor state unknown (measured on LIN bus) */
#define MOTOR_STATE_LIMIT_B			0x25		/**< Motor state unknown (measured on LIN bus) */
#define MOTOR_STATE_BLOCKED         0x62		/**< Motor state blocked (error state) */
#define MOTOR_STATE_ERROR			0x65		/**< Motor state general error condition */


enum desk_state {       // desk_mode, desk_operation
    IDLE                        = 0x00,
    MAINTENANCE_BLOCKED         = 0x10,    
    MAINTENANCE_CALIBRATION     = 0x11,
    STARTUP                     = 0x20,
    STARTUP_BEGIN               = 0x21,
    STARTUP_ANNOUNCEMENT_ONE    = 0x22,
    STARTUP_ANNOUNCEMENT_TWO    = 0x23,
    STARTUP_PREPROCESSING_ONE   = 0x24,
    STARTUP_PREPROCESSING_TWO   = 0x25,
    STARTUP_SCAN_NODE           = 0x26,
    STARTUP_READ_STATUS_BYTE    = 0x27,
    STARTUP_READ_UPPER_LIMIT_HI = 0x28,
    STARTUP_READ_UPPER_LIMIT_LO = 0x29,
    STARTUP_READ_LOWER_LIMIT_HI = 0x2A,
    STARTUP_READ_LOWER_LIMIT_LO = 0x2B,
    STARTUP_WRITE_IDENTIFIER    = 0x2C,
    STARTUP_POSTPROCESSING_ONE  = 0x2D,
    STARTUP_POSTPROCESSING_TWO  = 0x2E,
    OPERATION                   = 0x40,
    OPERATION_BEGIN             = 0x41,
    OPERATION_NORMAL            = 0x42,
    OPERATION_RESCUE            = 0x43,
    OPERATION_ANNOUNCING        = 0x44,
    OPERATION_MOVING_UP         = 0x45,
    OPERATION_MOVING_DOWN       = 0x46,
    OPERATION_MOVING_SLOW       = 0x47,
    OPERATION_MOVING_STOP       = 0x48,
    OPERATION_CALIBRATING       = 0x49,
    OPERATION_CALIBRATING_DONE  = 0x4A,
    OPERATION_LIMIT_UP          = 0x4B,     // not implemented
    OPERATION_LIMIT_DOWN        = 0x4C      // not implemented
};

enum motor_unit {
    UNIT_LEFT,
    UNIT_RIGHT,
    UNIT_MAX
};

struct node_instance {
    uint8_t node_id;
    uint8_t property;   
    uint8_t upper_limit_lo;
    uint8_t upper_limit_hi;
    uint8_t lower_limit_lo;
    uint8_t lower_limit_hi;
};

struct motor_instance {
    uint8_t     node_id;
    uint8_t     state;
    uint8_t     scan_id;
    uint8_t     property;
    uint8_t     outage_count;
    
    uint16_t    upper_limit;
    uint16_t    lower_limit;
    uint16_t    position;
};

struct desk_instance {
    bool        calibrate;
    uint8_t     op_mode;
    uint16_t    upper_limit;
    uint16_t    lower_limit;
    uint16_t    target_position;
    uint16_t    current_position;
};


void                desk_init(bool enable);
void                desk_startup();
void                desk_operation();

bool                desk_isBusy();
uint8_t             desk_getOpMode();
uint8_t             desk_getDrift();
uint16_t            desk_getUpperLimit();
uint16_t            desk_getLowerLimit();
uint16_t            desk_getPosition();
void                desk_setPosition(uint16_t position);
void                desk_runCalibration();


uint8_t             motor_getState(uint8_t unit);
uint8_t             motor_getNodeId(uint8_t unit);
uint8_t             motor_getScanId(uint8_t unit);
uint8_t             motor_getProperty(uint8_t unit);
uint16_t            motor_getPosition(uint8_t unit);
uint16_t            motor_getUpperLimit(uint8_t unit);
uint16_t            motor_getLowerLimit(uint8_t unit);

static void         motor_controller(uint8_t *command);  
static uint16_t     motor_getLowerPosition();
static uint16_t     motor_getHigherPosition();


static void         startup_announcement();
static void         startup_preprocessing(uint8_t step);
static void         startup_postprocessing(uint8_t step);
static void         startup_scanNode(uint8_t node);
static void         startup_readMagicByte(uint8_t node);
static void         startup_readUpperLimitHi(uint8_t node);
static void         startup_readUpperLimitLo(uint8_t node);
static void         startup_readLowerLimitHi(uint8_t node);
static void         startup_readLowerLimitLo(uint8_t node);
static void         startup_writeIdentifier(uint8_t node);
static void         startup_setCycle();


#endif	/* BEKANT_H */

