//code compiled in the Arduino environment
//may lack inclusion of avr-libc libraries in the source - the environment implicitly includes them

#define LEFT_PWM OCR0B
#define RIGHT_PWM OCR0A

#define JOY_CENTER_X 499
#define JOY_CENTER_Y 506
#define JOY_DEADZONE 10

#define JOY_FORWARD_MAX 506
#define JOY_REVERSE_MAX 517

#define JOY_LEFT_MAX 524
#define JOY_RIGHT_MAX 499

//function calculating the absolute value
int myabs(int value) {
  if (value >= 0)
    return value;
  else
    return -value;
}

//structure for joystick readings
typedef struct joy_t {
  int x;
  int y;
} Joy;

Joy joystick;

//reading the joystick deflection in vertical and horizontal axes from the analog-to-digital converter
void readJoy() {
  joystick.x = readADC(0);
  joystick.y = readADC(1);
}

int readADC(uint8_t channel) {
  //set the ADC read channel in the MUX register
  ADMUX = (1 << REFS0) | (channel & 0x07);

  //set in the control register to start ADC conversion (ADSC bit)
  ADCSRA |= (1 << ADSC);   

  //wait for the conversion to complete
  //after conversion, the ADIF flag is set in the ADCSRA register
  while (!(ADCSRA & (1 << ADIF)));

  //the conversion result can be obtained through the ADCW macro
  //the macro allows to obtain a 16-bit final read value from the ADC
  //even though the result is actually stored in 2 8-bit registers
  return ADCW;  
}

//setting motor rotation directions according to the connection of pins to the driver
void forward() {
  PORTD = (1 << PD4);
  PORTB = (1 << PB0);
}

void reverse() {
  PORTD = (1 << PD3) | (1 << PD7);
  PORTB = 0x00;
}

void left() {
  PORTD = (1 << PD3);
  PORTB = (1 << PB0);
}

void right() {
  PORTD = (1 << PD4) | (1 << PD7);
  PORTB = 0x00;
}

int main() {
  //setting pin 0 of port B as output
  DDRB |= (1 << PB0);

  //setting pins 3, 4, 5, and 6 of port D as outputs
  DDRD |= (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);

  //timer counter control register A for timer 0
  //WGM00 and WGM01 for Fast PWM mode (mode 3)
  //COM0A1 non-inverting mode for the output
  TCCR0A |= (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);

  //timer counter control register B for timer 0
  //CS0x - prescaler setting bits
  //CS00 === no prescaler
  TCCR0B |= (1 << CS00);

  //ADC multiplexer set to external reference voltage
  //MUX bits are used for channel selection
  ADMUX |= 1 << REFS0;

  //ADC control and status register A
  //ADEN - ADC enable
  //ADPSx - ADC prescaler (111 gives 128)
  ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

  //enable external interrupts
  sei();

  while (true) {
    //read the joystick
    readJoy();

    //get the joystick deflection from the center position
    //needed for calculating the proportional value of the PWM control signal fill
    double abs_x = myabs(joystick.x - JOY_CENTER_X);
    double abs_y = myabs(joystick.y - JOY_CENTER_Y);
    double pwm = 0;

    //if the joystick was not deflected from the center position in the x-axis
    //set the appropriate movement directions depending on the deflection from the y-axis
    if (abs_x < JOY_DEADZONE) {
      if (joystick.y < JOY_CENTER_Y) {
        forward();
        pwm = abs_y/JOY_FORWARD_MAX*255;
      }
      else {
        reverse();
        pwm = abs_y/JOY_REVERSE_MAX*255;
      }
    }
    //otherwise if the joystick was not deflected from the center position in the y-axis
    //set the appropriate movement directions depending on the deflection from the x-axis
    else if (abs_y < JOY_DEADZONE) {
      if (joystick.x < JOY_CENTER_X) {
        right();
        pwm = abs_x/JOY_RIGHT_MAX*255;
      }
      else {
        left();
        pwm = abs_x/JOY_LEFT_MAX*255;
      }
    }

    //set the PWM signal fill
    LEFT_PWM = (int)pwm;
    RIGHT_PWM = (int)pwm;
  }
}
