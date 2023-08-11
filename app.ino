#include <LiquidCrystal_I2C.h>

#define echoPin 2 // attach pin D2 Arduino to pin Echo of HC-SR04
#define trigPin 5 //attach pin D3 Arduino to pin Trig of HC-SR04

int ledState = LOW;             // ledState used to set the LED
// defines variables
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement
int VRx = A1;
int xPosition = 0;
int mapX = 0;
int motor;
int motorPin = 9;

LiquidCrystal_I2C lcd(0x27,20,4);

char encrypt(char in_char)
{
  char out_char;
  
  out_char = in_char;
  
  return out_char;
}


char decrypt(char in_char)
{
  char out_char;
  
  out_char = in_char;
  
  return out_char;
}



// the setup routine runs once when you press reset:
void setup()
{
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  lcd.init(); //initialize the lcd
  lcd.backlight(); //open the backlight 
  // set the digital pin as output:
  pinMode(3, OUTPUT);

  pinMode(VRx, INPUT);
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

}


const long txInterval = 40;              // interval at which to tx bit (milliseconds)
int tx_state = 0;
char tx_char = 'H';
char chr;
unsigned long previousTxMillis = 0;        // will store last time LED was updated




char tx_string[9] = 
{
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0x71
};

#define TX_START_OF_TEXT  -1
int tx_string_state = TX_START_OF_TEXT;

#define STX 0x70
#define ETX 0x71

char getTxChar()
{
  char chr;
  //input varibles
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  xPosition = analogRead(VRx);
  mapX = map(xPosition, 0, 1023, -512, 512);
  if (mapX > 20 or mapX < -20){
    motor = 100;
  }
  else{
    motor = 0;
  }

  switch (tx_string_state)
  {
    case TX_START_OF_TEXT:
    tx_string_state = 0;
    //list inputs
    tx_string[0] = distance;
    tx_string[1] = motor;
    return STX;
    break;
    
    default:
    chr = tx_string[tx_string_state];
    tx_string_state++;
    if (chr == ETX)  /* End of string? */
    {
      tx_string_state = TX_START_OF_TEXT;  /* Update the tx string state to start sending the string again */
      return ETX;  /* Send End of Text character */
    }
    else
    {
      return chr;  /* Send a character in the string */
    }
    break;
  }
}

void txChar()
{
  unsigned long currentTxMillis = millis();

  if (currentTxMillis - previousTxMillis >= txInterval)
  {
    // save the last time you blinked the LED (improved)
    previousTxMillis = previousTxMillis + txInterval;  // this version catches up with itself if a delay was introduced

    switch (tx_state)
    {
      case 0:
        chr = encrypt(getTxChar());
        digitalWrite(3, HIGH);  /* Transmit Start bit */
        tx_state++;
        break;

      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        if ((chr & 0x40) != 0)   /* Transmit each bit in turn */
        {
          digitalWrite(3, HIGH);
        }
        else
        {
          digitalWrite(3, LOW);
        }
        chr = chr << 1;  /* Shift left to present the next bit */
        tx_state++;
        break;

      case 8:
        digitalWrite(3, HIGH);  /* Transmit Stop bit */
        tx_state++;
        break;

      default:
        digitalWrite(3, LOW);
        tx_state++;
        if (tx_state > 20) tx_state = 0;  /* Start resending the character */
        break;
    }
  }
}



const long rxInterval = 4;              // interval at which to read bit (milliseconds)
int rx_state = 0;
char rx_char;
unsigned long previousRxMillis = 0;        // will store last time LED was updated
int rx_bits[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//top
int input_index;
char inputs[7];
char input_values[7];


void rxChar()
{
  unsigned long currentRxMillis = millis();
  int sensorValue;
  int i;

  if (currentRxMillis - previousRxMillis >= rxInterval)
  {
    // save the last time you read the analogue input (improved)
    previousRxMillis = previousRxMillis + rxInterval;  // this version catches up with itself if a delay was introduced

    sensorValue = analogRead(A0);
    //Serial.println(rx_state);

    switch (rx_state)
    {
      case 0:
        if (sensorValue >= 800)
        {
          rx_bits[0]++;
          rx_state++;
        }
        break;

      case 100:
        if ((rx_bits[0] >= 6) && (rx_bits[8] >= 6))  /* Valid start and stop bits */
        {
          rx_char = 0;

          for (i = 1; i < 8; i++)
          {
            rx_char = rx_char << 1;
            if (rx_bits[i] >= 6) rx_char = rx_char | 0x01;
          }
          rx_char = decrypt(rx_char);
          switch (rx_char)
          {
            case STX:
              input_index = 0;
              break;
            case ETX:
              //save input data
              for (i=0; i<7; i++){
                input_values[i]= inputs[i];
                Serial.println(input_values[i]+0);
                //Serial.println('*');
              }
              //outputs
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print(input_values[0]+0);
              
              analogWrite(motorPin, input_values[1]+0);
              
              Serial.println(' ');
              break;
              
            default:
              if (input_index < 7)
              {
                inputs [input_index] = rx_char;
                input_index ++;
              }
              break;
          }
          /*
          if (rx_char >= 0x20)
          {
            Serial.println(rx_char);
          }
          else
          {
            Serial.println(' ');
          }
          */
        }
        else
        {
          Serial.println("Rx error");
        }
//        for (i = 0; i < 10; i++)  /* Print the recieved bit on the monitor - debug purposes */
//        {
//          Serial.println(rx_bits[i]);
//        }
        for (i = 0; i < 10; i++)
        {
          rx_bits[i] = 0;
        }
        rx_state = 0;
        break;

      default:
        if (sensorValue >= 800)
        {
          rx_bits[rx_state / 10]++;
        }
        rx_state++;
        break;
    }
  }

}



// the loop routine runs over and over again forever:
void loop()
{
  //Serial.println(analogRead(A0));
  txChar();
  rxChar();
}