#include "BMS_Software_Main.h"
#include "RoveComm.h"
//rovecomm and packet instances
RoveCommEthernet RoveComm;
rovecomm_packet packet; 

//timekeeping variables
uint32_t last_update_time;

//declare the Ethernet Server in the top level sketch with the requisite port ID any time you want to use RoveComm
EthernetServer TCPServer(RC_ROVECOMM_BMSBOARD_PORT);
void setup() 
{
  Serial.begin(9600);   //
  Serial3.begin(9600); //comms with LCD display
  
  RoveComm.begin(RC_BMSBOARD_FOURTHOCTET, &TCPServer);
  delay(ROVECOMM_DELAY);
  
  setInputPins();
  setOutputPins();
  setOutputStates();
}


void loop() 
{
  //listen for rovecomm packets
  packet = RoveComm.read();
  
  //if estop, go directly to estop function
  if (packet.data_id = RC_BMSBOARD_BMSSTOP_DATA_ID)
  {
    estop(packet.data[0]);
  }

  //get telemetry and send it
  float temp = getTemperature();
  float cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT] = getCellVoltages();
  float pack_voltage = getPackVoltage(cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT]);
  float pack_current = getCurrent();
  RoveComm.write(RC_BMSBOARD_PACKI_MEAS_DATA_ID, pack_current);
  RoveComm.write(RC_BMSBOARD_PACKV_MEAS_DATA_ID, pack_voltage);
  RoveComm.write(RC_BMSBOARD_CELLV_MEAS_DATA_ID, cells[]);
  RoveComm.write(RC_BMSBOARD_TEMP_MEAS_DATA_ID, temp);
}

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
}

void setOutputPins()
{
  pinMode(PACK_OUT_CTRL_PIN,     OUTPUT);
  pinMode(LOGIC_SWITCH_CTRL_PIN, OUTPUT);
  pinMode(BUZZER_CTRL_PIN,       OUTPUT);
  pinMode(FAN_CTRL_PIN,          OUTPUT);
  pinMode(FAN_PWR_IND_PIN,      OUTPUT);
  pinMode(SW_IND_PIN,           OUTPUT);
  pinMode(SW_ERR_IND_PIN,       OUTPUT);
}

void setOutputStates() //is the pin on or off? must be set for all digital pins
{
  digitalWrite(PACK_OUT_CTRL_PIN,      HIGH);
  digitalWrite(LOGIC_SWITCH_CTRL_PIN,  LOW);
  digitalWrite(BUZZER_CTRL_PIN,        LOW);
  digitalWrite(FAN_CTRL_PIN,           LOW);
  digitalWrite(FAN_PWR_IND_PIN,       LOW);
  digitalWrite(SW_IND_PIN,            LOW);
  digitalWrite(SW_ERR_IND_PIN,        LOW);
}


void estop(uint8_t time)
{
  if (time > 0)
  {
    digitalWrite(PACK_OUT_CTRL_PIN, LOW);
    delay(time);
    digitalWrite(PACK_OUT_CTRL_PIN, HIGH);
  }
}

// get temp for battery pack and if too high, turn on fans
float getTemperature()
{
  temp = analog.read(TEMP_degC_MEAS_PIN);
  if (temp > TEMP_THRESHOLD)
  {
    delay(DEBOUNCE_DELAY);
    temp = analog.read(TEMP_degC_MEAS_PIN);
    if (temp >= TEMP_THRESHOLD)
    {
      uint8_t superhot = 1;
      RoveComm.write(RC_BMSBOARD_PACKSUPERHOT_DATA_ID, superhot);
    }
  }
  return temp/1000;
}

// get the individual voltage of 8 cells and put in an array
float* getCellVoltages()
{
  float cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT] = {};

  for a in range(RC_BMSBOARD_CELLV_MEAS_DATA_COUNT)
  {
    float cell = map(analogRead(CELL_MEAS_PINS[a]), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX);
    // check errors
    if(cell =< CELL_UNDERVOLTAGE)
    {
      delay(DEBOUNCE_DELAY);
      float cell = map(analogRead(CELL_MEAS_PINS[a]), PACK_V_ADC_MIN, PACK_V_ADC_MAX, VOLTS_MIN, PACK_VOLTS_MAX);
      if (cell =< CELL_UNDERVOLTAGE)
      {
        uint8_t undervoltcell = a;  
        RoveComm.write(RC_BMSBOARD_CELLUNDERVOLTAGE_DATA_ID, undervoltcell);
      }
    }
    cells[a] = cell/1000;
  }
  return float cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT];
}

float getPackVoltage(float cells[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT])
{
  packVoltage = 0;
  for a in range(RC_BMSBOARD_CELLV_MEAS_DATA_COUNT)
  {
    packVoltage += a;
  }
  if (packVoltage =< PACK_UNDERVOLTAGE)
  {
    uint8_t packerror = 1;
    RoveComm.write(RC_BMSBOARD_PACKUNDERVOLTAGE_DATA_ID,packerror);
  }
  return packVoltage/1000;
}


// get pack current
float getCurrent()
{
  float main_current = map(analogRead(PACK_I_MEAS_PIN), CURRENT_ADC_MIN, CURRENT_ADC_MAX, CURRENT_MIN, CURRENT_MAX);
  if (main_current >= OVERCURRENT)
  {
    delay(DEBOUNCE_DELAY);
    float main_current = map(analogRead(PACK_I_MEAS_PIN), CURRENT_ADC_MIN, CURRENT_ADC_MAX, CURRENT_MIN, CURRENT_MAX);
    if(main_current >= OVERCURRENT)
    {
      uint8_t overcurrent = 1;
      RoveComm.write(RC_BMSBOARD_PACKOVERCURRENT_DATA_ID, pvercurrent);
    }
  }
  return main_current/1000;
  }
}
