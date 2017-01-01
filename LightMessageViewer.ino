#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "Wifi.h"
#include "private_data.h"

#define PIXEL_PIN   D2

const int SIZE_PIXELS  = 4;
const int COUNT_PIXELS = SIZE_PIXELS * SIZE_PIXELS;

// enter here the url from the Service
const char* LIGHT_MESSAGE_URL = LIGHT_MESSAGE_SERVICE;

const int MAX_JSON_SIZE = 10000;
char g_aChar[MAX_JSON_SIZE];



Adafruit_NeoPixel pixels = Adafruit_NeoPixel(COUNT_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);



void clearLeds() {
  uint32_t offColor = pixels.Color(0,0,0);
  for (int i=0; i<COUNT_PIXELS; i++) {
    pixels.setPixelColor(i, offColor);
  }
  pixels.show();
}


bool fillCharArray(const String &str, char *aChar, int aCharLength) {
  bool ret = false;
  int strLength = str.length() + 1;
  if (strLength <= aCharLength ) {
    str.toCharArray(aChar, strLength);
    ret = true;
  }
  return ret;
}


int hexToDec(char hex, int len = 1) {
  char str[2];
  str[0] = hex;
  str[1] = 0;
  int val = strtol(str, NULL, 16) * 16;
  return val;
}


uint32_t convertToPixelColor(const char *color) {
  uint32_t pCol = 0;
  if (color[0] == '#') {
    if (strlen(color) == 4) {
      // #36a
      uint8_t r = hexToDec(color[1]);
      uint8_t g = hexToDec(color[2]);
      uint8_t b = hexToDec(color[3]);
      pCol = pixels.Color(r,g,b);
    }
    if (strlen(color) == 7) {
      // #fe87ae
      uint8_t r = hexToDec(color[1],2);
      uint8_t g = hexToDec(color[3],2);
      uint8_t b = hexToDec(color[5],2);
      pCol = pixels.Color(r,g,b);
    }
  } else {
    return 0;
  }
  return pCol;
}

bool fillLedArray(JsonObject &jObj, uint32_t *ledArray) {
  JsonArray &frames = jObj["frames"];
  Serial.println(frames.size());
  if (frames.size() > 0) {
    JsonObject &frame = frames[0];
    JsonArray &leds = frame["leds"];
    for (auto &pos : leds) {
      const char* value = pos.asString();
      uint32_t color = convertToPixelColor(value);
      *ledArray = color;
      ledArray++;    
    }
  }
}

void swapLeds(uint32_t *leds, int idx1, int idx2) {
  uint32_t tmp = leds[idx1];
  leds[idx1] = leds[idx2];
  leds[idx2] = tmp;
}

void flipLeds(uint32_t *leds) {
  for (int y=1; y<SIZE_PIXELS; y+=2) {
    for (int x=0; x<SIZE_PIXELS/2; x++) {
      int idx1 = y*SIZE_PIXELS + x;
      int idx2 = (y+1)*SIZE_PIXELS -1 - x;
      swapLeds(leds, idx1, idx2);
    }
  }
}

void showLeds(uint32_t *leds) {
  for (int i=0; i<COUNT_PIXELS; i++) {
    pixels.setPixelColor(i, *leds );
    leds++;
    pixels.show();
  }
  pixels.show();
}


void work() {
  if (WifiConnect()) {
    String sResult; 
    if (WifiGet(LIGHT_MESSAGE_URL, sResult)) {
      WifiDisconnect();
      if (fillCharArray(sResult, g_aChar, sizeof(g_aChar)) ) {
//        Serial.println(g_CharArray);
        StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
        JsonObject& obj = jsonBuffer.parseObject(g_aChar);
        if (obj.success()) {
          uint32_t leds[100];
          fillLedArray(obj, leds);   
//          Serial.println("output");
          
          flipLeds(leds);
          showLeds(leds);            
        }
      }

   
    }

    
  }
  else {
    Serial.println("can't connect wifi");
  }
  WifiDisconnect();
}


void setup() {
  Serial.begin(9600);
  delay(100);
  
  pixels.begin();
  clearLeds();
}



void loop() {

  work();

  // wait 60 seconds
  delay(10 * 1000);
  
}
