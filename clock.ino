
#include "PietteTech_DHT.h" // https://github.com/piettetech/PietteTech_DHT
#include <MD_MAX72xx.h>     // https://github.com/MajicDesigns/MD_MAX72XX
#include <SPI.h>
#include <NTPClient.h>      // https://github.com/arduino-libraries/NTPClient
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>        
#define DHTPIN 0
#define DHTTYPE DHT11
const char *ssid     = "Klapse";
const char *password = "8595042523671671";
void dht_wrapper(); // must be declared before the lib initialization

// Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

#define  DEBUG  1

#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTD(x) Serial.println(x, DEC)

#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTD(x)

#endif



#define MAX_DEVICES 8 // Define the number of devices (MAX72XX-Controler) you have in the chain and the hardware interface
// NOTE: These pin numbers may not work with your board and you need to adapt
#define CLK_PIN   14  // or SCK
#define DATA_PIN  13  // or MOSI
#define CS_PIN    2  // or SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);
// Arbitrary pins
// MD_MAX72XX mx = MD_MAX72XX(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Delaytime between updates of display
#define  DELAYTIME  100  // in milliseconds



//Pacman-Animation
#define ANIMATION_DELAY 75  // milliseconds
#define FRAMES      4   // number of animation frames

const uint8_t pacman_with_ghost[FRAMES][19] =  
{
  {0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0x7e, 0xff, 0xe7, 0xc3, 0x81, 0x00 },
  {0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0xff, 0xff, 0xe7, 0xe7, 0x42, 0x00 },
  {0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xe7, 0x66, 0x24 },
  {0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xf3, 0x7b, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c },
};
const uint8_t DATA_WIDTH = (sizeof(pacman_with_ghost[0]) / sizeof(pacman_with_ghost[0][0]));

uint32_t prevTimeAnim = 0;  // remember the millis() value in animations
int16_t idx = -DATA_WIDTH;  // display index (column)
uint8_t frame;              // current animation frame
uint8_t deltaFrame;         // the animation frame offset for the next frame
//Pacman-Animation End

char old[9]; // old Time

void pacman_clean() { // TODO


  frame = 0;
  deltaFrame = 1;


  prevTimeAnim = millis();
  for (idx; idx != mx.getColumnCount() - 13;)
  {
    if (millis() - prevTimeAnim < ANIMATION_DELAY) {
      delay(50);
      continue;
    }

    prevTimeAnim = millis();
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    idx++;
    for (uint8_t i = 0; i < DATA_WIDTH; i++)
    {
      mx.setColumn(idx - DATA_WIDTH + i, pacman_with_ghost[frame][i]);
    }

    // advance the animation frame
    frame += deltaFrame;
    if (frame == 0 || frame == FRAMES - 1)
    {
      deltaFrame = -deltaFrame;
    }

    // check if we are completed and set initialise for next time around


    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  }
  idx = -DATA_WIDTH;

}







void dht_wrapper() { // needed by NTP-libary
  DHT.isrCallback();
}

void scrollText(char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts

  mx.clear();

  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i < charWidth + 1; i++) // allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
}

void Text(char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];

  mx.clear();
  uint8_t j = 0;
  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i < charWidth + 1; i++) // allow space between characters
    {
      if (i < charWidth)
        mx.setColumn(4 * COL_SIZE - 1 - j, cBuf[i]);
      j++;
    }
  }
}

void Text(String text)
{
  char charBuf[50];
  text.toCharArray(charBuf, 50);
  Text(charBuf);
}

void Text_mid(char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];
  char* tmp = p;
  mx.clear();
  uint8_t j = 0;
    int tmp_charWidth = 0;
  while (*p != '\0')
  {
    tmp_charWidth += mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
  }
  j = (mx.getColumnCount() / 2 - tmp_charWidth) / 2; // calculate start position for placing time in the middle of the Matrix-Display
  p = tmp;
  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i < charWidth + 1; i++) // allow space between characters
    {
      if (i < charWidth)
        mx.setColumn(4 * COL_SIZE - 1 - j, cBuf[i]);
      j++;
    }
  }
}

void Text_mid(String text)
{
  char charBuf[50];
  text.toCharArray(charBuf, 50);
  Text_mid(charBuf);
}



void clock()
{

  char charBuf[50];

  char *p;
  timeClient.update();
  timeClient.getFormattedTime().toCharArray(charBuf, 50);
  charBuf[5] = ' ';
  charBuf[6] = '\0';
  p = charBuf;
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts
  uint8_t j = 0;
  int tmp_charWidth = 0;
  while (*p != '\0')
  {
    tmp_charWidth += mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
  }
  j = (mx.getColumnCount() / 2 - tmp_charWidth) / 2; // calculate start position for placing time in the middle of the Matrix-Display
  p = charBuf;
  if (old == NULL || p[4] != old[4]) {
    //mx.clear();
    /*while (*p != '\0')
    {
      charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

      for (uint8_t i = 0; i < charWidth + 1; i++) // allow space between characters
      {
        if (i < charWidth)
          mx.setColumn(4 * COL_SIZE - 1 - j, cBuf[i]);
        j++;
      }
    }*/
    Text_mid(p);
    strcpy(old, charBuf);
  }
//}
  delay(500);
}

void checkintensity() // dimm Display for the night
{
  int hour = timeClient.getHours();
  if (hour >= 20 || hour <= 6)
  {
    mx.control(MD_MAX72XX::INTENSITY, 0);
  }
  else
  {
    mx.control(MD_MAX72XX::INTENSITY, 5);
  }
}
void setup()
{
  mx.begin();

#if  DEBUG
  Serial.begin(115200);
  delay(1000);
#endif
  PRINTS("\n[MD_MAX72XX Test & Demo]");
  mx.clear();
  Text("MD_MAX72xx Test  ");
  //delay ( 5000 );
  WiFi.begin(ssid, password);
  
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( ".WIFI." );
  }

  timeClient.begin();
  //timeClient.update();
  mx.control(MD_MAX72XX::INTENSITY, 5);
  mx.clear();
  strcpy(old, "abcdefgh"); // writing dummy as old time
}

void loop()
{
  pacman_clean();
  String string;
  for (int i = 0;; i++)
  {

    clock();
    float h = DHT.readHumidity();

    float t = DHT.readTemperature();
    checkintensity();
    if (i == 30)
    {
      pacman_clean();
      string =  String(String(t, 1) + "C");
      Text(string);
      delay(10000);
      strcpy(old, "abcdefgh"); // writing dummy as old time
      //mx.clear();
    }
    if (i == 120)
    {
      pacman_clean();
      string =  String(String(t, 1) + "%");
      Text(string);
      delay(10000);
      strcpy(old, "abcdefgh"); // writing dummy as old time
      //mx.clear();
      i = 0;
    }
  }
}

