#include <Arduino.h>

/* =============== TEENSY ECU CAN FT TEST ===========================
 *  1/ FlexCAN_T4 library is used
 */

#include <FlexCAN_T4.h>
#include <typedef.h>

//===========SETTINGS==============//
bool passiveMode = false;
bool can0Listen = true;
bool printSensor = false;
unsigned int period = 20;
int Wake = 24;
int Enable = 32;
int Stb = 29;
int Err = 28;

volatile int state = 0;
int newdata = 0;
int j = 0;

// moving average filter
const int SAMPLES_NUM = 2;
uint counter = 0;

uint64_t total_IVT_Current = 0;
uint64_t total_LoadPlus_Voltage = 0;
uint64_t total_IVT_Temp = 0;
uint64_t total_Batt_Voltage = 0;
uint64_t total_IVT_Power = 0;
uint64_t total_Cell_Volt_Min = 0;
uint64_t total_Cell_Volt_Avg = 0;
uint64_t total_Cell_Volt_Max = 0;
uint64_t total_Cell_Temp_Min = 0;
uint64_t total_Cell_Temp_Avg = 0;
uint64_t total_Cell_Temp_Max = 0;

//==========TRANSLATED DATA=======//
// 0x405
uint8_t BMS_State = 0;
uint8_t IMD_State = 0;
uint8_t tempRelay = 0;
uint8_t Relays[8] = {0}; // index 0 = discharge, 1 = charge plus, 2 = charge gnd, 3 = hv plus, 4 = precharge, 5 = hv ground

// 0x402
uint32_t IVT_Current[SAMPLES_NUM];

// 0x403
uint32_t LoadPlus_Voltage[SAMPLES_NUM];
uint32_t IVT_Temp[SAMPLES_NUM];

// 0x404
uint32_t Batt_Voltage[SAMPLES_NUM];
uint32_t IVT_Power[SAMPLES_NUM];

// 0x400
uint32_t Cell_Volt_Min[SAMPLES_NUM];
uint32_t Cell_Volt_Avg[SAMPLES_NUM];
uint32_t Cell_Volt_Max[SAMPLES_NUM];

// 0x401
uint32_t Cell_Temp_Min[SAMPLES_NUM];
uint32_t Cell_Temp_Avg[SAMPLES_NUM];
uint32_t Cell_Temp_Max[SAMPLES_NUM];



uint64_t packet201 = 0;

#define HVon 49
#define HVoff 48

//===========VARIABLES============//
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> FTCan;
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;

void TranslatedData()
{
  Serial.print("DATA:  \t");

  // Print readable BMS State
  Serial.print("BMS Data\t");

  Serial.print("Battery Modules\n");
  Serial.print("\t\tCell Volt Min: ");
  float MA_Cell_Volt_Min = (float)total_Cell_Volt_Min / SAMPLES_NUM;
  if (MA_Cell_Volt_Min > 5000 && MA_Cell_Volt_Min < 500)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Cell_Volt_Min / 1000.0);
    Serial.print(" V");
  }


  Serial.print("\tCell Volt Avg: ");
  float MA_Cell_Volt_Avg = (float)total_Cell_Volt_Avg / SAMPLES_NUM;
  if (MA_Cell_Volt_Avg > 4000 && MA_Cell_Volt_Avg < 1000)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Cell_Volt_Avg / 1000.0);
    Serial.print(" V");
  }

  Serial.print("\tCell Volt Max: ");
  float MA_Cell_Volt_Max = (float)total_Cell_Volt_Avg / SAMPLES_NUM;
  if (MA_Cell_Volt_Max > 4000 && MA_Cell_Volt_Max < 1000)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Cell_Volt_Max / 1000.0);
    Serial.print(" V");
  }

  Serial.print("\t\tCell Temp Min: ");
  float MA_Cell_Temp_Min = (float)total_Cell_Temp_Min / SAMPLES_NUM;
  if (MA_Cell_Temp_Min > 600 && MA_Cell_Temp_Min < 10)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Cell_Temp_Min / 10.0);
    Serial.print(" degC");
  }

  Serial.print("\tCell Temp Avg: ");
  float MA_Cell_Temp_Avg = (float)total_Cell_Temp_Avg / SAMPLES_NUM;
  if (MA_Cell_Temp_Avg > 600 && MA_Cell_Temp_Avg < 10)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Cell_Temp_Avg / 10.0);
    Serial.print(" degC");
  }
  
  Serial.print("\tCell Temp Max: ");
  float MA_Cell_Temp_Max = (float)total_Cell_Temp_Max / SAMPLES_NUM;
  if (MA_Cell_Temp_Max > 600 && MA_Cell_Temp_Max < 10)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Cell_Temp_Max / 10.0);
    Serial.print(" degC");
  }

  Serial.print("\t\tBMS State: ");
  if (BMS_State == 0)
  {
    Serial.print("BMS_INIT");
  }
  else if (BMS_State == 1)
  {
    Serial.print("BMS_HV_OFF");
  }
  else if (BMS_State == 2)
  {
    Serial.print("BMS_HV_ON");
  }
  else if (BMS_State == 3)
  {
    Serial.print("BMS_PRECHARGE");
  }
  else if (BMS_State == 4)
  {
    Serial.print("BMS_CHARGER_CONNECTED");
  }
  else if (BMS_State == 5)
  {
    Serial.print("BMS_TEST");
  }
  else
  {
    Serial.print("BMS_ERROR");
  }

  // Print readable IMD State
  Serial.print("\tIMD State: ");
  if (IMD_State == 0)
  {
    Serial.println("NOT OK");
  }
  else
  {
    Serial.println("OK");
  }

  // Print readable Load+ Voltage
  Serial.print("\tIVT Data\n");
  Serial.print("\t\tLoad+ Voltage: ");
  float MA_LoadPlus_Voltage = (float)total_LoadPlus_Voltage / SAMPLES_NUM;
  if (MA_LoadPlus_Voltage > 300000)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_LoadPlus_Voltage / 1000.0);
    Serial.print(" V");
  }

  // Print readable Batt+ Voltage
  Serial.print("\tInput Voltage: ");
  float MA_Batt_Voltage = (float)total_Batt_Voltage / SAMPLES_NUM;
  if (MA_Batt_Voltage > 300000)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_Batt_Voltage / 1000.0);
    Serial.print(" V");
  }

  Serial.print("\tIVT Current: ");
  float MA_IVT_Current = (float)total_IVT_Current / SAMPLES_NUM;
  if (MA_IVT_Current > 100000)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_IVT_Current / 1000.0);
    Serial.print(" A");
  }

  Serial.print("\tIVT Temperature: ");
  float MA_IVT_Temp = (float)total_IVT_Temp / SAMPLES_NUM;
  if (MA_IVT_Temp > 500 && MA_IVT_Temp < 10)
  {
    Serial.print("N/A");
  }
  else
  {
    Serial.print(MA_IVT_Temp / 10.0);
    Serial.println(" degC");
  }
  // Print readable HV Relays
  Serial.print("\t\tRelays State  ");
  Serial.print("  HV-: ");
  Serial.print(Relays[5]);
  Serial.print("  Precharge: ");
  Serial.print(Relays[4]);
  Serial.print("  HV+: ");
  Serial.print(Relays[3]);
  Serial.print("  Charge-: ");
  Serial.print(Relays[2]);
  Serial.print("  Charge+: ");
  Serial.print(Relays[1]);
  Serial.print("  Discharge: ");
  Serial.println(Relays[0]);
  Serial.println();
}

//=========CAN TX FUNCTIONS=======//
void SendData(int can, uint64_t packet, uint32_t id)
{
  CAN_message_t msg;
  msg.id = id; // assign id
  msg.len = 8; // set msg length

  // compose frame buffer
  msg.buf[0] = (packet >> 0) & 0xFF;
  msg.buf[1] = (packet >> 8) & 0xFF;
  msg.buf[2] = (packet >> 16) & 0xFF;
  msg.buf[3] = (packet >> 24) & 0xFF;
  msg.buf[4] = (packet >> 32) & 0xFF;
  msg.buf[5] = (packet >> 40) & 0xFF;
  msg.buf[6] = (packet >> 48) & 0xFF;
  msg.buf[7] = (packet >> 56) & 0xFF;

  // send data over selected channel
  if (can == 1)
  {
    FTCan.write(msg);
  }
  else
  {
    Can1.write(msg);
  }
}

// concatenate data into one 64bit unsiged integer
uint64_t concatData(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
  // padding with 0xffff to reach 64bit size
  return ((uint64_t)d7 << 56) + ((uint64_t)d6 << 48) + ((uint64_t)d5 << 40) + ((uint64_t)d4 << 32) + ((uint64_t)d3 << 24) + ((uint64_t)d2 << 16) + ((uint64_t)d1 << 8) + ((uint64_t)d0);
}

void CreatePackets()
{
  // Reset packets
  packet201 = 0;

  switch (state)
  {
  case HVon:
    packet201 = concatData(0, 0, 0, 0, 0, 0, 0, 255);
    break;
  case HVoff:
    packet201 = concatData(0, 0, 0, 0, 0, 0, 0, 0);
    break;
  default:
    break;
  }
}

void printBinary(uint8_t num)
{
  for (int i = 7; i >= 0; i--)
  {
    Relays[i] = ((num >> i) & 1);
  }
}

//=========CAN RX FUNCTIONS=======//
void canSniff0(const CAN_message_t &msg)
{
  // to print on the console
  if (can0Listen)
  {
    if (counter == SAMPLES_NUM)
    {
      TranslatedData();
      counter = 0;
    }
    else
    {
      total_Batt_Voltage -= Batt_Voltage[counter];
      total_LoadPlus_Voltage -= LoadPlus_Voltage[counter];
      total_IVT_Current -= IVT_Current[counter];
      total_IVT_Power -= IVT_Power[counter];
      total_IVT_Temp -= IVT_Temp[counter];
      total_Cell_Volt_Min -= Cell_Volt_Min[counter];
      total_Cell_Volt_Avg -= Cell_Volt_Avg[counter];
      total_Cell_Volt_Max -= Cell_Volt_Max[counter];
      total_Cell_Temp_Min -= Cell_Temp_Min[counter];
      total_Cell_Temp_Avg -= Cell_Temp_Avg[counter];
      total_Cell_Temp_Max -= Cell_Temp_Max[counter];

      switch (msg.id)
      {
      case 0x405:
      {
        tempRelay = msg.buf[3];
        printBinary(tempRelay);
        IMD_State = msg.buf[5];
        BMS_State = msg.buf[7];
      }
      case 0x402:
      {
        IVT_Current[counter] = ((uint64_t)msg.buf[4] << 24) + ((uint64_t)msg.buf[5] << 16) + ((uint64_t)msg.buf[6] << 8) + ((uint64_t)msg.buf[7]);
        
        total_IVT_Current += IVT_Current[counter];
      }
      case 0x403:
      {
        LoadPlus_Voltage[counter] = ((uint64_t)msg.buf[0] << 24) + ((uint64_t)msg.buf[1] << 16) + ((uint64_t)msg.buf[2] << 8) + ((uint64_t)msg.buf[3]);
        IVT_Temp[counter] = ((uint64_t)msg.buf[4] << 24) + ((uint64_t)msg.buf[5] << 16) + ((uint64_t)msg.buf[6] << 8) + ((uint64_t)msg.buf[7]);
        
        total_LoadPlus_Voltage += LoadPlus_Voltage[counter];
        total_IVT_Temp += IVT_Temp[counter];
      }
      case 0x404:
      {
        Batt_Voltage[counter] = ((uint64_t)msg.buf[0] << 24) + ((uint64_t)msg.buf[1] << 16) + ((uint64_t)msg.buf[2] << 8) + ((uint64_t)msg.buf[3]);
        IVT_Power[counter] = ((uint64_t)msg.buf[4] << 24) + ((uint64_t)msg.buf[5] << 16) + ((uint64_t)msg.buf[6] << 8) + ((uint64_t)msg.buf[7]);
        
        total_Batt_Voltage += Batt_Voltage[counter];
        total_IVT_Power += IVT_Power[counter];
      }
      case 0x400:
      {
        Cell_Volt_Min[counter] = ((uint64_t)msg.buf[0] << 8) + ((uint64_t)msg.buf[1]);
        Cell_Volt_Avg[counter] = ((uint64_t)msg.buf[2] << 8) + ((uint64_t)msg.buf[3]);
        Cell_Volt_Max[counter] = ((uint64_t)msg.buf[4] << 8) + ((uint64_t)msg.buf[5]);
        
        total_Cell_Volt_Min += Cell_Volt_Min[counter];
        total_Cell_Volt_Avg += Cell_Volt_Avg[counter];
        total_Cell_Volt_Max += Cell_Volt_Max[counter];
      }
      case 0x401:
      {
        Cell_Temp_Min[counter] = ((uint64_t)msg.buf[0] << 8) + ((uint64_t)msg.buf[1]);
        Cell_Temp_Avg[counter] = ((uint64_t)msg.buf[2] << 8) + ((uint64_t)msg.buf[3]);
        Cell_Temp_Max[counter] = ((uint64_t)msg.buf[4] << 8) + ((uint64_t)msg.buf[5]);

        total_Cell_Temp_Min += Cell_Temp_Min[counter];
        total_Cell_Temp_Avg += Cell_Temp_Avg[counter];
        total_Cell_Temp_Max += Cell_Temp_Max[counter];
      }
      }
      counter++;
    }
  }
}

//------------- NOT USED ----------------//
void canSniff1(const CAN_message_t &msg)
{
  // to print on the console
  if (can0Listen)
  {
    Serial.print("CAN0:  ");
    Serial.print("ID ");
    Serial.print(msg.id, HEX);
    Serial.print("  LEN: ");
    Serial.print(msg.len);
    Serial.print(" Buffer: ");
    for (uint8_t i = 0; i < msg.len; i++)
    {
      Serial.print(msg.buf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(400);
  // CAN-FT setup
  FTCan.begin();
  FTCan.setBaudRate(125000);
  FTCan.setMaxMB(16);
  FTCan.enableFIFO();
  FTCan.enableFIFOInterrupt();
  FTCan.onReceive(canSniff0);
  FTCan.mailboxStatus();

  // CAN 2.0 setup
  Can1.begin();
  Can1.setBaudRate(500000);
  Can1.setMaxMB(16);
  Can1.enableFIFO();
  Can1.enableFIFOInterrupt();
  Can1.onReceive(canSniff1);
  Can1.mailboxStatus();

  pinMode(Stb, OUTPUT);
  pinMode(Enable, OUTPUT);
  pinMode(Wake, OUTPUT);
  pinMode(Err, OUTPUT);

  digitalWrite(Enable, HIGH);
  digitalWrite(Stb, HIGH);

  digitalWrite(Wake, HIGH);
  delay(10);
  digitalWrite(Wake, LOW);
}

//============DA LOOP============//
void loop()
{
  if (Serial.available() > 0)
  {
    // read the incoming byte:
    int substate = Serial.read();
    Serial.println(state);
    if (substate != 10)
    {
      newdata = 1;
      state = substate;
    }
  }
  FTCan.events();
  delay(10); // TODO: if data not good, delete this.

  Can1.events();

  CreatePackets();

  if (newdata != 0)
  {
    SendData(1, packet201, 0x100);
    delay(10);
    if (j < 10000)
    {
      newdata = 1;
      j++;
    }
    else
    {
      newdata = 0;
      j = 0;
    }
  }
}