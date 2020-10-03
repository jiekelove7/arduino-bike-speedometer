/**
 * Code for the project
 * 
 * @version 1.2
 * 
 * Changelog:
 *  ?/?   -   File created
 *  ?.?   -   Basic structure created
 *  3/10  -   Merged changes from basic_test
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#define N_DIGITS 4
#define N_SELECT 3
#define DECIMAL_PLACES 2
#define DEFAULT_WHEEL_CIRCUMFERENCE 2.105


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

// Used in case the position of pins is modified
const int DEM_OFFSET = DEM_0 - 8;
const int BCD_OFFSET = OUT_A - 14;

// Mapping of digit displays
// Change according to map - ensure each digit [0 - 7] appears only once
int digitMap[8] = {0, 1, 2, 3, 4, 5, 6, 7};
volatile int digitSelect;

volatile double circumference;
double velocity = 0;
int seconds = 0;
double distance = 0;
volatile char num[N_DIGITS];



void setup() {
  pinMode(SENS, INPUT);

  pinMode(ENCA, INPUT);
  pinMode(ENCB, INPUT);
  pinMode(RESET, INPUT);
  pinMode(SETTINGS, INPUT);
 
  DDRB = DDRB | _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5);
  PORTB = PORTB & ~(_BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5));
  DDRD = DDRD | _BV(4) | _BV(5);
  PORTD = PORTD & ~(_BV(4) | _BV(5));

  DDRC = DDRC | _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4);
  PORTC = PORTC & ~(_BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4));

  cli();

  // TIMER2 set up to display digit
  // NOTICE:  TIMER0 is not used because TIMER0 overflow is by default in use
  //          and thus may not compile properly
  //          TIMER0 may be used instead if compiled and downloaded to arduino
  //          without the use of the official arduino application
  TCCR2A = 0;
  TCCR2B = _BV(CS21); // 8 bit prescaler - Increase prescaler to increase period
  TIMSK2 = _BV(TOIE2); // Enables the use of timer overflow interrupt

  // Timer 1 set up
  TCCR1A = 0;
  // CS10 - CS12  :  Manages prescaler - Set to
  TCCR1B = _BV(CS10);
  TCNT1 = 0; // Clears Timer Count



  sei();

  formatOutput(0.0, num);
  circum = DEFAULT_WHEEL_CIRCUMFERENCE;
  seconds = 0;
  digitSelect = 0;

  attachInterrupt(SENS, updateSens, RISING);
  attachInterrupt(RESET, reset, RISING);
  attachInterrupt(SETTINGS, changeSetting, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

/*
 * When TIMER2 overflows, a digit is displayed
 */
ISR(TIMER2_OVF_vect) {
  displayValue(0); 
  // change to 1 to test other display
  // displayValue(num, 1);
}

void updateSens() {
  //TBD
}

/*
 *  Resets 
 */ 
void reset() {
  //TBD
}

/*
 * Cycles through digits, displaying a value specified in num
 * displayN (0 - 1) specifies the display
 */
void displayValue(int displayN) {
  int decimal = 0;
  if(digitSelect == 1) decimal++;
  setDemulti(digitMap[digitSelect + (displayN * N_DIGITS)]);
  setDisplay(num[digitSelect], decimal);
  digitSelect = (digitSelect + 1) % N_DIGITS;

}

/*
 *  Updates setting
 */
void changeSetting() {
  //TBD
}

/*
 *  Formats number into a 4 digit display format
 *  An itoa function
 */
void formatOutput(float number, char *array) {
  int truncated = (int) (number * pow(10, DECIMAL_PLACES));
  for(int i = 0; i < N_DIGITS; i++) {
    array[N_DIGITS - i - 1] = truncated % 10;
    truncated = truncated / 10;
  }
}


/*
 * Sets BCD output for a digit on 7 segment display + decimal
 * Input MUST be in valid range (0 - 9)
 */
void setDisplay(char input, int decimal) {
  if(input < 10 && input >= 0) { 
    char shifted = input << BCD_OFFSET;
    PORTC = PORTC & ~(_BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4));
    PORTC |= input;
    if(decimal) {
      PORTC = PORTC | _BV(4); // Sets decimal light on
    }
  }
}

/*
 * Sets demultiplexer - Assumes that B3 - B5 are used as input
 * Input MUST be in valid range
 */
void setDemulti(char input) {
  if(input < 8 && input >= 0) { 
    char shifted = input << DEM_OFFSET; // B0 - B2 are not relevant
    PORTB = PORTB & ~(_BV(3) | _BV(4) | _BV(5)); // CLEAR DEMULTI
    PORTB |= shifted;
  }
}