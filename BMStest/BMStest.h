#include <Energia.h>
#ifndef BMStest
#define BMStest

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

//Voltage measurements
#define PACK_VOLTS_MIN           0      //mV
#define CELL_VOLTS_MIN        2400   //mV
#define PACK_VOLTS_MAX        33600  //mV
#define CELL_VOLTS_MAX        4200   //mV
#define PACK_UNDERVOLTAGE     21600  //mV
#define PACK_LOWVOLTAGE       25000  //mV
#define PACK_SAFETY_LOW       PACK_UNDERVOLTAGE - 4000 //mV
#define CELL_UNDERVOLTAGE     2650   //mV
#define PACK_EFFECTIVE_ZERO   5000   //mV
#define CELL_EFFECTIVE_ZERO   1000   //mV
#define PACK_V_ADC_MIN        0      //bits
#define PACK_V_ADC_MAX        4096   //bits 
#define CELL_V_ADC_MIN        2450   //bits2755,2930
#define CELL_V_ADC_MAX        4270   //bits3450,3245

//Current Measurements
#define CURRENT_MAX           20000 //mA
#define CURRENT_MIN           -20000. //mA
#define OVERCURRENT           10000 //mA
#define CURRENT_ADC_MIN       0      //bits
#define CURRENT_ADC_MAX       4096   //bits

//temperature measurements
#define TEMP_MIN              0      //mdeg C
#define TEMP_MAX              160000 //mdeg
#define TEMP_THRESHOLD        38000  //mdeg C
#define TEMP_ADC_MIN          0      //bits 
#define TEMP_ADC_MAX          4096   //bits
#define NUM_TEMP_AVERAGE      10     //batt_temp will be average of this many measurements

static float cell_voltage[8];

//Average 
float cell_adc_average[8];
int count;
int countMax;
float pack_adc_v_average;
float pack_adc_i_average;
float temp_adc_average;
float pack_voltage;


int beep_time = 3000;

void setInputPins();

void setOutputPins();

void setOutputStates();

void getCellVoltage(float cell_voltage[],int count, float &adc_average, float pack_voltage);

void getPackVoltage(float pack_adc_v_average, int count, float pack_voltage);

void getPackCurrent(float pack_adc_i_average);

void getTemperature(float temp_adc_average,int count);

void initTest(int beep_time);

void beep(int beep_time);

void turnOffFans();

void turnOnFans();


#endif
