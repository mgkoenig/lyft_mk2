/*
 * File:   host.c
 * Author: sire
 *
 * Created on July 22, 2025, 2:50 PM
 */

#include "mcc_generated_files/uart/uart_drv_interface.h"
#include "host.h"


struct host_data_packet host_rx_packet, host_tx_packet;
enum host_transmission host_rx_state;
enum host_transmission host_tx_state;

bool        newData;
uint8_t     rx_byteCount;
uint8_t     tx_byteCount;


void host_init()
{    
    UART2.RxCompleteCallbackRegister(&cb_host_rx);
    UART2.TxCompleteCallbackRegister(&cb_host_tx);
    
    TMR2_OverflowCallbackRegister(&cb_tmr2);
    
    newData = false;
    host_rx_state = READY;
    host_tx_state = READY;       
}

bool host_newDataAvailable()
{
    return newData;
}

void host_read(struct host_data_packet *packet)
{
    if (packet != NULL)
    {
        *packet = host_rx_packet;
        host_rx_state = READY;
        newData = false;
    }
}

void host_write(struct host_data_packet packet)
{
    host_tx_packet = packet;
    
    tx_byteCount = 0;
    host_tx_state = TRANSMITTING;    
    UART2.Write(HOST_STX);     
}

void host_calcChecksum(struct host_data_packet *packet)
{
    uint8_t i, checksum = HOST_STX;
    
    if (packet != NULL)
    {
        checksum ^= packet->command;
        checksum ^= packet->length;

        for (i=0; i<packet->length; i++) {
            checksum ^= packet->data[i];
        }
        
        packet->checksum = checksum;
        
    }
}

bool host_verifyChecksum(struct host_data_packet packet)
{
    bool isValid;
    uint8_t i, calc_checksum;
    
    calc_checksum = HOST_STX;
    calc_checksum ^= packet.command;
    calc_checksum ^= packet.length;
    
    for (i=0; i<packet.length; i++) {
        calc_checksum ^= packet.data[i];
    }

    if (calc_checksum == packet.checksum) {
        isValid = true;
    } else {
        isValid = false;
    }
    
    return isValid;
}


static void cb_host_rx()
{
    uint8_t index, rx_byte;
    
    rx_byte = UART2.Read();    
    
    if ((host_rx_state == READY) && (rx_byte == HOST_STX))
    {
        // a transmission was initiated        
        host_rx_state = RECEIVING;
        rx_byteCount = 0;
        
        TMR2_Start();
    } 
    
    else if (host_rx_state == RECEIVING)
    {
        if (rx_byteCount == 0) {
            host_rx_packet.command = rx_byte;
        }    
        
        else if (rx_byteCount == 1) {
            host_rx_packet.length = rx_byte;
        }
        
        else 
        {
            if (host_rx_packet.length == 0) 
            {
                host_rx_packet.checksum = rx_byte;
                host_rx_state = FINISHED;
                newData = true;
                
                TMR2_Stop();
                TMR2_CounterSet(0);
            }
            
            else
            {
                index = rx_byteCount - 2;
                
                if (index == host_rx_packet.length) 
                {
                    host_rx_packet.checksum = rx_byte;
                    host_rx_state = FINISHED;
                    newData = true;

                    TMR2_Stop();
                    TMR2_CounterSet(0);
                }
                else {
                    host_rx_packet.data[index] = rx_byte;
                }
            }
        }
        
        rx_byteCount++;
        
        if (rx_byteCount > 10) {
            // something went wrong. discard this received data. 
            host_rx_state = READY;
            newData = false;
            TMR2_Stop();
            TMR2_CounterSet(0);
        }
    }    
}

static void cb_host_tx(void)
{
    uint8_t index;       
    
    if (tx_byteCount == (host_tx_packet.length + 3)) 
    {
        // last byte was already transmitted. transmission is complete.
        host_tx_state = READY;
    }
        
    else
    {
        if (tx_byteCount == 0)
        {
            UART2.Write(host_tx_packet.command);
        }

        else if (tx_byteCount == 1)
        {
            UART2.Write(host_tx_packet.length);
        }

        else if (tx_byteCount > 1)
        {
            if (host_tx_packet.length == 0)
            {
                UART2.Write(host_tx_packet.checksum);
            }
            else
            {
                if (tx_byteCount == (host_tx_packet.length + 2))
                {
                    UART2.Write(host_tx_packet.checksum);
                }
                else
                {
                    index = (tx_byteCount - 2);
                    UART2.Write(host_tx_packet.data[index]);
                }
            }
        }

        tx_byteCount++;    
    }
}

static void cb_tmr2()
{
    rx_byteCount = 0;
    newData = false;
    host_rx_state = READY;    
}