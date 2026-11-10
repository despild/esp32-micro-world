"""WTS notifications on a dedicated hidden window, isolated from Tcl/Tk."""
import ctypes as C
from ctypes import wintypes as W
import os,queue,threading

class SessionWatcher:
    def __init__(self,root,callback):
        self.root=root;self.callback=callback;self.hwnd=None;self.closed=False
        self.events=queue.Queue();self.started=threading.Event();self.error=None
        if os.name!='nt':return
        self.thread=threading.Thread(target=self._listen,daemon=True)
        self.thread.start()
        if not self.started.wait(3):raise RuntimeError('WTS window startup timeout')
        if self.error:raise self.error
        root.after(50,self._drain)

    def _drain(self):
        if self.closed:return
        while not self.events.empty():
            try:self.callback(self.events.get_nowait())
            except queue.Empty:break
        self.root.after(50,self._drain)

    def _listen(self):
        user=C.WinDLL('user32',use_last_error=True)
        kernel=C.WinDLL('kernel32',use_last_error=True)
        wts=C.WinDLL('wtsapi32',use_last_error=True)
        proc_type=C.WINFUNCTYPE(C.c_ssize_t,W.HWND,W.UINT,W.WPARAM,W.LPARAM)
        class WindowClass(C.Structure):
            _fields_=[('style',W.UINT),('lpfnWndProc',proc_type),('cbClsExtra',C.c_int),('cbWndExtra',C.c_int),
                ('hInstance',W.HANDLE),('hIcon',W.HANDLE),('hCursor',W.HANDLE),('hbrBackground',W.HANDLE),
                ('lpszMenuName',W.LPCWSTR),('lpszClassName',W.LPCWSTR)]
        kernel.GetModuleHandleW.argtypes=[W.LPCWSTR];kernel.GetModuleHandleW.restype=W.HMODULE
        user.RegisterClassW.argtypes=[C.POINTER(WindowClass)];user.RegisterClassW.restype=W.ATOM
        user.UnregisterClassW.argtypes=[W.LPCWSTR,W.HINSTANCE]
        user.CreateWindowExW.argtypes=[W.DWORD,W.LPCWSTR,W.LPCWSTR,W.DWORD,C.c_int,C.c_int,C.c_int,C.c_int,W.HWND,W.HMENU,W.HINSTANCE,C.c_void_p]
        user.CreateWindowExW.restype=W.HWND
        user.DefWindowProcW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM];user.DefWindowProcW.restype=C.c_ssize_t
        user.DestroyWindow.argtypes=[W.HWND]
        user.GetMessageW.argtypes=[C.POINTER(W.MSG),W.HWND,W.UINT,W.UINT]
        user.TranslateMessage.argtypes=[C.POINTER(W.MSG)]
        user.DispatchMessageW.argtypes=[C.POINTER(W.MSG)];user.DispatchMessageW.restype=C.c_ssize_t
        user.PostMessageW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM]
        wts.WTSRegisterSessionNotification.argtypes=[W.HWND,W.DWORD]
        wts.WTSUnRegisterSessionNotification.argtypes=[W.HWND]
        def window_proc(hwnd,message,wparam,lparam):
            if message==0x02B1 and wparam in (7,8):
                self.events.put(wparam==7);return 0
            if message==0x0010:
                wts.WTSUnRegisterSessionNotification(hwnd);user.DestroyWindow(hwnd);return 0
            if message==0x0002:user.PostQuitMessage(0);return 0
            return user.DefWindowProcW(hwnd,message,wparam,lparam)
        self.proc=proc_type(window_proc)
        instance=kernel.GetModuleHandleW(None)
        name=f'PocketWorldSession_{os.getpid()}_{threading.get_ident()}'
        wc=WindowClass();wc.lpfnWndProc=self.proc;wc.hInstance=instance;wc.lpszClassName=name
        registered=False
        try:
            if not user.RegisterClassW(C.byref(wc)):raise C.WinError(C.get_last_error())
            registered=True
            hwnd=user.CreateWindowExW(0,name,name,0,0,0,0,0,None,None,instance,None)
            if not hwnd:raise C.WinError(C.get_last_error())
            self.hwnd=hwnd;self.user=user
            if not wts.WTSRegisterSessionNotification(hwnd,0):raise C.WinError(C.get_last_error())
            self.started.set()
            message=W.MSG()
            while user.GetMessageW(C.byref(message),None,0,0)>0:
                user.TranslateMessage(C.byref(message));user.DispatchMessageW(C.byref(message))
        except Exception as exc:
            self.error=exc;self.started.set()
        finally:
            if self.hwnd:user.DestroyWindow(self.hwnd)
            self.hwnd=None
            if registered:user.UnregisterClassW(name,instance)

    def close(self):
        self.closed=True
        if self.hwnd:
            self.user.PostMessageW(self.hwnd,0x0010,0,0)
            self.thread.join(timeout=1)
