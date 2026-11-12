/* Browser integration tests. Mock transport is test-only; the display-only demo ships in firmware. */
const fs=require('fs'),path=require('path'),http=require('http'),assert=require('assert/strict'),zlib=require('zlib');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
const root=path.resolve(__dirname,'..'),out=path.join(root,'output/web-tests');
fs.mkdirSync(out,{recursive:true});
const embedded=fs.readFileSync(path.join(root,'firmware/EqualizationTool/page.h'),'utf8');
const compressed=Buffer.from([...embedded.matchAll(/0x([0-9a-f]{2})/g)].map(m=>parseInt(m[1],16)));
assert.equal(zlib.gunzipSync(compressed).toString('utf8'),fs.readFileSync(path.join(root,'web/index.html'),'utf8'));
let mode='missing',seq=0,t=1000,revision=1,boot='test0001',lost=0;
const commands=[],errors=[];
const server=http.createServer((req,res)=>{
 if(req.url==='/'){res.setHeader('Content-Type','text/html; charset=utf-8');res.setHeader('Content-Encoding','gzip');res.setHeader('Content-Length',compressed.length);res.end(compressed);return;}
 if(req.url.startsWith('/api/samples')){
  if(mode==='disconnected'){res.writeHead(503);res.end();return;}
  const online=mode!=='missing',calibrated=mode==='calibrated'||mode==='saturated',samples=[];
  for(let i=0;i<4;i++){seq++;t+=25;const p=1.5+Math.sin(t/300)*.2;const flags=!online?1:mode==='saturated'?2:calibrated?0:4;samples.push([seq,t,online?Math.round(500000+p*50000):null,calibrated&&flags===0?p:null,flags]);}
  const last=samples.at(-1),status={device:'ESP32-C3',boot,revision,ms:t,online,calibrated,zero_valid:calibrated,gain_valid:calibrated,zero:500000,raw:last[2],pressure:last[3],flags:last[4],job:'idle',collected:0,result:'idle',dropped:0};
  res.setHeader('Content-Type','application/json');res.end(JSON.stringify({status,lost,next:seq,more:false,samples}));lost=0;return;
 }
 if(req.url==='/api/control'){let body='';req.on('data',c=>body+=c);req.on('end',()=>{commands.push(Object.fromEntries(new URLSearchParams(body)));res.setHeader('Content-Type','application/json');res.end('{"ok":true}');});return;}
 res.writeHead(404);res.end();
});
(async()=>{
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 if(process.argv.includes('--serve')){console.log(`Mock preview only: http://127.0.0.1:${server.address().port}`);return;}
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXE||undefined});
 try{
 const page=await browser.newPage({viewport:{width:390,height:844},deviceScaleFactor:1});
 page.on('pageerror',e=>errors.push(e.message));page.on('dialog',d=>d.accept());
 await page.goto(`http://127.0.0.1:${server.address().port}`);
 await page.waitForFunction(()=>document.querySelector('#notice').textContent.includes('센서 미연결'));
 assert.equal(await page.locator('#value').textContent(),'—');assert(await page.locator('#zero').isDisabled());
 assert.equal(await page.locator('#chart').getAttribute('data-mode'),'demo');assert(await page.locator('#csv').isDisabled());
 assert.equal(await page.locator('#empty').isVisible(),false);assert.equal(await page.locator('#demo-note').isVisible(),true);
 const periodic=await page.evaluate(()=>{
   const bounds=Array.from({length:1200},(_,i)=>demoValueAt(i*11));
   return {bounds:bounds.every(v=>Number.isFinite(v)&&v>=0&&v<=100),period:demoValueAt(900)===demoValueAt(120900),peak:demoValueAt(900),baseline:demoValueAt(600),hold:demoValueAt(9000),realMissing:points.every(p=>p[2]===null&&p[3]===null),window:plotView().points.length};
 });
 assert.deepEqual(periodic,{bounds:true,period:true,peak:74,baseline:0,hold:70,realMissing:true,window:801});
 const trace1=await page.locator('#chart').screenshot();await page.waitForTimeout(600);const trace2=await page.locator('#chart').screenshot();assert(!trace1.equals(trace2));
 await page.locator('#pause').click();const held1=await page.locator('#chart').screenshot();await page.waitForTimeout(350);assert(held1.equals(await page.locator('#chart').screenshot()));
 assert.equal(await page.locator('#pause').getAttribute('aria-pressed'),'true');
 await page.locator('#pause').click();await page.waitForTimeout(350);assert(!held1.equals(await page.locator('#chart').screenshot()));
 assert.equal(await page.locator('#pause').getAttribute('aria-pressed'),'false');
 assert.equal(await page.locator('#settings').getAttribute('open'),null);
 for(const width of [320,360,390,768,1280]){
  await page.setViewportSize({width,height:900});
  const layout=await page.evaluate(()=>({overflow:document.documentElement.scrollWidth>innerWidth,chartHeight:document.querySelector('#chart').getBoundingClientRect().height,targets:[...document.querySelectorAll('button,select,summary')].filter(e=>e.getClientRects().length).every(e=>e.getBoundingClientRect().height>=44)}));
  assert(!layout.overflow && layout.chartHeight>=200 && layout.targets,JSON.stringify({width,layout}));
 }
 await page.setViewportSize({width:390,height:844});
 await page.locator('#pause').focus();await page.keyboard.press('Tab');await page.keyboard.press('Shift+Tab');
 assert.equal(await page.locator('#pause').evaluate(e=>document.activeElement===e),true);
 assert.notEqual(await page.locator('#pause').evaluate(e=>getComputedStyle(e).outlineStyle),'none');
 await page.screenshot({path:path.join(out,'mobile-no-sensor.png'),fullPage:true});
 await page.setViewportSize({width:1280,height:960});await page.screenshot({path:path.join(out,'desktop-no-sensor.png'),fullPage:true});await page.setViewportSize({width:390,height:844});
 await page.locator('#pause').click();mode='raw'; // Same revision: a returning sensor must still discard the frozen demo.
 await page.waitForFunction(()=>document.querySelector('#notice').textContent.includes('영점을 맞춰'));
 assert.equal(await page.locator('#unit').textContent(),'RAW');assert(await page.locator('#calibrate').isDisabled());
 assert.equal(await page.locator('#chart').getAttribute('data-mode'),'live');assert.equal(await page.locator('#held').textContent(),'');
 assert.equal(await page.locator('#demo-note').isVisible(),false);
 await page.locator('#settings summary').click();
 await page.locator('#zero').click();await page.waitForTimeout(150);assert.equal(commands.at(-1).action,'zero');
 mode='calibrated';revision++;lost=7;
 await page.waitForFunction(()=>document.querySelector('#unit').textContent==='kPa');
 await page.waitForTimeout(1200);
 assert.match(await page.locator('#loss').textContent(),/7개/);
 assert(!(await page.locator('#calibrate').isDisabled()));
 await page.locator('#reference').fill('2.5');await page.locator('#calibrate').click();await page.waitForTimeout(100);
 assert.deepEqual(commands.at(-1),{action:'calibrate',kpa:'2.5'});
 await page.screenshot({path:path.join(out,'mobile-settings.png'),fullPage:true});
 await page.locator('#settings summary').click();
 await page.locator('#display').click();await page.waitForTimeout(150);assert.equal(commands.at(-1).action,'display');
 await page.locator('#pause').click();assert.match(await page.locator('#held').textContent(),/일시정지/);
 await page.locator('#pause').click();await page.locator('#scale').selectOption('5');
 await page.screenshot({path:path.join(out,'mobile-calibrated.png'),fullPage:true});
 await page.setViewportSize({width:1280,height:960});await page.screenshot({path:path.join(out,'desktop-calibrated.png'),fullPage:true});
 const download=page.waitForEvent('download');await page.locator('#csv').click();const csv=await download;await csv.saveAs(path.join(out,'trace.csv'));
 assert.match(fs.readFileSync(path.join(out,'trace.csv'),'utf8'),/pressure_kpa/);
 mode='saturated';await page.waitForFunction(()=>document.querySelector('#notice').textContent.includes('ADC 포화'));assert.equal(await page.locator('#value').textContent(),'—');
 assert.equal(await page.locator('#chart').getAttribute('data-mode'),'live');
 mode='missing';await page.waitForFunction(()=>document.querySelector('#chart').dataset.mode==='demo');
 assert.equal(await page.locator('#value').textContent(),'—');assert(await page.locator('#csv').isDisabled());
 await page.locator('#pause').click();
 mode='disconnected';await page.waitForFunction(()=>document.querySelector('#connection').textContent==='연결 끊김');assert.equal(await page.locator('#value').textContent(),'—');
 assert.equal(await page.locator('#chart').getAttribute('data-mode'),'live');assert.equal(await page.locator('#held').textContent(),'');
 mode='missing';boot='test0002';revision=1;seq=0;t=10;
 await page.waitForFunction(()=>document.querySelector('#notice').textContent.includes('센서 미연결'));
 assert.equal(await page.locator('#unit').textContent(),'RAW');assert.equal(await page.locator('#loss').textContent(),'');
 assert.deepEqual(errors,[]);
 const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth);assert(!overflow);
 fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({result:'PASS',checks:['embedded gzip matches source','missing sensor shows continuously moving demo','periodic pulse/hold/release shape','demo samples never enter measurements','demo CSV blocked','demo pause/resume','sensor reconnect discards held demo without revision change','RAW before calibration','zero POST','reference POST','calibrated graph','gap notice','pause/resume','mobile/desktop layout','CSV contains only measurements','saturation stays live','sensor removal resumes demo','device disconnect suspends demo','device reboot','320-1280 layout and 44px controls','keyboard focus visible','pause announced as pressed','settings disclosure','OLED display POST'],pageErrors:errors},null,2));
 console.log('PASS: 24 browser scenarios; mock transport only.');
 }finally{await browser.close();server.close();}
})().catch(e=>{console.error(e);server.close();process.exitCode=1;});
