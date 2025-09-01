/*
 * File:   bekant.c
 * Author: sire
 *
 * Created on July 20, 2025, 11:27 AM
 */
#include "bekant.h"
#include "lin.h"


bool    startup_readback;
uint8_t startup_nodeCounter;
uint8_t startup_nodeIdentifier;
uint8_t startup_nodeFound;
uint8_t startup_retryCounter;

uint8_t timekeeper;
uint8_t rescueCounter;
uint8_t rescueCommand;
uint8_t proximityCounter;

bool    desk_isTalking;

struct desk_instance desk;
struct node_instance node[UNIT_MAX];
struct motor_instance motor[UNIT_MAX];


void desk_init(bool enable)
{
    uint8_t i;
    
    if (enable == true) {
        lin_init();
    } else {
        lin_deinit();
    }
            
    startup_readback = false;
    startup_nodeFound = 0xFF;
    startup_nodeCounter = 0x00;
    startup_nodeIdentifier = 0x07;   
    startup_retryCounter = 0;
    
    desk.op_mode = IDLE;
    desk.calibrate = false;
    timekeeper = 0;
    rescueCounter = 0;
    proximityCounter = 0;
    desk_isTalking = false;    
    
    for (i=0; i<UNIT_MAX; i++)
    {
        node[i].node_id = 0x00;        
        node[i].property = 0x00;
        node[i].lower_limit_lo = 0x00;
        node[i].lower_limit_hi = 0x00;
        node[i].upper_limit_lo = 0x00;
        node[i].upper_limit_hi = 0x00;
        
        motor[i].state = 0;
        motor[i].node_id = 0;
        motor[i].scan_id = 0;
        motor[i].position = 0;
        motor[i].property = 0;
        motor[i].outage_count = 0;
        motor[i].lower_limit = 0;
        motor[i].upper_limit = 0;        
    }
}

void desk_startup()
{
    if (desk.op_mode == IDLE) {
        desk.op_mode = STARTUP_BEGIN;
        desk_isTalking = true;
        timekeeper = 0;
    }
    
    if ((desk.op_mode & STARTUP) == STARTUP)
    {    
        timekeeper++;

        if (startup_readback == true)
        {
            if (timekeeper > 1)
            {
                // trigger read command
                lin_read(LIN_NODE_DIAG_RX);
                startup_readback = false;
                timekeeper = 0;
            }         
        }
        else 
        {
            if (timekeeper > 2)
            {
                startup_setCycle();

                switch (desk.op_mode)
                {
                    case STARTUP_ANNOUNCEMENT_ONE:
                    case STARTUP_ANNOUNCEMENT_TWO:
                        startup_announcement(); 
                        break;

                    case STARTUP_PREPROCESSING_ONE:
                        startup_preprocessing(1);
                        break;

                    case STARTUP_PREPROCESSING_TWO:
                        startup_preprocessing(2);
                        break;

                    case STARTUP_SCAN_NODE:
                        startup_scanNode(startup_nodeCounter);
                        break;

                    case STARTUP_READ_STATUS_BYTE:
                        startup_readMagicByte(startup_nodeCounter);
                        break;

                    case STARTUP_READ_UPPER_LIMIT_HI:
                        startup_readUpperLimitHi(startup_nodeCounter);
                        break;

                    case STARTUP_READ_UPPER_LIMIT_LO:
                        startup_readUpperLimitLo(startup_nodeCounter);
                        break;

                    case STARTUP_READ_LOWER_LIMIT_HI:
                        startup_readLowerLimitHi(startup_nodeCounter);
                        break;

                    case STARTUP_READ_LOWER_LIMIT_LO:
                        startup_readLowerLimitLo(startup_nodeCounter);
                        break;

                    case STARTUP_WRITE_IDENTIFIER:
                        startup_writeIdentifier(startup_nodeCounter);
                        break;

                    case STARTUP_POSTPROCESSING_ONE: 
                        startup_postprocessing(1);
                        break;

                    case STARTUP_POSTPROCESSING_TWO: 
                        startup_postprocessing(2);
                        break; 

                    default: break;
                }

                startup_readback = true; 
                timekeeper = 0;
            }
        }
    }
}

void desk_operation()
{
    const uint8_t heartbeat[DESK_PACKET_LEN] = {0x00, 0x00, 0x00};
    uint8_t state, rx_len, rx_data[DESK_PACKET_LEN];
    uint16_t position;
    uint8_t motor_command[DESK_PACKET_LEN];
    
    if ((desk.op_mode & OPERATION) == OPERATION)
    {
        if (timekeeper == 0)
        {
            desk_isTalking = true; 
            lin_write(DESK_ADDR_SEVENTEEN, heartbeat, DESK_PACKET_LEN);
        }
        else if (timekeeper == 1)
        {
            lin_read(DESK_ADDR_MOTOR_LEFT);
        }
        else if (timekeeper == 2)
        {
            // read back data from left motor
            rx_len = lin_getRxData(rx_data);
    
            if (rx_len == DESK_PACKET_LEN)
            {
                position = ((rx_data[1] << 8) | rx_data[0]);
                state = rx_data[2];

                motor[UNIT_LEFT].outage_count = 0;
                motor[UNIT_LEFT].position = position;
                motor[UNIT_LEFT].state = state;        
            } else {
                // if data is not valid/complete, keep previous data and 
                // increase an error counter, which is reset after next valid reading. 
                motor[UNIT_LEFT].outage_count++;
            }

            lin_read(DESK_ADDR_MOTOR_RIGHT);
        }
        else if (timekeeper == 3)
        {
            // read back data from right motor          
            rx_len = lin_getRxData(rx_data);
    
            if (rx_len == DESK_PACKET_LEN)
            {
                position = ((rx_data[1] << 8) | rx_data[0]);
                state = rx_data[2];

                motor[UNIT_RIGHT].outage_count = 0;
                motor[UNIT_RIGHT].position = position;
                motor[UNIT_RIGHT].state = state;        
            } else {
                // if data is not valid/complete, keep previous data and 
                // increase an error counter, which is reset after next valid reading. 
                motor[UNIT_RIGHT].outage_count++;
            }
            
            lin_read(DESK_ADDR_SIXTEEN);
        }
        else if (timekeeper == 4)
        {
            // update the desk position occasionally 
            lin_getRxData(NULL); 
            desk.current_position = motor[UNIT_LEFT].position;
            lin_read(DESK_ADDR_SIXTEEN);
        }
        else if (timekeeper == 5)
        {
            // we can check the motor states for failures: 
            switch (desk.op_mode)
            {
                case OPERATION_MOVING_UP:
                    if ((motor[UNIT_LEFT].state == MOTOR_STATE_BLOCKED) || (motor[UNIT_RIGHT].state == MOTOR_STATE_BLOCKED)) {
                       rescueCounter = 0;
                       rescueCommand = MOTOR_CMD_MOVE_DOWN;
                       desk.op_mode = OPERATION_RESCUE; // this is a severe situation
                   } break;
                case OPERATION_MOVING_DOWN:
                   if ((motor[UNIT_LEFT].state == MOTOR_STATE_BLOCKED) || (motor[UNIT_RIGHT].state == MOTOR_STATE_BLOCKED)) {
                       rescueCounter = 0;
                       rescueCommand = MOTOR_CMD_MOVE_UP;
                       desk.op_mode = OPERATION_RESCUE; // this is a severe situation
                   } break;
                case OPERATION_MOVING_SLOW:
                    if ((motor[UNIT_LEFT].state != MOTOR_STATE_MOVING_SLOW) || (motor[UNIT_RIGHT].state != MOTOR_STATE_MOVING_SLOW)) {
                        // no idea what to do in this case
                    } break;
                case OPERATION_CALIBRATING:
                    if ((motor[UNIT_LEFT].state != MOTOR_STATE_CALIBRATING) || (motor[UNIT_RIGHT].state != MOTOR_STATE_CALIBRATING)) {
                        // no idea what to do in this case
                    } break;
                default: break;
            }

            // do some empty readings (for nodes which do not exist)
            lin_getRxData(NULL); 
            lin_read(DESK_ADDR_SIXTEEN);
        }
        else if ((timekeeper >= 6) && (timekeeper <= 8))
        {
            // do some empty readings (for nodes which do not exist)
            lin_getRxData(NULL); 
            lin_read(DESK_ADDR_SIXTEEN);
        }
        else if (timekeeper == 9)
        {
            lin_getRxData(NULL); 
            motor_controller(motor_command);
            lin_read(DESK_ADDR_ONE);             
        }
        else if (timekeeper == 10)
        {
            // now, let the motors know about your perception
            lin_getRxData(NULL); 
            lin_write(DESK_ADDR_MASTER, motor_command, DESK_PACKET_LEN);            
        }
        else if (timekeeper == 11)
        {
            // transmission is done until next cycle. time for other things to handle.
            desk_isTalking = false; 
        }

        if (timekeeper > 18) {
            timekeeper = 0;
        } else {
            timekeeper++;
        }
    }
}


/*******************
 * 
 * D E S K   R O U T I N E S
 * 
 ******************/
bool desk_isBusy()
{
    return desk_isTalking;
}

uint8_t desk_getOpMode()
{
    return desk.op_mode;
}

uint8_t desk_getDrift()
{
    uint8_t drift;
    
    if (motor[UNIT_LEFT].position > motor[UNIT_RIGHT].position) {
        drift = (motor[UNIT_LEFT].position - motor[UNIT_RIGHT].position);
    } else if (motor[UNIT_LEFT].position < motor[UNIT_RIGHT].position) {
        drift = (motor[UNIT_RIGHT].position - motor[UNIT_LEFT].position);
    } else {
        drift = 0;
    }
    
    return drift;
}

uint16_t desk_getUpperLimit()
{
    return desk.upper_limit;
}

uint16_t desk_getLowerLimit()
{
    return desk.lower_limit;
}

uint16_t desk_getPosition()
{
    return desk.current_position;
}

void desk_setPosition(uint16_t position)
{
    uint16_t diff;
    
    if ((position >= desk.lower_limit) && (position <= desk.upper_limit)) {
        
        if (desk.op_mode == OPERATION_NORMAL)
        {
            if (desk.current_position > position) {
                diff = (desk.current_position - position);
            } else {
                diff = (position - desk.current_position);
            }

            if (diff > DESK_STOPPING_DISTANCE) {
                desk.target_position = position;
            }
        }
        else if (desk.op_mode == OPERATION_MOVING_UP) {
            desk.target_position = (desk.current_position + DESK_STOPPING_DISTANCE);
        }
        else if (desk.op_mode == OPERATION_MOVING_DOWN) {
            desk.target_position = (desk.current_position - DESK_STOPPING_DISTANCE);
        }
    }
}

void desk_runCalibration()
{
    desk.calibrate = true;
}


/*******************
 * 
 * M O T O R   R O U T I N E S
 * 
 ******************/
uint8_t motor_getState(uint8_t unit)
{
    uint8_t state = 0xFF;
    
    if (unit < UNIT_MAX) {
        state = motor[unit].state;
    }
    
    return state;
}

uint8_t motor_getNodeId(uint8_t unit)
{
    uint8_t node_id;
    
    /*
    if (unit < UNIT_MAX) {
        node_id = motor[unit].node_id;
    } */
    
    if (unit == UNIT_LEFT) {
        node_id = DESK_ADDR_MOTOR_LEFT;
    } else if (unit == UNIT_RIGHT) {
        node_id = DESK_ADDR_MOTOR_RIGHT;
    } else {
        node_id = 0xFF;
    }
    
    return node_id;
}

uint8_t motor_getScanId(uint8_t unit)
{
    uint8_t scan_id = 0xFF;
    
    if (unit < UNIT_MAX) {
        scan_id = motor[unit].scan_id;
    }
    
    return scan_id;
}

uint8_t motor_getProperty(uint8_t unit)
{
    uint8_t property = 0xFF; 
    
    if (unit < UNIT_MAX) {
        property = motor[unit].property;
    }
    
    return property;
}

uint16_t motor_getPosition(uint8_t unit)
{
    uint16_t position = 0xFFFF;
    
    if (unit < UNIT_MAX) {
        position = motor[unit].position;
    }
    
    return position;
}

uint16_t motor_getUpperLimit(uint8_t unit)
{
    uint16_t limit = 0xFFFF;
    
    if (unit < UNIT_MAX) {
        limit = motor[unit].upper_limit;
    }
    
    return limit;
}

uint16_t motor_getLowerLimit(uint8_t unit)
{
    uint16_t limit = 0xFFFF;
    
    if (unit < UNIT_MAX) {
        limit = motor[unit].lower_limit;
    }
    
    return limit;
}


static void motor_controller(uint8_t *command)
{
    uint8_t cmd_position_hi;
    uint8_t cmd_position_lo;
    uint8_t cmd_instruction;
    uint16_t position, distance;
    
    if (command != NULL)
    {
    
        if (desk.op_mode == OPERATION_BEGIN)
        {
            if ((motor[UNIT_LEFT].state == MOTOR_STATE_LIMIT_A) || (motor[UNIT_RIGHT].state == MOTOR_STATE_LIMIT_A))
            {
                cmd_position_hi = 0xFF;
                cmd_position_lo = 0xF6;
                cmd_instruction = 0xBF;
            } 
            else if ((motor[UNIT_LEFT].state == MOTOR_STATE_LIMIT_B) || (motor[UNIT_RIGHT].state == MOTOR_STATE_LIMIT_B))
            {
                cmd_position_hi = 0xFF;
                cmd_position_lo = 0xF6;
                cmd_instruction = 0xFF;
            }
            else 
            {
                cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_IDLE;
                desk.op_mode = OPERATION_NORMAL;
            }
            
            desk.current_position = motor[UNIT_LEFT].position;
            desk.target_position = motor[UNIT_LEFT].position;
        }
        
        else if (desk.op_mode == OPERATION_ANNOUNCING)
        {
            cmd_position_hi = (uint8_t) ((desk.current_position & 0xFF00) >> 8);
            cmd_position_lo = (uint8_t) (desk.current_position & 0xFF);            
            cmd_instruction = MOTOR_CMD_ANNOUNCEMENT;
            
            if (desk.calibrate == true) {
                desk.op_mode = OPERATION_CALIBRATING;
            } else if (desk.target_position > desk.current_position) {
                desk.op_mode = OPERATION_MOVING_UP;
            } else if (desk.target_position < desk.current_position) {
                desk.op_mode = OPERATION_MOVING_DOWN;
            }  else {
                desk.op_mode = OPERATION_NORMAL;
            }
        }
        
        else if (desk.op_mode == OPERATION_MOVING_UP)
        {
            position = motor_getLowerPosition();
            distance = desk.target_position - desk.current_position;
            
            cmd_position_hi = ((position & 0xFF00) >> 8);
            cmd_position_lo = (position & 0xFF);
            cmd_instruction = MOTOR_CMD_MOVE_UP;
            
            if (distance <= DESK_STOPPING_DISTANCE) { 
                proximityCounter = 0;
                desk.op_mode = OPERATION_MOVING_SLOW;
            }
        }
        
        else if (desk.op_mode == OPERATION_MOVING_DOWN)
        {
            position = motor_getHigherPosition();
            distance = desk.current_position - desk.target_position;
            
            cmd_position_hi = ((position & 0xFF00) >> 8);
            cmd_position_lo = (position & 0xFF);
            cmd_instruction = MOTOR_CMD_MOVE_DOWN;
            
            if (distance <= DESK_STOPPING_DISTANCE) { 
                proximityCounter = 0;
                desk.op_mode = OPERATION_MOVING_SLOW;
            }
        }
        
        else if (desk.op_mode == OPERATION_MOVING_SLOW)
        {
            position = desk.target_position;
            cmd_position_hi = (uint8_t) ((position & 0xFF00) >> 8);
            cmd_position_lo = (uint8_t) (position & 0xFF);
            cmd_instruction = MOTOR_CMD_MOVE_SLOW;
            
            proximityCounter++;
            
            if (proximityCounter > 2) {
                desk.op_mode = OPERATION_MOVING_STOP;
            }
        }
        
        else if (desk.op_mode == OPERATION_MOVING_STOP)
        {
            cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
            cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
            cmd_instruction = MOTOR_CMD_MOVE_STOP;
            
            desk.target_position = desk.current_position;            
            desk.op_mode = OPERATION_NORMAL;
        }
        
        else if (desk.op_mode == OPERATION_CALIBRATING)
        {
            cmd_position_hi = 0;
            cmd_position_lo = 0;
            cmd_instruction = MOTOR_CMD_CALIBRATION;
            
            if ((motor[UNIT_LEFT].state == MOTOR_STATE_CALIBRATED) && (motor[UNIT_RIGHT].state == MOTOR_STATE_CALIBRATED)) {
                desk.op_mode = OPERATION_CALIBRATING_DONE;
            }
        }
        
        else if (desk.op_mode == OPERATION_CALIBRATING_DONE)
        {
            position = motor_getHigherPosition();
            
            cmd_position_hi = ((position & 0xFF00) >> 8);
            cmd_position_lo = (position & 0xFF);
            cmd_instruction = MOTOR_CMD_CALIBRATION_STOP;
            
            desk.calibrate = false;
            desk.op_mode = OPERATION_NORMAL;
        }
        
        else if (desk.op_mode == OPERATION_NORMAL)
        {
            if (desk.calibrate == true) {
                desk.op_mode = OPERATION_ANNOUNCING;
            }
            // double check if the desk should be moved 
            else if (desk.target_position != desk.current_position)
            {
                if (desk.target_position > desk.current_position) {
                    distance = (desk.target_position - desk.current_position);
                } else {
                    distance = (desk.current_position - desk.target_position);
                }
                
                if (distance > DESK_STOPPING_DISTANCE) {
                    desk.op_mode = OPERATION_ANNOUNCING;
                } else {
                    desk.target_position = desk.current_position;
                }
            }
                       
            cmd_position_hi = (uint8_t) ((desk.current_position & 0xFF00) >> 8);
            cmd_position_lo = (uint8_t) (desk.current_position & 0xFF);
            
            cmd_instruction = MOTOR_CMD_IDLE;
        }
        
        else if (desk.op_mode == OPERATION_RESCUE) 
        {
            if (rescueCounter < 3)
            {
                cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_MOVE_STOP;
            }
            else if ((3 <= rescueCounter) && (rescueCounter < 11))
            {
                switch (rescueCommand)
                {
                    case MOTOR_CMD_MOVE_UP: position = motor_getLowerPosition(); break;                        
                    case MOTOR_CMD_MOVE_DOWN: position = motor_getHigherPosition(); break;                        
                    default: position = motor[UNIT_LEFT].position; break;
                }
                
                cmd_position_hi = ((position & 0xFF00) >> 8);
                cmd_position_lo = (position & 0xFF);
                cmd_instruction = rescueCommand;
            }
            else if ((11 <= rescueCounter) && (rescueCounter < 15))
            {
                cmd_position_hi = ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_MOVE_SLOW;
            }
            else if ((15 <= rescueCounter) && (rescueCounter < 18))
            {
                cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_MOVE_STOP;
            }
            else if (rescueCounter == 18)
            {
                cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_ANNOUNCEMENT;
            }
            else if ((19 <= rescueCounter) && (rescueCounter < 22))
            {
                cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_IDLE;
            }
            else if (rescueCounter == 22)
            {
                cmd_position_hi = (uint8_t) ((motor[UNIT_LEFT].position & 0xFF00) >> 8);
                cmd_position_lo = (uint8_t) (motor[UNIT_LEFT].position & 0xFF);
                cmd_instruction = MOTOR_CMD_IDLE;
                
                desk.target_position = desk.current_position;
                desk.op_mode = OPERATION_NORMAL;
            }
            
            rescueCounter++;
        }

        command[0] = cmd_position_lo;
        command[1] = cmd_position_hi;
        command[2] = cmd_instruction;        
    }
}

static uint16_t motor_getLowerPosition()
{
    uint16_t position;
    
    if ((motor[UNIT_LEFT].outage_count == 0) && (motor[UNIT_RIGHT].outage_count == 0))
    {
        if (motor[UNIT_LEFT].position < motor[UNIT_RIGHT].position) {
            position = motor[UNIT_LEFT].position;
        } else {
            position = motor[UNIT_RIGHT].position;
        }
    }
    else if (motor[UNIT_LEFT].outage_count == 0) {
        position = motor[UNIT_LEFT].position;
    } 
    else if (motor[UNIT_RIGHT].outage_count == 0) {
        position = motor[UNIT_RIGHT].position;
    } 
    else 
    {
        // we got a problem. both motors did not send valid data
    } 
    
    return position;
}

static uint16_t motor_getHigherPosition()
{
    uint16_t position;
    
    if ((motor[UNIT_LEFT].outage_count == 0) && (motor[UNIT_RIGHT].outage_count == 0))
    {
        if (motor[UNIT_LEFT].position > motor[UNIT_RIGHT].position) {
            position = motor[UNIT_LEFT].position;
        } else {
            position = motor[UNIT_RIGHT].position;
        }
    }
    else if (motor[UNIT_LEFT].outage_count == 0) {
        position = motor[UNIT_LEFT].position;
    } 
    else if (motor[UNIT_RIGHT].outage_count == 0) {
        position = motor[UNIT_RIGHT].position;
    } 
    else 
    {
        // we got a problem. both motors did not send valid data
    } 
    
    return position;
}


/*******************
 * 
 * S T A R T U P   R O U T I N E S
 * 
 ******************/
static void startup_announcement()
{
    const uint8_t announcement[LIN_DIAG_PACKET_LEN] = {0xFF, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, announcement, 8);
}

static void startup_preprocessing(uint8_t step)
{
    const uint8_t message01[LIN_DIAG_PACKET_LEN] = {0xFF, 0x01, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t message02[LIN_DIAG_PACKET_LEN] = {0xD0, 0x02, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    switch (step)
    {
        case 1: lin_write(LIN_NODE_DIAG_TX, message01, 8); break;
        case 2: lin_write(LIN_NODE_DIAG_TX, message02, 8); break;
        default: break;
    }
}

static void startup_postprocessing(uint8_t step)
{
    uint8_t message01[LIN_DIAG_PACKET_LEN] = {0xD0, 0x01, 0x07, startup_nodeFound, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t message02[LIN_DIAG_PACKET_LEN] = {0xD0, 0x02, 0x07, startup_nodeFound, 0xFF, 0xFF, 0xFF, 0xFF};
    
    switch (step)
    {
        case 1: lin_write(LIN_NODE_DIAG_TX, message01, 8); break;
        case 2: lin_write(LIN_NODE_DIAG_TX, message02, 8); break;
        default: break;
    }
}

static void startup_scanNode(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x02, startup_nodeIdentifier, startup_nodeFound, 0xFF, 0xFF, 0xFF, 0xFF};
        
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);    
}

static void startup_readMagicByte(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x06, 0x09, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);  
}

static void startup_readUpperLimitHi(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x06, 0x0C, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);  
}

static void startup_readUpperLimitLo(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x06, 0x0D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);  
}

static void startup_readLowerLimitHi(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x06, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);  
}

static void startup_readLowerLimitLo(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x06, 0x0B, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);  
}

static void startup_writeIdentifier(uint8_t node)
{
    uint8_t message[LIN_DIAG_PACKET_LEN] = {node, 0x04, startup_nodeIdentifier, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    lin_write(LIN_NODE_DIAG_TX, message, LIN_DIAG_PACKET_LEN);  
}

static void startup_setCycle()
{
    uint8_t i, rx_len, rx_data[10];
    
    if (desk.op_mode > STARTUP_BEGIN) {
        rx_len = lin_getRxData(rx_data);
    }
    
    if (desk.op_mode == STARTUP_BEGIN) {
        desk.op_mode = STARTUP_ANNOUNCEMENT_ONE;
    }
    else if (desk.op_mode == STARTUP_ANNOUNCEMENT_ONE) {
        desk.op_mode = STARTUP_ANNOUNCEMENT_TWO;
    }
    else if (desk.op_mode == STARTUP_ANNOUNCEMENT_TWO) {
        desk.op_mode = STARTUP_PREPROCESSING_ONE;
    }
    else if (desk.op_mode == STARTUP_PREPROCESSING_ONE) {
        desk.op_mode = STARTUP_PREPROCESSING_TWO;
    }
    else if (desk.op_mode == STARTUP_PREPROCESSING_TWO) {
        // we should receive a single byte here, but we are ignoring it. 
        // prepare for scanning all nodes:
        startup_nodeCounter = 0;
        startup_nodeIdentifier = 0x07;
        startup_nodeFound = 0xFF;
        desk.op_mode = STARTUP_SCAN_NODE;
    }
    else if (desk.op_mode == STARTUP_SCAN_NODE) {
        if (rx_len > 0) {
            // a node was found. clear the nodeFound byte and read all parameters from the node
            startup_nodeFound = 0x00;
            
            if (startup_nodeIdentifier == 0x07) {
                startup_nodeIdentifier = 0x00;
            } else {
                startup_nodeIdentifier++;
            }
            
            desk.op_mode = STARTUP_READ_STATUS_BYTE;
        }
        else 
        {
            // no node responded. so try next ID and stay within same cycle.
            if (startup_nodeCounter < 7) {
                startup_nodeCounter++;
            } else {
                desk.op_mode = STARTUP_POSTPROCESSING_ONE;
            }
        }
    }
    else if (desk.op_mode == STARTUP_READ_STATUS_BYTE) {
        if (rx_len == LIN_DIAG_PACKET_LEN) {
            if (rx_data[0] == STARTUP_REG_09) {
                node[startup_nodeIdentifier].property = rx_data[1];
                node[startup_nodeIdentifier].node_id = startup_nodeCounter;
                startup_retryCounter = 0;
                desk.op_mode = STARTUP_READ_UPPER_LIMIT_HI;
            }
        } else {
            startup_retryCounter++; 
            
            if (startup_retryCounter > STARTUP_READ_RETRY) {
                desk.op_mode = STARTUP_BEGIN;
            }
        }
    }
    else if (desk.op_mode == STARTUP_READ_UPPER_LIMIT_HI) {
        if (rx_len == LIN_DIAG_PACKET_LEN) {
            if (rx_data[0] == STARTUP_REG_UPPER_LIMIT_HI) {
                node[startup_nodeIdentifier].upper_limit_hi = rx_data[1];
                startup_retryCounter = 0;
                desk.op_mode = STARTUP_READ_UPPER_LIMIT_LO;
            }
        } else {
            startup_retryCounter++; 
            
            if (startup_retryCounter > STARTUP_READ_RETRY) {
                desk.op_mode = STARTUP_BEGIN;
            }
        }
    }
    else if (desk.op_mode == STARTUP_READ_UPPER_LIMIT_LO) {
        if (rx_len == LIN_DIAG_PACKET_LEN) {
            if (rx_data[0] == STARTUP_REG_UPPER_LIMIT_LO) {
                node[startup_nodeIdentifier].upper_limit_lo = rx_data[1];
                startup_retryCounter = 0;
                desk.op_mode = STARTUP_READ_LOWER_LIMIT_HI;
            }
        } else {
            startup_retryCounter++; 
            
            if (startup_retryCounter > STARTUP_READ_RETRY) {
                desk.op_mode = STARTUP_BEGIN;
            }
        }
    }
    else if (desk.op_mode == STARTUP_READ_LOWER_LIMIT_HI) {
        if (rx_len == LIN_DIAG_PACKET_LEN) {
            if (rx_data[0] == STARTUP_REG_LOWER_LIMIT_HI) {
                node[startup_nodeIdentifier].lower_limit_hi = rx_data[1];
                startup_retryCounter = 0;
                desk.op_mode = STARTUP_READ_LOWER_LIMIT_LO;
            }
        } else {
            startup_retryCounter++; 
            
            if (startup_retryCounter > STARTUP_READ_RETRY) {
                desk.op_mode = STARTUP_BEGIN;
            }
        }
    }
    else if (desk.op_mode == STARTUP_READ_LOWER_LIMIT_LO) {
        if (rx_len == LIN_DIAG_PACKET_LEN) {
            if (rx_data[0] == STARTUP_REG_LOWER_LIMIT_LO) {
                node[startup_nodeIdentifier].lower_limit_lo = rx_data[1];
                startup_retryCounter = 0;
                desk.op_mode = STARTUP_WRITE_IDENTIFIER;
            }
        } else {
            startup_retryCounter++; 
            
            if (startup_retryCounter > STARTUP_READ_RETRY) {
                desk.op_mode = STARTUP_BEGIN;
            }
        }
    }
    else if (desk.op_mode == STARTUP_WRITE_IDENTIFIER) {
        if (rx_len == LIN_DIAG_PACKET_LEN) {
            if (rx_data[0] == 0xFF) {
                startup_nodeCounter++;
                startup_retryCounter = 0;
                desk.op_mode = STARTUP_SCAN_NODE;
            }
        } else {
            startup_retryCounter++; 
            
            if (startup_retryCounter > STARTUP_READ_RETRY) {
                desk.op_mode = STARTUP_BEGIN;
            }
        }
    }        
    else if (desk.op_mode == STARTUP_POSTPROCESSING_ONE) {
        desk.op_mode = STARTUP_POSTPROCESSING_TWO;
    }
    else if (desk.op_mode == STARTUP_POSTPROCESSING_TWO) {
        if (startup_nodeFound == 0xFF) {
            // if no node was found: start scanning process again
            startup_nodeCounter = 0;
            startup_nodeIdentifier = 0x07;
            
            desk.op_mode = STARTUP_SCAN_NODE;
        } else {
            // we got all information, now create the motor instances and finish the startup process
            for (i=0; i<UNIT_MAX; i++)
            {
                motor[i].scan_id = node[i].node_id;
                motor[i].property = node[i].property;
                motor[i].outage_count = 0;

                motor[i].lower_limit = (node[i].lower_limit_hi << 8) | (node[i].lower_limit_lo);
                motor[i].upper_limit = (node[i].upper_limit_hi << 8) | (node[i].upper_limit_lo);
            }
            
            if (motor[UNIT_LEFT].lower_limit < motor[UNIT_RIGHT].lower_limit) {
                desk.lower_limit = motor[UNIT_RIGHT].lower_limit;
            } else {
                desk.lower_limit = motor[UNIT_LEFT].lower_limit;
            }
            
            if (motor[UNIT_LEFT].upper_limit > motor[UNIT_RIGHT].upper_limit) {
                desk.upper_limit = motor[UNIT_RIGHT].upper_limit;
            } else {
                desk.upper_limit = motor[UNIT_LEFT].upper_limit;
            } 
            
            desk.op_mode = OPERATION_BEGIN;
        }
    }
    else {
        // something completely unexpected happened
    }
}

