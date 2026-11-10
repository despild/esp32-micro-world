"""One serial owner, bounded queues, explicit protocol handshake, reconnect."""
import json
import queue
import threading
import time
import serial
from serial.tools import list_ports

class BoardLink:
    def __init__(self, events):
        self.events=events
        self.commands=queue.Queue(maxsize=64)
        self.keys=0
        self.mode=2
        self.locked=False
        self.mirror=True
        self.ready=False
        self.port=None
        self.stop=threading.Event()
        self.thread=threading.Thread(target=self._run,daemon=True)
        self.thread.start()

    def send(self,command):
        if not self.ready: return
        try: self.commands.put_nowait(command)
        except queue.Full: pass

    def close(self):
        self.stop.set()
        self.thread.join(timeout=1)

    def _event(self,value):
        try: self.events.put_nowait(value)
        except queue.Full: pass

    def _run(self):
        while not self.stop.is_set():
            ports=[p.device for p in list_ports.comports() if p.vid==0x303A and p.pid==0x1001]
            if not ports:
                self.stop.wait(1);continue
            port=self.port if self.port in ports else ports[0]
            try:
                with serial.Serial(port,115200,timeout=0.01,write_timeout=.3) as ser:
                    self.port=port
                    ser.dtr=True;ser.rts=False
                    last_hello=last_keys=0
                    buffer=bytearray()
                    while not self.stop.is_set():
                        now=time.monotonic()
                        if not self.ready and now-last_hello>1:
                            ser.write(b'HELLO\n');last_hello=now
                        if self.ready:
                            if now-last_keys>.08:
                                ser.write(f'KEY {0 if self.locked else self.keys}\n'.encode());last_keys=now
                            for _ in range(8):
                                try: cmd=self.commands.get_nowait()
                                except queue.Empty: break
                                ser.write((cmd+'\n').encode('ascii'))
                        buffer.extend(ser.read(4096))
                        while b'\n' in buffer:
                            raw,_,buffer=buffer.partition(b'\n')
                            try: msg=json.loads(raw)
                            except (ValueError,UnicodeDecodeError):
                                if raw.startswith(b'DOOM ERROR:'):
                                    self._event({'t':'error','message':raw.decode(errors='replace')})
                                continue
                            if not isinstance(msg,dict): continue
                            if msg.get('t')=='hello' and msg.get('id')=='oled-doom' and msg.get('v')==1:
                                self.ready=True
                                ser.write(f'LOCK {int(self.locked)}\nMODE {self.mode}\nMIRROR {int(self.mirror)}\n'.encode())
                                self._event({'t':'connected','port':port})
                            if self.ready: self._event(msg)
                        if len(buffer)>8192: buffer.clear()
                    if self.ready:
                        ser.write(b'KEY 0\nLOCK 1\n')
            except (serial.SerialException,OSError) as exc:
                self._event({'t':'disconnected','message':str(exc)})
            finally:
                self.ready=False;self.keys=0
                while not self.commands.empty():
                    try: self.commands.get_nowait()
                    except queue.Empty: break
            self.stop.wait(1)
