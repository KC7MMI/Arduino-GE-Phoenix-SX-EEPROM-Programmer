/* For programming the X2212 NVRAM chip
 * in GE Phoenix SX radios
 */

/* defines the Arduino pins as X2212 pins for ease of use */
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

/* defines other constants used in the program */
#define DLY 10         //EEPROM write and store delay time
#define MAXCHARS 16    //for character string input
#define IF 45000000    //radio IF in Hz
#define VLO 136000000  //radio VHF bottom band limit in Hz
#define VHI 174000000  //radio VHF top band limit in Hz
#define VSTEP 5000     //radio VHF channel step
#define ULO 403000000  //radio UHF bottom band limit in Hz
#define UHI 512000000  //radio UHF top band limit in Hz
#define USTEP 12500    //radio UHF channel step
#define MAXCHAN 7      //8 channels in the mode
#define TOT 1          //time-out timer (m:ss): 0=0:30, 1=1:00, 2=1:30, 3=2:00, etc.

void setup() {
/* set modes and initialize pins */
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
  Serial.begin(9600);
}

/***** START OF LOOP *****/
void loop() {
  long rxFreq, txFreq;
  byte mode, chan, rxTone, txTone, okay;
  byte maskF = 0xF;
  long mmask1 = 0x1;
  long mmask3 = 0x3;
  long mmask7 = 0x7;
  long mmaskF = 0xF;
  
/* Variables and arrays for channel data */
  byte s3rx = 0;                                            //rx channel number
  byte s3tx = 0;                                            //tx channel number  
  byte s4xx[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};  //mode columns
  byte s5rx[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};  //rx data
  byte s5tx[8] = {0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};  //tx data

/* Gets mode and sets up data columns for appropriate mode */
  mode = getMode();
  for (int i = 0; i < 8; i++) {
    s4xx[i] = s4xx[i] + mode;          //sets value of mode column array for B mode columns
  }

/* Gets channel number data and assigns to channel variables */
  chan = getChan();
  s3tx = chan & maskF;                 //puts tx channel number into s3tx
  chan >>= 4;                          //shifts rx channel number four places into first nibble of chan
  s3rx = chan & maskF;                 //puts rx channel number into s3rx
  
/* Gets rx frequency data and assigns to rx array places 1-5 */
  rxFreq = getRXfreq();
  s5rx[4] = rxFreq & mmaskF;                   //sets channel data nibble 4
  rxFreq >>= 4;
  s5rx[5] = rxFreq & mmask3;                   //sets 2 least significant bits of channel data nibble 5
  rxFreq >>= 2;
  s5rx[5] = s5rx[5] + ((rxFreq & mmask1) * 8); //sets most significant bit of channel data nibble 5
  rxFreq >>= 1;
  s5rx[2] = rxFreq & mmaskF;                   //sets channel data nibble 2
  rxFreq >>= 4;
  s5rx[3] = rxFreq & mmaskF;                   //sets channel data nibble 3
  rxFreq >>= 4;
  s5rx[1] = (rxFreq & mmask1) * 8;             //sets most significant bit of channel data nibble 1
  rxFreq >>= 1;
  s5rx[1] = s5rx[1] + (rxFreq & mmask7);       //sets 3 least significant bits of channel data nibble 1 for oscillator/channel step data
  rxFreq >>= 3;                                //shifts the last bit out of rxFreq

/* Gets rx cg tone data and assigns to rx array places 6-7 */
  rxTone = getRXtone();
  s5rx[7] = rxTone & maskF;            //puts tx channel number into s3tx
  rxTone >>= 4;                        //shifts rx channel number four places into first nibble of chan
  s5rx[6] = rxTone & maskF;            //puts rx channel number into s3rx

/* Gets tx frequency data and assigns to tx array places 1-5 */
  txFreq = getTXfreq();
  s5tx[4] = txFreq & mmaskF;                   //sets channel data nibble 4
  txFreq >>= 4;
  s5tx[5] = txFreq & mmask3;                   //sets 2 least significant bits of channel data nibble 5
  txFreq >>= 2;
  s5tx[5] = s5tx[5] + ((txFreq & mmask1) * 8); //sets most significant bit of channel data nibble 5
  txFreq >>= 1;
  s5tx[2] = txFreq & mmaskF;                   //sets channel data nibble 2
  txFreq >>= 4;
  s5tx[3] = txFreq & mmaskF;                   //sets channel data nibble 3
  txFreq >>= 4;
  s5tx[1] = (txFreq & mmask1) * 8;             //sets most significant bit of channel data nibble 1
  txFreq >>= 1;                                //shifts the last bit out of rxFreq
  if ((mode == 0) && (s3tx == 0xF)) {          //if programming mode A tx channel 7
    s5tx[1] = s5tx[1] + MAXCHAN;               //program max channels into mode A
  }
  if ((mode == 0) && (s3tx == 0x1)) {          //if programming mode A tx channel 8
    s5tx[1] = s5tx[1] + TOT;                   //program time-out timer
  }
  if ((mode == 8) && (s3tx == 0x1)) {          //if programming mode B tx channel 8
    s5tx[1] = s5tx[1] + MAXCHAN;               //program max channels into mode B
  }
  
/* Gets rx cg tone data and assigns to rx array places 6-7 */
  txTone = getTXtone();
  s5tx[7] = txTone & maskF;            //puts tx channel number into s3tx
  txTone >>= 4;                        //shifts rx channel number four places into first nibble of chan
  s5tx[6] = txTone & maskF;            //puts rx channel number into s3rx

/* Gets final OK prior to writing data for a single channel to the EEPROM */
  okay = getOkay();
  if (okay) {
    for (int i = 0; i < 8; i++) {
      prog(s3rx, s4xx[i], s5rx[i]); //write RX data one byte (nibble) at a time
      prog(s3tx, s4xx[i], s5tx[i]); //write TX data one byte (nibble) at a time
    }
    store();
    printPlug(s3rx, s3tx, s4xx, s5rx, s5tx);
  }
  else {
    Serial.println("OPERATION CANCELED!\n\n");
  }
}
/***** END OF LOOP *****/

/* Simple prompt to advise user of invalid entry--called by other functions taking input */
void invalid() {
  Serial.println("INVALID ENTRY, TRY AGAIN");
}

/* Takes input for channel mode and converts to required hex value (0 for mode A or 8 for mode B) */
byte getMode() {
  byte modeVal = 0;
  int redo;
  Serial.println("NEW GE PHOENIX SX CHANNEL--");
  do {
    Serial.print("Mode(A/B): ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    char modeChar = str[0];
    if ((modeChar == 'A') || (modeChar == 'a')) {
      redo = 0;
      modeVal = 0;
      Serial.println('A');
    }
    else if ((modeChar == 'B') || (modeChar == 'b')) {
      redo = 0;
      modeVal = 8;
      Serial.println('B');
    }
    else {
      redo = 1;
      invalid();
    }
  } while (redo);
  return modeVal;
}

/* Takes input for channel number and converts to required hex value */
byte getChan() {
  byte chanHex = 0;
  int redo;
  do {
    Serial.print("Chan(1-8): ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    int chan = atoi(str);
    Serial.println(chan);
    switch (chan) {
    case 1:
      redo = 0;
      chanHex = 0x23;
      break;
    case 2:
      redo = 0;
      chanHex = 0x45;
      break;
    case 3:
      redo = 0;
      chanHex = 0x67;
      break;
    case 4:
      redo = 0;
      chanHex = 0x89;
      break;
    case 5:
      redo = 0;
      chanHex = 0xAB;
      break;
    case 6:
      redo = 0;
      chanHex = 0xCD;
      break;
    case 7:
      redo = 0;
      chanHex = 0xEF;
      break;
    case 8:
      redo = 0;
      chanHex = 0x01;
      break;
    default:
      redo = 1;
      invalid();
      break;
    }
  } while (redo);
  return chanHex;
}

/* Takes input for RX frequency float value sends it to mtoh() for conversion to long int in Hz */
long getRXfreq() {
  long freqHex = 0;
  int redo;
  do {
    Serial.print("RX Freq: ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    float freqMHz = atof(str);
    Serial.print(freqMHz, 4);
    Serial.print("MHz");
    long freqHz = mtoh(freqMHz);
    if ((freqHz >= VLO) && (freqHz <= VHI) && (freqHz % VSTEP == 0)) { //checks frequency within VHF band
      redo = 0;
      freqHex = 0x20000;                              //sets channel step to 5kHz with 13.2MHz oscillator
      freqHex = freqHex + ((freqHz + IF) / VSTEP);
      Serial.println("...OK");
    }
    else if ((freqHz >= ULO) && (freqHz <= UHI) && (freqHz % USTEP == 0)) {
      redo = 0;
      freqHex = 0x10000;                               //sets channel step to 12.5kHz with 13.2MHz oscillator
      freqHex = freqHex + ((freqHz + IF) / USTEP);     //adds frequency data to output variable
      Serial.println("...OK");
    }
    else {
      redo = 1;
      invalid();
    }
  } while (redo);
  return freqHex;
}

/* Takes input for Channel Guard float value */
byte getRXtone() {
  byte cgHex;
  int redo;
  do {
    Serial.print("RX Tone: ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    float data = atof(str);
    int cg = data * 10;                //converts tone freq (float) to int for workability
    cgHex = getToneHex(cg);
    Serial.print(data, 1);
    Serial.print("Hz");
    if (cgHex == 0xFF) {
      redo = 1;
      Serial.println(" ");
      invalid();
    }
    else {
      redo = 0;
      Serial.println("...OK");
    }
  } while (redo);
  return cgHex;
}

/* Takes input for TX frequency float value sends it to mtoh() for conversion to long int in Hz */
long getTXfreq() {
  long freqHex = 0;
  int redo;
  do {
    Serial.print("TX Freq: ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    float freqMHz = atof(str);
    Serial.print(freqMHz, 4);
    Serial.print("MHz");
    long freqHz = mtoh(freqMHz);
    if ((freqHz >= VLO) && (freqHz <= VHI) && (freqHz % VSTEP == 0)) {
      redo = 0;
      freqHex = freqHz / VSTEP;
      Serial.println("...OK");
    }
    else if ((freqHz >= ULO) && (freqHz <= UHI) && (freqHz % USTEP == 0)) {
      redo = 0;
      freqHex = freqHz / USTEP;
      Serial.println("...OK");
    }
    else {
      redo = 1;
      invalid();
    }
  } while (redo);
  return freqHex;
}

/* Takes input for Channel Guard float value */
byte getTXtone() {
  byte cgHex;
  int redo;
  do {
    Serial.print("TX Tone: ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    float data = atof(str);
    int cg = data * 10;                //converts tone freq (float) to int for workability
    cgHex = getToneHex(cg);
    Serial.print(data, 1);
    Serial.print("Hz");
    if (cgHex == 0xFF) {
      redo = 1;
      Serial.println(" ");
      invalid();
    }
    else {
      redo = 0;
      Serial.println("...OK");
    }
  } while (redo);
  return cgHex;
}

/* Gets final OK prior to programming EEPROM */
byte getOkay() {
  byte okayToProg;
  int redo;
  do {
    Serial.print("OK to Prog(Y/N): ");
    char str[MAXCHARS];
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        index = Serial.readBytesUntil('\n', str, MAXCHARS);
        str[index] = '\0';
        break;
      }
    }
    char okay = str[0];
    if ((okay == 'Y') || (okay == 'y')) {
      redo = 0;
      okayToProg = 1;
      Serial.println('Y');
    }
    else if ((okay == 'N') || (okay == 'n')) {
      redo = 0;
      okayToProg = 0;
      Serial.println('N');
    }
    else {
      redo = 1;
      invalid();
    }
  } while (redo);
  return okayToProg;
}

/* Converts freq in MHz (float) to freq in Hz (long integer) to avoid precision issues with floating point values */
long mtoh(float mhz) {
  long hz = (int)mhz * 1000000;
  float k10 = (mhz - (int)mhz) * 10000;  //multiplies kHz by 10,000 to prepare for rounding to nearest .5kHz
  long khz = (round(k10)) * 100;         //converts kHz to long int
  hz = hz + khz;                         //adds newly converted kHz to previously converted MHz for final long int frequency value
  return hz;
} 

/* Returns GE Channel Guard hex values based on inputted CG frequency */
byte getToneHex(int freq) {
  byte hexTone;
  switch (freq) {
    case 0:                //CG off
      hexTone = 0x00;
      break;
    case 670:              //67.0Hz tone
      hexTone = 0x10;
      break;
    case 719:              //71.9Hz tone
      hexTone = 0x20;
      break;
    case 744:              //etc...
      hexTone = 0x30;
      break;
    case 770:
      hexTone = 0x40;
      break;
    case 797:
      hexTone = 0x50;
      break;
    case 825:
      hexTone = 0x60;
      break;
    case 854:
      hexTone = 0x70;
      break;
    case 885:
      hexTone = 0x80;
      break;
    case 915:
      hexTone = 0x90;
      break;
    case 948:
      hexTone = 0xA0;
      break;
    case 974:
      hexTone = 0x12;
      break;
    case 1000:
      hexTone = 0xB0;
      break;
    case 1035:
      hexTone = 0xC0;
      break;
    case 1072:
      hexTone = 0xD0;
      break;
    case 1109:
      hexTone = 0xE0;
      break;
    case 1148:
      hexTone = 0xF0;
      break;
    case 1188:
      hexTone = 0x01;
      break;
    case 1230:
      hexTone = 0x11;
      break;
    case 1273:
      hexTone = 0x21;
      break;
    case 1318:
      hexTone = 0x31;
      break;
    case 1365:
      hexTone = 0x41;
      break;
    case 1413:
      hexTone = 0x51;
      break;
    case 1462:
      hexTone = 0x61;
      break;
    case 1514:
      hexTone = 0x71;
      break;
    case 1567:
      hexTone = 0x81;
      break;
    case 1622:
      hexTone = 0x91;
      break;
    case 1679:
      hexTone = 0xA1;
      break;
    case 1738:
      hexTone = 0xB1;
      break;
    case 1799:
      hexTone = 0xC1;
      break;
    case 1862:
      hexTone = 0xD1;
      break;
    case 1928:
      hexTone = 0xE1;
      break;
    case 2035:
      hexTone = 0xF1;
      break;
    case 2107:
      hexTone = 0x02;
      break;
    default:
      hexTone = 0xFF;
      break;
  }
  return hexTone;
}

/* Breaks each nibble down to individual bits and writes out to pins */
void prog(byte s3, byte s4, byte s5) {
  int SW3[4] = {AD4, AD5, AD6, AD7}; //pin array for SW3 - channel number
  int SW4[4] = {AD0, AD1, AD2, AD3}; //pin array for SW4 - data columns
  int SW5[4] = {IO1, IO2, IO3, IO4}; //pin array for SW5 - data
  byte mask = 1;
  for(int i = 0; i < 4; i++) {
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
    mask <<= 1;                      //shifts the bit mask left once for determining value of each pin
  }
  wrt();
}

/* Pulses EEPROM write pin to temporarily store nibble */
void wrt() {
  delay(DLY);
  digitalWrite(WRT, LOW);
  delay(DLY);
  digitalWrite(WRT, HIGH);
  delay(DLY);
}

/* Pulses EEPROM store pin to store changes */
void store() {
  delay(DLY);
  digitalWrite(STO, LOW);
  delay(DLY);
  digitalWrite(STO, HIGH);
  Serial.println("STORED CODEPLUG:");
}

/* Serial prints individual channel codeplug after programming for reference and troubleshooting */
void printPlug(byte s3rxdata, byte s3txdata, byte s4data[], byte s5rxdata[], byte s5txdata[]) {
  Serial.print("  ");
  for (int i = 0; i < 8; i++) {
    Serial.print(s4data[i], HEX);    //prints data columns
  }
  Serial.println(" ");
  Serial.print(s3rxdata, HEX);       //prints rx channel number data
  Serial.print(" ");
  for (int i = 0; i < 8; i++) {
    Serial.print(s5rxdata[i], HEX);  //prints rx channel data
  }
  Serial.println(" ");
  Serial.print(s3txdata, HEX);       //prints tx channel number data
  Serial.print(" ");
  for (int i = 0; i < 8; i++) {
    Serial.print(s5txdata[i], HEX);  //prints tx channel data
  }
  Serial.println("\n\n");
}
