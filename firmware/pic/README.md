The PIC controller and its firmware act as bridge module between the ESP32 and the desk. On the one hand, the controller takes care of the desk communication (including timings, move commands, error handling, etc). 
On the other hand it provides an API which allows the ESP32 easily to access all supported features and read important information from the desk. 
