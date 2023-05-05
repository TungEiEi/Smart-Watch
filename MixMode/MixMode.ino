int NowMode = 0;

//////////////////////////////////////////////////////////////      NeoPixel       ///////////////////////////////////////////////////////////////////////////////////////
#include <Adafruit_NeoPixel.h>
#define PIN 26

Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);

uint32_t start_time = millis(); // Initialize start time

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}


//////////////////////////////////////////////////////////////      Clock       ///////////////////////////////////////////////////////////////////////////////////////
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#endif

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <U8x8lib.h>

// ========== user settings ==========

const char          *ssid          = "Tung_EiEi";             // your wifi name
const char          *pw            = "hellotung";             // your wifi password
const char          *ntpServer     = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay    = 900;            // NTP update interval (unit: seconds, default: 15 min)
const long          timezoneOffset = 7;              // timezone offset (unit: hours)

#define SCL_PIN SCL  // SCL pin of OLED. Default: D1 (ESP8266) or D22 (ESP32)
#define SDA_PIN SDA  // SDA pin of OLED. Default: D2 (ESP8266) or D21 (ESP32)

// ====================================

const String  weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
unsigned long lastCheckTime = 0;
unsigned int  second_prev = 0;
bool          colon_switch = false;

// NTPClient: https://github.com/arduino-libraries/NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

// U8X8 Display constructors: https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
// U8X8 Fonts: https://github.com/olikraus/u8g2/wiki/u8x8reference
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(SCL_PIN, SDA_PIN, U8X8_PIN_NONE);


//////////////////////////////////////////////////////////////      Temp       /////////////////////////////////////////////////////////////////////////////////////////
#include <Adafruit_GFX.h>       // Include core graphics library for the display
#include <Adafruit_SSD1306.h>   // Include Adafruit_SSD1306 library to drive the display
#include <Fonts/FreeMonoBold9pt7b.h>  // Add a custom font

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 25

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int temp;  // Create a variable to have something dynamic to show on the display

void DrawTemp() {
    display.clearDisplay();  // Clear the display so we can refresh

    // Print text:
    display.setTextColor(WHITE);  // Set color of the text
    display.setFont();
    display.setTextSize(1);
    display.setCursor(45, 5 + 15); // (x,y)
    display.println("TEMPERATURE");  // Text or value to print

    // Print temperature
    char string[10];  // Create a character array of 10 characters
    // Convert float to a string:
    dtostrf(temp, 4, 0, string);  // (variable, no. of digits we are going to use, no. of decimal digits, string name)

    display.setFont(&FreeMonoBold9pt7b);  // Set a custom font
    display.setCursor(22, 25 + 15); // (x,y)
    display.println(string);  // Text or value to print
    display.setCursor(90, 25 + 15); // (x,y)
    display.println("C");  // Text or value to print
    display.setFont();
    display.setCursor(78, 15 + 15); // (x,y)
    display.cp437(true);
    display.write(167);

    // Draw a filled circle:
    display.fillCircle(18, 27 + 15, 5, WHITE); // Draw filled circle (x,y,radius,color). X and Y are the coordinates for the center point

    // Draw rounded rectangle:
    display.drawRoundRect(16, 3 + 15, 5, 24, 2, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)

    // It draws from the location to down-right
    // Draw ruler step

    for (int i = 3; i <= 18 + 11; i = i + 2) {
      display.drawLine(21, i + 15, 22, i + 15, WHITE); // Draw line (x0,y0,x1,y1,color)
    }

    //Draw temperature
    temp = temp * 0.43; //ratio for show
    display.drawLine(18, 23 + 15, 18, 23 + 15 - temp, WHITE); // Draw line (x0,y0,x1,y1,color)

    display.display();  // Print everything we set previously
}

//////////////////////////////////////////////////////////////      NodeRed       ///////////////////////////////////////////////////////////////////////////////////////
#include <PubSubClient.h>

WiFiClient   espClient;
PubSubClient client(espClient);             //สร้างออปเจ็ค สำหรับเชื่อมต่อ mqtt
//=================================================================================================
const char* mqtt_broker = "broker.emqx.io";   //IP mqtt server
const char* mqtt_username = "smartwatch";        //mqtt username
const char* mqtt_password = "circuit2023";  //mqtt password
const int   mqtt_port = 1883;               //port mqtt server

char strTemp[10];
char strBPM[10];

void callback(char *topic, byte *payload, unsigned int length) {  //ฟังก์ชั่นsubscribe
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
    Serial.print((char) payload[i]);
  Serial.println();
  Serial.println("-----------------------");
}

void reconnect() {  //ฟังก์ชั่นเชื่อมต่อmqtt
  client.setServer(mqtt_broker, mqtt_port);   //เชื่อมต่อmqtt
  client.setCallback(callback);               //เลือกฟังก์ชั่นsubscribe
  while (!client.connected()) //รอจนกว่าจะเชื่อมต่อmqttสำเร็จ
  {
    String client_id = "esp32-New-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
      Serial.println("Public emqx mqtt broker connected");
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}


//////////////////////////////////////////////////////////////      HeartRate       ////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------Variable Declarations
unsigned long previousMillisGetHR = 0; //--> will store the last time Millis (to get Heartbeat) was updated.
unsigned long previousMillisResultHR = 0; //--> will store the last time Millis (to get BPM) was updated.

const long intervalGetHR = 20; //--> Interval for reading heart rate (Heartbeat) = 10ms.
const long intervalResultHR = 10000; //--> The reading interval for the result of the Heart Rate calculation is in 10 seconds.

int PulseSensorSignal; //--> Variable to accommodate the signal value from the sensor
const int PulseSensorHRWire = 36; //--> PulseSensor connected to ANALOG PIN 0 (A0 / ADC 0).
//const int LED_A1 = A1; //--> LED to detect when the heart is beating. The LED is connected to PIN A1 on the Arduino UNO.
int UpperThreshold = 300; //--> Determine which Signal to "count as a beat", and which to ingore.
int LowerThreshold = 280;

int cntHB = 0; //--> Variable for counting the number of heartbeats.
boolean ThresholdStat = true; //--> Variable for triggers in calculating heartbeats.
int BPMval = 0; //--> Variable to hold the result of heartbeats calculation.

int x = 0; //--> Variable axis x graph values to display on OLED
int y = 40 + 16; //--> Variable axis y graph values to display on OLED
int lastx = 0; //--> The graph's last x axis variable value to display on the OLED
int lasty = 40 + 16; //--> The graph's last y axis variable value to display on the OLED

const unsigned char Heart_Icon [] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56,
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

void DrawGraph() {
  //  Condition to reset the graphic display if it fills the width of the OLED screen
  if (x > 127) {
    display.fillRect(0, 0 + 16, 128, 55, BLACK);
    x = 0;
    lastx = 0;
  }

  //  Process signal data to be displayed on OLED in graphic form
  int ySignal = PulseSensorSignal;

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
    Serial.println(PulseSensorSignal);
    if (PulseSensorSignal > UpperThreshold && ThresholdStat == true) {
      cntHB++;
      ThresholdStat = false;
//      for (uint16_t i = 0; i < strip.numPixels(); i++) {
//        strip.setPixelColor(i, strip.Color(255, 0, 0));
//        strip.show();
//      }
    }

    if (PulseSensorSignal < LowerThreshold) {
      ThresholdStat = true;
//      for (uint16_t i = 0; i < strip.numPixels(); i++) {
//        strip.setPixelColor(i, strip.Color(0, 0, 0));
//        strip.show();
//      }
    }

    DrawGraph(); //--> Calling the DrawGraph() subroutine
  }

  //----------------------------------------The process for getting the BPM value.
  unsigned long currentMillisResultHR = millis();

  if (currentMillisResultHR - previousMillisResultHR >= intervalResultHR) {
    previousMillisResultHR = currentMillisResultHR;

    BPMval = cntHB * 6; //--> The taken heart rate is for 10 seconds. So to get the BPM value, the total heart rate in 10 seconds x 6.
//    Serial.print("BPM : ");
//    Serial.println(BPMval);

    display.fillRect(14 + 5, 48 - 47, 108, 20, BLACK);

    display.drawBitmap(0 + 5, 47 - 47, Heart_Icon, 16, 16, WHITE); //--> display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(14 + 5, 48 - 47); //--> (x position, y position)
    display.print(": ");
    display.print(BPMval);
    display.print(" BPM");
    display.display();

    cntHB = 0;

    //--------------- NodeRed ------------------
    sprintf(strBPM, "%d", BPMval);
    client.subscribe("circuit/heartbeat");
    client.publish("circuit/heartbeat", strBPM);
    
  }
}

int Start = 0;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  //-------------------------------------------- NeoPixel ------------------------------------------------------------
 
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'

  //-------------------------------------------- Clock ------------------------------------------------------------
  // initialize OLED
  u8x8.begin();
  u8x8.setBusClock(400000);
  u8x8.setFont(u8x8_font_7x14B_1x2_f);
  u8x8.drawString(0, 0, "Connecting");
  u8x8.drawString(0, 3, "  to WiFi...");

  // connect to wifi
  Serial.println("Connecting to WiFi...");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected. IP:");
  Serial.println(WiFi.localIP());

  // clear OLED
  u8x8.clear();

  // setup NTPClient
  timeClient.setUpdateInterval(updateDelay * 1000);
  timeClient.setTimeOffset(timezoneOffset * 60 * 60);
  timeClient.begin();
  while (!timeClient.update()) timeClient.forceUpdate();
  lastCheckTime = millis();

  //--------------------------------------------- Temp ------------------------------------------------------------
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize display with the I2C address of 0x3C
  //  display.clearDisplay();  // Clear the buffer
  sensors.begin();

  //-------------------------------------------- NodeRed ------------------------------------------------------------
  reconnect();
}

void loop() {
  if (digitalRead(27) == HIGH) {
    Serial.println("KGU");
    NowMode += 1;
    Start = 1;
    u8x8.clear();
    display.clearDisplay();
    if (NowMode == 3) {
      NowMode = 0;
    }
  }
//  Serial.print("Nowmode : ");
//  Serial.println(NowMode);
//  Serial.println(debouce(0));
  //-------------------------------------------------------------------------- Clock -----------------------------------------------------------------------------------
  if (NowMode == 0) {
    if (millis() - lastCheckTime >= updateDelay * 1000) {
      // reconnect wifi if needed
      if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
      // update NTP (and force update if needed)
      if (!timeClient.update()) timeClient.forceUpdate();
      lastCheckTime = millis();
    }

    // extract second from board's internal clock
    unsigned int second = timeClient.getSeconds();

    // when the second number changes, extract the rest of values
    // then re-draw the clock texts
    if (second != second_prev) {

      colon_switch = !colon_switch;

      time_t rawtime = timeClient.getEpochTime();
      struct tm * ti;
      ti = localtime(&rawtime);
      unsigned int year = ti->tm_year + 1900;
      unsigned int month = ti->tm_mon + 1;
      unsigned int day = ti->tm_mday;
      unsigned int hour = timeClient.getHours();
      unsigned int minute = timeClient.getMinutes();

      String fYear = String(year);
      String fDate = (month < 10 ? "0" : "") + String(month) + "/" + (day < 10 ? "0" : "") + String(day);
      String fTime = (hour < 10 ? "0" : "") + String(hour) + (colon_switch ? ":" : " ") + (minute < 10 ? "0" : "") + String(minute);
      String weekDay = weekDays[timeClient.getDay()];

      //      Serial.println("Board time: " + fYear + "/" + fDate + " (" + weekDay + ") " + timeClient.getFormattedTime());

      u8x8.setFont(u8x8_font_lucasarts_scumm_subtitle_o_2x2_f);
      u8x8.drawString(1, 0, strcpy(new char[fDate.length() + 1], fDate.c_str()));
      u8x8.setFont(u8x8_font_pxplusibmcga_f);
      u8x8.drawString(12, 0, strcpy(new char[fYear.length() + 1], fYear.c_str()));
      u8x8.setFont(u8x8_font_victoriamedium8_r);
      u8x8.drawString(12, 1, strcpy(new char[weekDay.length() + 1], weekDay.c_str()));
      u8x8.setFont(u8x8_font_inb33_3x6_f);
      u8x8.drawString(1, 2, strcpy(new char[fTime.length() + 1], fTime.c_str()));

    }

    second_prev = second;

    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(153, 0, 153));
        strip.show();
      }
  }

  //-------------------------------------------------------------------------- Temp -----------------------------------------------------------------------------------
  if (NowMode == 1) {
    sensors.requestTemperatures();
    temp++;  // Increase value for testing
    if (temp > 43) // If temp is greater than 150
    {
      temp = 0;  // Set temp to 0
    }
    temp = sensors.getTempCByIndex(0);

    if (temp < -1) // If temp is greater than 150
    {
      temp = 0;  // Set temp to 0
    }

    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 255, 255));
        strip.show();
      }
    
    DrawTemp();

    //--------------- NodeRed ------------------
    sprintf(strTemp, "%d", temp);
    client.subscribe("circuit/Temp");
    client.publish("circuit/Temp", strTemp);

  }


  //-------------------------------------------------------------------------- HeartRate -----------------------------------------------------------------------------------
  if (NowMode == 2) {
    if (Start == 1) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 127));
        strip.show();
      }
      x = 0;
      y = 40 + 16;
      lastx = 0;
      lasty = 40 + 16;
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

      //----------------------------------------Displays the initial display of BPM value
      display.clearDisplay(); //--> for Clearing the display
      display.drawBitmap(0 + 5, 47 - 47, Heart_Icon, 16, 16, WHITE); //--> display.drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)

      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(14 + 5, 48 - 47); //--> (x position, y position)
      display.print(": 0 BPM");
      display.display();
      Start = 0;
    }
    GetHeartRate();
  }
}
