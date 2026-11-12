// SPDX-License-Identifier: GPL-2.0-or-later
// Panel initialization/font derived from PocketWorld and WebQrTest.
#pragma once
#include <Wire.h>
#include "qr_bitmaps.h"
static uint8_t oled[360], oledAddress=0x3c;
static bool oledOk=false;
static void commandBytes(const uint8_t *data,size_t length) {
  Wire.beginTransmission(oledAddress);Wire.write(0);Wire.write(data,length);Wire.endTransmission();
}
static void displayInit() {
  Wire.begin(cfg::sda,cfg::scl);Wire.setClock(400000);
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
