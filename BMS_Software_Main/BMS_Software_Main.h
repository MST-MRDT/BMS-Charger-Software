
#ifndef BMS_Software_Main
#define BMS_Software_Main

#include <Energia.h>
#include "RoveComm.h"
//RoveCommEthernetUdp RoveComm; 

// Pinmap // 

//Control Pins
#define BUZZER_CTRL_PIN        PM_1    // change after new tiva
#define FAN_CTRL_PIN           PK_0
#define PACK_OUT_CTRL_PIN      PB_1
#define LOGIC_SWITCH_CTRL_PIN  PA_4

//Indicator Pins
#define FAN_PWR_IND_PIN       PM_2    // change after new tiva
#define SW_IND_PIN            PB_0
#define SW_ERR_IND_PIN        PA_6

//Sensor Volts/Amps Reading Pins
#define PACK_I_MEAS_PIN       PE_1
#define PACK_V_MEAS_PIN       PE_2    //Measures voltage outputting to rover. Will read zero if PACK_OUT_CTL_PIN is LOW.
#define LOGIC_V_MEAS_PIN      PE_3    //Measures total voltage of pack continuously while logic switch is on. 
#define TEMP_degC_MEAS_PIN    PE_0

//Cell Voltage Pins
#define C1_V_MEAS_PIN         PE_5    //CELL 1 - GND
#define C2_V_MEAS_PIN         PE_4    //CELL 2 - CELL 1
#define C3_V_MEAS_PIN         PB_4    //CELL 3 - CELL 2
#define C4_V_MEAS_PIN         PB_5    //CELL 4 - CELL 3
#define C5_V_MEAS_PIN         PD_3    //CELL 5 - CELL 4
#define C6_V_MEAS_PIN         PD_2    //CELL 6 - CELL 5
#define C7_V_MEAS_PIN         PD_1    //CELL 7 - CELL 6
#define C8_V_MEAS_PIN         PD_0    //CELL 8 - CELL 7
const int CELL_MEAS_PINS[] = {C1_V_MEAS_PIN,C2_V_MEAS_PIN,C3_V_MEAS_PIN,C4_V_MEAS_PIN,C5_V_MEAS_PIN,C6_V_MEAS_PIN,C7_V_MEAS_PIN,C8_V_MEAS_PIN};

//Sensor Measurement: Constants and Calculations//
//Tiva
#define VCC                   3300   //mV

//Current Sensor
#define SENSOR_SENSITIVITY    0.0066 //mV or mA
#define SENSOR_SCALE          0.5
#define SENSOR_BIAS           VCC * SENSOR_SCALE

#define CURRENT_MAX           200000 //mA
#define CURRENT_MIN          -196207 //mA
#define OVERCURRENT           100000 //mA
#define CURRENT_ADC_MIN       0      //bits
#define CURRENT_ADC_MAX       4096   //bits

static int num_overcurrent = 0;
static bool overcurrent_state = false;
static float time_of_overcurrent = 0;

//Voltage measurements
#define VOLTS_MIN             0      //mV
#define CELL_VOLTS_MIN        2400   //mV
#define PACK_VOLTS_MAX        33600  //mV
#define CELL_VOLTS_MAX        4200   //mV
#define PACK_UNDERVOLTAGE     21600  //mV
#define PACK_LOWVOLTAGE       25000  //mV
#define PACK_SAFETY_LOW       PACK_UNDERVOLTAGE - 4000 //mV
#define CELL_SAFETY_LOW       (PACK_SAFETY_LOW)/8 //mV
#define CELL_UNDERVOLTAGE     2650   //mV
#define PACK_EFFECTIVE_ZERO   5000   //mV
#define CELL_EFFECTIVE_ZERO   1000   //mV
#define PACK_V_ADC_MIN        0      //bits
#define PACK_V_ADC_MAX        4096   //bits 
#define CELL_V_ADC_MIN        2450   //bits
#define CELL_V_ADC_MAX        4270   //bits

static bool pack_undervoltage_state = false;
static bool cell_undervoltage_state = false;
static bool low_voltage_state = false;
static int num_low_voltage_reminder = 0;
static int time_of_low_voltage = 0;

//Temp 
#define TEMP_MIN              0     //mdeg C
#define TEMP_MAX              140000 //mdeg
#define TEMP_THRESHOLD        38000  //mdeg C
#define TEMP_ADC_MIN          0      //bits 
#define TEMP_ADC_MAX          4096   //bits
#define NUM_TEMP_AVERAGE      10     //batt_temp will be average of this many measurements

static int num_meas_batt_temp = 0;
static bool batt_temp_avail = false;
static bool overtemp_state = false;
static bool fans_on = false;

//Delay Constants
#define ROVECOMM_DELAY        5        //msec    
#define DEBOUNCE_DELAY        10       //msec  
#define RESTART_DELAY         5000     //msec   
#define RECHECK_DELAY         10000    //msec  
#define LOGIC_SWITCH_REMINDER   60000  //msec 
#define IDLE_SHUTOFF_TIME     2400000  //msec 
#define UPDATE_ON_LOOP        60       //loops   
#define ROVECOMM_UPDATE_DELAY 420      //ms

//Logic Switch
static bool forgotten_logic_switch = false;
static int num_out_voltage_loops = 0;
static int time_switch_forgotten = 0;
static int time_switch_reminder = 0;
static bool estop_released_beep = false;


//Average 

//Error

void setInputPins();

void setOutputPins();

void setOutputStates();

float getTemperature();
float cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT] getCellVoltages();
float getPackVoltage(float cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT]);
float getCurrent();

#endif
