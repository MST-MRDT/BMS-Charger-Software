#include "BMStest.h"


void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  setInputPins();
  setOutputPins();
  setOutputStates();
  initTest(beep_time);
  Serial.println("Pack Current:");
  
  //int count = 0;
  //int countMax = 20;
  //float pack_voltage = 0;
}

void loop() 
{
  // put your main code here, to run repeatedly:
  delay(1000);
 //getCellVoltage(cell_voltage,count,cell_adc_average,pack_voltage);
 //getPackVoltage(pack_adc_v_average, count, pack_voltage);
 getPackCurrent(pack_adc_i_average);
 //getTemperature(temp_adc_average, count);
 /*
 count++;
 if(count == 21)
 {
  count = 0;
 }
 delay(30);  */
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
  Serial.println("setInputPins Complete");
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
  Serial.println("setOutputPins Complete");
  return;
}

void setOutputStates()
{
  digitalWrite(PACK_OUT_CTRL_PIN,      HIGH);
  digitalWrite(LOGIC_SWITCH_CTRL_PIN,  LOW);
  digitalWrite(BUZZER_CTRL_PIN,        LOW);
  digitalWrite(FAN_CTRL_PIN,           LOW);
  digitalWrite(FAN_PWR_IND_PIN,       LOW);
  digitalWrite(SW_IND_PIN,            LOW);
  digitalWrite(SW_ERR_IND_PIN,        LOW);
  Serial.println("setOutputStates Complete");
  return;
}

void getCellVoltage(float cell_voltage[],int count, float cell_adc_average[], float pack_voltage)
{
  for(int i = 0; i < 8 ; i++)
  {
    int adc_reading = analogRead(CELL_MEAS_PINS[i]);   //Serial.println(analogRead(CELL_MEAS_PINS[1]));
    cell_adc_average[i]+= adc_reading; // adds cell voltages to an array of 8 (for average to be taken later)
  }
  if (count == 20)
  {
    pack_voltage = 0;
    //take average
    for(int i = 0; i < 8 ; i++)
    { 
      cell_adc_average[i]= cell_adc_average[i]/(count + 1);
      cell_voltage[i]= map(cell_adc_average[i],CELL_V_ADC_MIN, CELL_V_ADC_MAX, CELL_VOLTS_MIN, CELL_VOLTS_MAX);
      pack_voltage += cell_voltage[i];
    }
    
    //prints a table of adc and voltage average values
    Serial.println("CELL");
    Serial.print("ADC:");
    Serial.print("      ");
    Serial.print("VOLTS:");
    Serial.println();
    for(int i = 0; i < 8 ; i++) 
    {
      Serial.print(cell_adc_average[i]);
      Serial.print("      ");
      Serial.print(cell_voltage[i]);
      Serial.println();
    }
    Serial.println();
  }
  return;
}

void getPackVoltage(float pack_adc_v_average, int count, float pack_voltage)
{
  if (count == 20)
  { 
    Serial.println("PACK VOLTAGE");
    Serial.print("VOLTS:");
    Serial.println();
    Serial.print(pack_voltage);
    Serial.println();
  }
  return;
}
 
void getPackCurrent(float pack_adc_i_average)
{
  int adc_reading = analogRead(PACK_I_MEAS_PIN);
  //pack_adc_i_average += adc_reading;
  Serial.println("adc:");
  Serial.println(analogRead(PACK_I_MEAS_PIN));
  int mapped_current = map(adc_reading,CURRENT_ADC_MIN,CURRENT_ADC_MAX,CURRENT_MIN,CURRENT_MAX);
  Serial.println("map:");
  Serial.println(mapped_current);
  /*
  if (count == 20)
  {
    pack_adc_i_average = pack_adc_i_average/(count + 1);
    int pack_current = map(pack_adc_i_average, CURRENT_ADC_MIN, CURRENT_ADC_MAX, CURRENT_MIN, CURRENT_MAX);
    
    Serial.println("PACK CURRENT");
    Serial.print("ADC:");
    Serial.print("      ");
    Serial.print("CURRENT:");
    Serial.println();
    Serial.print(pack_adc_i_average);
    Serial.print("      ");
    Serial.print(pack_current);
    Serial.println();
    
  }*/
  return;
}

void getTemperature(float temp_adc_average, int count)
{
  int adc_reading = analogRead(TEMP_degC_MEAS_PIN);
  temp_adc_average += adc_reading;

  if (count == 20)
  {
    temp_adc_average = temp_adc_average/(count+1);
    int temp_average = map(temp_adc_average, TEMP_ADC_MIN, TEMP_ADC_MAX, TEMP_MIN, TEMP_MAX);

    Serial.println("TEMPERATURE");
    Serial.print("ADC:");
    Serial.print("      ");
    Serial.print("CELSUIS:");
    Serial.println();
    Serial.print(temp_adc_average);
    Serial.print("      ");
    Serial.print(temp_average);
    Serial.println();
  }
  return;
}

void initTest(int beep_time)
{
  turnOnFans();
  delay(1000);
  turnOffFans();
  //beep(beep_time);
  Serial.println("Rover Initialized");
  digitalWrite(FAN_PWR_IND_PIN,       HIGH);
  delay(beep_time);
  digitalWrite(SW_IND_PIN,            HIGH);
  delay(beep_time);
  digitalWrite(SW_ERR_IND_PIN,        HIGH);
  delay(beep_time);
  digitalWrite(FAN_PWR_IND_PIN,       LOW);
  delay(beep_time);
  digitalWrite(SW_IND_PIN,            LOW);
  delay(beep_time);
  digitalWrite(SW_ERR_IND_PIN,        LOW);
  
}
void turnOnFans()
{
 digitalWrite(FAN_CTRL_PIN, HIGH);
 digitalWrite(FAN_PWR_IND_PIN, HIGH);
}

void turnOffFans()
{
  digitalWrite(FAN_CTRL_PIN, LOW);
  digitalWrite(FAN_PWR_IND_PIN, LOW);
}

void beep(int beep_time)
{
  digitalWrite(BUZZER_CTRL_PIN, HIGH);
  delay(beep_time);
  digitalWrite(BUZZER_CTRL_PIN, LOW);
}
