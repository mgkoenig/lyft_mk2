The PIC controller and its firmware act as bridge module between the ESP32 and the desk. On the one hand, the controller takes care of the desk communication (including timings, move commands, error handling, etc). 
On the other hand, it provides an API which allows the ESP32 easily to access all supported features and read important information from the desk. 

## How to comunicate with the PIC controller
The controller is addressed using the UART interface with these settings: **115200@8N1**. 

## Sending a request to the controller
In order to trigger an event you need to send a certain request. Therefore, a simple communication protocol is used in order to make things easier. The structure of each request to the controller is as follows: 

[STX] [CMD] [LEN] [opt. DATA] [CHECKSUM] 

Where: 
| Element  | Description |
|----------|-------------|
| STX      | a start of transmission byte (0x4C) |
| CMD      | the requested command (see API table) |
| LEN      | length of optional data |
| DATA     | if data is sent, it will be here |
| CHECKSUM | each request ends with a checksum byte |


## Receiving a response from the controller 
Every single request follows a response from the controller. In some cases, this response is just a simple ACK to the previous request. In other cases this response contains the requested data (e.g. current desk position, firmware version, etc). The structure of the response is as follows: 

[STX] [ECHO_CMD] [LEN] [ACK] [opt. DATA] [CHECKSUM]

As you can see, the frame has a similar structure as the request, but the content is interpreted slightly different:

| Element  | Description |
|----------|-------------|
| STX      | a start of transmission byte (0x4C) |
| ECHO_CMD | this is the same value as the received command, but it's or-ed with 0x80. For example, you requested a command code 0x12, then the echo_command will be 0x92. |
| LEN      | length of optional data, but at least 0x01 due to the ACK byte |
| ACK      | if there was no error, this ACK byte has the value 0x00. In case of an error, you can read an error value here (e.g. invalid command or invalid checksum). |
| DATA     | the requested data (if no error occurred) |
| CHECKSUM | each response ends with a checksum byte |

..to be continued..
