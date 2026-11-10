#pragma once
#define POCKET_WORLD_CUSTOM
// Parent application only. The separate Doom repository has no pet/dungeon code.
static Preferences worldStore;
static int hunger=80,joy=80,xp=0,floorNo=1,bestFloor=1,health=6;
static int heroX=1,heroY=2,enemyHp[3]={2,2,2};
static const int enemyX[3]={4,7,9},enemyY[3]={2,1,3};
static bool hasKey=false,sleeping=false;
static unsigned long lastWorldSave=0,lastNeeds=0,lastMove=0,lastHit=0,celebrateUntil=0;
static uint16_t oldWorldKeys=0;
static int petX=30,petDirection=1;
inline void worldSetup() {
 worldStore.begin("pocket-world",false);
 hunger=worldStore.getInt("hunger",80);joy=worldStore.getInt("joy",80);
 xp=worldStore.getInt("xp",0);bestFloor=worldStore.getInt("best",1);
}
static void worldSave() {
 worldStore.putInt("hunger",hunger);worldStore.putInt("joy",joy);
 worldStore.putInt("xp",xp);worldStore.putInt("best",bestFloor);
}
static void resetFloor() {
 heroX=1;heroY=2;hasKey=false;
 for(int i=0;i<3;i++) enemyHp[i]=(floorNo%5==0?3:2);
}
inline void worldCommand(const char *command) {
 if(!strcmp(command,"PET FEED")) {hunger=min(100,hunger+20);joy=min(100,joy+5);xp++;celebrateUntil=millis()+2000;}
 if(!strcmp(command,"PET PLAY")) {joy=min(100,joy+20);hunger=max(0,hunger-5);xp+=2;celebrateUntil=millis()+2000;}
 if(!strcmp(command,"PET REST")) {sleeping=!sleeping;health=6;}
 if(!strcmp(command,"DUNGEON NEW")) {floorNo=1;health=6;resetFloor();}
}
static bool walkable(int x,int y) {
 return x>=1&&x<=10&&y>=0&&y<=4 && !(x==3&&y==1) && !(x==5&&y==3);
}
static void dungeonStep(uint16_t keys,bool autonomous) {
 if(millis()-lastMove<180) return;
 lastMove=millis();
 if(health<=0) {
   if(keys&3 || autonomous) {health=6;resetFloor();}
   return;
 }
 if(autonomous) {
   // Explore the current room. All choices and combat happen on the board.
   int targetX=hasKey?10:6,targetY=hasKey?2:3;
   for(int i=0;i<3;i++) if(enemyHp[i]>0) {targetX=enemyX[i];targetY=enemyY[i];break;}
   keys=2;
   if(abs(heroX-targetX)+abs(heroY-targetY)>1) {
     if(heroX<targetX&&walkable(heroX+1,heroY)) keys|=1<<7;
     else if(heroX>targetX&&walkable(heroX-1,heroY)) keys|=1<<6;
     else if(heroY<targetY&&walkable(heroX,heroY+1)) keys|=1<<5;
     else if(heroY>targetY&&walkable(heroX,heroY-1)) keys|=1<<4;
   } else if(targetX==6||hasKey) {
     if(heroX<targetX) keys|=1<<7; else if(heroX>targetX) keys|=1<<6;
     else if(heroY<targetY) keys|=1<<5; else if(heroY>targetY) keys|=1<<4;
   }
 }
 int dx=(keys&(1<<7)?1:0)-(keys&(1<<6)?1:0);
 int dy=(keys&(1<<5)?1:0)-(keys&(1<<4)?1:0);
 if(dx)dy=0;
 bool blocked=false;
 for(int i=0;i<3;i++) if(enemyHp[i]>0&&heroX+dx==enemyX[i]&&heroY+dy==enemyY[i]) blocked=true;
 if(!blocked&&walkable(heroX+dx,heroY+dy)){heroX+=dx;heroY+=dy;}
 if(keys&2) for(int i=0;i<3;i++) if(enemyHp[i]>0&&abs(heroX-enemyX[i])+abs(heroY-enemyY[i])<=1) {
   enemyHp[i]--;if(enemyHp[i]==0){xp+=3;joy=min(100,joy+3);}break;
 }
 if(millis()-lastHit>1000) for(int i=0;i<3;i++) if(enemyHp[i]>0&&abs(heroX-enemyX[i])+abs(heroY-enemyY[i])<=1) {
   health--;lastHit=millis();break;
 }
 if(enemyHp[0]+enemyHp[1]+enemyHp[2]==0&&heroX==6&&heroY==3)hasKey=true;
 if(hasKey&&heroX==10&&heroY==2) {floorNo++;bestFloor=max(bestFloor,floorNo);health=min(6,health+2);xp+=10;resetFloor();worldSave();}
}
static void drawDungeon() {
 char b[24];snprintf(b,sizeof(b),"F%d H%d XP%d",floorNo,max(0,health),xp);text3(1,1,b);
 for(int y=0;y<5;y++)for(int x=0;x<12;x++) {
   int xx=x*6,yy=9+y*6;
   if(!walkable(x,y)) {rect(xx,yy,5,5);continue;}
   if(x==10&&y==2) {rect(xx,yy,5,5);if(hasKey)pixel(xx+2,yy+2);}
   if(x==6&&y==3&&!hasKey&&enemyHp[0]+enemyHp[1]+enemyHp[2]==0) {rect(xx,yy,3,3);pixel(xx+2,yy+3);pixel(xx+3,yy+4);}
 }
 for(int i=0;i<3;i++)if(enemyHp[i]>0){int x=enemyX[i]*6,y=9+enemyY[i]*6;rect(x,y+1,5,4,true);pixel(x+1,y+2,false);pixel(x+3,y+2,false);}
 int x=heroX*6,y=9+heroY*6;pixel(x+2,y);rect(x+1,y+1,4,2,true);rect(x+1,y+3,3,2,true);
 if(health<=0) {memset(oled,0,360);text3(14,11,"TRY AGAIN");text3(6,23,"CTRL TO RESTART");}
}
static void drawPet(bool autonomous) {
 char b[24];snprintf(b,sizeof(b),"LV%d H%d J%d",1+xp/30,hunger,joy);text3(1,1,b);
 bool nap=sleeping || (autonomous&&(millis()/7000)%5==3);
 int activity=(millis()/5000)%4;
 if(autonomous&&!nap&&millis()%400<110){petX+=petDirection;if(petX>48||petX<9)petDirection=-petDirection;}
 int y=17+((!nap&&(millis()/300)%2)?1:0),x=petX;
 rect(x+2,y,11,12,true);rect(x,y+3,15,7,true);
 rect(x+2,y-3,3,4,true);rect(x+10,y-3,3,4,true);
 if(nap){pixel(x+4,y+4,false);pixel(x+5,y+4,false);pixel(x+9,y+4,false);pixel(x+10,y+4,false);text3(x+16,y-5,"Z");}
 else {rect(x+4,y+3,2,3,false);pixel(x+4,y+4,false);pixel(x+5,y+4,false);pixel(x+9,y+4,false);pixel(x+10,y+4,false);}
 pixel(x+7,y+7,false);pixel(x+6,y+8,false);pixel(x+8,y+8,false);
 rect(x+2,y+12,4,2,true);rect(x+9,y+12,4,2,true);
 for(int i=1;i<71;i+=3)pixel(i,34);
 rect(57,29,10,4);pixel(60,27);pixel(63,27);
 if(celebrateUntil>millis()){text3(25,8,"HAPPY");}
 else if(autonomous&&activity==1){pixel(5,20);pixel(6,19);pixel(7,20);pixel(6,21);}
}
inline void worldTick(int mode,bool autonomous) {
 if(millis()-lastNeeds>60000){lastNeeds=millis();hunger=max(0,hunger-1);joy=max(0,joy-1);if(sleeping)joy=min(100,joy+2);}
 if(millis()-lastWorldSave>60000){lastWorldSave=millis();worldSave();}
 memset(oled,0,360);
 if(mode==1){dungeonStep(keyMask,autonomous);drawDungeon();}else drawPet(autonomous);
}
