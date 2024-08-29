#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <TimeLib.h>
#include "WiFi.h"
#include "Protocol.h"
#include "blink.h"
#include "lookleft.h"
#include "lookright.h"

const char *ssid = "Tiem Giay 2hand";
const char *password = "khongbietmk";

// #define TFT_RST       D0 // Or set to -1 and connect to Arduino RESET pin
// #define TFT_DC        D1
// #define TFT_CS        D2
#define TFT_CS         10      //---------------
#define TFT_RST        9      //---------------                                      
#define TFT_DC         8      //---------------
#define TFT_BACKLIGHT  3
#define SPI_SPEED     40000000
#define ROTATION      1
#define SERIAL_SPEED  9600

#define TCP_PORT      80
#define TCP_TIMEOUT   2000

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 80

//timer available
unsigned long previousMillis = 0;
unsigned long startTime = 0;
int currentHour;
int currentMinute;
int currentSecond;

int currentWDay;
int currentDay;
int currentMonth;
int currentYear;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WiFiServer server(TCP_PORT);
WiFiClient imageStream;

Animation animation;
GFXcanvas16 canvas = GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Vector Cube TFT Startup.\n");

  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);
  // tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  tft.initR(INITR_BLACKTAB); 
  tft.setSPISpeed(SPI_SPEED);
  // tft.setRotation(ROTATION);
  // tft.fillScreen(ST77XX_BLACK);
  tft.fillScreen(ST77XX_GREEN);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);

  Serial.println("TFT initialized.\n");
  tft.println("Cube startup.");
  tft.println("TFT initialized.");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);
  connectWiFi();

  Serial.println("WiFi initialized.\n");
  tft.println("WiFi initialized.");

  server.begin();

  Serial.println("Initialization complete.\n");
  tft.println("Init complete.");
  char buf [10];
  sprintf (buf, "%d", sizeof(Animation));
  tft.println(buf);
  delay(5000);
  tft.fillScreen(ST77XX_BLACK);

  memcpy(&animation, animation_blink, sizeof(Animation));
  playAnimation();
  //test print text
  PtrintTest();
  //test real time oclock
  configTime(7 * 3600, 0, "pool.ntp.org");
  // tft.fillRect(10, 110, 118, 50, ST7735_BLACK);
}

void connectWiFi(void)
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  tft.println("Connecting to: ");
  tft.println(ssid);
  while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      tft.print(".");
  }
  Serial.println("");
  tft.println("");
  Serial.println("WiFi connected!");
  tft.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  tft.println("IP address: ");
  tft.println(WiFi.localIP());
}

void PtrintTest(){
  tft.setCursor(10, 100);
  tft.setTextColor(ST77XX_GREEN);
  tft.println("HELLO WORD");
}
void loop()
{
  // Serial.println("1");
  switch(random(10))
  {
    case 1:
      memcpy(&animation, animation_blink, sizeof(Animation));
      playAnimation();
      break;
    case 2:
      memcpy(&animation, animation_lookleft, sizeof(Animation));
      playAnimation();
      break;
    case 3:
      memcpy(&animation, animation_lookright, sizeof(Animation));
      playAnimation();
      break;
  }

  imageStream = server.available();
  if(imageStream)
  {
    TransmissionType transmissionType = getTransmissionType();
    switch(transmissionType){
      case _Animation:
        if(bytewiseReceive((uint8_t *)&animation, sizeof(animation)))
        {
          playAnimation();
          Serial.println("Animation received successfully.\n");
        }
        else
        {
          Serial.println("Animation receive failed.\n");
        }
        break;
      case _Image:
        uint8_t * canvasPointer;
        canvasPointer = (uint8_t *)canvas.getBuffer();
        if(bytewiseReceive(canvasPointer, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t)))
        {
          tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
          Serial.println("Image received successfully.\n");
          delay(5000);
        }
        else
        {
          Serial.println("Image receive failed.\n");
        }
        break;
      default:
        break;
    }
    imageStream.flush();
  }
  oclock();
}

bool bytewiseReceive(uint8_t *inputBuffer, size_t size)
{
  unsigned long startTime;

  for(int index = 0; index < size; index++)
  {
    startTime = millis();
    while(!imageStream.available())
    {
      if(millis() - startTime > TCP_TIMEOUT)
      {
        imageStream.flush();
        return false;
      }
    }
    imageStream.read(inputBuffer + index, 1);
  }
  return true;
}

TransmissionType getTransmissionType()
{  
  return (TransmissionType)getUint16tFromStream();
}

void playAnimation()
{
    for(int frameCounter = 0; frameCounter < animation.frameCount; frameCounter++){
      AnimationFrame currentFrame = animation.frames[frameCounter];
      canvas.fillScreen(currentFrame.fillColor);
      for(int primitiveCounter = 0; primitiveCounter < currentFrame.primitiveCount; primitiveCounter++){
        Primitive primitive = currentFrame.primitives[primitiveCounter];
        switch(primitive.type){
          case _Circle:
            Circle circle;
            circle = primitive.circle;
            canvas.fillCircle(circle.x0, circle.y0, circle.r, circle.color);
            break;
          case _QuarterCircle:
            QuarterCircle quarterCircle;
            quarterCircle = primitive.quarterCircle;
            canvas.fillCircleHelper(quarterCircle.x0, quarterCircle.y0, quarterCircle.r, quarterCircle.quadrants, quarterCircle.delta, quarterCircle.color);
            break;
          case _Triangle:
            Triangle triangle;
            triangle = primitive.triangle;
            canvas.fillTriangle(triangle.x0, triangle.y0, triangle.x1, triangle.y1, triangle.x2, triangle.y2, triangle.color);
            break;
          case _RoundRect:
            RoundRect roundRect;
            roundRect = primitive.roundRect;
            canvas.fillRoundRect(roundRect.x0, roundRect.y0, roundRect.w, roundRect.h, roundRect.radius, roundRect.color);
            break;
          case _Line:
            Line line;
            line = primitive.line;
            canvas.drawLine(line.x0, line.y0, line.x1, line.y1, line.color);
            break;
        }
      }
      tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
      delay(currentFrame.duration);
    }
}

uint16_t getUint16tFromStream()
{
  while(!imageStream.available());
  uint16_t lowerByte = (uint16_t)imageStream.read();
  while(!imageStream.available());
  uint16_t upperByte = (uint16_t)imageStream.read() << 8;
  return upperByte + lowerByte;
}

void oclock(){
  time_t now = time(nullptr); // Lấy thời gian hiện tại

  struct tm* timeinfo;
  timeinfo = localtime(&now); // Chuyển đổi thời gian thành struct tm
  if(currentHour != timeinfo->tm_hour){
    Serial.println("hour different");
    currentHour = timeinfo->tm_hour; 
    currentSecond = timeinfo->tm_sec;
    currentMinute = timeinfo->tm_min; 
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
   if(currentMinute != timeinfo->tm_min){
    Serial.println("minute different");
    currentSecond = timeinfo->tm_sec;
    currentMinute = timeinfo->tm_min; 
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
   if(currentSecond != timeinfo->tm_sec){
    Serial.println("second different");
    currentSecond = timeinfo->tm_sec;
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
  // currentHour = timeinfo->tm_hour; // Lấy giờ từ struct tm
  // currentMinute = timeinfo->tm_min; // Lấy phút từ struct tm
  // currentSecond = timeinfo->tm_sec; // Lấy giây từ struct tm
  
  // calendar
//  Serial.setCtlnrsor(0,0);
  tft.setCursor(10, 110);
if(currentYear != timeinfo->tm_year + 1900){
    Serial.println("hour different");
    currentWDay = timeinfo->tm_wday + 1;
    currentDay = timeinfo->tm_mday;
    currentMonth = timeinfo->tm_mon + 1; 
    currentYear = timeinfo->tm_year + 1900; 
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
   if(currentMonth != timeinfo->tm_mon + 1){
    Serial.println("minute different");
    currentWDay = timeinfo->tm_wday + 1;
    currentDay = timeinfo->tm_mday;
    currentMonth = timeinfo->tm_mon + 1; 
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
   if(currentDay != timeinfo->tm_mday){
    Serial.println("second different");
    currentWDay = timeinfo->tm_wday + 1;
    currentDay = timeinfo->tm_mday;
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
  if(currentWDay != timeinfo->tm_wday + 1 ){
    currentWDay = timeinfo->tm_wday + 1;
  tft.fillRect(10, 120, 118, 10, ST7735_BLACK);
  }
if(timeinfo->tm_year + 1900 >= 2023){
 tft.print(dayShortStr(timeinfo->tm_wday + 1)); // Hiển thị thứ
 tft.print(", ");
 tft.print(timeinfo->tm_mday);
 tft.print("/");
 tft.print(monthShortStr(timeinfo->tm_mon + 1)); // Tháng bắt đầu từ 0, cần cộng thêm 1
 tft.print("/");
 tft.println(timeinfo->tm_year + 1900); // Năm tính từ 1900
}
  // clock
//  tft.setCtlnrsor(0,1);
 tft.setCursor(10, 120);
  if(currentHour < 10) {
   tft.print("0");
  }
 tft.print(currentHour);
 tft.print(":");
  if(currentMinute < 10) {
   tft.print("0");
  }
 tft.print(currentMinute);
 tft.print(":");
  if(currentSecond < 10) {
   tft.print("0");
  }
 tft.println(currentSecond);
//  delay(700);
}

