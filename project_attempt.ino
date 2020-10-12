/**
 * Code for the project
 * 
 * @version 1.4
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

#define N_DISPLAYS 2
#define N_DIGITS 4
#define N_SETTINGS 4
#define N_NUMBERS 10

#define DECIMAL_PLACES 2
#define DEFAULT_WHEEL_CIRCUMFERENCE 2.105

// Mapping for array values[] (See below)
#define DISTANCE 0
#define SPEED_MAX 1
#define SPEED_AVE 2
#define CIRC_SETTING 3


#define TIMER1_RESET    TCNT1 = 0
#define LED_RESET       PORTD = PORTD & ~(_BV(0) | _BV(1) | _BV(5) | _BV(7))
#define DEMULTI_RESET   PORTB = PORTB & ~(_BV(3) | _BV(4) | _BV(5))
#define BCD_RESET       PORTC = PORTC & ~(_BV(0) | _BV(1) | _BV(3) | _BV(4) | _BV(5))

// Summary of Pins:
// Inputs of Rotary Encoder Switch
// ENCA       6   D6       
// ENCB       4   D4
// SWITCH     2   D2

// Sensor in
// SENS       8   B0

// Outputs to BCD decoder
// OUT_A      14  C0      lowest significant bit
// OUT_B      19  C5
// OUT_C      18  C4
// OUT_D      15  C1
// DECIMAL    17  C3

// Outputs to LED's (For mode)
// MODE_A     2   D2
// MODE_B     1   D1
// MODE_C     5   D5
// MODE_D     7   D7

// Outputs to Demultiplexer
// DEM_0      11  B3
// DEM_1      12  B4
// DEM_2      13  B5

// Mapping of LED Outputs in Port D
// LED numbers assigned from top to bottom on hardware [0:3]
// LEDMap order: {0, 1, 2, 3}
byte LEDMap[N_SETTINGS] = {0x02, 0x01, 0x20, 0x80};

// Mapping of Multiplexer to display values in Port B
// Display1 represents display on the left, Display2 represents display on the right
// Mapping of Displays [left:right] - D1 [0:3], D2 [4:7]
// displayMap order: {0, 1, 2, 3, 4, 5, 6, 7}
byte displayMap[N_DISPLAYS * N_DIGITS] = {0x30, 0x38, 0x00, 0x28, 0x10, 0x20, 0x18, 0x08};

// Mapping of digits to Port C 
// digitMap Order: {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
byte digitMap[N_NUMBERS] = {0x00, 0x01, 0x20, 0x21, 0x10, 0x11, 0x30, 0x31, 0x02, 0x03};
volatile int digitSelect;

volatile double circumference;

// One display used for strictly speed
float speed;

// Structure used to hold values
// value = {DISTANCE, SPEED_MAX, SPEED_AVE, CIRC_SETTING};
float value[N_SETTINGS] = {0.0, 0.0, 0.0, DEFAULT_WHEEL_CIRCUMFERENCE};

volatile int revolutions;
volatile int setting;
// The first reading from idle should be ignored
volatile int capture_enabled;

// Number to be shown on first display
// Defaulted to strictly show speed
volatile char num[N_DIGITS];
// Number to be shown on second display
// Shows data from value[] depending on setting
volatile char num2[N_DIGITS];

// Boolean to manage button press down and release
volatile int button;
// Time counted via TIMER2 OVF of button press-down
volatile int buttonTime;

// Used to poll rotary encoder
int ENCA_old;


void setup() {
  // Configure ports 
  // Sensor input
  // DDRB = DDRB & ~(_BV(0));
  // Rotary Encoder + Button {BUTTON, ENCB, ENCA}
  DDRD = DDRD & ~(_BV(2) | _BV(4) | _BV(6));
  PORTD = PORTD | _BV(2) | _BV(4) | _BV(6);

  // Settings LEDs
  DDRD = DDRD | _BV(0) | _BV(1) | _BV(5) | _BV(7);
  LED_RESET;
  // Demultiplexer
  DDRB = DDRB | _BV(3) | _BV(4) | _BV(5);
  DEMULTI_RESET;
  // BCD Decoder + Decimal point
  DDRC = DDRC | _BV(0) | _BV(1) | _BV(3) | _BV(4) | _BV(5);
  BCD_RESET;

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



  speed = 0.0;
  formatOutput(0.0, num); // Sets num to 0
  formatOutput(0.0, num2); // Sets num2 to 0
  circumference = value[CIRC_SETTING];
  digitSelect = 0;
  revolutions = 0;
  

  setting = DISTANCE; // default shows distance
  LED_RESET; // Clears LEDs
  PORTD = PORTD | LEDMap[setting];

  // Interrupt for button press
  attachInterrupt(digitalPinToInterrupt(2), buttonPress, CHANGE);

  capture_enabled = 0;
  sei(); // Enable interrupts
  TIMER1_RESET;
}

/**
 *  Polling method of reading encoder inputs/outputs
 */
void loop() {
  // put your main code here, to run repeatedly:

  if(setting == CIRC_SETTING) {
    int ENCA = PIND & _BV(6);
    int ENCB = PIND & _BV(4);

    if(!ENCA && ENCA_old) {
      if(ENCB) {
        // increment
        circumference = circumference + 0.01;
        value[CIRC_SETTING] = circumference;
        formatOutput(value[CIRC_SETTING], num2);
      } else {
        // decrement
        if((circumference - 0.01) > 0.01) {
          circumference = circumference - 0.01;
          formatOutput(value[CIRC_SETTING], num2);
        }
      }
    }
    
    ENCA_old = ENCA;
  }
  
}

/**
 *  Holding button changes setting, tapping resets
 */
void buttonPress() {
  button = !button;
  if(!button) {
    if(buttonTime > 1000) {
      changeSetting();
    } else {
      reset();
    }
    buttonTime = 0;
  }
}

/*
 * When TIMER2 overflows, a digit is displayed
 * Cycles through digits, displaying a value specified in num
 * to one display, and the value in num2 to the other
 */
ISR(TIMER2_OVF_vect) {
  int decimal = 0;
  if(digitSelect == 1 | digitSelect == 5) decimal++;
  setDemulti(digitSelect);
  if(digitSelect < 4) {
    setDisplay(num[digitSelect], decimal);
  } else {
    setDisplay(num2[digitSelect - 4], decimal); 
  }
  digitSelect = (digitSelect + 1) % (N_DIGITS * 2);
  // For button
  if(button) {
    buttonTime++;
  }
}

/*
 * When TIMER1 reaches OCR1A, the bike is assumed to be idle
 * Set to 3 seconds
 */
ISR(TIMER1_COMPA_vect) {
  //
  speed = 0.0;
  formatOutput(speed, num);
  capture_enabled = 0;
}

/*
 * Input from hall sensor 
 */
ISR(TIMER1_CAPT_vect) {
  //TBD
  if(capture_enabled) {
    speed = (28125 * circumference)/ ICR1; // 3.6 * ICR1-seconds factor

    value[DISTANCE] = (++revolutions * circumference) / 1000; // Display in km

    value[SPEED_MAX] = speed > value[SPEED_MAX] ? speed : value[SPEED_MAX]; 

    value[SPEED_AVE] = ((revolutions - 1) * value[SPEED_AVE] + speed) / revolutions;  

    formatOutput(speed, num);
    formatOutput(value[setting], num2);
  } else {
    capture_enabled = !capture_enabled;
  }

  TIMER1_RESET;
}

/*
 *  Resets distance
 */ 
void reset() {
  value[DISTANCE] = 0.0;
  if(setting == DISTANCE) {
    formatOutput(value[DISTANCE], num2);
  }
  revolutions = 0;
}


/*
 *  Toggles value to display in num2
 *  Also changes the settings LED
 */
void changeSetting() {
  setting = (setting + 1) % N_SETTINGS;
  LED_RESET; 
  PORTD = PORTD | LEDMap[setting];
  formatOutput(value[setting], num2);
}

/*
 *  Formats number into a array to be more easily
 *  multiplexed
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
 * Input MUST be in valid range (0:9)
 */
void setDisplay(int input, int decimal) {
  if(input >= 0 && input <= 9) {
    BCD_RESET;
    PORTC = PORTC | digitMap[input];
    if(decimal) {
      PORTC = PORTC | _BV(3);
    }
  }
}

/*
 * Sets demultiplexer - [0:7] where [0:3] is D1
 * [4:7] is D2
 * @param input MUST be in valid range [0:7]
 */
void setDemulti(int input) {
  if(input >= 0 && input <= 7) {
    DEMULTI_RESET;
    PORTB = PORTB | displayMap[input];
  }
}