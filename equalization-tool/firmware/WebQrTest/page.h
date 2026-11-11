#pragma once
#include <Arduino.h>
static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>EQ · ESP32 연결 테스트</title>
<style>
:root{color-scheme:dark;font-family:system-ui,sans-serif;background:#101a22;color:#eef7fa}*{box-sizing:border-box}body{margin:0;padding:28px 20px 40px}main{max-width:520px;margin:auto}small{letter-spacing:.14em;color:#8ba5b6}h1{font-size:34px;margin:12px 0}p{color:#acc1cf;line-height:1.7}.card{border:1px solid #2d4555;background:#172732;border-radius:22px;padding:22px;margin:20px 0}.pill{display:inline-block;border-radius:30px;background:#153f38;color:#88e7c4;padding:7px 12px;font-size:13px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-top:24px}.value{display:block;font-size:25px;margin-top:7px;color:#fff}.label{font-size:13px;color:#a5bbc9}button{border:0;border-radius:14px;background:#9debc8;color:#10251f;width:100%;padding:17px;font-weight:750;font-size:16px;cursor:pointer}button:disabled{opacity:.5}#message{min-height:28px;font-size:14px}footer{font-size:12px;color:#8ba5b6;line-height:1.8}a{color:#9debc8}</style>
<main><small>EQ TOOL / CONNECTION TEST</small><h1>연결됐어요.</h1><p>지금 보고 있는 페이지는 ESP32-C3에서 직접 제공하고 있어요.</p>
<section class="card"><span class="pill" id="state">기기 응답 확인 중</span><div class="grid"><div class="label">켜진 시간<span class="value" id="uptime">—</span></div><div class="label">접속 기기<span class="value" id="clients">—</span></div><div class="label">OLED<span class="value" id="oled">—</span></div><div class="label">응답 횟수<span class="value" id="responses">0</span></div></div></section>
<button id="test">기기 OLED에 확인 표시</button><p id="message" role="status">버튼을 누르면 OLED에 PHONE OK가 표시됩니다.</p>
<section class="card"><b>웹 연결 시험 중</b><p>압력센서는 아직 연결하지 않았습니다. 압력 측정값은 표시하지 않습니다.</p></section>
<footer>Wi-Fi: EQ-C3 · 암호: eqtest32<br>주소: <a href="http://192.168.4.1">http://192.168.4.1</a><br>인터넷 연결 없이 사용할 수 있어요. 화면을 닫거나 잠그면 상태 갱신이 멈출 수 있습니다.</footer></main>
<script>
const $=id=>document.getElementById(id);let responses=0;
async function poll(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();$('state').textContent='실시간 연결됨';$('uptime').textContent=Math.floor(s.uptime_ms/1000)+'초';$('clients').textContent=s.clients+'대';$('oled').textContent=s.oled?'정상':'확인 필요';$('responses').textContent=++responses}catch(e){$('state').textContent='연결 확인이 필요해요'}setTimeout(poll,1000)}
$('test').onclick=async()=>{const b=$('test');b.disabled=true;try{const r=await fetch('/api/display',{method:'POST'});if(!r.ok)throw Error();$('message').textContent='OLED에 PHONE OK를 표시했어요. 5초 뒤 QR로 돌아갑니다.'}catch(e){$('message').textContent='전송하지 못했어요. 기기 Wi-Fi 연결을 확인해 주세요.'}finally{b.disabled=false}};poll();
</script></html>)HTML";
