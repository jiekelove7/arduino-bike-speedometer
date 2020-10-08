/**
 * Code for the project
 * 
 * @version 1.2
 * 
 * Changelog:
 *  ?/?   -   File created
 *  ?.?   -   Basic structure created
 *  3/10  -   Merged changes from basic_test
 *        -   Refined comments
 *        -   Removed unecessary constants (See old vers. if required)
 *        -   Updated from using pin-read interrupt to using capture
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#define N_DIGITS 4
#define DECIMAL_PLACES 2
#define DEFAULT_WHEEL_CIRCUMFERENCE 2.105
// Mapping for array values[] (See below)
#define N_VALUES 3
#define DISTANCE 0
#define SPEED_MAX 1
#define SPEED_AVE 2


#define TIMER1_RESET TCNT1 = 0

// Summary of Pins:
// Inputs of Rotary Encoder Switch
// ENCB       0   D0
// ENCA       1   D1
// RESET      2   D2
// SETTINGS   3   D3

// Sensor in
// SENS       8   B0

// Outputs to BCD decoder
// OUT_A      14  C0      lowest significant bit
// OUT_B      15  C1
// OUT_C      16  C2
// OUT_D      17  C3
// DECIMAL    18  C4

// Outputs to LED's (For mode)
// MODE_A     4   D4
// MODE_B     5   D5
// MODE_C     9   B1
// MODE_D     10  B2

// Outputs to Demultiplexer
// DEM_0      11  B3
// DEM_1      12  B4
// DEM_2      13  B5

#define OUT_A 14
#define DEM_0 11

// Used in case the position of pins is modified
const int DEM_OFFSET = DEM_0 - 8;
const int BCD_OFFSET = OUT_A - 14;

// Mapping of Multiplexer to display values in Port B
// Display1 represents display on the left, Display2 represents display on the right
// Mapping of Displays [left:right] - D1 [0:3], D2 [4:7]
// displayMap order: {0, 1, 2, 3, 4, 5, 6, 7}
byte displayMap[8] = {0x30, 0x38, 0x00, 0x28, 0x10, 0x20, 0x18, 0x08};

// Mapping of digits to Port C 
// digitMap Order: {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
byte digitMap[10] = {0x00, 0x01, 0x20, 0x21, 0x10, 0x11, 0x30, 0x31, 0x02, 0x03};
volatile int digitSelect;

volatile double circumference;

// One display used for strictly speed
float speed;

// Structure used to hold values
// value = {DISTANCE, SPEED_MAX, SPEED_AVE};
float value[N_VALUES] = {0.0, 0.0, 0.0};
volatile int revolutions;
volatile int setting;

// Number to be shown on first display
// Defaulted to strictly show speed
volatile char num[N_DIGITS];
// Number to be shown on second display
// Shows data from value[] depending on setting
volatile char num2[N_DIGITS];


void setup() {
  // Configure ports 
  // Sensor input
  // DDRB = DDRB & ~(_BV(0));
  // Rotary Encoder + Buttons {ENCA, ENCB, RESET, SETTINGS}
  DDRD = DDRD & ~(_BV(0) | _BV(1) | _BV(2) | _BV(3));

  // Demultiplexer + Settings LEDs
  DDRB = DDRB | _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5);
  PORTB = PORTB & ~(_BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5));
  // Settings LEDs
  DDRD = DDRD | _BV(4) | _BV(5);
  PORTD = PORTD & ~(_BV(4) | _BV(5));
  // BCD Decoder + Decimal point
  DDRC = DDRC | _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4);
  PORTC = PORTC & ~(_BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4));

  cli(); // Disable interrupts

  // TIMER2 set up to display digit
  // NOTICE:  TIMER0 is not used because TIMER0 overflow is by default in use
  //          and thus may not compile properly
  //          TIMER0 may be used instead if compiled and downloaded to arduino
  //          without the use of the official arduino application
  TCCR2A = 0;
  TCCR2B = _BV(CS21); // 8 bit prescaler - Increase prescaler to increase period
  TIMSK2 = _BV(TOIE2); // Enables the use of timer overflow interrupt

  // TIMER1 set up to time the wheel
  // A maximum period is set in order to detect idleness
  TCCR1A = 0;
  TCCR1B = _BV(CS10) | _BV(CS12); // 16 bit prescaler - Set to 1024 

  // Enables CTC mode comparison with OCR1A  
  // Note that the physical pin itself is not connected for the timer
  TCCR1B = TCCR1B | _BV(WGM12);

  TIMSK1 = _BV(OCIE1A); // Enables use of timer compare interrupt

  // Set the maximum count for the timer
  // The formula is given as:   OCR1A = (Clock Frequency) / (N * (Desired Frequency)) - 1
  //                            where N is the set prescaler {1, 8, 64, 256, 1024}
  OCR1A = 46874; // Set to 3 seconds
  
  // Enables Capture mode to read input
  // ICNC1 - Noise Canceler, ICES1 - Rising edge triggered input capture
  TCCR1B = TCCR1B | _BV(ICNC1) | _BV(ICES1);

  // Enables capture interrupt
  TIMSK1 = TIMSK1 | _BV(ICIE1);


  sei(); // Enable interrupts

  speed = 0.0;
  formatOutput(0.0, num); // Sets num to 0
  circumference = DEFAULT_WHEEL_CIRCUMFERENCE;
  digitSelect = 0;
  

  setting = DISTANCE; // default

  // attachInterrupt(SENS, updateSens, RISING);
  attachInterrupt(RESET, reset, RISING);
  attachInterrupt(SETTINGS, changeSetting, RISING);
  TIMER1_RESET;
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

/*
 * When TIMER2 overflows, a digit is displayed
 * Cycles through digits, displaying a value specified in num
 * to one display, and the value in num2 to the other
 * This MAY not work correctly  : consider increasing prescaler?
 */
ISR(TIMER2_OVF_vect) {
  int decimal = 0;
  if(digitSelect == 1 || digitSelect == 5) decimal++;
  setDemulti(digitMap[digitSelect]);
  if(digitSelect < 4) {
    setDisplay(num[digitSelect]);
  } else {
    setDisplay(num2[digitSelect - N_DIGITS]);
  }
  digitSelect = (digitSelect + 1) % (N_DIGITS * 2);
}

/*
 * When TIMER1 reaches OCR1A, the bike is assumed to be idle
 */
ISR(TIMER1_COMPA_vect) {
  //
  speed = 0.0;
  formatOutput(0.0, num);

}

/*
 * Input from hall sensor 
 */
ISR(TIMER1_CAPT_vect) {
  //TBD


  TIMER1_RESET;
}

/*
 *  Resets distance
 */ 
void reset() {
  value[0] = 0;
}


/*
 *  Toggles value to display in num2
 */
void changeSetting() {
  setting = (setting + 1) % N_VALUES;
}

/*
 *  Formats number into a 4 digit display format
 *  An itoa function
 */
void formatOutput(float number, char *array) {
  int truncated = (int) ((number * pow(10, DECIMAL_PLACES)) + 0.01);
  for(int i = 0; i < N_DIGITS; i++) {
    array[N_DIGITS - i - 1] = truncated % 10;
    truncated = truncated / 10;
  }
}


/*
 * Sets BCD output for a digit on 7 segment display + decimal
 * Input MUST be in valid range (0 - 9)
 */
void setDisplay(int input) {
  if(input >= 0 && input <= 9) {
    PORTC = 0;
    PORTC = PORTC | digitMap[input];
  }
}

/*
 * Sets demultiplexer - [0:7] where [0:3] is D1
 * [4:7] is D2
 * @param input MUST be in valid range [0:7]
 */
void setDemulti(int input) {
  if(input >= 0 && input <= 7) {
    PORTB = 0;
    PORTB = PORTB | displayMap[input];
  }
}