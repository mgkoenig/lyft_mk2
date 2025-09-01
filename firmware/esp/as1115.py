from machine import Pin, I2C
import time

VERSION = 20250815


PIN_SDA = Pin(26)
PIN_SCL = Pin(25)
I2C_FREQ = 400000


ADDR_EEPROM = 0x50
ADDR_DISPLAY = 0x00


def set_bit(register, bit):
    return register | (1<<bit)

def clear_bit(register, bit):
    return register & ~(1<<bit)


char_map = {
    "0" : 0x7E,
    "1" : 0x30,
    "2" : 0x6D,
    "3" : 0x79,
    "4" : 0x33,
    "5" : 0x5B,
    "6" : 0x5F,
    "7" : 0x70,
    "8" : 0x7F,
    "9" : 0x7B,
    "A" : 0x77,
    "C" : 0x4E,
    "E" : 0x4F,
    "F" : 0x47,
    "G" : 0x5E,
    "H" : 0x37,
    "L" : 0x0E,
    "O" : 0x7E,
    "P" : 0x67,
    "U" : 0x3E,
    "c" : 0x0D,
    "d" : 0x3D,
    "i" : 0x10,
    "n" : 0x15,
    "o" : 0x1D,
    "r" : 0x05,
    "." : 0x80,
    " " : 0x00,
    ":" : 0x60,
    "-" : 0x01,
    "_" : 0x08,
    "*" : 0x10
}



class AS1115:

    class Keys:
        BUTTON_NONE = 0
        BUTTON_1 = 1
        BUTTON_2 = 2
        BUTTON_3 = 3
        BUTTON_4 = 4
        BUTTON_UP = 5
        BUTTON_DOWN = 6

    class DecodeMode:
        NUMERIC = 0
        ALPHANUMERIC = 2

    class Register:
        KEYSCAN_A = 0x1C
        KEYSCAN_B = 0x1D
        
        DIGIT_0 = 0x01
        DIGIT_1 = 0x02
        DIGIT_2 = 0x03
        DIGIT_3 = 0x04
        DIGIT_4 = 0x05
        DIGIT_5 = 0x06
        DIGIT_6 = 0x07
        DIGIT_7 = 0x08
        
        INTENSITY_GLOBAL = 0x0A
        INTENSITY_DIGIT10 = 0x10
        INTENSITY_DIGIT32 = 0x11
        INTENSITY_DIGIT54 = 0x12
        INTENSITY_DIGIT76 = 0x13
        
        SHUTDOWN = 0x0C
        TESTMODE = 0x0F
        SCANLIMIT = 0x0B
        DECODEMODE = 0x09
        FEATURE = 0x0E


    def __init__(self):
        self.i2c = I2C(0, scl=PIN_SCL, sda=PIN_SDA, freq=I2C_FREQ)
        
        '''
        devices = self.i2c.scan()
        if len(devices) != 0:
            print('Number of I2C devices found=', len(devices))
            for device in devices:
                print("Device Hexadecimel Address= ", hex(device))
        '''
        
        data = bytes([AS1115.Register.SCANLIMIT, 0x03])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        self._decode_mode = AS1115.DecodeMode.NUMERIC
        data = bytes([AS1115.Register.DECODEMODE, 0x3F])    # numeric
        #data = bytes([AS1115.Register.DECODEMODE, 0x00])   # alphanumeric
        self.i2c.writeto(ADDR_DISPLAY, data)
                
        data = bytes([AS1115.Register.SHUTDOWN, 0x01])
        self.i2c.writeto(ADDR_DISPLAY, data)

        self.i2c.writeto(ADDR_DISPLAY, bytes([AS1115.Register.KEYSCAN_A]))        
        keyscan = self.i2c.readfrom(ADDR_DISPLAY, 1)

        self.i2c.writeto(ADDR_DISPLAY, bytes([AS1115.Register.KEYSCAN_B]))        
        keyscan = self.i2c.readfrom(ADDR_DISPLAY, 1)

        
    def deinit(self):
        data = bytes([AS1115.Register.FEATURE, 0x02])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        data = bytes([AS1115.Register.SHUTDOWN, 0x00])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        self.i2c.deinit()
    
    
    def blink(self, enable:bool, low_frequency=False):        
        mode = 0x80
        
        if enable == True:
            mode = set_bit(mode, 4)
        
        if low_frequency == True:
            mode = set_bit(mode, 5)
        
        data = bytes([AS1115.Register.FEATURE, mode])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        
    def test(self, enable:bool):        
        if (enable == True):
            data = bytes([AS1115.Register.TESTMODE, 0x01])
            
        else:
            data = bytes([AS1115.Register.TESTMODE, 0x00])
        
        self.i2c.writeto(ADDR_DISPLAY, data)		# run a display test

        
    def clear(self):
        blank_value = 0x00 
        
        if (self._decode_mode == AS1115.DecodeMode.NUMERIC):
            blank_value = 0x0F
        elif (self._decode_mode == AS1115.DecodeMode.ALPHANUMERIC):
            blank_value = 0x00        

        data = bytes([AS1115.Register.DIGIT_0, blank_value])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        data = bytes([AS1115.Register.DIGIT_1, blank_value])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        data = bytes([AS1115.Register.DIGIT_2, blank_value])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        data = bytes([AS1115.Register.DIGIT_3, blank_value])
        self.i2c.writeto(ADDR_DISPLAY, data)
                
    
    def set_brightness(self, brightness):   

        if (0 <= brightness < 16):
            data = bytes([AS1115.Register.INTENSITY_GLOBAL, brightness])
            self.i2c.writeto(ADDR_DISPLAY, data)
           
    
    def show_number(self, value:int, dp:bool=False): 
        if (self._decode_mode != AS1115.DecodeMode.NUMERIC):
            self.clear()
            data = bytes([AS1115.Register.DECODEMODE, 0x3F])
            self.i2c.writeto(ADDR_DISPLAY, data)
            self._decode_mode = AS1115.DecodeMode.NUMERIC
            self.clear()

        hundreds = int(value / 100)
        tens = int((value / 10) % 10)
        ones = int(value % 10)
        
        if (hundreds == 0):
            hundreds = 0x0F
        
        data = bytes([AS1115.Register.DIGIT_0, hundreds])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        if (dp == True):
            data = bytes([AS1115.Register.DIGIT_1, (tens | 0x80)])
        else:
            data = bytes([AS1115.Register.DIGIT_1, tens])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        data = bytes([AS1115.Register.DIGIT_2, ones])
        self.i2c.writeto(ADDR_DISPLAY, data)
        
        data = bytes([AS1115.Register.DIGIT_3, 0x0F])
        self.i2c.writeto(ADDR_DISPLAY, data)

    
    def show_text(self, string:str):        
        if (self._decode_mode != AS1115.DecodeMode.ALPHANUMERIC):
            self.clear()
            data = bytes([AS1115.Register.DECODEMODE, 0x00])
            self.i2c.writeto(ADDR_DISPLAY, data)
            self._decode_mode = AS1115.DecodeMode.ALPHANUMERIC
            self.clear()        
        
        if (len(string) == 4):
            char_value = char_map[string[0]]
            data = bytes([AS1115.Register.DIGIT_0, char_value])
            self.i2c.writeto(ADDR_DISPLAY, data)

            char_value = char_map[string[2]]
            data = bytes([AS1115.Register.DIGIT_1, char_value])
            self.i2c.writeto(ADDR_DISPLAY, data)

            char_value = char_map[string[3]]
            data = bytes([AS1115.Register.DIGIT_2, char_value])
            self.i2c.writeto(ADDR_DISPLAY, data)

            char_value = char_map[string[1]]  # should be ':' or blank
            data = bytes([AS1115.Register.DIGIT_3, char_value])
            self.i2c.writeto(ADDR_DISPLAY, data)
    

    def update_character(self, char:str, position:int):
        if (self._decode_mode == AS1115.DecodeMode.ALPHANUMERIC):
        
            if ((len(char) == 1) and (position < 3)):
                data = bytes()
                char_value = char_map[char]
                
                if (position == 0):
                    data = bytes([AS1115.Register.DIGIT_2, char_value])
                elif (position == 1):
                    data = bytes([AS1115.Register.DIGIT_1, char_value])
                elif (position == 2):
                    data = bytes([AS1115.Register.DIGIT_0, char_value])
                
                self.i2c.writeto(ADDR_DISPLAY, data)


    def get_keys(self):        
        keys = []
        
        self.i2c.writeto(ADDR_DISPLAY, bytes([AS1115.Register.KEYSCAN_A]))        
        keyscan = self.i2c.readfrom(ADDR_DISPLAY, 1)
        
        keymask = int.from_bytes(keyscan, "big")
        keymask = (keymask ^ 0xFF)    
        
        if (keymask & 0x01):
            #print("Button A")	# BtnDOWN
            keys.append(AS1115.Keys.BUTTON_DOWN)
        if (keymask & 0x02):
            #print("Button B")	# Btn2
            keys.append(AS1115.Keys.BUTTON_2)
        if (keymask & 0x04):
            #print("Button C")	# Btn4
            keys.append(AS1115.Keys.BUTTON_4)
        if (keymask & 0x08):
            pass
            #print("Button D")
            #keys.append(AS1115.Keys.BUTTON_5)
        if (keymask & 0x10):
            pass
            #print("Button E")	# Btn3
            #keys.append(AS1115.Keys.BUTTON_3)
        if (keymask & 0x20):
            #print("Button F")
            keys.append(AS1115.Keys.BUTTON_3)
        if (keymask & 0x40):
            #print("Button G")	# Btn1
            keys.append(AS1115.Keys.BUTTON_1)
        if (keymask & 0x80):
            #print("Button H")	# BtnUP
            keys.append(AS1115.Keys.BUTTON_UP)
        
        return keys
    

    def silent_indicator(self, enable:bool):
        if (enable == True):
            if (self._decode_mode != AS1115.DecodeMode.ALPHANUMERIC):
                self.clear()
                data = bytes([AS1115.Register.DECODEMODE, 0x00])
                self.i2c.writeto(ADDR_DISPLAY, data)
                self._decode_mode = AS1115.DecodeMode.ALPHANUMERIC
                self.clear() 

            char_value = char_map["*"]
            data = bytes([AS1115.Register.DIGIT_3, char_value])
            self.i2c.writeto(ADDR_DISPLAY, data)
        else:
            self.clear()
