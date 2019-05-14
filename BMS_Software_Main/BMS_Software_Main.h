// Battery Managment System (BMS) Software ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Main Header File
//
// Created for 2019 Valkyrie by: Jacob Lipina, jrlwd5
//
//
// Libraries /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#ifndef BMS_Software_Main
#define BMS_Software_Main

#include <Energia.h>
#include "RoveComm.h"

RoveCommEthernetUdp RoveComm; //extantiates a class

// Pinmap ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Control Pins
#define PACK_OUT_CTR_PIN    	PD_1
#define LOGIC_SWITCH_CTR_PIN  	PD_0
#define BUZZER_CTR_PIN      	PN_2
#define FAN_1_CTR_PIN     		PH_2
#define FAN_2_CTR_PIN     		PH_3
#define FAN_3_CTR_PIN     		PL_5
#define FAN_4_CTR_PIN     		PL_4
#define FAN_PWR_IND_PIN     	PF_1
#define SW_IND_PIN        		PQ_3
#define SW_ERR_PIN        		PP_3

// Sensor Volts/Amps Readings Pins
#define PACK_I_MEAS_PIN     	PE_0
#define PACK_V_MEAS_PIN     	PE_1 //Measures voltage outputting to rover. Will read zero if PACK_OUT_CTR is LOW.
#define LOGIC_V_MEAS_PIN    	PE_2 //Measures total voltage of pack continuously while logic switch is on. 
#define TEMP_degC_MEAS_PIN    	PE_3 //Board routes to PM_3 but its not an analog pin. Must fix in hardware design
#define C1_V_MEAS_PIN     		PK_3 //CELL 1 - GND
#define C2_V_MEAS_PIN     		PK_2 //CELL 2 - CELL 1
#define C3_V_MEAS_PIN     		PK_1 //CELL 3 - CELL 2
#define C4_V_MEAS_PIN     		PK_0 //CELL 4 - CELL 3
#define C5_V_MEAS_PIN     		PB_5 //CELL 5 - CELL 4
#define C6_V_MEAS_PIN     		PB_4 //CELL 6 - CELL 5
#define C7_V_MEAS_PIN     		PD_5 //CELL 7 - CELL 6
#define C8_V_MEAS_PIN     		PD_4 //CELL 8 - CELL 7

const int CELL_MEAS_PINS[] = {LOGIC_V_MEAS_PIN, C1_V_MEAS_PIN, C2_V_MEAS_PIN, C3_V_MEAS_PIN, 
                C4_V_MEAS_PIN, C5_V_MEAS_PIN, C6_V_MEAS_PIN, C7_V_MEAS_PIN, C8_V_MEAS_PIN};

// Sensor Measurment: Constants and Calculations /////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tiva1294C RoveBoard Specs
#define VCC                   	3300 //mV
//#define TIVA_ADC_MAX          4096 //bits
//#define TIVA_MIN              0 //bits

// ACS759ECB-200B-PFF-T Current Sensor Specs
  // Find at: https://www.digikey.com/products/en?keywords=%20620-1466-5-ND%20
#define SENSOR_SENSITIVITY   	0.0066 //mV/mA
#define SENSOR_SCALE         	0.5
#define SENSOR_BIAS          	VCC * SENSOR_SCALE //Viout voltage when current is at 0A (aka quiescent output voltage)
                          						 // Noise is 2mV, meaning the smallest current that the device is able to resolve is 0.3A
#define CURRENT_MAX           	200000//mA; Current values must be sent over RoveComm as mA
#define CURRENT_MIN           	-196207//mA
#define OVERCURRENT         	50000 //mA
#define CURRENT_ADC_MIN     	0 //bits
#define CURRENT_ADC_MAX     	4096 //bits

// Voltage Measurments
#define VOLTS_MIN             	0 //mV		//Lowest possible pack voltage value we could measure. 
#define CELL_VOLTS_MIN        	2400//mV	//Lowest cell voltage I expect to be able to measure.
#define PACK_VOLTS_MAX        	33600 //mV	//Highest possible pack voltage value we could measure.
#define CELL_VOLTS_MAX      	4200 //mV	//Highest possible cell voltage value we could measure.
#define PACK_UNDERVOLTAGE   	21600 //mV	//If pack voltage reaches this voltage, turn off rover and BMS suicide.
#define PACK_LOWVOLTAGE     	25000 //mV	//If pack voltage reaches this voltage, notify of low voltage.
#define PACK_SAFETY_LOW     	PACK_UNDERVOLTAGE - 4000 //mV	//Even though I shouldn't be need to measure a voltage lower than undervoltage 
#define CELL_UNDERVOLTAGE   	2650 //mV	//If any cell voltage reaches this voltage, turn off rover and BMS suicide.
#define PACK_EFFECTIVE_ZERO   	5000 //mV 	//Pack voltage below this value is considered to be adc noise and not valid measurments. This is because the battery cannot physically be this low. So there must be an error with reading the voltage
#define CELL_EFFECTIVE_ZERO   	1000 //mV   //Cell voltage below this value is considered to be adc noise and not valid measurments.
#define PACK_V_ADC_MIN      	0 //bits
#define PACK_V_ADC_MAX      	4096 //bits
#define CELL_V_ADC_MIN      	2320 //bits
#define CELL_V_ADC_MAX      	3790 //bits

// TMP37 Temp Sensor Specs 
  //Find at: https://www.digikey.com/products/en?mpart=TMP37FT9Z&v=505
    //Voltage at 20 deg C is 500mV.
    //Scale Factor is 20mV/deg C.
#define TEMP_MIN          		0 //mdeg C
#define TEMP_MAX          		160000 //mdeg 	//Max temp than sensor can measure to and should correspond to adc value 4096.
#define TEMP_THRESHOLD      	38000 //mdeg C  //About 100 degF
#define TEMP_ADC_MIN      		0 //bits
#define TEMP_ADC_MAX      		4096 //bits

// Delay Constants
#define ROVECOMM_DELAY      	5 //msec		//Used after RoveComm.Write() to give data time to send.
#define DEBOUNCE_DELAY      	10 //msec		//After an overcurrent or undervoltage is detected, delay by this and measure the pin again to confirm whether error is occurring.
#define RESTART_DELAY     		5000 //msec		//Used after first overcurrent as delay before turning rover back on.
#define RECHECK_DELAY    		10000 //msec	//Used to measure time since first overcurrent. If second overcurrent occurs within this delay time, BMS commits suicide.
#define LOGIC_SWITCH_REMINDER 	60000 //msec 	//Every cycle of this period of time, the buzzer notifys someone that logic switch was forgotten.
#define IDLE_SHUTOFF_TIME   	2400000 //msec or 40 minutes	//After this period of time passes, the BMS will commit suicide.
#define UPDATE_ON_LOOP     		169 //loops		//Each time this number of loops passes, SW_IND will blink and LCD will update.
#define ROVECOMM_UPDATE_DELAY	420 //ms

// Function Declarations ///////////////////////////////////////////////////////////

void setInputPins();


void setOutputPins();


void setOutputStates();


void getMainCurrent(uint16_t &main_current);


void getCellVoltage(uint16_t cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT]);


void getOutVoltage(int &pack_out_voltage);


void getBattTemp(uint32_t &batt_temp);


void updateLCD(uint16_t batt_temp, uint16_t cellVoltages[]);


void reactOverCurrent();


void reactUnderVoltage();


void reactOverTemp();


void reactForgottenLogicSwitch();


void reactLowVoltage(uint16_t cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT]);


void setEstop(uint8_t data);


void notifyEstop(); //Buzzer sound: beeeeeeeeeeeeeeeeeeeep beeeeeeeeeep beeeeep beeep bep


void notifyLogicSwitch(); //Buzzer sound: beeep beeep


void notifyReboot(); //Buzzer sound: beeeeeeeeeep beeep beeep


void notifyOverCurrent(); //Buzzer Sound: beeeeeeeeeeeeeeeeeeeeeeeeeeeeeep


void notifyUnderVoltage(); //Buzzer Sound: beeep beeep beeep beeep beeeeeeeeeeeeeeeeeeeep


void notifyLowVoltage(); //Buzzer Sound: beeep beeep beeep


void startScreen();


void stars();


void asterisks();


void movingRover();

#endif
