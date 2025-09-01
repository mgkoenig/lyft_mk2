from machine import Pin, Timer, PWM, WDT
import as1115
import bekant


VERSION = "1.0.0"


TIMER_PERIOD_MS = 100
RESET_BUTTON_DELAY_TIME_MS = 5000
MOVING_BUTTON_DELAY_TIME_MS = 200
MEMORY_BUTTON_DELAY_TIME_MS = 500
SETTINGS_ENTER_DELAY_TIME_MS = 2000
SETTINGS_LEAVE_DELAY_TIME_MS = 4000

PIN_RESET_BUTTON = Pin(14)
PIN_BOARD_LED = Pin(27)
PIN_HMI_IRQ = Pin(32)
PIN_BUZZER = Pin(2)


class OperatingMode:     
    NONE                    = 0x00
    IDLE                    = 0x01
    STARTUP                 = 0x02
    RUNNING                 = 0x03
    SETTINGS                = 0x04
    RESTART                 = 0x05

class OperatingPhase:
    NONE                    = 0x00

class StartupPhase:
    NONE                    = 0x10
    INIT                    = 0x11
    INIT_DELAY              = 0x12
    READ_UPPER_LIMIT        = 0x13
    READ_LOWER_LIMIT        = 0x14
    WATCHDOG                = 0x15

class RunningPhase:
    NONE                    = 0x20
    WAKE_UP                 = 0x21
    READY                   = 0x22
    SLEEP                   = 0x23
    PRE_SLEEP               = 0x24

    MOVING_MANUAL           = 0x25
    MOVING_AUTOMATIC        = 0x26
    MOVING_CALIBRATION      = 0x27
    MOVING_ENDPOSITION      = 0x28

class SettingsPage:
    NONE                    = 0x30
    POSITION                = 0x31
    BRIGHTNESS              = 0x32
    ON_TIME                 = 0x33
    OFFSET                  = 0x34
    UNIT                    = 0x35
    AUDIO                   = 0x36

class Verbosity:
    SILENT                  = 0
    NORMAL                  = 1
    VERBOSE                 = 2
    DEBUG                   = 3


host_state = {
    "op_mode" : OperatingMode.NONE,
    "phase": OperatingPhase.NONE
}

default_config = {
    "desk_position_1": 200, 
    "desk_position_2": 1500,
    "desk_position_3": 4000,
    "desk_position_4": 6000,
    "desk_offset": 0,
    "display_unit": "cm",
    "display_brightness": 4,
    "display_on_time": 10,
    "audio": True
}


def debug(v_level, message):
    if (VERBOSITY_LEVEL >= v_level):
        print(message)


def cb_hmi(pin):
    if (host_state["phase"] == RunningPhase.SLEEP):
        host_state["phase"] = RunningPhase.WAKE_UP


def cb_timer(timer):
    global timer_lapsed
    timer_lapsed = True


def save_config(data, filename="config.json"):
    import ujson
    with open(filename, "w") as f:
        ujson.dump(data, f)


def load_config(filename="config.json"):
    import ujson
    try:
        with open(filename, "r") as f:
            pass

    except OSError:
        debug(Verbosity.VERBOSE, "Config file does not exist. Creating a default config...")
        with open(filename, "w") as f:
            ujson.dump(default_config, f)
    
    finally:
        with open(filename, "r") as f:
            return ujson.load(f)


def write_memory_position(memory_button:int, desk_position:int):
    if (memory_button == hmi.Keys.BUTTON_1):
        config["desk_position_1"] = desk_position
    elif (memory_button == hmi.Keys.BUTTON_2):
        config["desk_position_2"] = desk_position
    elif (memory_button == hmi.Keys.BUTTON_3):
        config["desk_position_3"] = desk_position
    elif (memory_button == hmi.Keys.BUTTON_4):
        config["desk_position_4"] = desk_position
    else:
        pass


def timer_start(period_ms: int):
    global hmi_timer 
    debug(Verbosity.DEBUG, "Runtime timer started.")
    hmi_timer.init(period=period_ms, mode=Timer.ONE_SHOT, callback=cb_timer)


def timer_stop():
    global hmi_timer
    debug(Verbosity.DEBUG, "Runtime timer stopped.")
    hmi_timer.deinit()


def buzzer_start():
    global buzzer
    debug(Verbosity.DEBUG, "Buzzer started.")
    buzzer = PWM(PIN_BUZZER, freq=4000, duty_u16=32768)


def buzzer_stop():
    global buzzer 
    if (buzzer != None):
        debug(Verbosity.DEBUG, "Buzzer stopped.")
        buzzer.deinit()


def led_enable(enable:bool):
    global board_led
    if (enable == True):
        board_led.value(0)
    else:
        board_led.value(1)


def display_brightness(brightness:int):
    if (brightness == 1):
        hmi.set_brightness(1)
    elif (brightness == 2):
        hmi.set_brightness(4)
    elif (brightness == 3):
        hmi.set_brightness(8)
    elif (brightness == 4):
        hmi.set_brightness(15)
    else:
        pass
        

def display_update():
    global desk_position 
    global prev_height
    global prev_phase
    height = 0

    if (host_state["op_mode"] == OperatingMode.RUNNING):

        if (host_state["phase"] in {RunningPhase.PRE_SLEEP, RunningPhase.SLEEP}):
            if (prev_phase != host_state["phase"]):
                hmi.clear()
       
        elif (host_state["phase"] == RunningPhase.READY):
            if (config["display_unit"] == "cm"):
                height = bekant.position_to_height(desk_position, offset=config["desk_offset"], unit="cm") 
                if ((height != prev_height) or (prev_phase != host_state["phase"])):
                    hmi.show_number(height)
                    prev_height = height
            elif (config["display_unit"] == "inch"):
                height = bekant.position_to_height(desk_position, offset=config["desk_offset"], unit="inch") 
                if ((height != prev_height) or (prev_phase != host_state["phase"])):
                    hmi.show_number(height, dp=True)
                    prev_height = height
       
        elif (host_state["phase"] in {RunningPhase.MOVING_MANUAL, RunningPhase.MOVING_AUTOMATIC}):
            if (config["display_unit"] == "cm"):
                height = bekant.position_to_height(desk_position, offset=config["desk_offset"], unit="cm") 
                hmi.show_number(height)
            elif (config["display_unit"] == "inch"):
                height = bekant.position_to_height(desk_position, offset=config["desk_offset"], unit="inch") 
                hmi.show_number(height, dp=True)

        elif (host_state["phase"] == RunningPhase.MOVING_CALIBRATION) and (prev_phase != RunningPhase.MOVING_CALIBRATION):
            display_text = str("C AL")
            hmi.show_text(display_text)
        
        elif (host_state["phase"] == RunningPhase.MOVING_ENDPOSITION) and (prev_phase != RunningPhase.MOVING_ENDPOSITION):
            display_text = str("E nd")
            hmi.show_text(display_text)


    elif (host_state["op_mode"] == OperatingMode.SETTINGS):

        if (host_state["phase"] == SettingsPage.POSITION) and (prev_phase != SettingsPage.POSITION):
            hmi.show_text("P: _")
       
        elif (host_state["phase"] == SettingsPage.BRIGHTNESS) and (prev_phase != SettingsPage.BRIGHTNESS):
            display_text = str("L: ")
            display_text += str(config["display_brightness"])
            hmi.show_text(display_text)

        elif (host_state["phase"] == SettingsPage.UNIT) and (prev_phase != SettingsPage.UNIT):
            if (config["display_unit"] == "cm"):
                hmi.show_text("U: c")
            elif (config["display_unit"] == "inch"):
                hmi.show_text("U:in")
            else:
                hmi.show_text("U: _")

        elif (host_state["phase"] == SettingsPage.OFFSET) and (prev_phase != SettingsPage.OFFSET):
            display_text = str("C:")
            abs_value = 0

            if (config["desk_offset"] < 0):
                display_text += "-"
                abs_value = (config["desk_offset"] * (-1))
            else:
                display_text += " "
                abs_value = config["desk_offset"]
            
            display_text += str(abs_value)
            hmi.show_text(display_text)

        elif (host_state["phase"] == SettingsPage.ON_TIME) and (prev_phase != SettingsPage.ON_TIME):
            display_text = str("F:")

            if (config["display_on_time"] == 5):
                display_text += " 5"
            elif (config["display_on_time"] == 10):
                display_text += "10"
            elif (config["display_on_time"] == 15):
                display_text += "15"
            elif (config["display_on_time"] == 20):
                display_text += "20"
            else:
                display_text += " _"

            hmi.show_text(display_text)
        
        elif (host_state["phase"] == SettingsPage.AUDIO) and (prev_phase != SettingsPage.AUDIO):
            display_text = str("A:")

            if (config["audio"] == True):
                display_text += "on"
            elif (config["audio"] == False):
                display_text += "oF"
            else:
                display_text += " _"
            
            hmi.show_text(display_text)

    
    else:
        pass

    prev_phase = host_state["phase"]


VERBOSITY_LEVEL = Verbosity.SILENT

desk = bekant.Bekant()
hmi = as1115.AS1115()

config = load_config()

host_state["op_mode"] = OperatingMode.IDLE
host_state["phase"] = OperatingPhase.NONE

hmi_timer = Timer(1)
timer_lapsed = False
timekeeper_idle = 0
timekeeper_button_pressed = 0

hmi_keys = []
HMI_IRQ = Pin(PIN_HMI_IRQ, Pin.IN, Pin.PULL_UP)
HMI_IRQ.irq(trigger=Pin.IRQ_FALLING, handler=cb_hmi)
temp_button = hmi.Keys.BUTTON_NONE

buzzer = None
reset_button = Pin(PIN_RESET_BUTTON, Pin.IN, Pin.PULL_UP)
board_led = Pin(PIN_BOARD_LED, Pin.OUT)

desk_position = 0
prev_height = 0
prev_phase = OperatingPhase.NONE

error_count = 0
reset_button_delay = 0
startup_delay_count = 0
restart_delay_count = 0


"""

try:
    desk_lower_limit = desk.get_lower_limit()
    print("Desk Lower Limit:", desk_lower_limit)
except Exception as e:
    print(e)

try:
    desk_upper_limit = desk.get_upper_limit()
    print("Desk Upper Limit:", desk_upper_limit)
except Exception as e:
    print(e)

try:
    motor_scan_id = desk.get_motor_scan_id("left")
    print("Motor Scan_ID Left:", motor_scan_id)
except Exception as e:
    print(e)

try:
    motor_scan_id = desk.get_motor_scan_id("right")
    print("Motor Scan_ID Right:", motor_scan_id)
except Exception as e:
    print(e)

try:
    desk_state = desk.get_state()
    print("Desk State:", hex(desk_state))
except Exception as e:
    print(e)

try:
    desk_drift = desk.get_drift()
    print("Desk Drift:", desk_drift)
except Exception as e:
    print(e)

try:
    desk_position = desk.get_position()
    print("Desk Position:", desk_position)
except Exception as e:
    print(e)

try:
    board_revision = desk.get_board_revision()
    print("Board Revision:", board_revision)
except Exception as e:
    print(e)

"""


debug_msg = "Verbosity Level: " + str(VERBOSITY_LEVEL) + "/" + str(int(Verbosity.DEBUG))
debug(Verbosity.SILENT, debug_msg)
debug(Verbosity.VERBOSE, config)

wdt = WDT(timeout=1000) 
timer_start(TIMER_PERIOD_MS)

while True:

    if (timer_lapsed == True):

        if (error_count > 5):
            # five reading errors in a row are not ususal
            debug(Verbosity.NORMAL, "Fault tolerance exceeded. Going to restart the desk controller... ")
            error_count = 0
            restart_delay_count = 0
            hmi.silent_indicator(True)
            host_state["op_mode"] = OperatingMode.RESTART


        reset_button_state = reset_button.value()
        if ((reset_button_state == False) and (host_state["op_mode"] != OperatingMode.RESTART)):
            reset_button_delay += 1
            delay_time = (reset_button_delay * TIMER_PERIOD_MS)
            hmi.silent_indicator(True)

            if (delay_time > RESET_BUTTON_DELAY_TIME_MS):
                    debug(Verbosity.VERBOSE, "Reset triggered!")
                    buzzer_start()
                    error_count = 0
                    restart_delay_count = 0
                    reset_button_delay = 0
                    host_state["op_mode"] = OperatingMode.RESTART
        else:
            if (reset_button_delay != 0):
                debug(Verbosity.DEBUG, "Reset Button Released.")
                hmi.silent_indicator(False)
                reset_button_delay = 0
        

        if (host_state["op_mode"] == OperatingMode.IDLE):
            host_state["op_mode"] = OperatingMode.STARTUP
            host_state["phase"] = StartupPhase.INIT
            buzzer_stop()

        
        elif (host_state["op_mode"] == OperatingMode.STARTUP):

            if (host_state["phase"] == StartupPhase.INIT):
                try:
                    debug(Verbosity.NORMAL, "STARTUP!")
                    desk.startup()

                except Exception as e:
                    debug_msg = "Startup response:" + e.args[0]
                    debug(Verbosity.DEBUG, debug_msg) 
                    if (e.args[1] == desk.Error.DESK_NOT_IDLE):
                        # desk already up and running
                        error_count = 0
                        host_state["phase"] = StartupPhase.READ_UPPER_LIMIT
                    else:
                        error_count += 1

                else:
                    error_count = 0
                    startup_delay_count = 0
                    debug(Verbosity.DEBUG, "Waiting for startup to finish...") 
                    host_state["phase"] = StartupPhase.INIT_DELAY

                finally:
                    hmi.clear()
                    display_brightness(config["display_brightness"])
                    hmi.clear()

                    hmi_keys = hmi.get_keys()
                    hmi_keys = []

                    led_enable(False)

            
            if (host_state["phase"] == StartupPhase.INIT_DELAY):
                startup_delay_count += 1
                if (startup_delay_count > 10):
                    host_state["phase"] = StartupPhase.READ_UPPER_LIMIT

            
            elif (host_state["phase"] == StartupPhase.READ_UPPER_LIMIT):
                try:
                    desk_upper_limit = desk.get_upper_limit()
                    debug_msg = "Desk Upper Limit: " + str(desk_upper_limit)
                    debug(Verbosity.NORMAL, debug_msg)
                except Exception as e:
                    error_count += 1
                    debug(Verbosity.DEBUG, e)
                else:
                    if (desk_upper_limit != 0):
                        error_count = 0
                        host_state["phase"] = StartupPhase.READ_LOWER_LIMIT
                    else:
                        error_count += 1
            

            elif (host_state["phase"] == StartupPhase.READ_LOWER_LIMIT):
                try:
                    desk_lower_limit = desk.get_lower_limit()
                    debug_msg = "Desk Lower Limit: " + str(desk_lower_limit)
                    debug(Verbosity.NORMAL, debug_msg)
                except Exception as e:
                    error_count += 1
                    debug(Verbosity.DEBUG, e)
                else:
                    if (desk_lower_limit != 0):
                        error_count = 0
                        host_state["phase"] = StartupPhase.WATCHDOG
                    else:
                        error_count += 1
            

            elif (host_state["phase"] == StartupPhase.WATCHDOG):
                try:
                    desk.watchdog_enable()
                    
                except Exception as e:
                    error_count += 1
                    debug(Verbosity.DEBUG, e)
                else:
                    error_count = 0
                    debug(Verbosity.NORMAL, "Watchdog Enabled!") 
                    host_state["op_mode"] = OperatingMode.RUNNING
                    host_state["phase"] = RunningPhase.SLEEP
        

        elif (host_state["op_mode"] == OperatingMode.RUNNING):
            try:
                desk_position = desk.get_position()  

            except Exception as e:
                error_count += 1
                debug_msg = "Could not read desk position: " + e.args[0]
                debug(Verbosity.DEBUG, debug_msg)
            
            else:
                if (desk_position == 0):
                    debug(Verbosity.DEBUG, "No valid position was returned.")
                    error_count += 1
                else:
                    error_count = 0
            

            if (host_state["phase"] == RunningPhase.WAKE_UP):
                height = bekant.position_to_height(desk_position, offset=config["desk_offset"], unit=config["display_unit"])
                if (config["display_unit"] == "cm"):
                    hmi.show_number(height)
                else:
                    hmi.show_number(height, True)

                timekeeper_button_pressed = 0
                timekeeper_idle = 0
                host_state["phase"] = RunningPhase.READY
            

            elif (host_state["phase"] == RunningPhase.READY):
                hmi_keys = hmi.get_keys()
                buzzer_stop()

                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    off_time = ((config["display_on_time"] * 1000) / TIMER_PERIOD_MS)
                    if (timekeeper_idle > off_time):
                        host_state["phase"] = RunningPhase.PRE_SLEEP
                
                elif (len(hmi_keys) == 1):                   
                    target_position = 0
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1

                    dwell_time = (timekeeper_button_pressed * TIMER_PERIOD_MS)

                    if ((hmi_keys[0] in {hmi.Keys.BUTTON_UP, hmi.Keys.BUTTON_DOWN}) and (dwell_time > MOVING_BUTTON_DELAY_TIME_MS)):
                        debug_msg = "Moving "
                        if (hmi.Keys.BUTTON_UP in hmi_keys):  
                            debug_msg += "up."                      
                            target_position = desk_upper_limit
                        
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys):
                            debug_msg += "down."
                            target_position = desk_lower_limit
                        
                        try:
                            desk.set_position(target_position)
                            debug(Verbosity.VERBOSE, debug_msg)
                        except Exception as e:
                            if (e.args[1] in {desk.Error.DESK_UPPER_LIMIT, desk.Error.DESK_LOWER_LIMIT}):
                                host_state["phase"] = RunningPhase.MOVING_ENDPOSITION
                                if (config["audio"] == True):
                                    buzzer_start()
                            else:
                                error_count += 1
                                debug_msg = "Missed set_position: " + e.args[0]
                                debug(Verbosity.DEBUG, debug_msg)
                        else:
                            error_count = 0
                            temp_button = hmi.Keys.BUTTON_NONE
                            host_state["phase"] = RunningPhase.MOVING_MANUAL


                    elif ((hmi_keys[0] in {hmi.Keys.BUTTON_1, hmi.Keys.BUTTON_2, hmi.Keys.BUTTON_3, hmi.Keys.BUTTON_4}) and (dwell_time > MEMORY_BUTTON_DELAY_TIME_MS)):
                        if (hmi.Keys.BUTTON_1 in hmi_keys):                        
                            target_position = config["desk_position_1"]
                            temp_button = hmi.Keys.BUTTON_1

                        elif (hmi.Keys.BUTTON_2 in hmi_keys):                        
                            target_position = config["desk_position_2"]
                            temp_button = hmi.Keys.BUTTON_2
                        
                        elif (hmi.Keys.BUTTON_3 in hmi_keys):                        
                            target_position = config["desk_position_3"]
                            temp_button = hmi.Keys.BUTTON_3
                        
                        elif (hmi.Keys.BUTTON_4 in hmi_keys):                        
                            target_position = config["desk_position_4"]
                            temp_button = hmi.Keys.BUTTON_4
                        
                        try:
                            desk.set_position(target_position)
                            debug_msg = "Moving to " + str(target_position)
                            debug(Verbosity.NORMAL, debug_msg)
                        except Exception as e:
                            error_count += 1
                            debug_msg = "Missed set_position: " + e.args[0]
                            debug(Verbosity.DEBUG, debug_msg)
                        else:
                            error_count = 0
                            host_state["phase"] = RunningPhase.MOVING_AUTOMATIC

                elif (len(hmi_keys) == 2):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                    settings_enter_delay = (timekeeper_button_pressed * TIMER_PERIOD_MS)

                    if (settings_enter_delay > SETTINGS_ENTER_DELAY_TIME_MS):
                        if ((hmi.Keys.BUTTON_UP and hmi.Keys.BUTTON_DOWN) in hmi_keys):
                            debug(Verbosity.VERBOSE, "Enter the display menu!")
                            host_state["op_mode"] = OperatingMode.SETTINGS
                            host_state["phase"] = SettingsPage.POSITION
                        
                        elif ((hmi.Keys.BUTTON_1 and hmi.Keys.BUTTON_2) in hmi_keys):
                            debug(Verbosity.VERBOSE, "Do calibration!")
                            try:
                                desk.calibrate()
                            except Exception as e:
                                debug_msg = "Calibration error: " + e.args[0]
                                debug(Verbosity.DEBUG, debug_msg)
                            else:
                                host_state["phase"] = RunningPhase.MOVING_CALIBRATION


            elif (host_state["phase"] == RunningPhase.MOVING_MANUAL):
                hmi_keys = hmi.get_keys()

                if (len(hmi_keys) != 1):
                    timekeeper_idle = 0
                    try:
                        desk.stop()
                        
                    except Exception as e: 
                        debug_msg = "Missed stop_command in moving_manual: " + e.args[0]
                        debug(Verbosity.DEBUG, debug_msg)
                    
                    else:
                        timekeeper_idle = 0
                        timekeeper_button_pressed = 0
                        host_state["phase"] = RunningPhase.READY
            

            elif (host_state["phase"] == RunningPhase.MOVING_AUTOMATIC):
                hmi_keys = hmi.get_keys()

                try:
                    desk_state = desk.get_state()
                except Exception as e:
                    debug_msg = "Could not read current desk state in moving_automatic: " + e.args[0]
                    debug(Verbosity.DEBUG, debug_msg)

                if (len(hmi_keys) == 0): 
                    if (temp_button != hmi.Keys.BUTTON_NONE):
                        temp_button = hmi.Keys.BUTTON_NONE
                
                elif ((len(hmi_keys) == 1) and (temp_button in hmi_keys)):
                    pass
            
                else:
                    try:
                        desk.stop()
                        
                    except Exception as e: 
                        debug_msg = "Missed stop_command in moving_automatic: " + e.args[0]
                        debug(Verbosity.DEBUG, debug_msg)
                    
                    else:
                        timekeeper_idle = 0
                        timekeeper_button_pressed = 0
                        host_state["phase"] = RunningPhase.READY
                    
                if (desk_state == desk.State.OPERATION_NORMAL):
                    timekeeper_idle = 0
                    timekeeper_button_pressed = 0
                    host_state["phase"] = RunningPhase.READY
                    if (config["audio"] == True):
                        buzzer_start()


            elif (host_state["phase"] == RunningPhase.MOVING_CALIBRATION):
                try:
                    desk_state = desk.get_state()
                except Exception as e:
                    debug_msg = "Could not read current desk state in moving_calibration: " + e.args[0]
                    debug(Verbosity.DEBUG, debug_msg)
                else:
                    if (desk_state == desk.State.OPERATION_NORMAL):
                        timekeeper_idle = 0
                        timekeeper_button_pressed = 0
                        host_state["phase"] = RunningPhase.READY
                        if (config["audio"] == True):
                            buzzer_start()
            

            elif (host_state["phase"] == RunningPhase.MOVING_ENDPOSITION):
                hmi_keys = hmi.get_keys()

                if (len(hmi_keys) == 0): 
                    host_state["phase"] = RunningPhase.READY
            

            elif (host_state["phase"] == RunningPhase.PRE_SLEEP):
                timekeeper_idle = 0
                timekeeper_button_pressed = 0
                host_state["phase"] = RunningPhase.SLEEP


        elif (host_state["op_mode"] == OperatingMode.SETTINGS):
            try:
                desk.get_state()  

            except Exception as e:
                error_count += 1
                debug_msg = "Could not read desk state: " + e.args[0]
                debug(Verbosity.DEBUG, debug_msg)
            
            else:
                error_count = 0
            
            hmi_keys = hmi.get_keys()

            if (host_state["phase"] == SettingsPage.POSITION):
                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    leave_time = (SETTINGS_LEAVE_DELAY_TIME_MS / TIMER_PERIOD_MS)
                    if (timekeeper_idle > leave_time):                        
                        write_memory_position(temp_button, desk_position)
                        save_config(config)
                        timekeeper_idle = 0
                        host_state["op_mode"] = OperatingMode.RUNNING
                        host_state["phase"] = RunningPhase.READY

                elif (len(hmi_keys) == 1):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                
                    if (timekeeper_button_pressed == 1):

                        if (hmi.Keys.BUTTON_1 in hmi_keys):   
                            hmi.update_character("1", 0)                     
                            temp_button = hmi.Keys.BUTTON_1
                        
                        elif (hmi.Keys.BUTTON_2 in hmi_keys):   
                            hmi.update_character("2", 0)                     
                            temp_button = hmi.Keys.BUTTON_2
                        
                        elif (hmi.Keys.BUTTON_3 in hmi_keys):   
                            hmi.update_character("3", 0)                     
                            temp_button = hmi.Keys.BUTTON_3

                        elif (hmi.Keys.BUTTON_4 in hmi_keys):   
                            hmi.update_character("4", 0)                     
                            temp_button = hmi.Keys.BUTTON_4

                        elif (hmi.Keys.BUTTON_UP in hmi_keys): 
                            write_memory_position(temp_button, desk_position)
                            host_state["phase"] = SettingsPage.BRIGHTNESS
                    
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys): 
                            write_memory_position(temp_button, desk_position)
                            host_state["phase"] = SettingsPage.AUDIO


            elif (host_state["phase"] == SettingsPage.BRIGHTNESS):
                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    leave_time = (SETTINGS_LEAVE_DELAY_TIME_MS / TIMER_PERIOD_MS)
                    if (timekeeper_idle > leave_time):
                        timekeeper_idle = 0
                        host_state["op_mode"] = OperatingMode.RUNNING
                        host_state["phase"] = RunningPhase.READY
                
                elif (len(hmi_keys) == 1):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                
                    if (timekeeper_button_pressed == 1):

                        if (hmi.Keys.BUTTON_1 in hmi_keys):  
                            display_brightness(1)
                            hmi.update_character("1", 0)
                            config["display_brightness"] = 1
                        
                        elif (hmi.Keys.BUTTON_2 in hmi_keys):  
                            display_brightness(2)
                            hmi.update_character("2", 0)
                            config["display_brightness"] = 2
                        
                        elif (hmi.Keys.BUTTON_3 in hmi_keys):  
                            display_brightness(3)
                            hmi.update_character("3", 0)
                            config["display_brightness"] = 3

                        elif (hmi.Keys.BUTTON_4 in hmi_keys):  
                            display_brightness(4)
                            hmi.update_character("4", 0)
                            config["display_brightness"] = 4
                        
                        elif (hmi.Keys.BUTTON_UP in hmi_keys):   
                            host_state["phase"] = SettingsPage.UNIT
                    
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys):   
                            host_state["phase"] = SettingsPage.POSITION
                    

            elif (host_state["phase"] == SettingsPage.UNIT):
                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    leave_time = (SETTINGS_LEAVE_DELAY_TIME_MS / TIMER_PERIOD_MS)
                    if (timekeeper_idle > leave_time):
                        save_config(config)
                        timekeeper_idle = 0
                        host_state["op_mode"] = OperatingMode.RUNNING
                        host_state["phase"] = RunningPhase.READY
                
                elif (len(hmi_keys) == 1):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                
                    if (timekeeper_button_pressed == 1):

                        if (hmi.Keys.BUTTON_1 in hmi_keys):  
                            hmi.update_character(" ", 1) 
                            hmi.update_character("c", 0)
                            config["display_unit"] = "cm"
                        
                        elif (hmi.Keys.BUTTON_2 in hmi_keys):   
                            hmi.update_character("i", 1)
                            hmi.update_character("n", 0)
                            config["display_unit"] = "inch" 

                        elif (hmi.Keys.BUTTON_UP in hmi_keys):   
                            host_state["phase"] = SettingsPage.OFFSET
                    
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys):   
                            host_state["phase"] = SettingsPage.BRIGHTNESS
            

            elif (host_state["phase"] == SettingsPage.OFFSET):
                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    leave_time = (SETTINGS_LEAVE_DELAY_TIME_MS / TIMER_PERIOD_MS)
                    if (timekeeper_idle > leave_time):
                        save_config(config)
                        timekeeper_idle = 0
                        host_state["op_mode"] = OperatingMode.RUNNING
                        host_state["phase"] = RunningPhase.READY
                
                elif (len(hmi_keys) == 1):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                
                    if (timekeeper_button_pressed == 1):

                        if (hmi.Keys.BUTTON_2 in hmi_keys):  
                            if (config["desk_offset"] < 10):
                                config["desk_offset"] += 1
                                abs_value = 0
                                if (config["desk_offset"] < 0):
                                    hmi.update_character("-", 1)
                                    abs_value = (config["desk_offset"] * (-1))
                                else:
                                    hmi.update_character(" ", 1)
                                    abs_value = config["desk_offset"]
                                
                                hmi.update_character(str(abs_value), 0)

                        elif (hmi.Keys.BUTTON_1 in hmi_keys):  
                            if (config["desk_offset"] > -10): 
                                config["desk_offset"] -= 1
                                abs_value = 0
                                if (config["desk_offset"] < 0):
                                    hmi.update_character("-", 1)
                                    abs_value = (config["desk_offset"] * (-1))
                                else:
                                    hmi.update_character(" ", 1)
                                    abs_value = config["desk_offset"]
                                
                                hmi.update_character(str(abs_value), 0)               

                        elif (hmi.Keys.BUTTON_UP in hmi_keys):   
                            host_state["phase"] = SettingsPage.ON_TIME 
                    
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys):   
                            host_state["phase"] = SettingsPage.UNIT

            
            elif (host_state["phase"] == SettingsPage.ON_TIME):
                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    leave_time = (SETTINGS_LEAVE_DELAY_TIME_MS / TIMER_PERIOD_MS)
                    if (timekeeper_idle > leave_time):
                        save_config(config)
                        timekeeper_idle = 0
                        host_state["op_mode"] = OperatingMode.RUNNING
                        host_state["phase"] = RunningPhase.READY
                
                elif (len(hmi_keys) == 1):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                
                    if (timekeeper_button_pressed == 1):

                        if (hmi.Keys.BUTTON_2 in hmi_keys):  
                            if (config["display_on_time"] < 20):
                                config["display_on_time"] += 5

                                if (config["display_on_time"] == 10):
                                    hmi.update_character("1", 1)
                                    hmi.update_character("0", 0)

                                elif (config["display_on_time"] == 15):
                                    hmi.update_character("1", 1)
                                    hmi.update_character("5", 0)

                                elif (config["display_on_time"] == 20):
                                    hmi.update_character("2", 1)
                                    hmi.update_character("0", 0)
                                
                        elif (hmi.Keys.BUTTON_1 in hmi_keys):  
                            if (config["display_on_time"] > 5):
                                config["display_on_time"] -= 5

                                if (config["display_on_time"] == 5):
                                    hmi.update_character(" ", 1)
                                    hmi.update_character("5", 0)

                                elif (config["display_on_time"] == 10):
                                    hmi.update_character("1", 1)
                                    hmi.update_character("0", 0)

                                elif (config["display_on_time"] == 15):
                                    hmi.update_character("1", 1)
                                    hmi.update_character("5", 0)      

                        elif (hmi.Keys.BUTTON_UP in hmi_keys):   
                            host_state["phase"] = SettingsPage.AUDIO
                    
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys):   
                            host_state["phase"] = SettingsPage.OFFSET
            

            elif (host_state["phase"] == SettingsPage.AUDIO):
                if (len(hmi_keys) == 0):
                    timekeeper_idle += 1
                    timekeeper_button_pressed = 0
                    leave_time = (SETTINGS_LEAVE_DELAY_TIME_MS / TIMER_PERIOD_MS)
                    if (timekeeper_idle > leave_time):
                        save_config(config)
                        timekeeper_idle = 0
                        host_state["op_mode"] = OperatingMode.RUNNING
                        host_state["phase"] = RunningPhase.READY
                
                elif (len(hmi_keys) == 1):
                    timekeeper_idle = 0
                    timekeeper_button_pressed += 1
                
                    if (timekeeper_button_pressed == 1):

                        if (hmi.Keys.BUTTON_1 in hmi_keys):  
                            hmi.update_character("o", 1) 
                            hmi.update_character("n", 0)
                            config["audio"] = True
                        
                        elif (hmi.Keys.BUTTON_2 in hmi_keys):   
                            hmi.update_character("o", 1)
                            hmi.update_character("F", 0)
                            config["audio"] = False

                        elif (hmi.Keys.BUTTON_UP in hmi_keys):   
                            host_state["phase"] = SettingsPage.POSITION
                    
                        elif (hmi.Keys.BUTTON_DOWN in hmi_keys):   
                            host_state["phase"] = SettingsPage.ON_TIME
        

        elif (host_state["op_mode"] == OperatingMode.RESTART):
            # do not make any requests for more than 512ms to the desk controller. 
            # the watchdog timer will reset the controller so we can startup again afterwards. 
            debug_msg = "Restart Delay Cycle: " + str(restart_delay_count)
            debug(Verbosity.DEBUG, debug_msg)
            hmi.silent_indicator(True)
            restart_delay_count += 1

            if (restart_delay_count > 10):
                debug(Verbosity.NORMAL, "Trying to re-init the desk controller...")
                error_count = 0
                host_state["op_mode"] = OperatingMode.IDLE
                host_state["phase"] = OperatingPhase.NONE

        else:
            pass

        wdt.feed()
        display_update() 
        timer_lapsed = False        
        timer_start(TIMER_PERIOD_MS)
