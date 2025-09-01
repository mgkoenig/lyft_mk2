from machine import Pin, UART

VERSION = 20250813
HOST_STX = 0x4C


def position_to_height(position:int, offset:int=0, unit:str="cm") -> int:
    height = ((position / 100) + 60)
    height += offset
    if (unit == "cm"):
        return int(height)
    elif (unit == "inch"):
	    return int((height / 2.54) * 10)
    

class Bekant:
    
    class Command:
        GET_DESK_STATE              = 0x10
        GET_DESK_DRIFT              = 0x11
        GET_DESK_POSITION           = 0x12
        GET_DESK_UPPER_LIMIT        = 0x13
        GET_DESK_LOWER_LIMIT        = 0x14

        GET_MOTOR_LEFT_STATE        = 0x20
        GET_MOTOR_LEFT_POSITION     = 0x21
        GET_MOTOR_LEFT_UPPER_LIMIT  = 0x22
        GET_MOTOR_LEFT_LOWER_LIMIT  = 0x23
        GET_MOTOR_LEFT_NODE_ID      = 0x24
        GET_MOTOR_LEFT_SCAN_ID      = 0x25
        GET_MOTOR_LEFT_PROPERTY     = 0x26

        GET_MOTOR_RIGHT_STATE       = 0x30
        GET_MOTOR_RIGHT_POSITION    = 0x31
        GET_MOTOR_RIGHT_UPPER_LIMIT = 0x32
        GET_MOTOR_RIGHT_LOWER_LIMIT = 0x33
        GET_MOTOR_RIGHT_NODE_ID     = 0x34
        GET_MOTOR_RIGHT_SCAN_ID     = 0x35
        GET_MOTOR_RIGHT_PROPERTY    = 0x36

        CALL_DESK_INIT              = 0x40
        CALL_DESK_DEINIT            = 0x41
        CALL_DESK_CALIBRATION       = 0x42

        SET_DESK_POSITION           = 0x50
        SET_DESK_HALT               = 0x51

        GET_PROTOCOL_VERSION        = 0x70
        GET_FIRMWARE_VERSION        = 0x71
        GET_BOARD_REVISION          = 0x72
        CALL_WATCHDOG_ENABLE        = 0x73
        CALL_WATCHDOG_DISABLE       = 0x74


    class State:
        IDLE                        = 0x00
        MAINTENANCE_BLOCKED         = 0x10
        MAINTENANCE_CALIBRATION     = 0x11
        STARTUP                     = 0x20
        STARTUP_BEGIN               = 0x21
        STARTUP_ANNOUNCEMENT_ONE    = 0x22
        STARTUP_ANNOUNCEMENT_TWO    = 0x23
        STARTUP_PREPROCESSING_ONE   = 0x24
        STARTUP_PREPROCESSING_TWO   = 0x25
        STARTUP_SCAN_NODE           = 0x26
        STARTUP_READ_STATUS_BYTE    = 0x27
        STARTUP_READ_UPPER_LIMIT_HI = 0x28
        STARTUP_READ_UPPER_LIMIT_LO = 0x29
        STARTUP_READ_LOWER_LIMIT_HI = 0x2A
        STARTUP_READ_LOWER_LIMIT_LO = 0x2B
        STARTUP_WRITE_IDENTIFIER    = 0x2C
        STARTUP_POSTPROCESSING_ONE  = 0x2D
        STARTUP_POSTPROCESSING_TWO  = 0x2E
        OPERATION                   = 0x40
        OPERATION_BEGIN             = 0x41
        OPERATION_NORMAL            = 0x42
        OPERATION_RESCUE            = 0x43
        OPERATION_ANNOUNCING        = 0x44
        OPERATION_MOVING_UP         = 0x45
        OPERATION_MOVING_DOWN       = 0x46
        OPERATION_MOVING_SLOW       = 0x47
        OPERATION_MOVING_STOP       = 0x48
        OPERATION_CALIBRATING       = 0x49
        OPERATION_CALIBRATING_DONE  = 0x4A
        OPERATION_LIMIT_UP          = 0x4B  # not implemented	
        OPERATION_LIMIT_DOWN        = 0x4C  # not implemented
    

    class Error:
        NO_ERROR                    = 0x00

        HOST_INVALID_DATA           = 0x01
        HOST_INVALID_LENGTH         = 0x02
        HOST_INVALID_COMMAND        = 0x03
        HOST_INVALID_CHECKSUM       = 0x04
        HOST_INVALID_PARAMETER      = 0x05
        HOST_INVALID_IDENTIFIER     = 0x06
        HOST_TIMEOUT                = 0x07

        DESK_GENERAL_ERROR          = 0xD0
        DESK_TIMEOUT                = 0xD1
        DESK_INVALID_DATA           = 0xD2
        DESK_INVALID_COMMAND        = 0xD3
        DESK_INVALID_CHECKSUM       = 0xD4
        DESK_BUSY                   = 0xD5
        DESK_NOT_IDLE               = 0xD6
        DESK_NOT_READY              = 0xD7
        DESK_TARGET_POSITION        = 0xD8
        DESK_UPPER_LIMIT            = 0xD9
        DESK_LOWER_LIMIT            = 0xDA

      
    def __init__(self):
        self.uart = UART(2, baudrate=115200, rx=16, tx=17, timeout=80)
    
    
    def _checksum(self, data: bytes) -> bytes:
        cc = 0x00
        for b in data:
            cc ^= b
        return bytes([cc])    
    

    def _flush_uart(self):
        while self.uart.any():
            self.uart.read()
    
    
    def _create_packet(self, command: Bekant.Command, data: bytes = None) -> bytes:
        # structure of a data packet:
        # [STX] [CMD] [LEN (of data)] [opt. DATA] [CC]
        data = data or b''
        length = len(data)
        
        packet = bytes([HOST_STX, command, length])
        
        if (length > 0):
            packet += data
            
        packet += self._checksum(packet) 
       
        return packet
    

    def _inspect_packet(self, command:int, expected_length:int, packet:bytes):
        if (len(packet) < 5):
            self._flush_uart()
            raise Exception("incomplete packet frame received", Bekant.Error.HOST_INVALID_LENGTH)

        if (packet[0] != HOST_STX):
            self._flush_uart()
            raise Exception("invalid STX identifier", Bekant.Error.HOST_INVALID_IDENTIFIER)

        resp_cmd = (command | 0x80)
        if (packet[1] != resp_cmd):
            self._flush_uart()
            raise Exception("received command_id does not match requested command_id", Bekant.Error.HOST_INVALID_COMMAND)

        cc_calc = self._checksum(packet[:-1])
        cc_recv = packet[-1].to_bytes(1, 'big', False)
        if (cc_calc != cc_recv):
            self._flush_uart()
            raise Exception("invalid checksum", Bekant.Error.HOST_INVALID_CHECKSUM)

        if (packet[2] < 0x01):
            self._flush_uart()
            raise Exception("invalid payload length", Bekant.Error.HOST_INVALID_LENGTH)
        
        if (packet[3] != 0x00):
            err_arg = 0x00
            err_msg = "desk responded error code "

            if (packet[3] == 0x01):
                err_arg = Bekant.Error.DESK_TIMEOUT
                err_msg += "0x01 (timeout occured)."
            elif (packet[3] == 0x02):
                err_arg = Bekant.Error.DESK_INVALID_DATA
                err_msg += "0x02 (invalid data)."            
            elif (packet[3] == 0x03):
                err_arg = Bekant.Error.DESK_INVALID_COMMAND
                err_msg += "0x03 (invalid command)."
            elif (packet[3] == 0x04):
                err_arg = Bekant.Error.DESK_INVALID_CHECKSUM
                err_msg += "0x04 (invalid checksum)."
            elif (packet[3] == 0x05):
                err_arg = Bekant.Error.DESK_BUSY
                err_msg += "0x05 (desk is busy)."
            elif (packet[3] == 0x06):
                err_arg = Bekant.Error.DESK_NOT_IDLE
                err_msg += "0x06 (desk not in idle state)."
            elif (packet[3] == 0x07):
                err_arg = Bekant.Error.DESK_NOT_READY
                err_msg += "0x07 (desk not ready)."
            elif (packet[3] == 0x08):
                err_arg = Bekant.Error.DESK_TARGET_POSITION
                err_msg += "0x08 (target position too close to move)."            
            elif (packet[3] == 0x09):
                err_arg = Bekant.Error.DESK_UPPER_LIMIT
                err_msg += "0x09 (upper limit reached)."
            elif (packet[3] == 0x0A):
                err_arg = Bekant.Error.DESK_LOWER_LIMIT
                err_msg += "0x0A (lower limit reached)."
            else:
                err_arg = Bekant.Error.DESK_GENERAL_ERROR
                err_msg = "desk responded an unknown error code: " + str(packet[3])
            
            raise Exception(err_msg, err_arg)

        if (len(packet) != expected_length): 
            self._flush_uart()
            raise Exception("unexpected packet length", Bekant.Error.HOST_INVALID_LENGTH)
       
    
    def startup(self): 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.CALL_DESK_INIT)
        self.uart.write(request)
        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.CALL_DESK_INIT, 5, response)
            
            except Exception as e:
                err_msg = "Error in 'startup': " + e.args[0] 
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)
            
        else:
            self._flush_uart()
            raise Exception("Error in 'startup': UART Timeout", Bekant.Error.HOST_TIMEOUT)
            
    
    def shutdown(self): 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.CALL_DESK_DEINIT)
        self.uart.write(request)
        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.CALL_DESK_DEINIT, 5, response)

            except Exception as e:
                err_msg = "Error in 'shutdown': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)

        else:
            self._flush_uart()
            raise Exception("Error (shutdown): UART Timeout", Bekant.Error.HOST_TIMEOUT)

        self._upper_limit = 0
        self._lower_limit = 0
        
    
    
    def calibrate(self): 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.CALL_DESK_CALIBRATION)
        self.uart.write(request)
        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.CALL_DESK_CALIBRATION, 5, response)

            except Exception as e:
                err_msg = "Error in 'calibrate': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)

        else:
            self._flush_uart()
            raise Exception("Error (calibrate): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    
    
    def stop(self): 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.SET_DESK_HALT)
        self.uart.write(request)
        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.SET_DESK_HALT, 5, response)

            except Exception as e:
                err_msg = "Error in 'stop': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)

        else:
            self._flush_uart()
            raise Exception("Error (stop): UART Timeout", Bekant.Error.HOST_TIMEOUT)
                

    def get_state(self) -> int: 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_DESK_STATE)
        self.uart.write(request)
        
        response = self.uart.read(6)        
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.GET_DESK_STATE, 6, response)

            except Exception as e:
                err_msg = "Error in 'get_state': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)
            
            else:
                return response[4]

        else:
            self._flush_uart()
            raise Exception("Error (get_state): UART Timeout", Bekant.Error.HOST_TIMEOUT)
            
    
    def get_drift(self) -> int: 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_DESK_DRIFT)
        self.uart.write(request)
        
        response = self.uart.read(6)        
        if (response != None):   
            try:
                self._inspect_packet(Bekant.Command.GET_DESK_DRIFT, 6, response)

            except Exception as e:
                err_msg = "Error in 'get_drift': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)
            
            else:
                return response[4]
            
        else:
            self._flush_uart()
            raise Exception("Error (get_drift): UART Timeout", Bekant.Error.HOST_TIMEOUT)
                
    
    #@property
    def get_position(self) -> int:
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_DESK_POSITION)
        #print(request.hex())
        self.uart.write(request)
        
        response = self.uart.read(7)        
        if (response != None):
        #print(response.hex())
        #print("***************")
            try:
                self._inspect_packet(Bekant.Command.GET_DESK_POSITION, 7, response)

            except Exception as e:
                err_msg = "Error in 'get_position': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)
            
            else:
                b_value = response[4:6]
                return int.from_bytes(b_value, 'big')
            
        else:
            self._flush_uart()
            raise Exception("Error (get_position): UART Timeout", Bekant.Error.HOST_TIMEOUT)
        
        
    #@position.setter
    def set_position(self, position: int):
        self._flush_uart()
        bytes_position = position.to_bytes(2, 'big', False)
        request = self._create_packet(Bekant.Command.SET_DESK_POSITION, bytes_position)
        self.uart.write(request)
        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.SET_DESK_POSITION, 5, response)

            except Exception as e:
                err_msg = "Error in 'set_position': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)

        else:
            self._flush_uart()
            raise Exception("Error (set_position): UART Timeout", Bekant.Error.HOST_TIMEOUT)


    def get_upper_limit(self) -> int:
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_DESK_UPPER_LIMIT)
        self.uart.write(request)
        
        response = self.uart.read(7)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.GET_DESK_UPPER_LIMIT, 7, response)

            except Exception as e:
                err_msg = "Error in 'get_upper_limit': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)
            
            else:
                b_value = response[4:6]
                self._upper_limit = int.from_bytes(b_value, 'big')
                return self._upper_limit

        else:
            self._flush_uart()
            raise Exception("Error (get_upper_limit): UART Timeout", Bekant.Error.HOST_TIMEOUT)

    
    def get_lower_limit(self) -> int:
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_DESK_LOWER_LIMIT)
        self.uart.write(request)
        
        response = self.uart.read(7)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.GET_DESK_LOWER_LIMIT, 7, response)

            except Exception as e:
                err_msg = "Error in 'get_lower_limit': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)
            
            else:
                b_value = response[4:6]
                i_value = int.from_bytes(b_value, 'big')

                if (i_value != 0):
                    self._lower_limit = int.from_bytes(b_value, 'big')
                    return self._lower_limit
                else:
                    raise Exception("Error (get_lower_limit): invalid data", Bekant.Error.HOST_INVALID_DATA)

        else:
            self._flush_uart()
            raise Exception("Error (get_lower_limit): UART Timeout", Bekant.Error.HOST_TIMEOUT)

    
    def get_motor_scan_id(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_SCAN_ID)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_SCAN_ID)
        else:
            raise Exception("Error (get_motor_scan_id): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)
        
        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(6)
        if (response != None):
            if (unit == "left"):
                try: 
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_SCAN_ID, 6, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_scan_id' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
                
                else:
                    return response[4]

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_SCAN_ID, 6, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_scan_id' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    return response[4]

        else:
            self._flush_uart()
            raise Exception("Error (get_motor_scan_id): UART Timeout", Bekant.Error.HOST_TIMEOUT)


    def get_motor_node_id(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_NODE_ID)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_NODE_ID)
        else:
            raise Exception("Error (get_motor_node_id): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)
        
        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(6)
        if (response != None):
            if (unit == "left"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_NODE_ID, 6, response)
                
                except Exception as e:
                    err_msg = "Error in 'get_motor_node_id' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
                
                else:
                    return response[4]

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_NODE_ID, 6, response)
                
                except Exception as e:
                    err_msg = "Error in 'get_motor_node_id' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
                
                else:
                    return response[4]

        else:
            self._flush_uart()
            raise Exception("Error (get_motor_node_id): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_motor_property(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_PROPERTY)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_PROPERTY)
        else:
            raise Exception("Error (get_motor_property): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)
        
        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(6)
        if (response != None):
            if (unit == "left"):
                try: 
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_PROPERTY, 6, response)
                
                except Exception as e:
                    err_msg = "Error in 'get_motor_property' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
                
                else:
                    return response[4]

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_PROPERTY, 6, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_property' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)     

                else:
                    return response[4]      

        else:
            self._flush_uart()
            raise Exception("Error (get_motor_property): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_motor_state(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_STATE)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_STATE)
        else:
            raise Exception("Error (get_motor_state): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)
        
        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(6)
        if (response != None):
            if (unit == "left"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_STATE, 6, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_state' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
                
                else:
                    return response[4]

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_STATE, 6, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_state' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    return response[4] 

        else:
            self._flush_uart()
            raise Exception("Error (get_motor_state): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_motor_position(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_POSITION)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_POSITION)
        else:
            raise Exception("Error (get_motor_position): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)

        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(7)
        if (response != None):
            if (unit == "left"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_POSITION, 7, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_position' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    b_value = response[4:6]
                    return int.from_bytes(b_value, 'big')

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_POSITION, 7, response)
                
                except Exception as e:
                    err_msg = "Error in 'get_motor_position' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    b_value = response[4:6]
                    return int.from_bytes(b_value, 'big') 
            
        else:
            self._flush_uart()
            raise Exception("Error (get_motor_position): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_motor_upper_limit(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_UPPER_LIMIT)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_UPPER_LIMIT)
        else:
            raise Exception("Error (get_motor_upper_limit): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)
        
        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(7)
        if (response != None):

            if (unit == "left"):
                try: 
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_UPPER_LIMIT, 7, response)
                
                except Exception as e:
                    err_msg = "Error in 'get_motor_upper_limit' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    b_value = response[4:6]
                    return int.from_bytes(b_value, 'big')

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_UPPER_LIMIT, 7, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_upper_limit' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    b_value = response[4:6]
                    return int.from_bytes(b_value, 'big')
    
        else:
            self._flush_uart()
            raise Exception("Error (get_motor_upper_limit): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_motor_lower_limit(self, unit: str) -> int:
        if (unit == "left"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_LEFT_LOWER_LIMIT)
        elif (unit == "right"):
            request = self._create_packet(Bekant.Command.GET_MOTOR_RIGHT_LOWER_LIMIT)
        else:
            raise Exception("Error (get_motor_lower_limit): invalid paramater. should be 'left' or 'right'.", Bekant.Error.HOST_INVALID_PARAMETER)
        
        self._flush_uart()
        self.uart.write(request)
        response = self.uart.read(7)
        if (response != None):

            if (unit == "left"):
                try: 
                    self._inspect_packet(Bekant.Command.GET_MOTOR_LEFT_LOWER_LIMIT, 7, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_lower_limit' (left): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
                
                else:
                    b_value = response[4:6]
                    return int.from_bytes(b_value, 'big')

            elif (unit == "right"):
                try:
                    self._inspect_packet(Bekant.Command.GET_MOTOR_RIGHT_LOWER_LIMIT, 7, response)

                except Exception as e:
                    err_msg = "Error in 'get_motor_lower_limit' (right): " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)

                else:
                    b_value = response[4:6]
                    return int.from_bytes(b_value, 'big') 
    
        else:
            self._flush_uart()
            raise Exception("Error (get_motor_lower_limit): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_firmware_version(self) -> str:
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_FIRMWARE_VERSION)        
        self.uart.write(request)        

        response = self.uart.read(8)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.GET_FIRMWARE_VERSION, 8, response)

            except Exception as e:
                    err_msg = "Error in 'get_firmware_version': " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
            
            else:
                version_major = response[4]
                version_minor = response[5]
                version_patch = response[6]
                return str(version_major) + "." + str(version_minor) + "." + str(version_patch)

        else:
            self._flush_uart()
            raise Exception("Error (get_firmware_version): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def get_board_revision(self) -> str:
        self._flush_uart()
        request = self._create_packet(Bekant.Command.GET_BOARD_REVISION)
        self.uart.write(request)
        
        response = self.uart.read(8)
        if (response != None):
            try: 
                self._inspect_packet(Bekant.Command.GET_BOARD_REVISION, 8, response)

            except Exception as e:
                    err_msg = "Error in 'get_board_revision': " + e.args[0]
                    err_arg = e.args[1]
                    raise Exception(err_msg, err_arg)
            
            else:
                b_value = response[4:7]
                return b_value.decode("utf-8")  
            
        else:
            self._flush_uart()
            raise Exception("Error (get_board_revision): UART Timeout", Bekant.Error.HOST_TIMEOUT)


    def watchdog_enable(self): 
        self._flush_uart()
        request = self._create_packet(Bekant.Command.CALL_WATCHDOG_ENABLE)
        
        self.uart.write(request)        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.CALL_WATCHDOG_ENABLE, 5, response)

            except Exception as e:
                err_msg = "Error in 'watchdog_enable': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)

        else:
            self._flush_uart()
            raise Exception("Error (watchdog_enable): UART Timeout", Bekant.Error.HOST_TIMEOUT)
    

    def watchdog_disable(self):
        self._flush_uart()
        request = self._create_packet(Bekant.Command.CALL_WATCHDOG_DISABLE)
        
        self.uart.write(request)        
        response = self.uart.read(5)
        if (response != None):
            try:
                self._inspect_packet(Bekant.Command.CALL_WATCHDOG_DISABLE, 5, response)

            except Exception as e:
                err_msg = "Error in 'watchdog_disable': " + e.args[0]
                err_arg = e.args[1]
                raise Exception(err_msg, err_arg)

        else:
            self._flush_uart()
            raise Exception("Error (watchdog_disable): UART Timeout", Bekant.Error.HOST_TIMEOUT)
