#include <driver/dac.h>
#include <M5Stack.h>
#include "time.h"
#include "Omron2SMPB02E.h"
#define TINY_GSM_MODEM_UBLOX
#include <TinyGsmClient.h>

// Omron2SMPB02E prs(0); // in case of SDO=0 configuration
Omron2SMPB02E prs;

 // Serial2 is Modem of 3G Module
TinyGsm modem(Serial2);
TinyGsmClient ctx(modem);

// 時刻補正
const char* ntpServer =  "ntp.nict.jp";
const long  gmtOffset_sec = 9 * 3600;
const int   daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  M5.begin();

  dac_output_disable(DAC_CHANNEL_1);
  prs.begin();
  Serial.begin(9600);
  prs.set_mode(MODE_NORMAL);
  delay(300);

  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println(F("M5Stack + 3G Module"));

  M5.Lcd.print(F("modem.restart()"));
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  modem.restart();
  M5.Lcd.println(F("done"));

  M5.Lcd.print(F("getModemInfo:"));
  String modemInfo = modem.getModemInfo();
  M5.Lcd.println(modemInfo);

  M5.Lcd.print(F("waitForNetwork()"));
  while (!modem.waitForNetwork()) M5.Lcd.print(".");
  M5.Lcd.println(F("Ok"));

  M5.Lcd.print(F("gprsConnect(soracom.io)"));
  modem.gprsConnect("soracom.io", "sora", "sora");
  M5.Lcd.println(F("done"));

  M5.Lcd.print(F("isNetworkConnected()"));
  while (!modem.isNetworkConnected()) M5.Lcd.print(".");
  M5.Lcd.println(F("Ok"));

  M5.Lcd.print(F("My IP addr: "));
  IPAddress ipaddr = modem.localIP();
  M5.Lcd.print(ipaddr);

  //init and get the time
//  M5.Lcd.print(F("init and get the time"));
//  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//  delay(1000);

//  struct tm timeinfo;
//  if (!getLocalTime(&timeinfo)) {
//    M5.Lcd.print(F("Failed to obtain time"));
//    return;
//  }

  delay(2000);
}

void loop()
{
  delay(2000);

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  float temperature = prs.read_temp();
  float pressure = prs.read_pressure();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
//  M5.Lcd.printf("Time:\r\n%04d/%02d/%02d %02d:%02d:%02d\r\n\r\n",
//          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
//          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  M5.Lcd.printf("Temperature:\r\n%f[degC]\r\n\r\n", temperature);
  M5.Lcd.printf("Pressure:\r\n%f[Pa]\r\n", pressure);

  // 送信データ生成
  sprintf(data, "{\"absolute_pressure\":%f,\"temperature\":%f}", pressure, temperature);

  /* HTTP PUT example */
  if (!ctx.connect("unified.soracom.io", 80)) {
    Serial.println(F("Connect failed."));
    return;
  }
  Serial.println(F("connected."));

  /* send request */
  ctx.println("GET /api/timezone/Asia/Tokyo.txt HTTP/1.0");
  ctx.println("Host: worldtimeapi.org");
  ctx.println();
  Serial.println("sent.");

  /* receive response */
  while (ctx.connected()) {
    String line = ctx.readStringUntil('\n');
    Serial.println(line);
    if (line == "\r") {
      Serial.println("headers received.");
      break;
    }
  }
  char buf[1 * 1024] = {0};
  ctx.readBytes(buf, sizeof(buf)); /* body */
  ctx.stop();
  M5.Lcd.println(buf);
}
