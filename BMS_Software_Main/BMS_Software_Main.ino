// Battery Managment System (BMS) Software /////////////////////////////////////////
	// Main cpp File
//
// Created for 2019 Valkyrie by: Jacob Lipina, jrlwd5
//
//
// Libraries ///////////////////////////////////////////////////////////////////////
//
#include "BMS_Software_Functions.h" 
#include "BMS_Software_Main.h"

/*
// Standard C
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
*/ //May not need standard C

RoveCommEthernetUdp RoveComm; //Extantiates a class

// Function Declarations ////////////////////////////////////////////////////////////
//
void setInputPins();
void setOutputPins();
void setOutputStates();
void getMainCurrent(RC_BMSBOARD_MAINIMEASmA_DATATYPE &main_current);
void getCellVoltage(RC_BMSBOARD_VMEASmV_DATATYPE cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT]); //??How do the values in the array get back to the loop?
void getOutVoltage(int &pack_out_voltage);
void getBattTemp(RC_BMSBOARD_TEMPMEASmDEGC_DATATYPE &batt_temp);
bool singleDebounceCurrent(int bouncing_pin, int overcurrent_threshold);
bool singleDebounceVoltage(int bouncing_pin, int undervoltage_threshold, int volts_max, int volts_safety_low);
void checkOverCurrent(RC_BMSBOARD_EVENT_DATATYPE event_report[RC_BMSBOARD_EVENT_DATACOUNT]);
void checkUnderVoltage(RC_BMSBOARD_EVENT_DATATYPE event_report[RC_BMSBOARD_EVENT_DATACOUNT]);
void reactOverCurrent(RC_BMSBOARD_EVENT_DATATYPE event_report[RC_BMSBOARD_EVENT_DATACOUNT], int &num_overcurrent, float &time_of_overcurrent);
void reactUnderVoltage(RC_BMSBOARD_EVENT_DATATYPE event_report[RC_BMSBOARD_EVENT_DATACOUNT]);
void reactLowVoltage();
void reactOverTemp(RC_BMSBOARD_TEMPMEASmDEGC_DATATYPE batt_temp, bool &overtemp_state);
void reactForgottenLogicSwitch(int pack_out_voltage, bool &forgotten_logic_switch, int &time_switch_forgotten, int &time_switch_reminder);
void setEstop(RC_BMSBOARD_SWESTOPs_DATATYPE data);
void setFans(RC_BMSBOARD_FANEN_DATATYPE data);
void notifyEstop();
void notifyLogicSwitch();
void notifyReboot();
void notifyOverCurrent();
void notifyUnderVoltage();
void notifyLowVoltage();


void setup()
{
	Serial.begin(9600);
	Serial3.begin(9600);
	RoveComm.begin(RC_BMSBOARD_FOURTHOCTET);
	delay(ROVECOMM_DELAY);
	
	setInputPins();
	setOutputPins();
	setOutputStates();

	Serial.println("Setup Complete!");
} //end setup

void loop()
{
	RC_BMSBOARD_MAINIMEASmA_DATATYPE main_current;
	RC_BMSBOARD_VMEASmV_DATATYPE cell_voltages[RC_BMSBOARD_VMEASmV_DATACOUNT]; //??"cell_voltages." Is the 's' there for a reason?
	RC_BMSBOARD_TEMPMEASmDEGC_DATATYPE batt_temp;
	int pack_out_voltage;
	RC_BMSBOARD_EVENT_DATATYPE event_report[RC_BMSBOARD_EVENT_DATACOUNT];
  int num_overcurrent = 0;
  float time_of_overcurrent = 0;
  bool overtemp_state = false;
  //bool low_voltage_state = false;
  bool forgotten_logic_switch = false;
  int time_switch_forgotten = 0;
  int time_switch_reminder = 0;
  //int time_low_voltage_reminder = 0;
	rovecomm_packet packet;

	getMainCurrent(main_current);
	getCellVoltage(cell_voltages);
	getOutVoltage(pack_out_voltage);
	getBattTemp(batt_temp);
	
	RoveComm.write(RC_BMSBOARD_MAINIMEASmA_HEADER, main_current);
  delay(ROVECOMM_DELAY);
	RoveComm.write(RC_BMSBOARD_VMEASmV_HEADER, cell_voltages);
  delay(ROVECOMM_DELAY);
	RoveComm.write(RC_BMSBOARD_TEMPMEASmDEGC_HEADER, batt_temp);
  delay(ROVECOMM_DELAY);

  checkOverCurrent(event_report);
  checkUnderVoltage(event_report);

  RoveComm.write(RC_BMSBOARD_EVENT_HEADER, event_report);

  reactOverCurrent(event_report, num_overcurrent, time_of_overcurrent);
  reactUnderVoltage(event_report);
  //reactLowVoltage();
  reactOverTemp(batt_temp, overtemp_state);
  reactForgottenLogicSwitch(pack_out_voltage, forgotten_logic_switch, time_switch_forgotten, time_switch_reminder);

	packet = RoveComm.read();
  	if(packet.data_id!=0)
  	{
      Serial.println(packet.data_id);
      Serial.println(packet.data_count);
      for(int i = 0; i<packet.data_count; i++)
      {
        Serial.print(packet.data[i]);
      } //end for
    
      switch(packet.data_id) //andrew needs to fix the type to int for the dataid
      {
        case RC_BMSBOARD_SWESTOPs_DATAID:
        {
          setEstop(packet.data[0]);
          break;
        }
        case RC_BMSBOARD_FANEN_DATAID:
        {
          setFans(packet.data[0]);
          break;
        }
      } //end switch
    } //end if

} //end loop