#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
//----------------------------------------Configure OLED screen size in pixels
#define SCREEN_WIDTH 128 //--> OLED display width, in pixels
#define SCREEN_HEIGHT 64 //--> OLED display height, in pixels
#define PIN 26
//----------------------------------------Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);
//----------------------------------------

//----------------------------------------Variable Declarations
unsigned long previousMillisGetHR = 0; //--> will store the last time Millis (to get Heartbeat) was updated.
unsigned long previousMillisResultHR = 0; //--> will store the last time Millis (to get BPM) was updated.

const long intervalGetHR = 20; //--> Interval for reading heart rate (Heartbeat) = 10ms.
const long intervalResultHR = 10000; //--> The reading interval for the result of the Heart Rate calculation is in 10 seconds.

int PulseSensorSignal; //--> Variable to accommodate the signal value from the sensor
const int PulseSensorHRWire = 36; //--> PulseSensor connected to ANALOG PIN 0 (A0 / ADC 0).
//const int LED_A1 = A1; //--> LED to detect when the heart is beating. The LED is connected to PIN A1 on the Arduino UNO.
int UpperThreshold = 200; //--> Determine which Signal to "count as a beat", and which to ingore.
int LowerThreshold = 100;

int cntHB = 0; //--> Variable for counting the number of heartbeats.
boolean ThresholdStat = true; //--> Variable for triggers in calculating heartbeats.
int BPMval = 0; //--> Variable to hold the result of heartbeats calculation.

int x = 0; //--> Variable axis x graph values to display on OLED
int y = 0 + 16; //--> Variable axis y graph values to display on OLED
int lastx = 0; //--> The graph's last x axis variable value to display on the OLED
int lasty = 0 + 16; //--> The graph's last y axis variable value to display on the OLED

const unsigned char Heart_Icon [] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56,
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};


void setup() {
  //  pinMode(LED_A1,OUTPUT); //--> Set LED_3 PIN as Output.
  Serial.begin(9600); //--> Set's up Serial Communication at certain speed.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize display with the I2C address of 0x3C
  display.clearDisplay();  // Clear the buffer

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 6); //--> (x position, y position)
  display.print("     Please wait");

  display.setCursor(0, 22); //--> (x position, y position)
  display.print("     10  seconds");

  display.setCursor(0, 32); //--> (x position, y position)
  display.print("       to get");

  display.setCursor(0, 42); //--> (x position, y position)
  display.print(" the Heart Rate value");;

  display.display();
  delay(3000);
  //----------------------------------------

  //----------------------------------------Displays the initial display of BPM value
  display.clearDisplay(); //--> for Clearing the display
  display.drawBitmap(0 + 10, 47 - 46, Heart_Icon, 16, 16, WHITE); //--> display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20 + 10, 48 - 46); //--> (x position, y position)
  display.print(": 0 BPM");
  display.display();
  //----------------------------------------

  Serial.println();
  Serial.println("Please wait 10 seconds to get the BPM Value");

  strip.begin();
  strip.setBrightness(50);
  strip.show();
}

void DrawGraph() {
  //  Condition to reset the graphic display if it fills the width of the OLED screen
  if (x > 127) {
    display.fillRect(0, 0 + 16, 128, 55, BLACK);
    x = 0;
    lastx = 0;
  }

  //  Process signal data to be displayed on OLED in graphic form
  int ySignal = PulseSensorSignal;
  Serial.println(PulseSensorSignal);
  if (ySignal > 700) ySignal = 700;
  if (ySignal < 60) ySignal = 60;

  int ySignalMap = map(ySignal, 60, 700, 0, 40 + 16); //--> The y-axis used on OLEDs is from 0 to 40

  y = 40 + 16 - ySignalMap;

  //  Displays the heart rate graph
  display.writeLine(lastx, lasty, x, y, WHITE);
  display.display();

  lastx = x;
  lasty = y;

  x++;
}

void GetHeartRate() {
  //  Process of reading heart rate.
  unsigned long currentMillisGetHR = millis();

  if (currentMillisGetHR - previousMillisGetHR >= intervalGetHR) {
    previousMillisGetHR = currentMillisGetHR;

    PulseSensorSignal = analogRead(PulseSensorHRWire) / 7; //--> holds the incoming raw data. Signal value can range from 0-1024

    if (PulseSensorSignal > UpperThreshold && ThresholdStat == true) {
      cntHB++;
      ThresholdStat = false;
    }

    if (PulseSensorSignal < LowerThreshold) {
      ThresholdStat = true;
    }

    DrawGraph(); //--> Calling the DrawGraph() subroutine
  }

  //----------------------------------------The process for getting the BPM value.
  unsigned long currentMillisResultHR = millis();

  if (currentMillisResultHR - previousMillisResultHR >= intervalResultHR) {
    previousMillisResultHR = currentMillisResultHR;

    BPMval = cntHB * 6; //--> The taken heart rate is for 10 seconds. So to get the BPM value, the total heart rate in 10 seconds x 6.
    Serial.print("BPM : ");
    Serial.println(BPMval);

    display.fillRect(20 + 10, 48 - 46, 108, 20, BLACK);

    display.drawBitmap(0 + 10, 47 - 46, Heart_Icon, 16, 16, WHITE); //--> display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(20 + 10, 48 - 46); //--> (x position, y position)
    display.print(": ");
    display.print(BPMval);
    display.print(" BPM");
    display.display();

    cntHB = 0;
  }
}

void loop() {
  GetHeartRate(); //--> Calling the GetHeartRate() subroutine
}
