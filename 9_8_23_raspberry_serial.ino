#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(10);

bool blockA = false;
bool blockB = false;

bool shutdown = false;
bool android;

bool motor = false;   // 0 is uit - 1 is draaid
bool opslot = false;  // 0 is uit 1 is aan

bool Verlichting = false;  // 0 is uit 1 is aan
bool rear_light = false;
bool contact = false;  // 0 is uit 1 is aan

int rasp_dead = 32;  //relais hoofd spanning raspberry
int Can_dead = 33;  //canbus onderbreken om storingen in auto te verkomen

int rasp_android_connect = 22;  //gpio 20

int rasp_shutdown = 31;    //gpio 21 zat op 23
int rasp_rear_cam = 25;    //gpio 16
int rasp_scherm_dim = 24;  //gpio 26

unsigned long id_ontvangen;

unsigned long data0;
unsigned long data1;
unsigned long data2;
unsigned long data3;
unsigned long data4;
unsigned long data5;
unsigned long data6;
unsigned long data7;

unsigned long data0A = 0x00;

unsigned long data0B;
unsigned long data1B;

unsigned long speed;

unsigned long previousMillisA = 0;
const long intervalA = 30000;  // wacht tijd opstarten

unsigned long previousMillisB = 0;
const long intervalB = 30000;  // wacht tijd dat raspberry word uit geschakelt

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(rasp_dead, OUTPUT);
  pinMode(Can_dead, OUTPUT);
  pinMode(rasp_rear_cam, OUTPUT);
  pinMode(rasp_shutdown, OUTPUT);
  pinMode(rasp_scherm_dim, OUTPUT);

  pinMode(rasp_android_connect, INPUT);

  digitalWrite(rasp_dead, LOW);
  digitalWrite(Can_dead, LOW);
  digitalWrite(rasp_rear_cam, LOW);
  digitalWrite(rasp_shutdown, HIGH);
  digitalWrite(rasp_scherm_dim, LOW);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_33KBPS, MCP_16MHZ);
  mcp2515.setListenOnlyMode();
  //Serial.println("begin");
}

void loop() {
  unsigned long currentMillis = millis();

  android = digitalRead(rasp_android_connect);


  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    id_ontvangen = canMsg.can_id;
    //Serial.print(id_ontvangen);
    //contact status
    if (id_ontvangen == 0x170) {
      for (byte i = 0; i < canMsg.can_dlc; i++) {
        if (i == 0) {
          data0A = canMsg.data[i];
        }
        if (data0A == 0x00) {
          contact = false;
          Serial.println("contact uit");
          digitalWrite(Can_dead, HIGH);
        } else {
          contact = true;
           Serial.println("contact aan");
        }
      }
    }
    if (id_ontvangen == 0x108) {
      for (byte i = 0; i < canMsg.can_dlc; i++) {
        if (i == 0) {
          data0 = canMsg.data[i];
        }
        if (i == 1) {
          data1 = canMsg.data[i];
        }
        if (i == 2) {
          data2 = canMsg.data[i];
        }
        if (i == 3) {
          data3 = canMsg.data[i];
        }
        if (i == 4) {
          data4 = canMsg.data[i];
        }
        if (i == 5) {
          data5 = canMsg.data[i];
        }
        if (i == 6) {
          data6 = canMsg.data[i];
        }
        if (i == 7) {
          data7 = canMsg.data[i];
        }
        if (data0 == 0x11) {
          motor = true;
        } else {
          motor = false;
        }
        //speed
        speed = data3 + data4;
      }
    }
    //status verlichting
    if (id_ontvangen == 0x350) {
      // Serial.println("verlichting ontvangen");
      for (byte i = 0; i < canMsg.can_dlc; i++) {
        if (i == 0) {
          data0B = canMsg.data[i];
        }
        if (i == 1) {
          data1B = canMsg.data[i];
        }
        if (data0B == 0x03) {  //licht uit - achteruitrijlicht uit
          Verlichting = false;
          rear_light = false;
        }
        if (data0B == 0x13) {  //licht uit - achteruitrijlicht aan
          Verlichting = false;
          rear_light = true;
        }
        if (data0B == 0x07) {  //licht aan - achteruitrijlicht uit
          Verlichting = true;
          rear_light = false;
        }
        if (data0B == 0x17) {  //licht aan - achteruitrijlicht aan
          Verlichting = true;
          rear_light = true;
        }
      }
    }
    //if (id_ontvangen == 0x350) { dit voor het lezen van de stuurwiel knoppen
    //}
  }
  //eind canbus

  if (Verlichting == true) {
    digitalWrite(rasp_scherm_dim, HIGH);
    Serial1.println(1);
  } else {
    digitalWrite(rasp_scherm_dim, LOW);
    Serial1.println(2);
  }

  if (rear_light == true) {
    digitalWrite(rasp_rear_cam, HIGH);
    Serial1.println(3);
  } else {
    digitalWrite(rasp_rear_cam, LOW);
    Serial1.println(4);
  }
  //Serial.println(contact);
  if (contact == true) {
    blockB = false;
    digitalWrite(rasp_dead, LOW);
    digitalWrite(rasp_shutdown, HIGH);
    digitalWrite(Can_dead, LOW);
    shutdown = false;
    if (blockA == false) {
      previousMillisA = currentMillis;
      blockA = true;
    }
  }
  if (currentMillis - previousMillisA >= intervalA) {
    if (contact == false) {
      if (android == false) {
        digitalWrite(rasp_shutdown, LOW);
        digitalWrite(Can_dead, HIGH);
        if (blockB == false) {
          previousMillisB = currentMillis;
          blockB = true;
        }
        if (blockB == true) {
          if (currentMillis - previousMillisB >= intervalB) {
            digitalWrite(rasp_dead, HIGH);
            blockA = false;
          }
        }
      }
    }
  }
}