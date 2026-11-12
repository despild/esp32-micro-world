/* SPDX-License-Identifier: GPL-2.0-or-later
 * ESP32-C3, HX710B + MPS20N0040D, 72x40 SSD1306, two active-low buttons.
 * ADC task never waits for a browser; invalid samples never become zero pressure.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include "Config.h"
#include "Hx710b.h"
#include "Display.h"
#include "page.h"
#include "CoreTests.h"
#include "DemoPattern.h"

static eq::Measurement measurement;
static eq::Button buttonA,buttonB;
static WebServer server(80);
static DNSServer dns;
static Preferences prefs;
static bool apOk=false,prefsOk=false,hold=false;
static uint8_t heldFrame[360];
static float graphTop=5;
static char bootId[9],serialLine[48];
static unsigned serialLength=0,selection=0,refIndex=1;
static bool serialPending=false;
static uint32_t displayUntil=0,lastDisplay=0,webRequests=0;
static const float refs[]={1,2,5,10};
static const float scales[]={2,5,10,20,40};
enum class Screen { Graph,Menu,Phone,WifiQr,WebQr,Zero,Reference,RefConfirm,Scale,Info,Busy,Result };
static Screen screen=Screen::Graph;
struct ProbeResult { uint32_t bytes,expected; int path; bool ok; };
static QueueHandle_t probeQueue;
static bool probeBusy=false;
static const char *probePaths[]={"/","/api/status","/api/samples"};

static void httpProbe(void *argument){
  const int path=int(intptr_t(argument));
  ProbeResult result{0,0,path,false};
  WiFiClient client;client.setTimeout(3000);
  if(client.connect(IPAddress(127,0,0,1),80)){
    client.print(String("GET ")+probePaths[path]+" HTTP/1.1\r\nHost: 192.168.4.1\r\nConnection: close\r\n\r\n");
    String header;const uint32_t started=millis();
    while(millis()-started<4000&&header.length()<2048&&!header.endsWith("\r\n\r\n")){
      if(client.available())header+=(char)client.read();else vTaskDelay(1);
    }
    int offset=header.indexOf("Content-Length: ");
    if(offset>=0)result.expected=header.substring(offset+16).toInt();
    bool match=true;String json;
    if(path)json.reserve(min(result.expected,uint32_t(6000)));
    while(result.bytes<result.expected&&millis()-started<6000){
      if(!client.available()){vTaskDelay(1);continue;}
      const char c=client.read();
      if(!path){if(result.bytes>=sizeof(PAGE)||uint8_t(c)!=PAGE[result.bytes])match=false;}
      else if(json.length()<6000)json+=c;
      ++result.bytes;
    }
    if(!path)match=match&&result.bytes==sizeof(PAGE);
    else match=json.indexOf(path==1?"\"firmware\":\"equalization-1.1\"":"\"samples\":[")>=0;
    result.ok=header.startsWith("HTTP/1.1 200")&&result.expected>0&&result.bytes==result.expected&&match;
  }
  client.stop();xQueueSend(probeQueue,&result,0);vTaskDelete(nullptr);
}

static void wifiOn(){
  if(apOk)return;
  WiFi.persistent(false);WiFi.mode(WIFI_AP);
  const IPAddress ip(192,168,4,1),mask(255,255,255,0);
  apOk=WiFi.softAPConfig(ip,ip,mask)&&WiFi.softAP(WIFI_SSID,WIFI_PASSWORD,6,false,2);
  if(apOk){WiFi.setSleep(false);WiFi.softAPbandwidth(WIFI_BW_HT20);WiFi.setTxPower(WIFI_POWER_11dBm);dns.start(53,"*",ip);server.begin();}
}
static void wifiOff(){dns.stop();server.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_OFF);apOk=false;}
static void number(String &s,float v,unsigned decimals=4){if(isfinite(v))s+=String(v,decimals);else s+="null";}
static String statusJson(){
  String s;s.reserve(650);
  s="{\"device\":\"ESP32-C3\",\"firmware\":\"equalization-1.1\",\"boot\":\"";s+=bootId;
  s+="\",\"revision\":";s+=measurement.revision;s+=",\"ms\":";s+=millis();
  s+=",\"ap\":";s+=apOk?"true":"false";s+=",\"ip\":\"192.168.4.1\",\"clients\":";s+=apOk?WiFi.softAPgetStationNum():0;
  s+=",\"oled\":";s+=oledOk?"true":"false";s+=",\"adc_task\":";s+=adcTaskOk?"true":"false";
  s+=",\"online\":";s+=measurement.online?"true":"false";
  s+=",\"demo\":";s+=measurement.online?"false":"true";
  s+=",\"zero_valid\":";s+=measurement.zeroValid?"true":"false";
  s+=",\"gain_valid\":";s+=measurement.gainValid?"true":"false";
  s+=",\"calibrated\":";s+=measurement.calibrated()?"true":"false";
  s+=",\"zero\":";number(s,measurement.zero,1);
  s+=",\"raw\":";if(measurement.online)s+=measurement.currentRaw;else s+="null";
  s+=",\"pressure\":";number(s,measurement.currentPressure());
  s+=",\"flags\":";s+=measurement.currentFlags;
  s+=",\"job\":\"";s+=measurement.job==eq::Job::Zero?"zero":measurement.job==eq::Job::Reference?"reference":"idle";
  s+="\",\"collected\":";s+=measurement.collected;
  s+=",\"result\":\"";s+=measurement.result;s+="\",\"dropped\":";s+=adcDropped;
  s+=",\"requests\":";s+=webRequests;s+=",\"screen\":";s+=int(screen);s+="}";return s;
}
static void noCache(){server.sendHeader("Cache-Control","no-store");}
static bool parseCursor(const String &s,uint32_t &value){
  if(!s.length()||s.length()>10)return false;
  uint64_t n=0;for(unsigned i=0;i<s.length();i++){if(s[i]<'0'||s[i]>'9')return false;n=n*10+s[i]-'0';}
  if(n>UINT32_MAX)return false;value=uint32_t(n);return true;
}
static void sendSamples(){
  ++webRequests;noCache();uint32_t after=0,revision=0;
  if(server.hasArg("after")&&!parseCursor(server.arg("after"),after)){server.send(400,"application/json","{\"error\":\"invalid cursor\"}");return;}
  if(server.hasArg("revision")&&!parseCursor(server.arg("revision"),revision)){server.send(400,"application/json","{\"error\":\"invalid revision\"}");return;}
  const bool reset=server.arg("boot")!=bootId||revision!=measurement.revision||after>=measurement.next;
  if(reset)after=measurement.oldest()-1;
  const uint32_t first=measurement.oldest(),wanted=after+1,start=wanted<first?first:wanted;
  const uint32_t end=min(measurement.next,start+64),lost=start-wanted;
  String s;s.reserve(4800);s="{\"status\":";s+=statusJson();s+=",\"lost\":";s+=lost;
  s+=",\"next\":";s+=end-1;s+=",\"more\":";s+=end<measurement.next?"true":"false";s+=",\"samples\":[";
  for(uint32_t seq=start;seq<end;seq++){
    const auto &p=measurement.at(seq);if(seq!=start)s+=',';
    s+='[';s+=p.seq;s+=',';s+=p.ms;s+=',';
    if(p.flags&eq::Missing)s+="null";else s+=p.raw;
    s+=',';number(s,p.pressure);s+=',';s+=p.flags;s+=']';
  }
  s+="]}";server.send(200,"application/json",s);
}
static bool startJob(eq::Job job,float p){
  if(!measurement.start(job,p,millis()))return false;
  hold=false;screen=Screen::Busy;return true;
}
static void control(){
  ++webRequests;noCache();
  // Reject cross-site form posts while allowing same-origin fetch and USB diagnostics.
  const String origin=server.header("Origin");
  if(origin.length()&&origin!="http://192.168.4.1"){server.send(403,"application/json","{\"error\":\"origin not allowed\"}");return;}
  const String action=server.arg("action");bool ok=false;
  if(action=="display"){displayUntil=millis()+3000;ok=true;}
  else if(action=="zero")ok=startJob(eq::Job::Zero,0);
  else if(action=="calibrate"){
    const String arg=server.arg("kpa");char *end=nullptr;float p=strtof(arg.c_str(),&end);
    if(arg.length()&&end==arg.c_str()+arg.length())ok=startJob(eq::Job::Reference,p);
  }else {server.send(400,"application/json","{\"error\":\"unknown action\"}");return;}
  server.send(ok?200:409,"application/json",ok?"{\"ok\":true}":"{\"error\":\"Sensor must be ready; check zero, reference pressure and active job\"}");
}
static void back(){
  if(screen==Screen::Busy){measurement.cancel();screen=Screen::Result;return;}
  if(screen==Screen::WifiQr||screen==Screen::WebQr){screen=Screen::Phone;selection=0;}
  else if(screen==Screen::Phone||screen==Screen::Zero||screen==Screen::Reference||screen==Screen::Scale||screen==Screen::Info){screen=Screen::Menu;selection=0;}
  else if(screen==Screen::RefConfirm)screen=Screen::Reference;
  else screen=Screen::Graph;
}
static void buttonEvent(bool a,int event){
  if(!event)return;
  displayUntil=0;
  if(event==2){if(a){measurement.cancel();screen=Screen::Graph;hold=false;}else if(screen==Screen::Graph){screen=Screen::Zero;hold=false;}else back();return;}
  if(screen==Screen::Graph){if(a){screen=Screen::Menu;selection=0;hold=false;}else{hold=!hold;if(hold)memcpy(heldFrame,oled,sizeof(oled));}return;}
  if(screen==Screen::Menu){
    if(a){selection=(selection+1)%6;return;}
    switch(selection){case 0:wifiOn();screen=Screen::Phone;break;case 1:screen=Screen::Zero;break;case 2:screen=Screen::Reference;break;case 3:screen=Screen::Scale;break;case 4:screen=Screen::Info;break;default:screen=Screen::Graph;}
    selection=0;return;
  }
  if(screen==Screen::Phone){
    if(a){selection=(selection+1)%4;return;}
    if(selection==0){wifiOn();screen=Screen::WifiQr;}else if(selection==1){wifiOn();screen=Screen::WebQr;}else if(selection==2){wifiOff();screen=Screen::Graph;}else {screen=Screen::Menu;selection=0;}return;
  }
  if(screen==Screen::WifiQr||screen==Screen::WebQr){if(a)screen=screen==Screen::WifiQr?Screen::WebQr:Screen::WifiQr;else back();return;}
  if(screen==Screen::Zero){if(a)back();else if(!startJob(eq::Job::Zero,0)){measurement.result="sensor_error";screen=Screen::Result;}return;}
  if(screen==Screen::Reference){if(a)refIndex=(refIndex+1)%4;else screen=Screen::RefConfirm;return;}
  if(screen==Screen::RefConfirm){if(a)back();else if(!startJob(eq::Job::Reference,refs[refIndex])){measurement.result=measurement.zeroValid?"sensor_error":"zero_first";screen=Screen::Result;}return;}
  if(screen==Screen::Scale){if(a)selection=(selection+1)%5;else{graphTop=scales[selection];screen=Screen::Graph;}return;}
  if(screen==Screen::Busy){if(!a){measurement.cancel();screen=Screen::Result;}return;}
  screen=Screen::Graph;
}
static void line(int x0,int y0,int x1,int y1){
  int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
  for(;;){pixel(x0,y0);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
}
static void graph(){
  if(hold){memcpy(oled,heldFrame,sizeof(oled));for(int y=0;y<7;y++)for(int x=0;x<72;x++)pixel(x,y,false);text3(0,0,measurement.online?"HOLD":"DEMO HOLD");return;}
  if(!measurement.online){
    // Separate visual fallback: the ADC history, pressure and calibration stay missing.
    text3(0,0,"DEMO NO SENSOR");if(apOk)text3(68,0,"W");
    const uint32_t now=millis();
    auto yOf=[](float percent){return 31-int(percent*.21f);};
    line(0,9,0,32);line(0,32,71,32);
    int previous=yOf(eq::demoPercentAt(now,cfg::graphMs));
    for(int x=1;x<=70;++x){
      const uint32_t age=(70-x)*cfg::graphMs/70;
      const int y=yOf(eq::demoPercentAt(now,age));line(x-1,previous,x,y);previous=y;
    }
    text3(0,35,"A MENU");text3(40,35,"B HOLD");return;
  }
  char label[24];
  if(measurement.currentFlags&eq::Saturated)text3(0,0,"ADC LIMIT");
  else if(measurement.calibrated()){const float p=measurement.currentPressure();if(isfinite(p))snprintf(label,sizeof(label),"%.2f KPA",p);else snprintf(label,sizeof(label),"WAIT");text3(0,0,label);}
  else{text3(0,0,measurement.zeroValid?"UNCAL RAW":"ZERO FIRST");}
  if(apOk)text3(68,0,"W");
  float low=measurement.calibrated()?-graphTop*.1f:INFINITY,high=measurement.calibrated()?graphTop:-INFINITY;
  const uint32_t now=millis();
  auto valid=[](const eq::Sample &p){return !(p.flags&(eq::Missing|eq::Saturated|eq::Collecting|eq::Gap));};
  auto value=[](const eq::Sample &p){return measurement.calibrated()?p.pressure:float(p.raw)-(measurement.zeroValid?measurement.zero:0);};
  if(!measurement.calibrated()){
    for(uint32_t n=measurement.oldest();n<measurement.next;n++){const auto &p=measurement.at(n);if(uint32_t(now-p.ms)<=cfg::graphMs&&valid(p)){low=min(low,value(p));high=max(high,value(p));}}
    if(!isfinite(low)){low=-100;high=100;}else{const float pad=max(100.f,(high-low)*.1f);low-=pad;high+=pad;}
  }
  // Per-column min/max preserves narrow peaks on the tiny 72-pixel screen.
  float mins[70],maxs[70];for(int x=0;x<70;x++){mins[x]=INFINITY;maxs[x]=-INFINITY;}
  for(uint32_t n=measurement.oldest();n<measurement.next;n++){
    const auto &p=measurement.at(n);const uint32_t age=now-p.ms;
    if(age>cfg::graphMs||!valid(p)||!isfinite(value(p)))continue;
    int x=69-int(age*69/cfg::graphMs);mins[x]=min(mins[x],value(p));maxs[x]=max(maxs[x],value(p));
  }
  auto yOf=[&](float v){return constrain(31-int((v-low)/(high-low)*21),10,31);};
  line(0,9,0,32);line(0,32,71,32);if(low<=0&&high>=0)for(int x=1;x<72;x+=3)pixel(x,yOf(0));
  for(int x=0;x<70;x++)if(isfinite(mins[x]))line(x+1,yOf(mins[x]),x+1,yOf(maxs[x]));
  text3(0,35,"A MENU");text3(40,35,"B HOLD");
}
static void menuDraw(const char *title,const char *const *items,unsigned count){
  text3(0,0,title);const unsigned start=selection/3*3;
  for(unsigned i=start;i<min(start+3,count);i++){if(i==selection)text3(0,11+(i-start)*8,"-");text3(5,11+(i-start)*8,items[i]);}
  text3(0,35,"A NEXT B OK");
}
static void render(){
  memset(oled,0,sizeof(oled));char label[24];
  if(displayUntil&&int32_t(displayUntil-millis())>0){text3(20,8,"PHONE");text3(30,20,"OK");displayFlush();return;}
  switch(screen){
    case Screen::Graph:graph();break;
    case Screen::Menu:{const char *items[]={"PHONE","ZERO","CALIBRATE","SCALE","INFO","GRAPH"};menuDraw("MENU",items,6);break;}
    case Screen::Phone:{const char *items[]={"WIFI QR","WEB QR","WIFI OFF","BACK"};menuDraw(apOk?"PHONE":"WIFI ERROR",items,4);break;}
    case Screen::WifiQr:case Screen::WebQr:{
      if(!apOk){text3(0,12,"WIFI ERROR");break;}
      bool url=screen==Screen::WebQr;qrDraw(url?QR_URL:QR_WIFI,url?QR_URL_SIZE:QR_WIFI_SIZE);
      text3(43,3,url?"WEB":"WIFI");text3(43,13,"EQ-C3");text3(43,23,"A QR");text3(43,32,"B OUT");break;
    }
    case Screen::Zero:text3(0,0,"ZERO");text3(0,12,"OPEN HOSE");text3(0,22,"NO PRESSURE");text3(0,35,"A BACK B START");break;
    case Screen::Reference:text3(0,0,"REFERENCE");snprintf(label,sizeof(label),"%.1f KPA",refs[refIndex]);text3(0,14,label);text3(0,24,"GAUGE REQUIRED");text3(0,35,"A NEXT B OK");break;
    case Screen::RefConfirm:text3(0,0,"APPLY PRESSURE");snprintf(label,sizeof(label),"%.1f KPA",refs[refIndex]);text3(0,12,label);text3(0,22,"HOLD STEADY");text3(0,35,"A BACK B START");break;
    case Screen::Scale:{const char *items[]={"2 KPA","5 KPA","10 KPA","20 KPA","40 KPA"};menuDraw("OLED SCALE",items,5);break;}
    case Screen::Info:text3(0,0,measurement.online?"HX710B ONLINE":"HX710B OFFLINE");text3(0,10,measurement.gainValid?"GAIN SAVED":"GAIN UNCAL");text3(0,20,measurement.zeroValid?"ZERO OK":"ZERO REQUIRED");text3(0,35,"B BACK");break;
    case Screen::Busy:text3(0,0,"MEASURING");snprintf(label,sizeof(label),"%u OF 80",measurement.collected);text3(0,14,label);text3(0,24,"HOLD STEADY");text3(0,35,"B CANCEL");break;
    case Screen::Result:text3(0,0,"RESULT");{
      const char *r=measurement.result;
      if(!strcmp(r,"zero_ok"))r="ZERO OK";else if(!strcmp(r,"calibration_ok"))r="CAL SAVED";else if(!strcmp(r,"zero_first"))r="ZERO FIRST";
      else if(!strcmp(r,"sensor_error"))r="CHECK SENSOR";else if(!strcmp(r,"reference_too_small"))r="REF TOO SMALL";
      else if(!strcmp(r,"storage_error"))r="SAVE FAILED";
      text3(0,14,r);text3(0,35,"B GRAPH");break;
    }
  }
  displayFlush();
}
static void sendFrame(){
  render();Serial.print("{\"frame\":\"");const char hex[]="0123456789abcdef";
  for(uint8_t v:oled){Serial.write(hex[v>>4]);Serial.write(hex[v&15]);}Serial.println("\"}");
}
static void serialCommand(){
  serialPending=true;
  if(!strcmp(serialLine,"STATUS"))Serial.println(statusJson());
  else if(!strcmp(serialLine,"FRAME"))sendFrame();
  else if(!strcmp(serialLine,"A")){buttonEvent(true,1);render();Serial.println(statusJson());}
  else if(!strcmp(serialLine,"B")){buttonEvent(false,1);render();Serial.println(statusJson());}
  else if(!strcmp(serialLine,"ALONG")){buttonEvent(true,2);render();Serial.println(statusJson());}
  else if(!strcmp(serialLine,"BLONG")){buttonEvent(false,2);render();Serial.println(statusJson());}
  else if(!strcmp(serialLine,"SELFTEST")){Serial.print("{\"core_tests\":");Serial.print(runCoreTests());Serial.println("}");}
  else if(!strncmp(serialLine,"HTTPTEST ",9)){
    if(probeBusy)return;
    int path=-1;for(int i=0;i<3;i++)if(!strcmp(serialLine+9,probePaths[i]))path=i;
    if(path<0)return;
    if(apOk&&probeQueue&&xTaskCreate(httpProbe,"http-check",4096,(void*)intptr_t(path),1,nullptr)==pdPASS)probeBusy=true;
    else Serial.println("{\"http_selftest\":false}");
  }
}
void setup(){
  // Native USB's default 256-byte TX buffer is smaller than STATUS and FRAME.
  // Keep complete diagnostic replies queued even while the Windows host stalls.
  Serial.setTxBufferSize(2048);Serial.setTxTimeoutMs(250);
  Serial.begin(115200);snprintf(bootId,sizeof(bootId),"%08lx",(unsigned long)esp_random());
  displayInit();pinMode(cfg::buttonA,INPUT_PULLUP);pinMode(cfg::buttonB,INPUT_PULLUP);
  prefsOk=prefs.begin("eqtool",false);
  if(prefsOk){const float g=prefs.getFloat("gain",NAN);if(isfinite(g)&&fabsf(g)>1e-10f&&fabsf(g)<=.04f){measurement.gain=g;measurement.gainValid=true;}}
  adcBegin();probeQueue=xQueueCreate(1,sizeof(ProbeResult));
  const char *headers[]={"Origin"};server.collectHeaders(headers,1);
  server.on("/",HTTP_GET,[](){++webRequests;noCache();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/html; charset=utf-8",reinterpret_cast<const char*>(PAGE),sizeof(PAGE));});
  server.on("/api/status",HTTP_GET,[](){++webRequests;noCache();server.send(200,"application/json",statusJson());});
  server.on("/api/samples",HTTP_GET,sendSamples);server.on("/api/control",HTTP_POST,control);
  server.onNotFound([](){if(server.uri().startsWith("/api/"))server.send(404,"application/json","{\"error\":\"not found\"}");else{server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","Open http://192.168.4.1");}});
  wifiOn();render();Serial.println(statusJson());serialPending=true;
}
void loop(){
  const bool wasOnline=measurement.online;
  AdcFrame f;
  if(adcQueue)while(xQueueReceive(adcQueue,&f,0)==pdTRUE){if(f.fault)measurement.fault(f.ms);else measurement.ingest(f.raw,f.ms,f.gap);}
  measurement.tick(millis());
  if(wasOnline!=measurement.online)hold=false;
  if(measurement.saveGain){measurement.saveGain=false;if(!prefsOk||prefs.putFloat("gain",measurement.gain)!=sizeof(float))measurement.result="storage_error";}
  if(screen==Screen::Busy&&measurement.job==eq::Job::None)screen=Screen::Result;
  buttonEvent(true,buttonA.update(digitalRead(cfg::buttonA)==LOW,millis()));
  buttonEvent(false,buttonB.update(digitalRead(cfg::buttonB)==LOW,millis()));
  if(apOk){dns.processNextRequest();server.handleClient();}
  ProbeResult probe;
  if(probeQueue&&xQueueReceive(probeQueue,&probe,0)==pdTRUE){
    serialPending=true;
    probeBusy=false;Serial.print("{\"http_selftest\":");Serial.print(probe.ok?"true":"false");Serial.print(",\"path\":\"");Serial.print(probePaths[probe.path]);Serial.print("\",\"bytes\":");Serial.print(probe.bytes);Serial.print(",\"expected\":");Serial.print(probe.expected);Serial.println("}");
  }
  while(Serial.available()){const char c=Serial.read();if(c=='\n'){serialLine[serialLength]=0;serialCommand();serialLength=0;}else if(c!='\r'&&serialLength<sizeof(serialLine)-1)serialLine[serialLength++]=c;}
  // HWCDC may queue a reply while its USB connection check is transiently false.
  // Drain, then send a separate short delimiter: Windows CDC can otherwise
  // hold a reply ending exactly at a 64-byte USB packet boundary.
  if(serialPending&&Serial){Serial.flush();Serial.println();Serial.flush();serialPending=false;}
  if(millis()-lastDisplay>=100){lastDisplay=millis();render();}
  delay(1);
}
