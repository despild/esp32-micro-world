"""Pocket World: native Windows controller; no game rendering on the PC."""
import ctypes
import json
import os
from pathlib import Path
import queue
import sys
import time
import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image,ImageTk
from link import BoardLink
from windows_session import SessionWatcher

BG='#111820'; PANEL='#1a2530'; BORDER='#304252'; FG='#e9f2ed'; MUTED='#91a7b1'; ACCENT='#a8e7a0'
class Controller:
    def __init__(self,root):
        self.root=root
        root.title('Pocket World · ESP32 OLED Controller')
        root.geometry('1120x820');root.minsize(1000,760);root.configure(bg=BG)
        self.events=queue.Queue(maxsize=100)
        self.link=BoardLink(self.events)
        self.pressed=set();self.mode=2;self.locked=False;self.test_lock=False
        self.last_frame=time.monotonic();self.last_state=0
        self.image=None;self.photo=None;self.session=None
        self.build_ui()
        root.bind('<KeyPress>',self.keydown);root.bind('<KeyRelease>',self.keyup)
        root.bind('<FocusOut>',lambda e: self.clear_keys())
        root.protocol('WM_DELETE_WINDOW',self.close)
        try: self.session=SessionWatcher(root,self.session_change)
        except Exception as exc: self.log('잠금 감지 등록 실패: '+str(exc))
        root.after(30,self.poll)

    def label(self,parent,text,size=11,color=FG,**kw):
        return tk.Label(parent,text=text,bg=parent['bg'],fg=color,font=('Malgun Gothic',size),**kw)
    def button(self,parent,text,command,accent=False):
        return tk.Button(parent,text=text,command=command,bg=ACCENT if accent else '#283a49',
            fg=BG if accent else FG,activebackground='#c6f6bf',activeforeground=BG,
            relief='flat',bd=0,padx=12,pady=6,font=('Malgun Gothic',10),cursor='hand2',takefocus=False)
    def build_ui(self):
        head=tk.Frame(self.root,bg=BG);head.pack(fill='x',padx=28,pady=(22,15))
        self.label(head,'POCKET WORLD',22).pack(side='left')
        self.connection=self.label(head,'● 보드 연결 대기',11,MUTED);self.connection.pack(side='right')
        self.label(self.root,'작은 세계는 보드 안에서. PC에서는 선택하고, 조작하세요.',11,MUTED).pack(anchor='w',padx=29)
        content=tk.Frame(self.root,bg=BG);content.pack(fill='both',expand=True,padx=28,pady=18)
        left=tk.Frame(content,bg=PANEL,padx=20,pady=18);left.pack(side='left',fill='both',expand=True)
        right=tk.Frame(content,bg=BG,width=284);right.pack(side='right',fill='y',padx=(20,0));right.pack_propagate(False)
        title=tk.Frame(left,bg=PANEL);title.pack(fill='x')
        self.label(title,'LIVE OLED',12).pack(side='left')
        self.live_view=tk.BooleanVar(value=True)
        tk.Checkbutton(title,text='라이브 뷰',variable=self.live_view,command=self.toggle_live,
            bg=PANEL,fg=FG,selectcolor=PANEL,activebackground=PANEL,activeforeground=FG,
            font=('Malgun Gothic',10)).pack(side='right')
        self.canvas=tk.Canvas(left,width=648,height=360,bg='#030608',highlightthickness=1,highlightbackground=BORDER,takefocus=True)
        self.canvas.pack(pady=(15,12),fill='x')
        self.canvas.bind('<Button-1>',lambda e:self.root.focus_set())
        self.canvas.create_text(324,180,text='ESP32 연결을 기다리고 있습니다',fill=MUTED,font=('Malgun Gothic',13),tags='waiting')
        self.status=self.label(left,'원작 DOOM · 무음 · 셰어웨어 에피소드 1',11,ACCENT);self.status.pack(anchor='w')
        self.metrics=self.label(left,'체력 —    탄약 —    맵 —    프레임 —',11,MUTED);self.metrics.pack(anchor='w',pady=(8,12))
        self.label(left,'↑ ↓ 이동   ← → 회전   Ctrl 공격   Space 문 / 확인',11).pack(anchor='w')
        self.label(left,'Q / E 옆걸음   Enter 메뉴   Tab 지도',10,MUTED).pack(anchor='w',pady=(5,10))
        self.label(left,'컨트롤러 창이 활성화되어 있을 때만 키를 전송합니다.',9,MUTED).pack(anchor='w')
        self.logbox=tk.Text(left,height=4,bg='#111b23',fg=MUTED,font=('Malgun Gothic',9),relief='flat',wrap='word',state='disabled')
        self.logbox.pack(fill='both',expand=True,pady=(16,0))
        self.label(right,'화면과 게임',13).pack(anchor='w',pady=(0,10))
        self.modebuttons=[]
        for mode,label in [(0,'펫 · 함께 생활하기'),(1,'던전 · 탐험하기'),(2,'DOOM · 원작 플레이')]:
            btn=self.button(right,label,lambda m=mode:self.select_mode(m),mode==2);btn.pack(fill='x',pady=4);self.modebuttons.append(btn)
        self.label(right,'DOOM 스테이지',10,MUTED).pack(anchor='w',pady=(12,4))
        stage=tk.Frame(right,bg=BG);stage.pack(fill='x')
        self.stage=ttk.Combobox(stage,state='readonly',values=[f'E1M{i}' for i in range(1,10)],width=7,font=('Segoe UI',11));self.stage.current(0);self.stage.pack(side='left',ipady=6)
        self.button(stage,'새 게임',self.new_game).pack(side='right')
        self.label(right,'펫 상호 작용',10,MUTED).pack(anchor='w',pady=(12,4))
        care=tk.Frame(right,bg=BG);care.pack(fill='x')
        for text,cmd in [('먹이','FEED'),('놀이','PLAY'),('휴식','REST')]:
            self.button(care,text,lambda c=cmd:self.link.send('PET '+c)).pack(side='left',expand=True,fill='x',padx=2)
        self.label(right,'잠금 시 자동 생활',12).pack(anchor='w',pady=(14,3))
        self.locklabel=self.label(right,'Windows 잠금 감지 활성',10,ACCENT);self.locklabel.pack(anchor='w')
        self.button(right,'자동 생활 미리보기',self.toggle_auto).pack(fill='x',pady=9)
        self.label(right,'잠금 해제 시 선택한 게임으로 복귀합니다.\n앱을 최소화해도 감지는 계속됩니다.',9,MUTED,justify='left').pack(anchor='w')
        for label,command,start,end,value in [('OLED 밝기','LIGHT',20,220,150),('어두운 장면 보정','GAMMA',0,100,50),('디더링 강도','DITHER',0,100,15)]:
            self.label(right,label,10,MUTED).pack(anchor='w',pady=(8,0))
            scale=tk.Scale(right,from_=start,to=end,orient='horizontal',showvalue=False,width=12,bg=BG,fg=FG,troughcolor=PANEL,highlightthickness=0,bd=0,activebackground=ACCENT,
                command=lambda v,c=command:self.link.send(f'{c} {int(float(v))}'))
            scale.set(value);scale.pack(fill='x')
        self.log('USB 자동 연결을 기다립니다. 원작 DOOM 연산과 화면 출력은 ESP32가 담당합니다.')

    def log(self,message):
        self.logbox.configure(state='normal');self.logbox.insert('end',time.strftime('%H:%M:%S')+'  '+message+'\n')
        if int(self.logbox.index('end-1c').split('.')[0])>40: self.logbox.delete('1.0','10.0')
        self.logbox.see('end');self.logbox.configure(state='disabled')
    def select_mode(self,mode):
        self.clear_keys();self.mode=mode;self.link.mode=mode;self.link.send(f'MODE {mode}')
        for i,b in enumerate(self.modebuttons):b.configure(bg=ACCENT if i==mode else '#283a49',fg=BG if i==mode else FG)
        self.root.focus_set()
    def new_game(self):
        self.select_mode(2);self.link.send(f'NEW {self.stage.current()+1}');self.log('새 게임: '+self.stage.get())
    def session_change(self,locked):
        self.locked=locked;self.apply_lock();self.log('Windows 잠금 → 자동 생활' if locked else 'Windows 잠금 해제 → 직접 조작')
    def toggle_auto(self):
        self.test_lock=not self.test_lock;self.apply_lock();self.root.focus_set()
    def apply_lock(self):
        effective=self.locked or self.test_lock;self.link.locked=effective;self.clear_keys();self.link.send(f'LOCK {int(effective)}')
        self.locklabel.configure(text='자동 생활 중 · 게임 일시 정지' if effective else 'Windows 잠금 감지 활성')
    def clear_keys(self): self.pressed.clear();self.link.keys=0
    def toggle_live(self):
        self.link.mirror=self.live_view.get();self.link.send(f'MIRROR {int(self.link.mirror)}')
        self.canvas.delete('frame');self.canvas.delete('waiting')
        if not self.link.mirror:
            self.canvas.create_text(self.canvas.winfo_width()/2,self.canvas.winfo_height()/2,
                text='라이브 뷰 꺼짐 · OLED에서 실행 중',fill=MUTED,font=('Malgun Gothic',12),tags='waiting')
        self.root.focus_set()
    def keydown(self,event):
        if isinstance(self.root.focus_get(),(tk.Entry,ttk.Combobox,tk.Text)):return
        self.pressed.add(event.keysym);self.update_keys();return 'break'
    def keyup(self,event): self.pressed.discard(event.keysym);self.update_keys()
    def update_keys(self):
        # GBADoom keys: A=use, B=fire, L/R=strafe, U/D/L/R, START, SELECT.
        mapping={'space':0,'Control_L':1,'Control_R':1,'z':1,'q':2,'e':3,'Up':4,'w':4,'Down':5,'s':5,'Left':6,'a':6,'Right':7,'d':7,'Return':8,'Escape':8,'Tab':9}
        self.link.keys=0 if self.locked or self.test_lock else sum(1<<bit for bit in {mapping[k] for k in self.pressed if k in mapping})
    def show_frame(self,data):
        if not self.live_view.get():return
        try: packed=bytes.fromhex(data)
        except ValueError:return
        if len(packed)!=360:return
        image=Image.new('RGB',(72,40),'#030608');pixels=image.load()
        for y in range(40):
            for x in range(72):
                if packed[y//8*72+x]&(1<<(y%8)):pixels[x,y]=(220,246,233)
        self.image=image
        scale=max(1,min(self.canvas.winfo_width()//72,self.canvas.winfo_height()//40))
        self.photo=ImageTk.PhotoImage(image.resize((72*scale,40*scale),Image.Resampling.NEAREST))
        self.canvas.delete('waiting');self.canvas.delete('frame');self.canvas.create_image(self.canvas.winfo_width()/2,self.canvas.winfo_height()/2,anchor='center',image=self.photo,tags='frame')
        self.last_frame=time.monotonic()
    def poll(self):
        for _ in range(50):
            try:msg=self.events.get_nowait()
            except queue.Empty:break
            kind=msg.get('t')
            if kind=='connected':
                self.connection.configure(text='● '+msg['port']+' 연결됨',fg=ACCENT);self.log('보드 연결 완료 · 네이티브 실행')
            elif kind=='disconnected':
                self.connection.configure(text='● 다시 연결하는 중',fg='#edb584');self.clear_keys();self.log('USB 연결이 끊겼습니다. 자동으로 다시 연결합니다.')
            elif kind=='frame':self.show_frame(msg.get('hex',''))
            elif kind=='state':
                self.last_state=time.monotonic()
                names={0:'펫 · 생활',1:'던전 · 탐험',2:'DOOM · 원작 플레이'}
                name='자동 생활 · 게임 일시 정지' if msg.get('auto') else names.get(msg.get('mode'),'대기')
                self.status.configure(text=name)
                self.metrics.configure(text=f"체력 {msg.get('hp',0)}    탄약 {msg.get('ammo',0)}    E1M{msg.get('map',0)}    누적 {msg.get('frames',0)} 프레임")
                if msg.get('fault'):self.status.configure(text='보드 오류 · 로그를 확인하세요')
                elif not msg.get('oled',True):self.status.configure(text='OLED I²C 응답 없음 · 연결 확인 필요')
            elif kind=='error':self.log(msg['message'])
        if self.link.ready and self.last_state and time.monotonic()-self.last_state>5:
            self.connection.configure(text='● 보드 응답 지연',fg='#edb584')
        self.root.after(30,self.poll)
    def close(self):
        if self.session:self.session.close()
        self.link.close();self.root.destroy()

if __name__=='__main__':
    if os.name=='nt':
        try:ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except Exception:pass
    root=tk.Tk();app=Controller(root);root.mainloop()
