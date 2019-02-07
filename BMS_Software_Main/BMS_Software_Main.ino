// Battery Managment System (BMS) Software /////////////////////////////////////////
	// Main cpp File
//
// Created for 2019 Valkyrie by: Jacob Lipina, jrlwd5
//
//
// Libraries ///////////////////////////////////////////////////////////////////////

#include "BMS_Software_Main.h"

// Setup & Main Loop ////////////////////////////////////////////////////////////
//
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
	uint16_t main_current;
	uint16_t cell_voltages[RC_BMSBOARD_VMEASmV_DATACOUNT];
	int pack_out_voltage;
  uint16_t batt_temp;
	uint8_t error_report[RC_BMSBOARD_ERROR_DATACOUNT];
  static int num_loop = 0;
  static bool sw_ind_state = false;
	rovecomm_packet packet;

	getMainCurrent(main_current);
	getCellVoltage(cell_voltages, error_report);
	getOutVoltage(pack_out_voltage);
	getBattTemp(batt_temp);
	
	RoveComm.write(RC_BMSBOARD_MAINIMEASmA_HEADER, main_current);
  delay(ROVECOMM_DELAY);
	RoveComm.write(RC_BMSBOARD_VMEASmV_HEADER, cell_voltages);
  delay(ROVECOMM_DELAY);
	RoveComm.write(RC_BMSBOARD_TEMPMEASmDEGC_HEADER, batt_temp);
  delay(ROVECOMM_DELAY);

  reactOverCurrent(error_report);
  reactUnderVoltage(error_report);
  //reactLowVoltage();
  reactOverTemp();
  reactForgottenLogicSwitch();

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
        //expand by adding more cases if more dataids are created
      } //end switch
    } //end if

    if(num_loop % BLINK_ON_LOOP == 0) //SW_IND led will blink while the code is looping
    {
      if(sw_ind_state == false)
      {
        digitalWrite(SW_IND_PIN, HIGH);
        sw_ind_state = true;
      }//end if
      if(sw_ind_state == true)
      {
        digitalWrite(SW_IND_PIN, LOW);
        sw_ind_state = false;
      }//end if
    }//end if

    num_loop++;
} //end loop



// Static Variables for Below Functions ////////////////////////////////////////////////////////////////
  //Current 
static int num_overcurrent = 0;
static bool overcurrent_state = false;
static float time_of_overcurrent = 0;
  //Voltage
//static int num_pack_undervoltage_measured = 0;
//static int num_cell_undervoltage_measured = 0;
static bool pack_undervoltage_state = false;
static bool cell_undervoltage_state = false;
//static bool low_voltage_state = false;
//int static time_low_voltage_reminder = 0;
  //Temp
static bool overtemp_state = false;
static bool fans_on = false;
  //Logic Switch
static bool forgotten_logic_switch = false;
static int num_out_voltage_loops = 0;
static int time_switch_forgotten = 0;
static int time_switch_reminder = 0;

// Functions ///////////////////////////////////////////////////////////////////////

void setInputPins()
{
  pinMode(PACK_I_MEAS_PIN,      INPUT);
  pinMode(PACK_V_MEAS_PIN,      INPUT);
  pinMode(LOGIC_V_MEAS_PIN,     INPUT);
  pinMode(TEMP_degC_MEAS_PIN,   INPUT);
  pinMode(C1_V_MEAS_PIN,        INPUT);
  pinMode(C2_V_MEAS_PIN,        INPUT);
  pinMode(C3_V_MEAS_PIN,        INPUT);
  pinMode(C4_V_MEAS_PIN,        INPUT);
  pinMode(C5_V_MEAS_PIN,        INPUT);
  pinMode(C6_V_MEAS_PIN,        INPUT);
  pinMode(C7_V_MEAS_PIN,        INPUT);
  pinMode(C8_V_MEAS_PIN,        INPUT);

  return;
}//end func

void setOutputPins()
{
  pinMode(PACK_OUT_CTR_PIN,     OUTPUT);
  pinMode(PACK_OUT_IND_PIN,     OUTPUT);
  pinMode(LOGIC_SWITCH_CTR_PIN, OUTPUT);
  pinMode(BUZZER_CTR_PIN,       OUTPUT);
  pinMode(FAN_1_CTR_PIN,        OUTPUT);
  pinMode(FAN_2_CTR_PIN,        OUTPUT);
  pinMode(FAN_3_CTR_PIN,        OUTPUT);
  pinMode(FAN_4_CTR_PIN,        OUTPUT);
  pinMode(FAN_PWR_IND_PIN,      OUTPUT);
  pinMode(SW_IND_PIN,           OUTPUT);
  pinMode(SW_ERR_PIN,           OUTPUT);

  return;
}//end func

void setOutputStates()
{
  digitalWrite(PACK_OUT_CTR_PIN,      LOW);
  digitalWrite(PACK_OUT_IND_PIN,      LOW);
  digitalWrite(LOGIC_SWITCH_CTR_PIN,  LOW);
  digitalWrite(BUZZER_CTR_PIN,        LOW);
  digitalWrite(FAN_1_CTR_PIN,         LOW);
  digitalWrite(FAN_2_CTR_PIN,         LOW);
  digitalWrite(FAN_3_CTR_PIN,         LOW);
  digitalWrite(FAN_4_CTR_PIN,         LOW);
  digitalWrite(FAN_PWR_IND_PIN,       LOW);
  digitalWrite(SW_IND_PIN,            LOW);
  digitalWrite(SW_ERR_PIN,            LOW);
  
  return;
}//end func

void getMainCurrent(uint16_t &main_current)
{
  main_current = map(analogRead(PACK_I_MEAS_PIN), CURRENT_ADC_MIN, CURRENT_ADC_MAX, CURRENT_MIN, CURRENT_MAX);

  if(main_current > OVERCURRENT)
  {
    delay(DEBOUNCE_DELAY);

    if(map(analogRead(PACK_I_MEAS_PIN), CURRENT_ADC_MIN, CURRENT_ADC_MAX, CURRENT_MIN, CURRENT_MAX) > OVERCURRENT)
    {
      overcurrent_state = true;

      if(num_overcurrent == 1)
      {
        num_overcurrent++;
      }//end if 
    }//end if 
  }//end if
  return;
}//end func

void getCellVoltage(uint16_t cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT], uint8_t error_report[RC_BMSBOARD_ERROR_DATACOUNT])
{   
  for(int i = 0; i<RC_BMSBOARD_VMEASmV_DATACOUNT; i++)
  {
    if (i == RC_BMSBOARD_VMEASmV_PACKENTRY)
    {
      cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] = map(analogRead(CELL_MEAS_PINS[RC_BMSBOARD_VMEASmV_PACKENTRY]), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX);
   
      if(cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] < PACK_SAFETY_LOW)
      {
        delay(DEBOUNCE_DELAY);

        if(map(analogRead(CELL_MEAS_PINS[RC_BMSBOARD_VMEASmV_PACKENTRY]), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX) < PACK_SAFETY_LOW)
        {
          pack_undervoltage_state = true;
  
          error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_UNDERVOLTAGE;
          RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
          delay(DEBOUNCE_DELAY);
        }//end if
      }//end if
      if(cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] < PACK_EFFECTIVE_ZERO)
      {
        error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_PINFAULT;
        RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
        delay(DEBOUNCE_DELAY);
      }//end if
    }//end if
    else
    {
      cell_voltage[i] = map(analogRead(CELL_MEAS_PINS[i]), CELL_V_ADC_MIN, CELL_V_ADC_MAX, VOLTS_MIN, CELL_VOLTS_MAX);

      if(cell_voltage[i] < CELL_SAFETY_LOW)
      {
        delay(DEBOUNCE_DELAY);

        if(map(analogRead(CELL_MEAS_PINS[i]), CELL_V_ADC_MIN, CELL_V_ADC_MAX, VOLTS_MIN, CELL_VOLTS_MAX) < CELL_SAFETY_LOW)
        {
          cell_undervoltage_state = true;
  
          error_report[i] = RC_BMSBOARD_ERROR_UNDERVOLTAGE;
          RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
          delay(DEBOUNCE_DELAY);
        }//end if
      }//end if
      if(cell_voltage[i] < CELL_EFFECTIVE_ZERO)
      {
        error_report[i] = RC_BMSBOARD_ERROR_PINFAULT;
        RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
        delay(DEBOUNCE_DELAY);
      }
    }//end if
    return;
  }//end for
}//end func

void getOutVoltage(int &pack_out_voltage)
{
  pack_out_voltage = map(analogRead(PACK_V_MEAS_PIN), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX);

  if(pack_out_voltage > PACK_SAFETY_LOW)
  {
    forgotten_logic_switch = false;
    time_switch_forgotten = 0;
  }
  if(pack_out_voltage < PACK_EFFECTIVE_ZERO)
  {
    delay(DEBOUNCE_DELAY);

    if(map(analogRead(PACK_V_MEAS_PIN), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX))
    {
      forgotten_logic_switch = true;
      num_out_voltage_loops++;
    } 
  }
  return;
}//end func

void getBattTemp(uint16_t &batt_temp)
{
  batt_temp = map(analogRead(TEMP_degC_MEAS_PIN), TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX);

  if(batt_temp < TEMP_THRESHOLD)
  {
    overtemp_state = false;

  }
  if(batt_temp > TEMP_THRESHOLD)
  {
    delay(DEBOUNCE_DELAY);

    if(map(analogRead(TEMP_degC_MEAS_PIN), TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX) > TEMP_THRESHOLD)
    {
      overtemp_state = true;
    } 
  }
  return;
}//end func

void reactOverCurrent(uint8_t error_report[RC_BMSBOARD_ERROR_DATACOUNT])
{ //TODO: RED will see overcurrent for 10sec before recheck time is up.
  if(overcurrent_state == false)
  {
    error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_OVERCURRENT;
  }//end if
  if(overcurrent_state == true)
  {
    switch(num_overcurrent)
    {
      case 0:
        error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_OVERCURRENT;
        RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
        delay(ROVECOMM_DELAY);
    
        digitalWrite(PACK_OUT_CTR_PIN, LOW);
        digitalWrite(PACK_OUT_IND_PIN, LOW);
    
        time_of_overcurrent = millis(); 
        num_overcurrent++;
    
        notifyOverCurrent();
        break;
      
      case 1:
        if(millis() >= (time_of_overcurrent + RESTART_DELAY))
        {
          digitalWrite(PACK_OUT_CTR_PIN, HIGH);
          digitalWrite(PACK_OUT_IND_PIN, HIGH);
        }//end if
        if(millis() >= (time_of_overcurrent + RESTART_DELAY + RECHECK_DELAY))
        {
          overcurrent_state = false;
          time_of_overcurrent = 0;
        }//end if
      
      case 2:
        error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_OVERCURRENT;
        RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
        delay(ROVECOMM_DELAY);
      
        digitalWrite(PACK_OUT_CTR_PIN, LOW);
        digitalWrite(PACK_OUT_IND_PIN, LOW);

        notifyOverCurrent();

        digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide
        break;
    break;
    }//end switch
  }//end if
  return;
}//end func

void reactUnderVoltage(uint8_t error_report[RC_BMSBOARD_ERROR_DATACOUNT])
{
  if(pack_undervoltage_state == true || cell_undervoltage_state == true)
  {
    digitalWrite(PACK_OUT_CTR_PIN, LOW);
    digitalWrite(PACK_OUT_IND_PIN, LOW);

    notifyUnderVoltage();

    digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide
  }
  return;
}//end func

/*void reactLowVoltage(uint16_t cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT], bool &low_voltage_state, int &time_low_voltage_reminder)
{
  if(low_voltage_state == false && (cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] <= PACK_UNDERVOLTAGE) && (cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] > PACK_UNDERVOLTAGE))
  {

  }
  break;
}//end func*/

void reactOverTemp()
{
  if(overtemp_state == true && fans_on == false) 
  {   
    fans_on = true;               
    digitalWrite(FAN_1_CTR_PIN, HIGH);
    digitalWrite(FAN_2_CTR_PIN, HIGH);
    digitalWrite(FAN_3_CTR_PIN, HIGH);
    digitalWrite(FAN_4_CTR_PIN, HIGH);
    digitalWrite(FAN_PWR_IND_PIN, HIGH);
  }
  if(overtemp_state == false && fans_on == true)
  {
    fans_on = false;
    digitalWrite(FAN_1_CTR_PIN, LOW);
    digitalWrite(FAN_2_CTR_PIN, LOW);
    digitalWrite(FAN_3_CTR_PIN, LOW);
    digitalWrite(FAN_4_CTR_PIN, LOW);
    digitalWrite(FAN_PWR_IND_PIN, LOW);
  }
  return;
}//end func

void reactForgottenLogicSwitch()
{
  if(forgotten_logic_switch == true)
  {
    if(num_out_voltage_loops == 1)
    {
      time_switch_forgotten = millis();
      time_switch_reminder = millis();
    }
    if(num_out_voltage_loops > 1)
    {
      if(millis() >= time_switch_reminder + LOGIC_SWITCH_REMINDER)
      {
        time_switch_reminder = millis();
        notifyLogicSwitch();
      }//end if
      if(millis() >= time_switch_forgotten + IDLE_SHUTOFF_TIME)
      {
        digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide 
      }//end if   
    }//end if 
  }//end if
  return;
}//end func

void setEstop(uint8_t data)
{
  if(data == 0)
  {
    digitalWrite(PACK_OUT_CTR_PIN, LOW);
    digitalWrite(PACK_OUT_IND_PIN, LOW);
    
    notifyEstop();

    digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide
    //If BMS is not turned off here, the PACK_OUT_CTR_PIN would be low and there would be no way to get it high again without reseting BMS anyway.
  }
  else
  {
    digitalWrite(PACK_OUT_CTR_PIN, LOW);
    digitalWrite(PACK_OUT_IND_PIN, LOW);

    notifyReboot();

    delay(data);

    digitalWrite(PACK_OUT_CTR_PIN, HIGH);
    digitalWrite(PACK_OUT_IND_PIN, HIGH);
  } //end if
  return;
}//end func

void notifyEstop() //Buzzer sound: beeeeeeeeeeeeeeeeeeeep beeeeeeeeeep beeeeep beeep bep
{
  Serial.println("notifyEstop();");

  /*digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);*/

  return;
}//end func

void notifyLogicSwitch() //Buzzer sound: beeep beeep
{
  Serial.println("notifyLogicSwitch();");

  /*digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);*/

  return;
}//end func

void notifyReboot() //Buzzer sound: beeeeeeeeeep beeep beeep
{
  Serial.println("notifyReboot();");

  /*digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);*/

  return;
}//end func

void notifyOverCurrent() //Buzzer Sound: beeeeeeeeeeeeeeeeeeeeeeeeeeeeeep
{
  Serial.println("notifyOverCurrent();");

  /*digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(3000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);*/

  return;
}//end func

void notifyUnderVoltage() //Buzzer Sound: beeep beeep beeep beeep beeeeeeeeeeeeeeeeeeeep
{
  Serial.println("notifyUnderVoltage();");

  /*digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);*/

  return;
}//end func

void notifyLowVoltage() //Buzzer Sound: beeep beeep beeep
{
  Serial.println("notifyLowVoltage();");

  /*digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(250);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);*/

  return;
}//end func
