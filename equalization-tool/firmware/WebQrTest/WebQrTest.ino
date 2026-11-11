/* SPDX-License-Identifier: GPL-2.0-or-later
 * ESP32-C3 / 72x40 SSD1306 local Wi-Fi and QR hardware test.
 * Panel initialization and small font derived from this repository's PocketWorld.
 */
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include "qr_bitmaps.h"
#include "page.h"

static WebServer server(80);
static DNSServer dns;
static WiFiClient probe;
static String probeReply;
static uint32_t probeStarted=0;
static uint8_t oled[360], oledAddress=0x3c;
static bool oledOk=false, apOk=false;
static uint32_t displayUntil=0, lastDisplay=0, lastReport=0, webRequests=0;
static int screenOverride=-1;
static char serialLine[40];
static size_t serialLength=0;

static void commandBytes(const uint8_t *data,size_t length) {
  Wire.beginTransmission(oledAddress);Wire.write(0);Wire.write(data,length);Wire.endTransmission();
}
static void displayInit() {
  Wire.begin(5,6);Wire.setClock(400000);
  Wire.beginTransmission(oledAddress);oledOk=Wire.endTransmission()==0;
  if(!oledOk){oledAddress=0x3d;Wire.beginTransmission(oledAddress);oledOk=Wire.endTransmission()==0;}
  const uint8_t init[]={0xae,0xd5,0x80,0xa8,0x27,0xd3,0x00,0xad,0x30,0x8d,0x14,0x40,0xa6,0xa4,0x20,0x00,0xa1,0xc8,0xda,0x12,0x81,180,0xd9,0x22,0xdb,0x20,0x2e,0xaf};
  if(oledOk)commandBytes(init,sizeof(init));
}
static void pixel(int x,int y,bool value=true) {
  if((unsigned)x>=72||(unsigned)y>=40)return;
  if(value)oled[(y/8)*72+x]|=1<<(y%8);else oled[(y/8)*72+x]&=~(1<<(y%8));
}
static void displayFlush() {
  if(!oledOk)return;
  const uint8_t address[]={0x21,28,99,0x22,0,4};commandBytes(address,sizeof(address));
  for(int i=0;i<360;i+=24){Wire.beginTransmission(oledAddress);Wire.write(0x40);Wire.write(oled+i,24);if(Wire.endTransmission()!=0)oledOk=false;}
}
static const uint8_t font[][5]={
 {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
 {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7},
 {2,5,7,5,5},{6,5,6,5,6},{7,4,4,4,7},{6,5,5,5,6},{7,4,6,4,7},
 {7,4,6,4,4},{7,4,5,5,7},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,7},
 {5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{5,7,7,7,5},{7,5,5,5,7},
 {7,5,7,4,4},{7,5,5,7,1},{6,5,6,5,5},{7,4,7,1,7},{7,2,2,2,2},
 {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
static void text3(int x,int y,const char *s) {
  for(;*s;s++,x+=4){char c=toupper(*s);int n=c>='0'&&c<='9'?c-'0':(c>='A'&&c<='Z'?c-'A'+10:-1);
    if(n>=0)for(int r=0;r<5;r++)for(int col=0;col<3;col++)if(font[n][r]&(4>>col))pixel(x+col,y+r);
    if(c=='-')for(int j=0;j<3;j++)pixel(x+j,y+2);
    if(c=='.')pixel(x+1,y+4);
  }
}
static void qrDraw(const uint8_t *bits,uint8_t size) {
  const int x0=(40-size)/2,y0=(40-size)/2;
  for(int y=0;y<size;y++)for(int x=0;x<size;x++){unsigned i=y*size+x;pixel(x+x0,y+y0,!(bits[i/8]&(0x80>>(i%8))));}
}
static void render() {
  memset(oled,0,sizeof(oled));
  if(!apOk){text3(4,12,"WIFI ERROR");}
  else if(displayUntil&&(int32_t)(displayUntil-millis())>0){text3(20,8,"PHONE");text3(30,20,"OK");}
  else {
    const bool url=screenOverride<0?WiFi.softAPgetStationNum()>0:screenOverride==1;
    qrDraw(url?QR_URL:QR_WIFI,url?QR_URL_SIZE:QR_WIFI_SIZE);
    text3(43,3,url?"WEB":"WIFI");text3(43,13,"EQ-C3");
    text3(43,23,url?"OPEN":"SCAN");text3(43,32,url?"PAGE":"JOIN");
  }
  displayFlush();
}
static String statusJson() {
  String s="{\"device\":\"ESP32-C3\",\"ssid\":\"";s+=WIFI_SSID;
  s+="\",\"ip\":\"";s+=WiFi.softAPIP().toString();s+="\",\"uptime_ms\":";s+=millis();
  s+=",\"clients\":";s+=WiFi.softAPgetStationNum();s+=",\"oled\":";s+=oledOk?"true":"false";
  s+=",\"ap\":";s+=apOk?"true":"false";s+=",\"requests\":";s+=webRequests;
  wifi_config_t conf={};const esp_err_t confErr=esp_wifi_get_config(WIFI_IF_AP,&conf);
  int8_t power=0;esp_wifi_get_max_tx_power(&power);
  s+=",\"ap_started\":";s+=WiFi.AP.started()?"true":"false";
  s+=",\"channel\":";s+=conf.ap.channel;s+=",\"hidden\":";s+=conf.ap.ssid_hidden;
  s+=",\"tx_power_qdbm\":";s+=power;s+=",\"config_error\":";s+=confErr;s+="}";return s;
}
static void sendFrame() {
  Serial.print("{\"frame\":\"");
  static const char hex[]="0123456789abcdef";
  for(uint8_t v:oled){Serial.write(hex[v>>4]);Serial.write(hex[v&15]);}Serial.println("\"}");
}
static void serialCommand() {
  if(!strcmp(serialLine,"STATUS"))Serial.println(statusJson());
  else if(!strcmp(serialLine,"FRAME"))sendFrame();
  else if(!strcmp(serialLine,"WIFI")){screenOverride=0;displayUntil=0;render();sendFrame();}
  else if(!strcmp(serialLine,"URL")){screenOverride=1;displayUntil=0;render();sendFrame();}
  else if(!strcmp(serialLine,"AUTO")){screenOverride=-1;displayUntil=0;render();}
  else if(!strncmp(serialLine,"POWER ",6)){
    const int dbm=atoi(serialLine+6);
    if(dbm>=2&&dbm<=19)esp_wifi_set_max_tx_power(dbm*4);
    Serial.println(statusJson());
  }
  else if(!strncmp(serialLine,"CHANNEL ",8)){
    const int channel=atoi(serialLine+8);
    if(channel>=1&&channel<=11){wifi_config_t c={};if(esp_wifi_get_config(WIFI_IF_AP,&c)==ESP_OK){c.ap.channel=channel;esp_wifi_set_config(WIFI_IF_AP,&c);}}
    Serial.println(statusJson());
  }
  else if(!strcmp(serialLine,"SCAN")){
    WiFi.mode(WIFI_AP_STA);
    const int found=WiFi.scanNetworks(false,true);
    Serial.print("{\"scan_count\":");Serial.print(found);Serial.println("}");
    WiFi.scanDelete();WiFi.mode(WIFI_AP);
  }
  else if(!strcmp(serialLine,"HTTPTEST")){
    probe.stop();probeReply="";
    if(probe.connect(IPAddress(127,0,0,1),80)){
      probe.print("GET /api/status HTTP/1.1\r\nHost: 192.168.4.1\r\nConnection: close\r\n\r\n");probeStarted=millis();
    }else Serial.println("{\"http_selftest\":false,\"reason\":\"connect\"}");
  }
}
static void sendPage(){webRequests++;server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",PAGE);}
void setup() {
  Serial.begin(115200);displayInit();
  WiFi.persistent(false);WiFi.mode(WIFI_AP);
  const IPAddress ip(192,168,4,1),mask(255,255,255,0);
  apOk=WiFi.softAPConfig(ip,ip,mask)&&WiFi.softAP(WIFI_SSID,WIFI_PASSWORD,6,false,2);
  if(apOk){WiFi.setSleep(false);WiFi.softAPbandwidth(WIFI_BW_HT20);WiFi.setTxPower(WIFI_POWER_11dBm);}
  if(apOk)dns.start(53,"*",ip);
  server.on("/",HTTP_GET,sendPage);
  server.on("/api/status",HTTP_GET,[](){webRequests++;server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",statusJson());});
  server.on("/api/display",HTTP_POST,[](){webRequests++;displayUntil=millis()+5000;render();server.send(200,"application/json","{\"ok\":true}");});
  server.onNotFound([](){server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","Open http://192.168.4.1");});
  server.begin();render();Serial.println(statusJson());
}
void loop() {
  dns.processNextRequest();server.handleClient();
  if(probeStarted){
    while(probe.available()&&probeReply.length()<2048)probeReply+=(char)probe.read();
    if(!probe.connected()||millis()-probeStarted>2500){
      const bool ok=probeReply.startsWith("HTTP/1.1 200")&&probeReply.indexOf("\"device\":\"ESP32-C3\"")>=0;
      Serial.print("{\"http_selftest\":");Serial.print(ok?"true":"false");Serial.print(",\"bytes\":");Serial.print(probeReply.length());Serial.println("}");
      probe.stop();probeStarted=0;
    }
  }
  while(Serial.available()){char c=Serial.read();if(c=='\n'){serialLine[serialLength]=0;serialCommand();serialLength=0;}else if(c!='\r'&&serialLength<sizeof(serialLine)-1)serialLine[serialLength++]=c;}
  if(millis()-lastDisplay>500){lastDisplay=millis();render();}
  if(Serial&&millis()-lastReport>5000){lastReport=millis();Serial.println(statusJson());}
  delay(2);
}
