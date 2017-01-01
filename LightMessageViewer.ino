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

const int MAX_FRAMES = 100;
int   g_nCountFrames = 0;
uint32_t* g_LedFrames[MAX_FRAMES];
uint32_t g_Leds[MAX_FRAMES * COUNT_PIXELS];
bool g_bShowFrames = false;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(COUNT_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);


void initFrameArray() {
  g_nCountFrames = 0;
  uint32_t *ptr = g_Leds;
  for (int i=0; i<MAX_FRAMES; i++) {
     g_LedFrames[i] = ptr;
     ptr += COUNT_PIXELS;
  }
}


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

bool fillLedArray(JsonObject &jObj) {
  JsonArray &frames = jObj["frames"];
  Serial.println(frames.size());
  g_nCountFrames = 0;
  for (int i=0; i<frames.size(); i++) {
    JsonObject &frame = frames[i];
    JsonArray &leds = frame["leds"];
    if (leds.size() <= COUNT_PIXELS) {
      uint32_t *pLeds = g_LedFrames[i];
      for (auto &pos : leds) {
        const char* value = pos.asString();
        uint32_t color = convertToPixelColor(value);
        *pLeds = color;
        pLeds++;  
      }  
      flipLeds(g_LedFrames[i]);
    }
  }
  g_nCountFrames = frames.size();
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


void showFrames() {
  int rounds = 10;
  while (rounds-- > 0) {
    for (int i=0; i<g_nCountFrames; i++) {
      uint32_t *leds = g_LedFrames[i];
      showLeds(leds);  
      delay(150);    
    }
  }
}

void work() {
  g_bShowFrames = false;
  if (WifiConnect()) {
    String sResult; 
    if (WifiGet(LIGHT_MESSAGE_URL, sResult)) {
      WifiDisconnect();
      if (fillCharArray(sResult, g_aChar, sizeof(g_aChar)) ) {
        StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
        JsonObject& obj = jsonBuffer.parseObject(g_aChar);
        if (obj.success()) {
          fillLedArray(obj);   
        }
      }
    }
  }
  else {
    Serial.println("can't connect wifi");
  }
  WifiDisconnect();

  g_bShowFrames = true;
  showFrames();
}


void setup() {
  Serial.begin(9600);
  delay(100);
  
  pixels.begin();
  clearLeds();
  initFrameArray();
}



void loop() {

  work();

  // wait 60 seconds
//  delay(10 * 1000);
  
}
