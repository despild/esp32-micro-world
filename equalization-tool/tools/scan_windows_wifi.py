"""Request a fresh Windows WLAN scan without changing the active connection."""
import argparse
import ctypes as c
from ctypes import wintypes as w
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import subprocess
import time

class GUID(c.Structure):
    _fields_=[('a',w.DWORD),('b',w.WORD),('c',w.WORD),('d',c.c_ubyte*8)]

class Interface(c.Structure):
    _fields_=[('guid',GUID),('description',w.WCHAR*256),('state',w.DWORD)]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    api=c.WinDLL('wlanapi.dll')
    api.WlanOpenHandle.argtypes=[w.DWORD,c.c_void_p,c.POINTER(w.DWORD),c.POINTER(w.HANDLE)]
    api.WlanEnumInterfaces.argtypes=[w.HANDLE,c.c_void_p,c.POINTER(c.c_void_p)]
    api.WlanScan.argtypes=[w.HANDLE,c.POINTER(GUID),c.c_void_p,c.c_void_p,c.c_void_p]
    api.WlanFreeMemory.argtypes=[c.c_void_p]
    api.WlanCloseHandle.argtypes=[w.HANDLE,c.c_void_p]
    version=w.DWORD();handle=w.HANDLE();data=c.c_void_p()
    status=api.WlanOpenHandle(2,None,c.byref(version),c.byref(handle))
    if status:raise OSError(f'WlanOpenHandle: {status}')
    try:
        status=api.WlanEnumInterfaces(handle,None,c.byref(data))
        if status:raise OSError(f'WlanEnumInterfaces: {status}')
        count=c.cast(data,c.POINTER(w.DWORD))[0]
        items=(Interface*count).from_address(data.value+8)
        scan_results=[]
        for item in items:
            result=api.WlanScan(handle,c.byref(item.guid),None,None,None)
            scan_results.append(result)
            print('Scan request result:',result,flush=True)
        time.sleep(6)
        report=subprocess.check_output(['netsh','wlan','show','networks','mode=bssid']).decode(errors='replace')
        # Do not print unrelated nearby SSIDs or BSSIDs.
        visible=bool(re.search(r'^SSID\s+\d+\s*:\s*EQ-C3\s*$',report,re.M))
        print('EQ-C3 visible:',visible)
        lines=report.splitlines()
        for i,line in enumerate(lines):
            if re.match(r'^SSID\s+\d+\s*:\s*EQ-C3\s*$',line):
                end=next((j for j in range(i+1,len(lines)) if re.match(r'^SSID\s+\d+\s*:',lines[j])),len(lines))
                print('\n'.join(lines[i:end]))
        if args.output:
            args.output.parent.mkdir(parents=True,exist_ok=True)
            args.output.write_text(json.dumps({
                'time_utc':datetime.now(timezone.utc).isoformat(),
                'ssid':'EQ-C3','scan_api_results':scan_results,'visible':visible,
                'method':'Windows WlanScan, 6 second wait, netsh wlan show networks mode=bssid',
                'limits':'Discovery only; no association or HTTP over Wi-Fi tested.'
            },indent=2),encoding='utf-8')
    finally:
        if data:api.WlanFreeMemory(data)
        api.WlanCloseHandle(handle,None)

if __name__=='__main__':main()
