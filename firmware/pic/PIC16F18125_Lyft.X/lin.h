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
#ifndef LIN_H
#define	LIN_H
#include "mcc_generated_files/system/system.h"


#define BIT(byte,nbit)						((byte>>nbit) & 1)

#define BIT_SET(byte,nbit)					((byte) |=  (1<<(nbit)))
#define BIT_CLEAR(byte,nbit)				((byte) &= ~(1<<(nbit)))
#define BIT_FLIP(byte,nbit)					((byte) ^=  (1<<(nbit)))
#define BIT_CHECK(byte,nbit)				((byte) &   (1<<(nbit)))

#define BITMASK_SET(byte,mask)				((byte) |= (mask))
#define BITMASK_CLEAR(byte,mask)			((byte) &= (~(mask)))
#define BITMASK_FLIP(byte,mask)				((byte) ^= (mask))
#define BITMASK_CHECK_ALL(byte,mask)		(!(~(byte) & (mask)))
#define BITMASK_CHECK_ANY(byte,mask)		((byte) & (mask))

#define MIN(a,b)							(((a) < (b)) ? (a) : (b))
#define MAX(a,b)							(((a) > (b)) ? (a) : (b))


#define LIN_NODE_DIAG_TX			0x3C
#define LIN_NODE_DIAG_RX			0x3D
#define LIN_SYNC_FIELD              0x55
#define LIN_DIAG_PACKET_LEN         8


enum lin_bus_state {
    LIN_BUS_IDLE,
    LIN_BUS_READY,
    LIN_BUS_SENDING,
    LIN_BUS_RECEIVING
};

struct lin_packet {
    uint8_t pid;
    uint8_t data[LIN_DIAG_PACKET_LEN];
    uint8_t length;
    uint8_t checksum;
};


void lin_init(void);
void lin_deinit(void);
void lin_write(uint8_t node_id, uint8_t *data, uint8_t length);
void lin_read(uint8_t node_id);
uint8_t lin_getRxData(uint8_t *buffer);

static uint8_t lin_parity(uint8_t node_id);
static uint8_t lin_checksum_classic(const uint8_t *data, uint8_t len);
static uint8_t lin_checksum_enhanced(uint8_t pid, const uint8_t *data, uint8_t len);
static bool    lin_checksum_validation();

static void cb_lin_rx();
static void cb_lin_tx();


#endif	/* LIN_H */

