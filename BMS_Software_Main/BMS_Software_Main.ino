// Battery Managment System (BMS) Software //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Main cpp File
//
// Created for 2019 Valkyrie by: Jacob Lipina, jrlwd5
//
//
// Libraries ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "BMS_Software_Main.h"

// Setup & Main Loop ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
uint8_t error_report[RC_BMSBOARD_ERROR_DATACOUNT] = {0,0,0,0,0,0,0,0,0};
bool pinfault_state = false;
int num_loop = 0;
bool sw_ind_state = false;
uint32_t time_since_Rovecomm_update = 0;
uint32_t meas_batt_temp[NUM_TEMP_AVERAGE];

void setup()
{
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial3.begin(9600);
  startScreen();
  RoveComm.begin(RC_BMSBOARD_FOURTHOCTET);
  delay(ROVECOMM_DELAY);

  setInputPins();
  setOutputPins();
  setOutputStates();

  Serial.println("Setup Complete.");
} //end setup

void loop()
{
  int32_t main_current;
  uint16_t cell_voltages[RC_BMSBOARD_VMEASmV_DATACOUNT];
  int pack_out_voltage;
  uint32_t batt_temp;
  rovecomm_packet packet;


  getMainCurrent(main_current);
  reactOverCurrent();

  getCellVoltage(cell_voltages);
  reactUnderVoltage();
  reactLowVoltage(cell_voltages);

  getOutVoltage(pack_out_voltage);
  reactForgottenLogicSwitch();

  getBattTemp(batt_temp);
  reactOverTemp();

  if((millis() - time_since_Rovecomm_update) >= ROVECOMM_UPDATE_DELAY)
  {  
    // batt_temp = 23456;//just for fixin reds temp values
    //Serial.println(batt_temp);
    RoveComm.write(RC_BMSBOARD_MAINIMEASmA_HEADER, main_current);
    delay(ROVECOMM_DELAY);
    RoveComm.write(RC_BMSBOARD_VMEASmV_HEADER, cell_voltages);
    delay(ROVECOMM_DELAY);
    RoveComm.write(RC_BMSBOARD_TEMPMEASmDEGC_HEADER, batt_temp);
    delay(ROVECOMM_DELAY);

    if(pinfault_state == true)
    {
      RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
      delay(DEBOUNCE_DELAY);
    }
    
    time_since_Rovecomm_update = millis();
  }

  packet = RoveComm.read();
  if(packet.data_id!=0)
  {
    switch(packet.data_id)
    {
      case RC_BMSBOARD_SWESTOPs_DATAID:
      {
        setEstop(packet.data[0]);
        break;
      }
      //expand by adding more cases if more dataids are created
    } //end switch
  } //end if

  if((num_loop % UPDATE_ON_LOOP) == 0) //SW_IND led will blink while the code is looping and LCD will update
  {
    if(sw_ind_state == false)
    {
      digitalWrite(SW_IND_PIN, HIGH);
      sw_ind_state = true;
    }//end if
    else
    {
      digitalWrite(SW_IND_PIN, LOW);
      sw_ind_state = false;
    }//end if
    updateLCD(batt_temp, cell_voltages);
  }//end if

  num_loop++;                  //Weird phenomenon: If you don't output num_loop on the serial monitor, then whenever
  Serial.print("num_loop: ");  //the estop is released, num_loop increments much faster than the program loops (somehow) 
  Serial.println(num_loop);    //and thus the SW_LED blinks and LCD updates much too fast.
} //end loop


// Static Variables for Below Functions /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //Current 
static int num_overcurrent = 0;
static bool overcurrent_state = false;
static float time_of_overcurrent = 0;
  //Voltage
static bool pack_undervoltage_state = false;
static bool cell_undervoltage_state = false;
static bool low_voltage_state = false;
static int num_low_voltage_reminder = 0;
static int time_of_low_voltage = 0;

  //Temp
static int num_meas_batt_temp = 0;
static bool batt_temp_avail = false;
static bool overtemp_state = false;
static bool fans_on = false;
  //Logic Switch
static bool forgotten_logic_switch = false;
static int num_out_voltage_loops = 0;
static int time_switch_forgotten = 0;
static int time_switch_reminder = 0;

// Functions /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setOutputPins()
{
  pinMode(PACK_OUT_CTR_PIN,     OUTPUT);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setOutputStates()
{
  digitalWrite(PACK_OUT_CTR_PIN,      HIGH);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void getMainCurrent(int32_t &main_current)
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void getCellVoltage(uint16_t cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT])
{  
  pinfault_state = false;

  for(int i = 0; i<RC_BMSBOARD_VMEASmV_DATACOUNT; i++)
  {
    if (i == RC_BMSBOARD_VMEASmV_PACKENTRY)
    {
      int adc_read = analogRead(CELL_MEAS_PINS[RC_BMSBOARD_VMEASmV_PACKENTRY]);

      cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] = (1215*map(adc_read, PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX)/1000); //TODO: Fix voltage divider for pack meas so that we can remove this weird scaling.
      error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_NOERROR;

      if((cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] > PACK_SAFETY_LOW) && (cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] < PACK_UNDERVOLTAGE))
      {
        delay(DEBOUNCE_DELAY);

        if((((1094 * map(analogRead(CELL_MEAS_PINS[RC_BMSBOARD_VMEASmV_PACKENTRY]), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX)) / 1000) < PACK_UNDERVOLTAGE)
            && (((1094 * map(analogRead(CELL_MEAS_PINS[RC_BMSBOARD_VMEASmV_PACKENTRY]), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX)) / 1000) > PACK_SAFETY_LOW))
        {
          pack_undervoltage_state = true;
  
          error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_UNDERVOLTAGE;
        }//end if
      }//end if
      if(cell_voltage[RC_BMSBOARD_VMEASmV_PACKENTRY] < PACK_EFFECTIVE_ZERO)
      {
        error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_PINFAULT;
        pinfault_state = true;
      }//end if
    }//end if
    
    if(i > RC_BMSBOARD_VMEASmV_PACKENTRY)
    {
      int adc_reading = analogRead(CELL_MEAS_PINS[i]);
      
      if(adc_reading < CELL_V_ADC_MIN)
      {
        adc_reading = CELL_V_ADC_MIN;
      }//end if
      if(adc_reading > CELL_V_ADC_MAX)
      {
        adc_reading = CELL_V_ADC_MAX;
      }//end if
      cell_voltage[i] = map(adc_reading, CELL_V_ADC_MIN, CELL_V_ADC_MAX, CELL_VOLTS_MIN, CELL_VOLTS_MAX);

      error_report[i] = RC_BMSBOARD_ERROR_NOERROR;

      if((cell_voltage[i] > CELL_VOLTS_MIN) && (cell_voltage[i] < CELL_UNDERVOLTAGE))
      {
        delay(DEBOUNCE_DELAY);

        adc_reading = analogRead(CELL_MEAS_PINS[i]);
      
        if(adc_reading < CELL_V_ADC_MIN)
        {
          adc_reading = CELL_V_ADC_MIN;
        }//end if
        if(adc_reading > CELL_V_ADC_MAX)
        {
          adc_reading = CELL_V_ADC_MAX;
        }//end if
        if((map(adc_reading, CELL_V_ADC_MIN, CELL_V_ADC_MAX, CELL_VOLTS_MIN, CELL_VOLTS_MAX) <= CELL_UNDERVOLTAGE)
            && (map(adc_reading, CELL_V_ADC_MIN, CELL_V_ADC_MAX, CELL_VOLTS_MIN, CELL_VOLTS_MAX) > CELL_VOLTS_MIN))
        {
          cell_undervoltage_state = true;
  
          error_report[i] = RC_BMSBOARD_ERROR_UNDERVOLTAGE;

        }//end if
      }//end if
      if(cell_voltage[i] == CELL_VOLTS_MIN)
      {
        error_report[i] = RC_BMSBOARD_ERROR_PINFAULT;
        pinfault_state = true;
      }//end if
    }//end if
  }//end for
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void getOutVoltage(int &pack_out_voltage)
{
  int adc_reading = analogRead(PACK_V_MEAS_PIN);
  pack_out_voltage = ((1269 * map(analogRead(PACK_V_MEAS_PIN), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX)) / 1000);

  if(pack_out_voltage > PACK_SAFETY_LOW)
  {
    forgotten_logic_switch = false;
    time_switch_forgotten = 0;
    num_out_voltage_loops = 0;
  }//end if
  if(pack_out_voltage < PACK_EFFECTIVE_ZERO)
  {
    delay(DEBOUNCE_DELAY);

    if(((1269 *map(analogRead(PACK_V_MEAS_PIN), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX)) / 1000) < PACK_EFFECTIVE_ZERO)
    {
      forgotten_logic_switch = true;
      num_out_voltage_loops++;
    }//end if 
  }//end if
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void getBattTemp(uint32_t &batt_temp)
{
  meas_batt_temp[num_meas_batt_temp] = (1060 * (map(analogRead(TEMP_degC_MEAS_PIN), TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX))/1000);
  num_meas_batt_temp ++;
  
  if(num_meas_batt_temp % NUM_TEMP_AVERAGE == 0)
  {
    for(int i = 0; i < NUM_TEMP_AVERAGE; i++)
    {
      batt_temp += meas_batt_temp[i];
    }//end for
    batt_temp /= NUM_TEMP_AVERAGE; //batt_temp is the average of all the measurments in the meas_batt_temp[] array.
    num_meas_batt_temp = 0;
    batt_temp_avail = true; //Set to true after first batt_temp value is avail. Avoids acting on overtemp before the first average is computed.
  
    //Serial.print("batt_temp: ");
    //Serial.println(batt_temp);
  }//end if
  
  if(batt_temp_avail == true)
  {
    if(batt_temp < TEMP_THRESHOLD)
    {
      overtemp_state = false;
    }//end if
    if(batt_temp > TEMP_THRESHOLD)
    {
      delay(DEBOUNCE_DELAY);
  
      if(map(analogRead(TEMP_degC_MEAS_PIN), TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX) > TEMP_THRESHOLD)
      {
        overtemp_state = true;
      }//end if
    }//end if
  }//end if
  
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void updateLCD(uint16_t batt_temp, uint16_t cellVoltages[])
{  
  const int NUM_CELLS = 8; //this should be in header?
  bool LCD_Update = false;
  float time1 = 0;
  float packVoltage = 0; //V not mV
  float packTemp = 0; //degC not mdegC

  packVoltage = (static_cast<float>(cellVoltages[0]) / 1000);
  packTemp = (static_cast<float>(batt_temp) / 1000);
  packTemp = roundf(packTemp * 10) / 10; //Rounds temp to closest tenth of a degree

  if(LCD_Update == false)
  {
    time1 = millis();
    LCD_Update = true;
  }

  //Send the clear command to the display - this returns the cursor to the beginning of the display
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
  
  Serial3.print("Pack:");
  Serial3.print(packVoltage, 1); //packVoltage from BMS, in V
  Serial3.print("V");
  Serial3.print(" Tmp:");
  Serial3.print(packTemp, 1); //packTemp from BMS, in C
  Serial3.print("C");
  
  for(int i = 0; i < NUM_CELLS; i++)
  {
    float temp_cell_voltage = 0;
    temp_cell_voltage = (static_cast<float>(cellVoltages[i+1]) / 1000);
    Serial3.print(i+1);
    Serial3.print(":");
    Serial3.print(temp_cell_voltage, 1); //cellVoltage from BMS in V?  shows one decimal place
    if((i+1)%3 == 0)
    {
      Serial3.print("V");
    }//end if     
    else
    {
      Serial3.print("V "); 
    }//end else
  }//end for
  LCD_Update = false;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void reactOverCurrent()
{
  if(overcurrent_state == false)
  {
    error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_OVERCURRENT;
  }//end if
  if(overcurrent_state == true)
  {
    if(num_overcurrent == 0)
    {
      error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_OVERCURRENT;
      RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
      delay(ROVECOMM_DELAY);
      
      digitalWrite(PACK_OUT_CTR_PIN, LOW);
    
      time_of_overcurrent = millis(); 
      num_overcurrent++;
    
      notifyOverCurrent();
    }//end if
    else if(num_overcurrent == 1)
    {
      if(millis() >= (time_of_overcurrent + RESTART_DELAY))
      {
        digitalWrite(PACK_OUT_CTR_PIN, HIGH);
      }//end if
      if(millis() >= (time_of_overcurrent + RESTART_DELAY + RECHECK_DELAY))
      {
        overcurrent_state = false;
        num_overcurrent = 0;
        time_of_overcurrent = 0;
      }//end if
    }//end else if
    else
    {
      error_report[RC_BMSBOARD_ERROR_PACKENTRY] = RC_BMSBOARD_ERROR_OVERCURRENT;
      RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
      delay(ROVECOMM_DELAY);
      
      digitalWrite(PACK_OUT_CTR_PIN, LOW);

      notifyOverCurrent();

      digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide   
    }//end else
  }//end if
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void reactUnderVoltage()
{
  if(pack_undervoltage_state == true || cell_undervoltage_state == true)
  {
    RoveComm.write(RC_BMSBOARD_ERROR_HEADER, error_report);
    delay(DEBOUNCE_DELAY);

    digitalWrite(PACK_OUT_CTR_PIN, LOW);
    notifyUnderVoltage();
    digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide
  }//end if
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
  }//end if
  if(overtemp_state == false && fans_on == true)
  {
    fans_on = false;
    digitalWrite(FAN_1_CTR_PIN, LOW);
    digitalWrite(FAN_2_CTR_PIN, LOW);
    digitalWrite(FAN_3_CTR_PIN, LOW);
    digitalWrite(FAN_4_CTR_PIN, LOW);
    digitalWrite(FAN_PWR_IND_PIN, LOW);
  }//end if
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void reactForgottenLogicSwitch()
{
  if(forgotten_logic_switch == true)
  {
    if(num_out_voltage_loops == 1)
    {
      time_switch_forgotten = millis();
      time_switch_reminder = millis();
    }//end if
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void reactLowVoltage( uint16_t cell_voltage[RC_BMSBOARD_VMEASmV_DATACOUNT])
{
  if((cell_voltage[0] > PACK_UNDERVOLTAGE) && (cell_voltage[0] <= PACK_LOWVOLTAGE) && (low_voltage_state = false))//first instance of low voltage
  { 
    low_voltage_state = true;
    notifyLowVoltage();
    time_of_low_voltage = millis();
    num_low_voltage_reminder = 1;
  }//end if
  else if((cell_voltage[0] > PACK_UNDERVOLTAGE) && (cell_voltage[0] <= PACK_LOWVOLTAGE) && (low_voltage_state = true))//following instances of low voltage
  {
    if(millis() >= (time_of_low_voltage + (num_low_voltage_reminder * LOGIC_SWITCH_REMINDER)))
    { 
      notifyLowVoltage();
      num_low_voltage_reminder++;
    }//end if
  }//end else if
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setEstop(uint8_t data)
{
  if(data == 0)
  {
    digitalWrite(PACK_OUT_CTR_PIN, LOW);

    notifyEstop();

    digitalWrite(LOGIC_SWITCH_CTR_PIN, HIGH); //BMS Suicide
    //If BMS is not turned off here, the PACK_OUT_CTR_PIN would be low and there would be no way to get it high again without reseting BMS anyway.
  }//end if
  else
  {
    digitalWrite(PACK_OUT_CTR_PIN, LOW);
    notifyReboot();  
    delay(data * 1000); //Receiving delay in seconds so it needs to be converted to msec.

    digitalWrite(PACK_OUT_CTR_PIN, HIGH);
  }//end else
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void notifyEstop() //Buzzer sound: beeeeeeeeeeeeeeeeeeeep beeeeeeeeeep beeeeep beeep bep
{
  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(50);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(25);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);

  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void notifyLogicSwitch() //Buzzer sound: beeep beeep
{
  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);

  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void notifyReboot() //Buzzer sound: beeeeeeeeeep beeep beeep
{
  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(50);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(25);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(600);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(25);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(50);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(100);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  
  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void notifyOverCurrent() //Buzzer Sound: beeeeeeeeeeeeeeeeeeeeeeeeeeeeeep
{
  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(3000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);

  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void notifyUnderVoltage() //Buzzer Sound: beeep beeep beeep beeep beeeeeeeeeeeeeeeeeeeep
{
  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(150);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(150);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(150);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);
  delay(150);

  digitalWrite(BUZZER_CTR_PIN, HIGH);
  digitalWrite(SW_ERR_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_CTR_PIN, LOW);
  digitalWrite(SW_ERR_PIN, LOW);

  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void notifyLowVoltage() //Buzzer Sound: beeep beeep beeep
{
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

  return;
}//end func

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void startScreen()
{ 
  delay(350);
  //Send the clear command to the display - this returns the cursor to the beginning of the display
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
  
  /*Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);*/
  
  //movingRover();
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
  
  for(int i = 0; i<3; i++)
  {
    asterisks();
    stars();
  }
  delay(10);

  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
}

void movingRover()
{
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
  
  for(int i = 0; i<14;i++)
  {
    delay(450);
    //delay(200);
    //Serial3.write(0x20);
    //Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    
    Serial3.write(0xA1);
    
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    
    Serial3.write(0xDB);
    Serial3.write(0xBA);
    Serial3.write(0xDA);
    Serial3.write(0xCD);
   
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20); 
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
       
    Serial3.write(0x6F);
    Serial3.write(0x5E);
    Serial3.write(0x6F);
    Serial3.write(0x5E);
    Serial3.write(0x6F);
      
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
    Serial3.write(0x20);
  /*  
    Serial3.write(0x23);//#
    Serial3.write(0x52);//R
    Serial3.write(0x4F);//O
    Serial3.write(0x56);//V
    Serial3.write(0x45);//E
    Serial3.write(0x53);//S
    Serial3.write(0x4F);//O
    Serial3.write(0x48);//H
    Serial3.write(0x41);//A
    Serial3.write(0x52);//R
    Serial3.write(0x44);//D*/
  }

  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
}

void asterisks()
{
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
  Serial3.write(0x56);//V
  Serial3.write(0x41);//A
  Serial3.write(0x4C);//L
  Serial3.write(0x4B);//K
  Serial3.write(0x59);//Y
  ////////////////////////////////////////////////////////////////need a y
  Serial3.write(0x52);//R
  Serial3.write(0x49);//I
  Serial3.write(0x45);//E
  Serial3.write(0x20);
  Serial3.write(0x49);//I
  Serial3.write(0x4E);//N
  Serial3.write(0x49);//I
  Serial3.write(0x54);//T
  Serial3.write(0x49);//I
  Serial3.write(0x41);//A
  Serial3.write(0x4C);//L
  Serial3.write(0x49);//I
  Serial3.write(0x5A);//Z
  Serial3.write(0x49);//I
  Serial3.write(0x4E);//N
  //Serial3.write(0x47);//G
 
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x20);
  
  Serial3.write(0xA1);
  
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0xDB);
  Serial3.write(0xBA);
  Serial3.write(0xDA);
  Serial3.write(0xCD);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20); 
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);   
  Serial3.write(0x6F);
  Serial3.write(0x5E);
  Serial3.write(0x6F);
  Serial3.write(0x5E);
  Serial3.write(0x6F);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  //Serial3.write(0x2A);

  delay(500);
  
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
}

void stars()
{
  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
  Serial3.write(0x56);//V
  Serial3.write(0x41);//A
  Serial3.write(0x4C);//L
  Serial3.write(0x4B);//K
  Serial3.write(0x59);//Y
  ////////////////////////////////////////////////////////////////need a y
  Serial3.write(0x52);//R
  Serial3.write(0x49);//I
  Serial3.write(0x45);//E
  Serial3.write(0x20);
  Serial3.write(0x49);//I
  Serial3.write(0x4E);//N
  Serial3.write(0x49);//I
  Serial3.write(0x54);//T
  Serial3.write(0x49);//I
  Serial3.write(0x41);//A
  Serial3.write(0x4C);//L
  Serial3.write(0x49);//I
  Serial3.write(0x5A);//Z
  Serial3.write(0x49);//I
  Serial3.write(0x4E);//N
  //Serial3.write(0x47);//G
 
  Serial3.write(0x20);
  Serial3.write(0x2B);
  
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0xA1);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0xDB);
  Serial3.write(0xBA);
  Serial3.write(0xDA);
  Serial3.write(0xCD);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20); 
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x2A);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  Serial3.write(0x20);
  Serial3.write(0x20);
  Serial3.write(0x20);   
  Serial3.write(0x6F);
  Serial3.write(0x5E);
  Serial3.write(0x6F);
  Serial3.write(0x5E);
  Serial3.write(0x6F);
  Serial3.write(0x20);
  Serial3.write(0x2B);
  //Serial3.write(0x2B);

  delay(500);

  Serial3.write('|'); //Setting character
  Serial3.write('-'); //Clear display
}