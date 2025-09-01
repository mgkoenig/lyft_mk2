/*
 * File:   lin.c
 * Author: sire
 *
 * Created on July 20, 2025, 11:27 AM
 */
#include "lin.h"

uint8_t rx_byteCount, tx_byteCount;
uint8_t rx_node_pid;
uint8_t rx_buffer[12];

struct lin_packet tx_packet;
enum lin_bus_state lin_state;


void lin_init(void)
{
    UART1.RxCompleteCallbackRegister(&cb_lin_rx);
    UART1.TxCompleteCallbackRegister(&cb_lin_tx);
    EUSART1_ReceiveDisable();
    
    rx_byteCount = 0;
    tx_byteCount = 0;
        
    lin_state = LIN_BUS_READY;
}

void lin_deinit(void)
{
    //UART1.RxCompleteCallbackRegister(NULL);
    //UART1.TxCompleteCallbackRegister(NULL);
    //UART1.Deinitialize();
    
    lin_state = LIN_BUS_IDLE;
}

void lin_write(uint8_t node_id, uint8_t *data, uint8_t length)
{
    uint8_t i, pid, checksum;
	
    if (lin_state == LIN_BUS_READY)
    {
        if ((data != NULL) && (length > 0))
        {
            pid = (lin_parity(node_id) | node_id);

            if (node_id == LIN_NODE_DIAG_TX) {
                checksum = lin_checksum_classic(data, length);
            } else {
                checksum = lin_checksum_enhanced(pid, data, length);
            }

            tx_packet.pid = pid;
            tx_packet.length = length;

            for (i=0; i<length; i++) {
                tx_packet.data[i] = data[i];
            }

            tx_packet.checksum = checksum;

            tx_byteCount = 0;
            lin_state = LIN_BUS_SENDING;

            EUSART1_SendBreakControlEnable();
            UART1.Write(0x00);
        }
    }
}

void lin_read(uint8_t node_id)
{    
    if (lin_state == LIN_BUS_READY)
    {                   
        lin_state = LIN_BUS_RECEIVING;
        tx_byteCount = 0;
        rx_byteCount = 0;
        rx_node_pid = (lin_parity(node_id) | node_id);            

        EUSART1_SendBreakControlEnable();
        UART1.Write(0x00);        
    }
}

uint8_t lin_getRxData(uint8_t *buffer)
{
    // at this point, receiving data should be finished. therefore, we can 
    // validate the checksum and if the packet was received correctly. 
    uint8_t i, length = 0;
    bool cc_correct;
    
    if (rx_byteCount > 1)
    {
        cc_correct = lin_checksum_validation();
        
        if (cc_correct == true)
        {
            length = (rx_byteCount - 1);
            
            if (buffer != NULL)
            {
                // copy rx_data to buffer. ignore last byte which is checksum and was validated.
                for (i=0; i<length; i++) {
                    buffer[i] = rx_buffer[i];
                }
            }
        }
    }
    
    EUSART1_ReceiveDisable();
    rx_byteCount = 0;
    lin_state = LIN_BUS_READY;
    
    return length;
} 


/**********************************************************
 * HELPER FUNCTIONS
 *********************************************************/
static uint8_t lin_parity(uint8_t node_id)
{
	uint8_t par, p0, p1;
	
	p0 = BIT(node_id,0) ^ BIT(node_id,1) ^ BIT(node_id,2) ^ BIT(node_id,4);
	p1 = ~(BIT(node_id,1) ^ BIT(node_id,3) ^ BIT(node_id,4) ^ BIT(node_id,5));
	par = ((p0 | (p1<<1)) << 6);

	return par;
}

static uint8_t lin_checksum_classic(const uint8_t *data, uint8_t len)
{
	uint8_t     i;
	uint16_t    cc = 0;

	for (i=0; i<len; i++) {
		cc += data[i];

		if (cc > 255) { 
			cc -= 255;
		}
	}

	cc = ((~cc) & 0xFF);

	return (uint8_t) cc;
}

static uint8_t lin_checksum_enhanced(uint8_t pid, const uint8_t *data, uint8_t len)
{
	uint8_t     i;
	uint16_t    cc = pid;

	for (i=0; i<len; i++) {
		cc += data[i];

		if (cc > 255) {
			cc -= 255;
		}
	}

	cc = ((~cc) & 0xFF);

	return (uint8_t) cc;
}

static bool lin_checksum_validation()
{
    bool isValid = false;
    uint8_t data[8], length;
    uint8_t i, calc_checksum, rx_checksum;    
    
    rx_checksum = rx_buffer[rx_byteCount-1];
    length = (rx_byteCount - 1);
    
    for (i=0; i<length; i++) {
        data[i] = rx_buffer[i];
    }

    if (rx_node_pid == 0x7D) {  // there is only one DIAG_RX_NODE_PID
        calc_checksum = lin_checksum_classic(data, length);
    } else {
        calc_checksum = lin_checksum_enhanced(rx_node_pid, data, length);
    }
    
    if (calc_checksum == rx_checksum) {
        isValid = true;
    }    
    
    return isValid;
}


/**********************************************************
 * CALLBACK FUNCTIONS
 *********************************************************/

static void cb_lin_rx()
{
    uint8_t rx_byte;
    
    rx_byte = UART1.Read();
    
    if (lin_state == LIN_BUS_RECEIVING)
    {
        if ((rx_byteCount == 0) && (rx_byte == rx_node_pid))
        {
            // ignore this first echo byte
        } else {
            rx_buffer[rx_byteCount] = rx_byte;        
            rx_byteCount++;
        }
    }
} 

static void cb_lin_tx()
{
    uint8_t index; 
    
    if (lin_state == LIN_BUS_SENDING)
    {   // send a whole data packet
        if (tx_byteCount == 0) {
            UART1.Write(LIN_SYNC_FIELD);
        }
        else if (tx_byteCount == 1) {
            UART1.Write(tx_packet.pid);
        }
        else if (tx_byteCount >= 2) {        
            index = tx_byteCount - 2;

            if (index < tx_packet.length) {
                UART1.Write(tx_packet.data[index]);
            }
            else if (index == tx_packet.length) {
                UART1.Write(tx_packet.checksum);
            }
            else {
                // last byte was transmitted
                lin_state = LIN_BUS_READY;
            }
        }
    }
    else if (lin_state == LIN_BUS_RECEIVING)
    {   // just send a read announcement
        if (tx_byteCount == 0) {
            UART1.Write(LIN_SYNC_FIELD);
        }
        else if (tx_byteCount == 1) {
            UART1.Write(rx_node_pid);
        }
        else {
            // after last byte was transmitted, we are now ready to receive data
            EUSART1_ReceiveEnable();
        }
    }
    
    tx_byteCount++;
}