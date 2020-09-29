/*
 * Attempt 1 of code for Project
 */
#include <avr/io.h>

#define N_DIGITS 4
#define N_SELECT 3
#define DEFAULT_WHEEL_CIRCUMFERENCE 2.105


// Set as 0 or 1
#define DEBUG_SETTINGS 0


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

double circum;
double velocity = 0;
int seconds = 0;
double distance = 0;
char num[N_DIGITS];



void setup() {

  // Sets B
  //DDRB &= 0b11111110; // B0 as input (SENS)
  pinMode(SENS, LOW);
  DDRB |= 0b00111110; // B1 - B5 as output
  PORTB &= 0b11000001;
  DDRD |= 0b00110000; // D4, D5 as output
  PORTD &= 0b11001111; 
  // pinMode(OUT_A, HIGH);
  // pinMode(OUT_B, HIGH);
  // pinMode(OUT_C, HIGH);
  // pinMode(OUT_D, HIGH);
  // pinMode(DECIMAL, HIGH);
  // pinMode(MODE_A, HIGH);
  // pinMode(MODE_B, HIGH);
  // pinMode(MODE_C, HIGH);
  // pinMode(MODE_D, HIGH);
  // pinMode(DEM_0, HIGH);
  // pinMode(DEM_1, HIGH);
  // pinMode(DEM_2, HIGH);
  // If using pinMode, also manually set PORTs using digitalWrite();




  circum = DEFAULT_WHEEL_CIRCUMFERENCE;
  seconds = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

/*
 * Cycles through digits, displaying a value
 * displayN (0 - 1) specifies the display
 */
void displayValue(char *input, int displayN) {
  int decimal;
  for(int i = 0; i < N_DIGITS; i++) {
    decimal = 0;
    setD(i + (displayN * N_DIGITS));
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
