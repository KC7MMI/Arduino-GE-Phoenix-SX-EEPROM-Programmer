/* For programming the X2212 NVRAM chip
 * in GE Phoenix SX radios
 */

//defines the Arduino pins as X2212 pins for ease of use
#define WRT 2
#define AD7 6
#define AD4 7
#define AD3 8
#define AD2 9
#define AD1 10
#define AD0 11
#define STO 12
#define LED 13
#define IO1 14
#define IO2 15
#define IO3 16
#define IO4 17
#define AD5 18
#define AD6 19
#define DLY 10

void setup() {
//set modes and initialize pins
  for(int pin = 2; pin < 20; pin++) {
    pinMode(pin, OUTPUT);
  }
  for(int pin = 6; pin < 12; pin++) {
    digitalWrite(pin, LOW);
  }
  for(int pin = 13; pin < 20; pin++) {
    digitalWrite(pin, LOW);
  }
  digitalWrite(WRT, HIGH);
  digitalWrite(STO, HIGH);

//channel data s3, s4, and s5 are the hex switches described in the manual programmer manual
  byte s3rx = 0x2; //ch 1 rx                                //rx channel number--refer to manual for hex values for other channels
  byte s3tx = 0x3; //ch 1 tx                                //tx channel number--refer to manual for hex values for other channels  
  byte s4ax[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};  //mode A columns--for mode B, add 8 to each value
  byte s5rx[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};  //enter rx data per GE Phoenix programming worksheet
  byte s5tx[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};  //enter tx data per GE Phoenix programming worksheet

//begin programming
  digitalWrite(LED, HIGH);
  for(int i = 0; i < 8; i++) {
    prog(s3rx, s4ax[i], s5rx[i]); //write RX data one byte (nibble) at a time
    prog(s3tx, s4ax[i], s5tx[i]); //write TX data one byte (nibble) at a time
  }
  store();
  digitalWrite(LED, LOW);
}

//breaks each nibble down to individual bits and writes out to pins
void prog(byte s3, byte s4, byte s5) {
  int SW3[4] = {AD4, AD5, AD6, AD7}; //pin array for SW3 - channel number
  int SW4[4] = {AD0, AD1, AD2, AD3}; //pin array for SW4 - data columns
  int SW5[4] = {IO1, IO2, IO3, IO4}; //pin array for SW5 - data
  byte mask = 1;
  for(int i = 0; i < 4; i++) {       //checks value of each bit and writes each nibble to appropriate pins
    if(s3 & mask) {
      digitalWrite(SW3[i], HIGH);
    }
    else {
      digitalWrite(SW3[i], LOW);
    }
    if(s4 & mask) {
      digitalWrite(SW4[i], HIGH);
    }
    else {
      digitalWrite(SW4[i], LOW);
    }
    if(s5 & mask) {
      digitalWrite(SW5[i], HIGH);
    }
    else {
      digitalWrite(SW5[i], LOW);
    }
    mask <<= 1;
  }
  wrt();
}

//writes nibble to address
void wrt() {
  delay(DLY);
  digitalWrite(WRT, LOW);
  delay(DLY);
  digitalWrite(WRT, HIGH);
  delay(DLY);
}

//stores changes to EEPROM
void store() {
  delay(DLY);
  digitalWrite(STO, LOW);
  delay(DLY);
  digitalWrite(STO, HIGH);
}

void loop() {
}
