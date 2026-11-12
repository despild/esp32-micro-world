/* Run Impeccable against a settled preview via CDP. Continuous sensor polling
 * prevents its URL navigation-idle check from finishing on some browsers. */
const fs=require('fs'),path=require('path'),net=require('net'),{execFile}=require('child_process');
const {promisify}=require('util');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
const root=path.resolve(__dirname,'..'),url=process.argv[2];
if(!/^http:\/\/127\.0\.0\.1:\d+\/?$/.test(url||''))throw Error('Pass the local --serve preview URL');
if(!process.env.IMPECCABLE_ENGINE)throw Error('Set IMPECCABLE_ENGINE to the installed, verified engine executable');
(async()=>{
 const socket=net.createServer();await new Promise(r=>socket.listen(0,'127.0.0.1',r));
 const port=socket.address().port;await new Promise(r=>socket.close(r));
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXE||undefined,args:[`--remote-debugging-port=${port}`,'--remote-debugging-address=127.0.0.1']});
 try{
  const page=await browser.newPage();await page.goto(url,{waitUntil:'domcontentloaded'});
  await page.waitForFunction(()=>document.querySelector('#chart').dataset.mode==='demo');
  const results=[];
  for(const [name,width,height] of [['mobile',390,844],['desktop',1280,960]]){
   await page.setViewportSize({width,height});await page.waitForTimeout(200);
   let output;
   try{output=await promisify(execFile)(process.env.IMPECCABLE_ENGINE,['detect','--json','--cdp',String(port),'--target',url],{cwd:root,timeout:60000,windowsHide:true});}
   catch(error){
    if(error.code===2&&error.stdout)output=error;
    else{fs.writeFileSync(path.join(root,`output/impeccable-${name}-error.txt`),String(error.stderr||error));throw error;}
   }
   const findings=JSON.parse(output.stdout);
   if(findings.some(f=>!f.advisory))process.exitCode=2;
   fs.writeFileSync(path.join(root,`output/impeccable-${name}.json`),JSON.stringify(findings,null,2)+'\n');
   results.push({viewport:`${width}x${height}`,findings});
   console.log(`${name}: ${findings.length} Impeccable findings`);
  }
  fs.writeFileSync(path.join(root,'output/impeccable-rendered.json'),JSON.stringify({method:'Official Impeccable detect --cdp, Playwright-managed Edge, mock device transport',results},null,2)+'\n');
 }finally{await browser.close();}
})().catch(e=>{console.error(e.message);process.exitCode=1;});
