 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
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
#include "mcc_generated_files/system/system.h"
#include "config.h"
#include "bekant.h"
#include "host.h"

/*
    Main application
*/


static void respond_callDeskInit();
static void respond_callDeskDeinit();
static void respond_callDeskCalibration();

static void respond_getDeskState();
static void respond_getDeskDrift();
static void respond_getDeskPosition();
static void respond_getDeskUpperLimit();
static void respond_getDeskLowerLimit();

static void respond_setDeskHalt();
static void respond_setDeskPosition(uint16_t position);

static void respond_getMotorState(uint8_t unit);
static void respond_getMotorNodeId(uint8_t unit);
static void respond_getMotorScanId(uint8_t unit);
static void respond_getMotorProperty(uint8_t unit);
static void respond_getMotorPosition(uint8_t unit);
static void respond_getMotorUpperLimit(uint8_t unit);
static void respond_getMotorLowerLimit(uint8_t unit);

static void respond_getBoardRevision();
static void respond_getFirmwareVersion();
static void respond_getProtocolVersion();

static void respond_callWatchdogEnable();
static void respond_callWatchdogDisable();

static void respond_invalidCommand(uint8_t command);
static void respond_invalidChecksum(uint8_t command);
static void respond_invalidData(uint8_t command);

void wdt_enable();
void wdt_disable();
void wdt_clear();

void cb_tmr0();


bool wdt_isEnabled;
bool event_trigger;


int main(void)
{
    uint16_t position;
    enum desk_state op_mode;
    struct host_data_packet host_request;
    
    SYSTEM_Initialize();
    TMR0_OverflowCallbackRegister(&cb_tmr0);    // timer set to 5ms
    
    wdt_isEnabled = false;
    event_trigger = false;
 
    desk_init(true);
    host_init();
    
    INTERRUPT_GlobalInterruptEnable(); 
    INTERRUPT_PeripheralInterruptEnable(); 
    
    //desk_startup();

    while(1)
    {
        if (event_trigger == true)
        {
            op_mode = desk_getOpMode();
            if ((op_mode & STARTUP) == STARTUP) {
                desk_startup();
            }
            else if ((op_mode & OPERATION) == OPERATION) {
                desk_operation();
            } else {
                // desk is idle - do nothing.
            }

            event_trigger = false;
        }
        
        if ((host_newDataAvailable() == true) && (desk_isBusy() == false))
        {
            if (wdt_isEnabled == true) {
                wdt_clear();
            }
            
            // only process host_requests after desk communication is done. 
            // this ensures un-interrupted desk communication as well as stable values of data. 
            host_read(&host_request);
            
            if (host_verifyChecksum(host_request) == true)
            {
                switch (host_request.command)
                {
                    case CALL_DESK_INIT: respond_callDeskInit(); break;
                    case CALL_DESK_DEINIT: respond_callDeskDeinit(); break;
                    case CALL_DESK_CALIBRATION: respond_callDeskCalibration(); break;
                    
                    case GET_DESK_STATE: respond_getDeskState(); break; 
                    case GET_DESK_DRIFT: respond_getDeskDrift(); break; 
                    case GET_DESK_POSITION: respond_getDeskPosition(); break;                    
                    case GET_DESK_UPPER_LIMIT: respond_getDeskUpperLimit(); break;
                    case GET_DESK_LOWER_LIMIT: respond_getDeskLowerLimit(); break;
                    
                    case SET_DESK_HALT: respond_setDeskHalt(); break;
                    case SET_DESK_POSITION: 
                        if (host_request.length == 2) {
                            position = ((host_request.data[0] << 8) | host_request.data[1]);
                            respond_setDeskPosition(position);
                        } else {
                            respond_invalidData(host_request.command);
                        }
                        break;
                    
                    case GET_MOTOR_LEFT_STATE: respond_getMotorState(UNIT_LEFT); break;
                    case GET_MOTOR_LEFT_POSITION: respond_getMotorPosition(UNIT_LEFT); break;
                    case GET_MOTOR_LEFT_UPPER_LIMIT: respond_getMotorUpperLimit(UNIT_LEFT); break;
                    case GET_MOTOR_LEFT_LOWER_LIMIT: respond_getMotorLowerLimit(UNIT_LEFT); break;
                    case GET_MOTOR_LEFT_PROPERTY: respond_getMotorProperty(UNIT_LEFT); break;
                    case GET_MOTOR_LEFT_NODE_ID: respond_getMotorNodeId(UNIT_LEFT); break;
                    case GET_MOTOR_LEFT_SCAN_ID: respond_getMotorScanId(UNIT_LEFT); break;
                        
                    case GET_MOTOR_RIGHT_STATE: respond_getMotorState(UNIT_RIGHT); break;
                    case GET_MOTOR_RIGHT_POSITION: respond_getMotorPosition(UNIT_RIGHT); break;
                    case GET_MOTOR_RIGHT_UPPER_LIMIT: respond_getMotorUpperLimit(UNIT_RIGHT); break;
                    case GET_MOTOR_RIGHT_LOWER_LIMIT: respond_getMotorLowerLimit(UNIT_RIGHT); break;
                    case GET_MOTOR_RIGHT_PROPERTY: respond_getMotorProperty(UNIT_RIGHT); break;
                    case GET_MOTOR_RIGHT_NODE_ID: respond_getMotorNodeId(UNIT_RIGHT); break;
                    case GET_MOTOR_RIGHT_SCAN_ID: respond_getMotorScanId(UNIT_RIGHT); break;
                    
                    case GET_BOARD_REVISION: respond_getBoardRevision(); break;
                    case GET_FIRMWARE_VERSION: respond_getFirmwareVersion(); break;
                    case GET_PROTOCOL_VERSION: respond_getProtocolVersion(); break;
                    
                    case CALL_WATCHDOG_ENABLE: respond_callWatchdogEnable(); break;
                    case CALL_WATCHDOG_DISABLE: respond_callWatchdogDisable(); break;

                    default: respond_invalidCommand(host_request.command); break;
                }
            }
            else 
            {
                respond_invalidChecksum(host_request.command);
            }
        }
    }    
}


void wdt_enable()
{
    volatile uint8_t t = WDTCON0;
    WDTCON0bits.WDTSEN = 0b1;
    wdt_isEnabled = true;
}

void wdt_disable()
{
    WDTCON0bits.WDTSEN = 0b0;
    wdt_isEnabled = false;
}

void wdt_clear()
{
    volatile uint8_t t = WDTCON0;   // arm the WDT before clearing it
    CLRWDT();
} 

void cb_tmr0()
{
    event_trigger = true;
}

static void respond_callDeskInit()
{
    uint8_t state;
    struct host_data_packet host_response;
    
    state = desk_getOpMode();
    host_response.command = (CALL_DESK_INIT | 0x80);
    host_response.length = 1;
    
    if (state == IDLE)
    {
        desk_startup();
        TMR0_Start();     // timer has to be enabled all time since it's used for the watchdog
        //wdt_enable();
        
        host_response.data[0] = E_OK;
    }
    else
    {
        host_response.data[0] = E_DESK_NOT_IDLE;        
    }

    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_callDeskDeinit()
{
    struct host_data_packet host_response;
    
    TMR0_Stop();      
    wdt_disable();
    desk_init(false);
    
    host_response.command = (CALL_DESK_DEINIT | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_OK;
    host_calcChecksum(&host_response);

    host_write(host_response);
}

static void respond_callDeskCalibration()
{
    uint8_t state;
    struct host_data_packet host_response;
    
    state = desk_getOpMode();
    host_response.command = (CALL_DESK_CALIBRATION | 0x80);
    host_response.length = 1;
    
    if (state == OPERATION_NORMAL) 
    {
        desk_runCalibration();
        host_response.data[0] = E_OK;
    }
    else
    {
        host_response.data[0] = E_DESK_BUSY;
    }
        
    host_calcChecksum(&host_response);    
    host_write(host_response);
}

static void respond_getBoardRevision()
{    
    struct host_data_packet host_response;
    
    host_response.command = (GET_BOARD_REVISION | 0x80);
    host_response.length = 4;
    host_response.data[0] = E_OK;
    host_response.data[1] = (uint8_t) BOARD_REVISION[0];
    host_response.data[2] = (uint8_t) BOARD_REVISION[1];
    host_response.data[3] = (uint8_t) BOARD_REVISION[2];    
    host_calcChecksum(&host_response);
        
    host_write(host_response);
}

static void respond_getFirmwareVersion()
{
    struct host_data_packet host_response;
    
    host_response.command = (GET_FIRMWARE_VERSION | 0x80);
    host_response.length = 4;
    host_response.data[0] = E_OK;
    host_response.data[1] = FIRMWARE_VERSION_MAJOR; 
    host_response.data[2] = FIRMWARE_VERSION_MINOR;
    host_response.data[3] = FIRMWARE_VERSION_PATCH;
    host_calcChecksum(&host_response);

    host_write(host_response);
}

static void respond_getProtocolVersion()
{
    struct host_data_packet host_response;
    
    host_response.command = (GET_PROTOCOL_VERSION | 0x80);
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = PROTOCOL_VERSION; 
    host_calcChecksum(&host_response);

    host_write(host_response);
}

static void respond_getDeskDrift()
{
    uint8_t drift;
    struct host_data_packet host_response;
    
    drift = desk_getDrift();
    
    host_response.command = (GET_DESK_DRIFT | 0x80);
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = drift; 
    host_calcChecksum(&host_response);

    host_write(host_response);
}

static void respond_getDeskPosition()
{
    uint8_t state;
    uint16_t position;
    struct host_data_packet host_response;
    
    state = desk_getOpMode();    
    host_response.command = (GET_DESK_POSITION | 0x80);
    
    if ((state & OPERATION) == OPERATION)
    {
        position = desk_getPosition();
    
        host_response.length = 3;
        host_response.data[0] = E_OK;
        host_response.data[1] = ((position & 0xFF00) >> 8); 
        host_response.data[2] = (position & 0x00FF);
        
    }
    else
    {
        host_response.length = 1;
        host_response.data[0] = E_DESK_NOT_READY;
    }
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_setDeskPosition(uint16_t position)
{
    uint8_t state;
    uint16_t diff;
    uint16_t current_position;
    uint16_t upper_limit, lower_limit;
    struct host_data_packet host_response;
    
    state = desk_getOpMode();
    upper_limit = desk_getUpperLimit();
    lower_limit = desk_getLowerLimit();
    current_position = desk_getPosition();
    host_response.command = (SET_DESK_POSITION | 0x80);
    host_response.length = 1;
    
    // some preliminary checks
    if (state == OPERATION_NORMAL)
    {
        if ((lower_limit <= position) && (position <= upper_limit))
        {            
            if (position > current_position) 
            { // we want to go up                
                if (upper_limit > (current_position + DESK_STOPPING_DISTANCE))
                {                    
                    diff = (position - current_position);
                
                    if (diff > DESK_STOPPING_DISTANCE) 
                    {
                        desk_setPosition(position);        
                        host_response.data[0] = E_OK;  
                    }
                    else
                    {
                        // error min distance
                        host_response.data[0] = E_DESK_MIN_DISTANCE;
                    }                 
                }
                else 
                {
                    // error upper limit reached
                    host_response.data[0] = E_DESK_UPPER_LIMIT_REACHED;
                }
            }
            
            else if (position < current_position)
            { // we want to go down 
                
                
                if (lower_limit < (current_position - DESK_STOPPING_DISTANCE))
                {                    
                    diff = (current_position - position);
                
                    if (diff > DESK_STOPPING_DISTANCE) 
                    {
                        desk_setPosition(position);        
                        host_response.data[0] = E_OK;  
                    }
                    else
                    {
                        // error min distance
                        host_response.data[0] = E_DESK_MIN_DISTANCE;
                    }                 
                }
                else 
                {
                    // error lower limit reached
                    host_response.data[0] = E_DESK_LOWER_LIMIT_REACHED;
                }
            }
        }
        else
        {
            host_response.data[0] = E_INVALID_DATA;
        }
    }
    else 
    {
        host_response.data[0] = E_DESK_BUSY;
    }
        
    host_calcChecksum(&host_response);         
    host_write(host_response);
}

static void respond_getDeskUpperLimit()
{
    uint16_t limit;
    struct host_data_packet host_response;
    
    limit = desk_getUpperLimit();
    
    host_response.command = (GET_DESK_UPPER_LIMIT | 0x80);
    host_response.length = 3;
    host_response.data[0] = E_OK;
    host_response.data[1] = ((limit & 0xFF00) >> 8); 
    host_response.data[2] = (limit & 0x00FF);
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getDeskLowerLimit()
{
    uint16_t limit;
    struct host_data_packet host_response;
    
    limit = desk_getLowerLimit();
    
    host_response.command = (GET_DESK_LOWER_LIMIT | 0x80);
    host_response.length = 3;
    host_response.data[0] = E_OK;
    host_response.data[1] = ((limit & 0xFF00) >> 8); 
    host_response.data[2] = (limit & 0x00FF);
    host_calcChecksum(&host_response);

    host_write(host_response);
}

static void respond_setDeskHalt()
{
    uint16_t position;
    struct host_data_packet host_response;
    
    position = desk_getPosition();
    desk_setPosition(position);
    
    host_response.command = (SET_DESK_HALT | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_OK;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getDeskState()
{
    uint8_t state;
    struct host_data_packet host_response;
    
    state = desk_getOpMode();
    
    host_response.command = (GET_DESK_STATE | 0x80);
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = state; 
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorProperty(uint8_t unit)
{
    uint8_t property = 0xFF;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT:
            host_response.command = (GET_MOTOR_LEFT_PROPERTY | 0x80);
            property = motor_getProperty(UNIT_LEFT);
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_PROPERTY | 0x80);
            property = motor_getProperty(UNIT_RIGHT);
            break;
        default: break;
    }
    
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = property;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorState(uint8_t unit)
{
    uint8_t state;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT: 
            host_response.command = (GET_MOTOR_LEFT_STATE | 0x80);
            state = motor_getState(UNIT_LEFT); 
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_STATE | 0x80);
            state = motor_getState(UNIT_RIGHT); 
            break;
        default: break;
    }
    
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = state;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorPosition(uint8_t unit)
{
    uint16_t position;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT: 
            host_response.command = (GET_MOTOR_LEFT_POSITION | 0x80);
            position = motor_getPosition(UNIT_LEFT); 
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_POSITION | 0x80);
            position = motor_getPosition(UNIT_RIGHT); 
            break;
        default: break;
    }
    
    host_response.length = 3;
    host_response.data[0] = E_OK;
    host_response.data[1] = ((position & 0xFF00) >> 8);
    host_response.data[2] = (position & 0xFF);
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorUpperLimit(uint8_t unit)
{
    uint16_t limit;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT: 
            host_response.command = (GET_MOTOR_LEFT_UPPER_LIMIT | 0x80);
            limit = motor_getUpperLimit(UNIT_LEFT); 
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_UPPER_LIMIT | 0x80);
            limit = motor_getUpperLimit(UNIT_RIGHT); 
            break;
        default: break;
    }
    
    host_response.length = 3;
    host_response.data[0] = E_OK;
    host_response.data[1] = ((limit & 0xFF00) >> 8);
    host_response.data[2] = (limit & 0xFF);
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorLowerLimit(uint8_t unit)
{
    uint16_t limit;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT: 
            host_response.command = (GET_MOTOR_LEFT_LOWER_LIMIT | 0x80);
            limit = motor_getLowerLimit(UNIT_LEFT); 
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_LOWER_LIMIT | 0x80);
            limit = motor_getLowerLimit(UNIT_RIGHT); 
            break;
        default: break;
    }
    
    host_response.length = 3;
    host_response.data[0] = E_OK;
    host_response.data[1] = ((limit & 0xFF00) >> 8);
    host_response.data[2] = (limit & 0xFF);
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorNodeId(uint8_t unit)
{
    uint8_t node_id;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT: 
            host_response.command = (GET_MOTOR_LEFT_NODE_ID | 0x80);
            node_id = motor_getNodeId(UNIT_LEFT); 
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_NODE_ID | 0x80);
            node_id = motor_getNodeId(UNIT_RIGHT); 
            break;
        default: break;
    }
    
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = node_id;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_getMotorScanId(uint8_t unit)
{
    uint8_t scan_id;
    struct host_data_packet host_response;
    
    switch (unit)
    {
        case UNIT_LEFT: 
            host_response.command = (GET_MOTOR_LEFT_SCAN_ID | 0x80);
            scan_id = motor_getScanId(UNIT_LEFT); 
            break;
        case UNIT_RIGHT:
            host_response.command = (GET_MOTOR_RIGHT_SCAN_ID | 0x80);
            scan_id = motor_getScanId(UNIT_RIGHT); 
            break;
        default: break;
    }
    
    host_response.length = 2;
    host_response.data[0] = E_OK;
    host_response.data[1] = scan_id;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}


static void respond_callWatchdogEnable()
{
    struct host_data_packet host_response;
    
    wdt_enable();
    
    host_response.command = (CALL_WATCHDOG_ENABLE | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_OK;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}


static void respond_callWatchdogDisable()
{
    struct host_data_packet host_response;
    
    wdt_disable();
    
    host_response.command = (CALL_WATCHDOG_DISABLE | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_OK;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}


static void respond_invalidCommand(uint8_t command)
{
    struct host_data_packet host_response;
    
    host_response.command = (command | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_INVALID_COMMAND;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_invalidChecksum(uint8_t command)
{
    struct host_data_packet host_response;
    
    host_response.command = (command | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_INVALID_CHECKSUM;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}

static void respond_invalidData(uint8_t command)
{
    struct host_data_packet host_response;
    
    host_response.command = (command | 0x80);
    host_response.length = 1;
    host_response.data[0] = E_INVALID_DATA;
    
    host_calcChecksum(&host_response);
    host_write(host_response);
}
