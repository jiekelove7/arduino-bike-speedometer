/**
 * Basic test - Rotary not fully implemented
 * Tests multiplexer + 4 digit displays
 * 
 * Changelog:
 *  29/9 - Added ability to map digits
 */
#include <avr/io.h>

#define N_DIGITS 4
#define N_SELECT 3
#define DECIMAL_PLACES 2
#define N_DEBUG 3 // number of settings for debug

// Inputs of Rotary Encoder Switch
const int ENCB = 0;
const int ENCA = 1;
const int RESET = 2;
const int SETTINGS = 3;

// Sensor in
const int SENS = 8; // B0

// Outputs to BCD decoder
const int OUT_A = 14; // C0, lowest significant bit
const int OUT_B = 15;
const int OUT_C = 16;
const int OUT_D = 17; // C3
const int DECIMAL = 18; // C4

// Outputs to LED's (For mode)
const int MODE_A = 4; // D4
const int MODE_B = 5; // D5
const int MODE_C = 9; // B1
const int MODE_D = 10; // B2

// Outputs to Demultiplexer
const int DEM_0 = 11; // B3
const int DEM_1 = 12; // B4
const int DEM_2 = 13; // B5

const int DEM_OFFSET = DEM_0 - 8;
const int BCD_OFFSET = OUT_A - 14;

// Mapping of digit displays
// Change according to map - ensure each digit [0 - 7] appears only once
int digitMap[8] = {0, 1, 2, 3, 4, 5, 6, 7};


// may require volatile if modified during button press instead of loop
char num[N_DIGITS];
volatile float testcount;

// 0 - increment by 1
// 1 - increment by 2
// 2 - increment by 0.01
// 3 - increment by 0.10
volatile int debugsetting;



void setup() {

  // Sets B
  //DDRB &= 0b11111110; // B0 as input (SENS)
  pinMode(SENS, INPUT);

  pinMode(ENCA, INPUT);
  pinMode(ENCB, INPUT);
  pinMode(RESET, INPUT);
  pinMode(SETTINGS, INPUT);

  DDRB |= 0b00111110; // B1 - B5 as output
  PORTB &= 0b11000001;
  DDRD |= 0b00110000; // D4, D5 as output
  PORTD &= 0b11001111; 
  // pinMode(OUT_A, OUTPUT);
  // pinMode(OUT_B, OUTPUT);
  // pinMode(OUT_C, OUTPUT);
  // pinMode(OUT_D, OUTPUT);
  // pinMode(DECIMAL, OUTPUT);
  // pinMode(MODE_A, OUTPUT);
  // pinMode(MODE_B, OUTPUT);
  // pinMode(MODE_C, OUTPUT);
  // pinMode(MODE_D, OUTPUT);
  // pinMode(DEM_0, OUTPUT);
  // pinMode(DEM_1, OUTPUT);
  // pinMode(DEM_2, OUTPUT);
  // If using pinMode, also manually set PORTs using digitalWrite();

  testcount = 0.0;
  debugsetting = 0;

  attachInterrupt(SENS, updateCount, RISING);
  attachInterrupt(RESET, reset, RISING);
  attachInterrupt(SETTINGS, changeSetting, RISING);

}

void loop() {
  formatOutput(testcount, num);
  displayValue(num, 0); 
  // change to 1 to test other display
  // displayValue(num, 1);

}

void updateCount() {
  switch(debugsetting) {
      case 3 :
        testcount += 0.01;
        break; 
      case 2 :
        testcount += 0.1;
        break;
      case 1 :
        testcount += 1;
      default :
        testcount += 1;
    }
}

/*
 *  Resets the count
 */ 
void reset() {
  testcount = 0;
}

/*
 *  Updates setting
 */
void changeSetting() {
  debugsetting = (debugsetting + 1) % N_DEBUG;
}

/*
 *  Formats number into a 4 digit display format
 *  
 */
void formatOutput(float number, char *array) {
  int truncated = (int) (number * pow(10, DECIMAL_PLACES));
  for(int i = 0; i < N_DIGITS; i++) {
    array[N_DIGITS - i - 1] = truncated % 10;
    truncated = truncated / 10;
  }
}

/*
 * Cycles through digits, displaying a value
 * displayN (0 - 1) specifies the display
 */
void displayValue(char *input, int displayN) {
  int decimal;
  for(int i = 0; i < N_DIGITS; i++) {
    decimal = 0;
    setD(digitMap[i] + (displayN * N_DIGITS));
    if(i == 1) {
      decimal = 1;
    }
    setA(input[i], decimal);
  }
}

/*
 * Sets BCD output for a digit on 7 segment display + decimal
 * Input MUST be in valid range (0 - 9)
 */
void setA(char input, int decimal) {
  if(input < 10 && input >= 0) { 
    char shifted = input << BCD_OFFSET;
    PORTD &= 0b11100000;
    PORTD |= input;
    if(decimal) {
      PORTD |= 0b00010000; // Sets decimal light on
    }
  }
}

/*
 * Sets demultiplexer - Assumes that B3 - B5 are used as input
 * Input MUST be in valid range
 */
void setD(char input) {
  if(input < 8 && input >= 0) { 
    char shifted = input << DEM_OFFSET; // B0 - B2 are not relevant
    PORTB &= 0b11000111; // CLEAR DEMULTI
    PORTB |= shifted;
  }
}
