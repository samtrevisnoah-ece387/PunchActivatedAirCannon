#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

//TouchScreen
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "TouchScreen.h"

// Used for software SPI
#define LIS3DH_CLK 7
#define LIS3DH_MISO 5 
#define LIS3DH_MOSI 6
// Used for hardware & software SPI
#define LIS3DH_CS 4

// Touchscreen
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 920
#define TS_MAXY 940

#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

boolean RecordOn = false;

#define FRAME_X 210
#define FRAME_Y 180
#define FRAME_W 100
#define FRAME_H 50

#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W/2)
#define REDBUTTON_H FRAME_H

#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W/2)
#define GREENBUTTON_H FRAME_H

//software SPI
Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);

unsigned long punchStart = 0;//variable for non-blocking punch timeframe check
const long punchInterval = 200;//timeframe of a punch in ms, from max acceleration to max deceleration, 200 is very generous
int punchAccel = 2;//the beginning of a punch in m/s^2, could be over 50m/s^2 depending on the puncher
int punchDecel = -2;//the end of a punch in m/s^2, could be less than -100m/s^2 depending on the puncher
int airTime = 250;//how long the air lasts, in ms

void drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, ILI9341_BLACK);
}

void redBtn()
{ 
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, ILI9341_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, ILI9341_BLUE);
  drawFrame();
  tft.setCursor(GREENBUTTON_X + 6 , GREENBUTTON_Y + (GREENBUTTON_H/2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("ON");
  RecordOn = false;
}

void greenBtn()
{
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, ILI9341_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, ILI9341_BLUE);
  drawFrame();
  tft.setCursor(REDBUTTON_X + 6 , REDBUTTON_Y + (REDBUTTON_H/2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("OFF");
  RecordOn = true;
}

void setup(void) {
  //Test to see if accelerometer is communicating
  Serial.begin(9600);
  Serial.println("LIS3DH test!");  
  if (! lis.begin(0x18)) {   
    Serial.println("Couldnt start");
    while (1);
  }
  Serial.println("LIS3DH found!");

  tft.begin();

  tft.fillScreen(ILI9341_BLUE);
  // origin = left,top landscape (USB left upper)
  tft.setRotation(1); 
  redBtn();
  
  lis.setRange(LIS3DH_RANGE_16_G);   //+-16G range for good punch detection
  
  pinMode(2, OUTPUT); //Solenoid valve
  digitalWrite(2, LOW);
}

void loop() {
  lis.read();
  sensors_event_t event; 
  lis.getEvent(&event);
  
 //look for punch starting, at least 20 m/s^2
  if (event.acceleration.x > punchAccel){
     Serial.println(event.acceleration.x);
     punchStart = millis();
  }
  
  unsigned long currentMillis = millis();

  //look for punch ending, less than -40 m/s^2 and safety check
  if (event.acceleration.x < punchDecel && currentMillis - punchStart < punchInterval && RecordOn){
      Serial.println(event.acceleration.x);
      Serial.println("Punch");
      Wind(airTime);
    }

// See if there's any  touch data for us   
    // Retrieve a point  
    TSPoint p = ts.getPoint(); 
    // Scale using the calibration #'s
    // and rotate coordinate system
    p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;

    if (RecordOn)
    {
      if((x > REDBUTTON_X) && (x < (REDBUTTON_X + REDBUTTON_W))) {
        if ((y > REDBUTTON_Y) && (y <= (REDBUTTON_Y + REDBUTTON_H))) {
          Serial.println("Red btn hit"); 
          redBtn();
        }
      }
    }
    else //Record is off (RecordOn == false)
    {
      if((x > GREENBUTTON_X) && (x < (GREENBUTTON_X + GREENBUTTON_W))) {
        if ((y > GREENBUTTON_Y) && (y <= (GREENBUTTON_Y + GREENBUTTON_H))) {
          Serial.println("Green btn hit"); 
          greenBtn();
        }
      }
    }

    Serial.println(RecordOn); 
    
}

void Wind(int airTime){
  digitalWrite(2, HIGH);
  delay(airTime);
  digitalWrite(2, LOW);
}
