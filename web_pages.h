#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <pgmspace.h>

// Single PROGMEM string for the index page. This was moved intact from Arc_Reactor_MK1_v4c9z5.ino
// Keep this as a single literal so Arduino IDE can handle it easily.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ARC REACTOR MK1</title>
<style>
  :root { color-scheme: dark; }
  html,body{ margin:0; padding:0; -webkit-text-size-adjust:100%; }
  body{ background:#000 !important; color:#fff !important; font-family:Arial,Helvetica,sans-serif; margin:10px; padding-bottom:80px; }
  h1{ margin:0 0 6px 0; font-size:24px; color:#fff !important; }
.build-badge{ display:inline-block; margin-left:8px; padding:2px 8px; border-radius:999px; font-size:16px; font-weight:600; border:1px solid rgba(0,255,255,0.35); background:rgba(0,255,255,0.08); color:#cfefff; vertical-align:middle; }
  .small{ font-size:0.9rem; color:#eee; }
  .statusbar{ display:flex; gap:10px; align-items:center; flex-wrap:wrap; }
  .dot{ width:10px; height:10px; border-radius:50%; background:#ccc; margin-left:auto; }
  .dot.dirty{ background:#ff9800; }
  .card{ border:1px solid #e0e0e0; border-radius:12px; padding:10px; margin:8px 0; background:#fff; color:#111; }
  .gridA{ display:grid; grid-template-columns:1fr 1fr; gap:10px; }
  label{ display:block; margin-top:6px; font-weight:600; color:#111; }
  .lblEffect{ font-size:1.3rem; }
  .lblSpeed, .lblIntensity{ font-size:1rem; }
  select{ width:100%; height:38px; font-size:1rem; padding:4px; }
  input[type=range]{ width:100%; }
  input[type=text]{ width:100%; height:36px; font-size:1rem; padding:6px; border:1px solid #aaa; border-radius:6px; }
  button{ padding:10px 12px; font-size:1rem; border:0; border-radius:6px; cursor:pointer; }
  .swatch{ position:relative; width:100%; height:38px; border:1px solid #aaa; border-radius:6px; display:flex; align-items:center; justify-content:center; font-weight:700; }
  .sticky{ position:fixed; left:0; right:0; bottom:0; background:#111; border-top:1px solid #333; padding:10px; }
  .primary{ width:100%; background:#0a84ff; color:#fff; }
  .modal{ position:fixed; inset:0; background:rgba(0,0,0,0.5); display:none; align-items:center; justify-content:center; z-index:9999; }
  .card2{ background:#fff; color:#111; border-radius:12px; padding:12px; width:min(380px,94vw); box-shadow:0 10px 24px rgba(0,0,0,.2); }
  .row{ display:flex; gap:10px; align-items:center; justify-content:space-between; }
  #wheel{ touch-action:none; display:block; margin:8px auto; }
  .flex{ display:flex; gap:8px; align-items:center; }
  .grow{ flex:1; }
  .favbar{ display:flex; gap:8px; margin-top:6px; flex-wrap:wrap; }
  .fav{ width:30px; height:30px; border:1px solid #888; border-radius:6px; }
  details summary{ cursor:pointer; list-style:none; font-weight:700; font-size:1.25rem; }
  details summary::-webkit-details-marker{ display:none; }
  .presetsGrid{ display:grid; grid-template-columns:repeat(4,1fr); gap:10px; }
  .presetBtn{ height:44px; border:2px solid #999; border-radius:8px; background:#fff; color:#111; font-weight:700; }
  .presetActive{ border-color:#00C8FF !important; box-shadow:0 0 0 2px rgba(0,200,255,0.25); }
  @keyframes flashBorder { 0%{box-shadow:0 0 0 0 rgba(0,0,0,0)} 30%{box-shadow:0 0 0 3px rgba(10,132,255,.8)} 100%{box-shadow:0 0 0 0 rgba(0,0,0,0)} }
  .flashOnce{ animation:flashBorder 450ms ease-out 1; }
  .cardNote{ font-size:0.7rem; color:#333; margin-top:6px; }
.app-title, #appTitle, h1#appTitle { font-size: 30px; }

.section-title { font-size: 24px; letter-spacing: 0.02em; }
.subhead { font-size: 12px; font-weight: 500; margin-left: 8px; opacity: 0.85; }

#messages { white-space: pre-wrap; }

.picker-wheel, .color-wheel, .wheel {
  background: conic-gradient(from 1deg,
    hsl(0 100% 50%) 0deg,
    hsl(60 100% 50%) 60deg,
    hsl(120 100% 50%) 120deg,
    hsl(180 100% 50%) 180deg,
    hsl(240 100% 50%) 240deg,
    hsl(250 100% 50%) 250deg,
    hsl(260 100% 50%) 260deg,
    hsl(270 100% 50%) 270deg,
    hsl(280 100% 50%) 280deg,
    hsl(290 100% 50%) 290deg,
    hsl(300 100% 50%) 300deg,
    hsl(360 100% 50%) 360deg
  );
  background: conic-gradient(in oklch from 1deg,
    oklch(0.7 0.27 0) 0deg,
    oklch(0.7 0.27 60) 60deg,
    oklch(0.7 0.27 120) 120deg,
    oklch(0.7 0.27 180) 180deg,
    oklch(0.7 0.27 240) 240deg,
    oklch(0.7 0.27 250) 250deg,
    oklch(0.7 0.27 260) 260deg,
    oklch(0.7 0.27 270) 270deg,
    oklch(0.7 0.27 280) 280deg,
    oklch(0.7 0.27 290) 290deg,
    oklch(0.7 0.27 300) 300deg,
    oklch(0.7 0.27 360) 360deg
  );
}

/* Master Brightness layout tuning */
#dfThreshText, #simVbatText { width:40px !important; text-align:center;  height:30px; height:24px;}

#wifiIdleLabel{ white-space:nowrap; font-size:0.95rem; }
</style>
</head>
<body>

<div class="statusbar">
  <h1 class="app-title">ARC REACTOR MK1</h1><span id="buildTag" class="build-badge">v4c9z0</span>
    <span class="small" id="messages"></span>
  <span id="dirty" class="dot"></span>
</div>

<div class="card">
  <div class="row" style="justify-content:space-between; align-items:center;"><label class="section-title" style="font-size:20px">MASTER BRIGHTNESS</label><span class="small" style="color:#000">Batt. <span id="batteryMB">--.--</span> V</span>
</div>
  <input type="range" id="master" min="0" max="255" value="128">
  <div class="row" style="margin-top:6px; align-items:center; gap:12px;">
  <label style="display:flex;align-items:center;gap:8px;margin:0; width:140px; flex:0 0 140px">
    <input type="checkbox" id="autoDF">
    <span>DyingFlicker trigger volts: <span id="dfThreshLabel" style="display:none"></span></span>
  </label>
  <input type="text" id="dfThreshText" value="3.60" style="width:40px; height:24px; text-align:center;">
  <input type="range" id="dfThresh" min="3.55" max="3.70" step="0.01" value="3.60" style="flex:1;">
</div>
  <div class="row" style="align-items:center; gap:12px;">
  <label style="display:flex;align-items:center;gap:8px;margin:0; width:140px; flex:0 0 140px">
    <input type="checkbox" id="simVbatEn">
    <span>Simulate lowV</span>
  </label>
  <input type="text" id="simVbatText" value="3.60" style="width:40px; height:24px; text-align:center;">
  <input type="range" id="simVbat" min="3.50" max="3.80" step="0.01" value="3.60" style="flex:1;">
</div>
  <div class="row" style="align-items:center; gap:12px;">
  <label style="display:flex;align-items:center;gap:8px;margin:0; width:140px; flex:0 0 140px">
    <input type="checkbox" id="wifiIdleAutoOff">
    <span id="wifiIdleLabel">Wi‑Fi idle timer ON</span>
  </label>
</div>
  <div class="cardNote" id="simNote" style="display:none"></div>
  <div class="cardNote" id="mbNote">
    PUSHBUTTON: (Short-tap→Restore Wi-Fi)<br>(Long-press→Sleep/Wake) (Double-tap→Next preset).
  </div>
</div>

<details class="card">
  <summary class="section-title">WiFi CONFIGURATION <span class="subhead">(Network Settings)</span></summary>
  <div style="margin-top:10px">
    <div class="small" style="margin-bottom:10px; color:#000">
      <strong>Status:</strong> <span id="wifiStatus">Loading...</span><br>
      <strong>IP Address:</strong> <span id="wifiIP">-</span><br>
      <strong id="hostnameLabel" style="display:none">Easy Access:</strong> <a id="wifiHostname" href="#" target="_blank" style="display:none; color:#0a84ff; font-weight:600">-</a>
    </div>
    
    <div style="display:flex; gap:8px; align-items:flex-end">
      <div style="flex:1">
        <label style="font-weight:600; color:#111; margin-top:10px">Home WiFi SSID</label>
        <select id="wifiSSID" style="width:100%; height:38px; font-size:1rem; padding:4px">
          <option value="">-- Select Network --</option>
        </select>
      </div>
      <button id="scanNetworksBtn" style="padding:10px 12px; height:38px; background:#9C27B0; color:#fff; border:0; border-radius:6px; cursor:pointer; white-space:nowrap">Scan Networks</button>
    </div>
    
    <label style="font-weight:600; color:#111; margin-top:10px">Home WiFi Password</label>
    <input type="text" id="wifiPassword" placeholder="Enter WiFi password" maxlength="64">
    
    <label style="font-weight:600; color:#111; margin-top:10px">Access Point SSID</label>
    <input type="text" id="apSSID" placeholder="ArcReactorMK1" maxlength="32">
    
    <label style="font-weight:600; color:#111; margin-top:10px">Access Point Password</label>
    <input type="text" id="apPassword" placeholder="Minimum 8 characters" maxlength="64">
    
    <div class="gridA" style="margin-top:10px;">
      <button id="wifiSaveBtn" style="background:#4CAF50; color:#fff">Save WiFi Settings</button>
      <button id="wifiReconnectBtn" style="background:#0a84ff; color:#fff">Reconnect Now</button>
    </div>
    
    <div class="cardNote" style="margin-top:10px">
      <strong>Easy Access:</strong><br>
      • <strong>AP Mode:</strong> Browser opens automatically at http://192.168.4.1<br>
      • <strong>Home WiFi:</strong> Access at <strong>http://arc.local</strong> or use IP address shown above<br>
      <br>
      <small><em>Note: Some Android browsers may not support .local addresses. If http://arc.local doesn't work, use the IP address instead.</em></small><br>
      <br>
      On boot, the device attempts to connect to home WiFi for 1 minute. If unsuccessful, it creates an Access Point with the configured SSID/password. Changes to AP settings require a restart.
    </div>
  </div>
</details>

<details class="card">
  <summary class="section-title">PLASMA HALO <span class="subhead">(Outer Ring)</span></summary>
  <div class="gridA">
    <div>
      <label class="lblEffect">Effect A</label><select id="z0a"></select>
      <label class="lblSpeed">Speed</label><input type="range" id="z0speedA" min="10" max="1000" value="300">
      <label class="lblIntensity">Intensity</label><input type="range" id="z0intA" min="0" max="255" value="255">
      <button class="swatch" id="z0a_color" data-value="#FFA500" style="background:#FFA500;color:#000">#FFA500</button>
    </div>
    <div>
      <label class="lblEffect">Effect B</label><select id="z0b"></select>
      <label class="lblSpeed">Speed</label><input type="range" id="z0speedB" min="10" max="1000" value="300">
      <label class="lblIntensity">Intensity</label><input type="range" id="z0intB" min="0" max="255" value="255">
      <button class="swatch" id="z0b_color" data-value="#0000FF" style="background:#0000FF;color:#fff">#0000FF</button>
    </div>
    <div class="cardNote" style="grid-column:1/-1">
      BLEND:<br>B overlays A where B ≠ OFF. If A+B are OFF → zone is dark.
    </div>
  </div>
</details>

<details class="card">
  <summary class="section-title">FLUX MATRIX <span class="subhead">(Inner Ring)</span></summary>
  <div class="gridA">
    <div>
      <label class="lblEffect">Effect A</label><select id="z1a"></select>
      <label class="lblSpeed">Speed</label><input type="range" id="z1speedA" min="10" max="1000" value="400">
      <label class="lblIntensity">Intensity</label><input type="range" id="z1intA" min="0" max="255" value="255">
      <button class="swatch" id="z1a_color" data-value="#0000FF" style="background:#0000FF;color:#fff">#0000FF</button>
    </div>
    <div>
      <label class="lblEffect">Effect B</label><select id="z1b"></select>
      <label class="lblSpeed">Speed</label><input type="range" id="z1speedB" min="10" max="1000" value="400">
      <label class="lblIntensity">Intensity</label><input type="range" id="z1intB" min="0" max="255" value="255">
      <button class="swatch" id="z1b_color" data-value="#00FFFF" style="background:#00FFFF;color:#000">#00FFFF</button>
    </div>
    <div class="cardNote" style="grid-column:1/-1">
      BLEND:<br>B overlays A where B ≠ OFF. If A+B are OFF → zone is dark.
    </div>
  </div>
</details>

<details class="card">
  <summary class="section-title">REPULSOR NEXUS <span class="subhead">(Center Core)</span></summary>
  <div class="gridA">
    <div>
      <label class="lblEffect">Effect A</label><select id="z2a"></select>
      <label class="lblSpeed">Speed</label><input type="range" id="z2speedA" min="10" max="1000" value="350">
      <label class="lblIntensity">Intensity</label><input type="range" id="z2intA" min="0" max="255" value="255">
      <button class="swatch" id="z2a_color" data-value="#FFFFFF" style="background:#FFFFFF;color:#000">#FFFFFF</button>
    </div>
    <div>
      <label class="lblEffect">Effect B</label><select id="z2b"></select>
      <label class="lblSpeed">Speed</label><input type="range" id="z2speedB" min="10" max="1000" value="350">
      <label class="lblIntensity">Intensity</label><input type="range" id="z2intB" min="0" max="255" value="255">
      <button class="swatch" id="z2b_color" data-value="#000000" style="background:#000000;color:#fff">#000000</button>
    </div>
    <div class="cardNote" style="grid-column:1/-1">
      BLEND:<br>B overlays A where B ≠ OFF. If A+B are OFF → zone is dark.
    </div>
  </div>
</details>

<div class="card">
  <label class="section-title" style="font-size:20px">PRESETS <span class="subhead">(Short-tap→Apply)  (Long-press→Save)</span></label>
  <div class="presetsGrid" id="presetsGrid"></div>
</div>

<div class="sticky"><button class="primary" id="saveBtn">Save Settings</button></div>

<!-- Color Picker Modal -->
<div class="modal" id="pickerModal">
  <div class="card2">
    <div class="row"><b>Pick color</b><span id="preview" style="width:28px;height:28px;border-radius:6px;border:1px solid #aaa;display:inline-block"></span></div>
    <canvas id="wheel" width="240" height="240"></canvas>
    <div class="flex"><span>Brightness</span><input class="grow" type="range" id="val" min="0" max="100" value="100"></div>
    <div class="flex" style="margin-top:8px"><span>Hex</span><input id="hex" type="text" value="#FFFFFF" maxlength="7" placeholder="#RRGGBB"></div>
    <div class="small" style="margin-top:8px">Favorites</div>
    <div class="favbar" id="favRow"></div>
    <div class="gridA" style="grid-template-columns:1fr 1fr; margin-top:10px">
      <button id="cancel">Cancel</button>
      <button id="ok" style="background:#0a84ff;color:#fff">OK</button>
    </div>
  </div>
</div>

<script>
let lastActivePreset=-1;

function hexToRgb(h){ h=h.replace('#',''); if(h.length===3){h=h[0]+h[0]+h[1]+h[1]+h[2]+h[2];}
  let n=parseInt(h,16); return {r:(n>>16)&255,g:(n>>8)&255,b:n&255};}
function rgbToHex(r,g,b){const s=n=>('0'+n.toString(16)).slice(-2);return '#'+s(r)+s(g)+s(b);}
function rgbToHsv(r,g,b){r/=255;g/=255;b/=255;let max=Math.max(r,g,b),min=Math.min(r,g,b),h,s,v=max,d=max-min;
  s=max==0?0:d/max;if(d==0)h=0;else{switch(max){case r:h=(g-b)/d+(g<b?6:0);break;case g:h=(b-r)/d+2;break;case b:h=(r-g)/d+4;break;}h/=6;}
  return {h:h*360,s:s,v:v};}
function hsvToRgb(h,s,v){h/=60;let c=v*s,x=c*(1-Math.abs(h%2-1)),m=v-c;let r=0,g=0,b=0;
  if(0<=h&&h<1){r=c;g=x;}else if(1<=h&&h<2){r=x;g=c;}else if(2<=h&&h<3){g=c;b=x;}
  else if(3<=h&&h<4){g=x;b=c;}else if(4<=h&&h<5){r=x;b=c;}else{r=c;b=x;}
  return {r:Math.round((r+m)*255),g:Math.round((g+m)*255),b:Math.round((b+m)*255)};}

function optHtml(sel){const opts=['Off','On','ArcFlicker','Aurora','Breathe','HaloBreath','Lightning','Plasma','PowerPulse'];
  return opts.map(o=>'<option'+(o===sel?' selected':'')+'>'+o+'</option>').join('');}

function clampHex(txt){let t=txt.toUpperCase();if(!t.startsWith('#'))t='#'+t; if(t.length>7)t=t.slice(0,7); return t;}
function contrastText(hex){ const {r,g,b}=hexToRgb(hex); const yiq=((r*299)+(g*587)+(b*114))/1000; return yiq>=128?'#000':'#fff'; }
function vibrate(ms){ if(navigator.vibrate) navigator.vibrate(ms); }

function collect(){
  let s={ master:parseInt(document.getElementById('master').value),
          autoDF:document.getElementById('autoDF').checked,
          dfThresholdV:parseFloat(document.getElementById('dfThresh').value),
          simVbatEnabled:document.getElementById('simVbatEn').checked,
          simVbat:parseFloat(document.getElementById('simVbat').value),
          wifiIdleAutoOff:document.getElementById('wifiIdleAutoOff').checked,
          zones:[] };
  for(let z=0;z<3;z++){
    s.zones.push({
      effectA:document.getElementById('z'+z+'a').value,
      effectB:document.getElementById('z'+z+'b').value,
      colorA:document.getElementById('z'+z+'a_color').dataset.value,
      colorB:document.getElementById('z'+z+'b_color').dataset.value,
      speedA:parseInt(document.getElementById('z'+z+'speedA').value),
      speedB:parseInt(document.getElementById('z'+z+'speedB').value),
      intensityA:parseInt(document.getElementById('z'+z+'intA').value),
      intensityB:parseInt(document.getElementById('z'+z+'intB').value)
    });
  }
  return s;
}
let dirty=false; function markDirty(x){dirty=x;document.getElementById('dirty').className='dot'+(dirty?' dirty':'');}
let liveTimer=0, applyGuardUntil=0;
function live(){ fetch('/update',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(collect())}); markDirty(true); }
function clearLiveThrottle(){ clearTimeout(liveTimer); }
function liveThrottle(){ if(Date.now()<applyGuardUntil)return; clearTimeout(liveTimer); liveTimer=setTimeout(()=>live(),60); }

let pickerSendTimer=0, lastPickerSend=0;
function nowMs(){ return (typeof performance!=='undefined'&&performance.now)?performance.now():Date.now(); }
function livePicker(){
  const n=nowMs(); const dt=n-lastPickerSend;
  if (dt>=30){ if(pickerSendTimer){clearTimeout(pickerSendTimer);pickerSendTimer=0;} lastPickerSend=n; live(); }
  else if(!pickerSendTimer){ pickerSendTimer=setTimeout(()=>{ lastPickerSend=nowMs(); pickerSendTimer=0; live(); }, 30-dt); }
}

function doSave(){ fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(collect())})
  .then(r=>r.text()).then(t=>{ alert(t); markDirty(false); }); }

let modal,canvas,ctx,preview,valSlider,hexInput,currentTarget=null,h=0,s=1,v=1,center={x:120,y:120},radius=110;
let favs=['#FF6A00','#00C8FF','#FFFFFF','#00FFAA','#FF00FF','#FFA500','#00FF00','#0000FF','#FFFF00'];
let pickerPrevColor=null, isPickerOpen=false;
let wheelImg=null;

function setFromHex(hex){
  hex=clampHex(hex);
  let rgb=hexToRgb(hex), hsv=rgbToHsv(rgb.r,rgb.g,rgb.b);
  h=hsv.h; s=hsv.s; v=hsv.v;
  if(hexInput) hexInput.value=hex;
  drawWheel(); updatePreview();
}
function hexFromHSV(){let rgb=hsvToRgb(h,s,v);return rgbToHex(rgb.r,rgb.g,rgb.b).toUpperCase();}

function openPicker(targetId){
  currentTarget=document.getElementById(targetId);
  setFromHex(currentTarget.dataset.value);
  pickerPrevColor=currentTarget.dataset.value;
  isPickerOpen=true; modal.style.display='flex';
}
function closePicker(){ modal.style.display='none'; currentTarget=null; isPickerOpen=false; }

function updatePreview(){let rgb=hsvToRgb(h,s,v); preview.style.background=rgbToHex(rgb.r,rgb.g,rgb.b);}

function buildWheel(){
  if(!ctx) return;
  let img=ctx.createImageData(canvas.width,canvas.height);
  for(let y=0;y<canvas.height;y++){
    for(let x=0;x<canvas.width;x++){
      let dx = x - center.x, dy = y - center.y;
      let r=Math.sqrt(dx*dx+dy*dy);
      if(r>radius||r<10) continue;
      let angle=Math.atan2(dy,dx);let deg=(angle*180/Math.PI+360)%360;
      let sat=Math.min(1,Math.max(0,r/radius)); let rgb=hsvToRgb(deg,sat,1);
      let idx=(y*canvas.width+x)*4; img.data[idx]=rgb.r; img.data[idx+1]=rgb.g; img.data[idx+2]=rgb.b; img.data[idx+3]=255;
    }
  }
  wheelImg=img;
}
function drawWheel(){
  if(!wheelImg) buildWheel();
  ctx.putImageData(wheelImg,0,0);
  ctx.beginPath();ctx.arc(center.x,center.y,10,0,Math.PI*2);ctx.fillStyle='#fff';ctx.fill();
  let mx=center.x+Math.cos(h*Math.PI/180)*s*radius, my=center.y+Math.sin(h*Math.PI/180)*s*radius;
  ctx.beginPath();ctx.arc(mx,my,6,0,Math.PI*2);ctx.fillStyle='#000';ctx.fill();
  ctx.beginPath();ctx.arc(mx,my,4,0,Math.PI*2);ctx.fillStyle='#fff';
}

let pickPending=false;
function pickAt(ev){
  if(v===0){ v=1; if(valSlider) valSlider.value=100; }
  const rect=canvas.getBoundingClientRect();
  const x=(ev.touches?ev.touches[0].clientX:ev.clientX)-rect.left;
  const y=(ev.touches?ev.touches[0].clientY:ev.clientY)-rect.top;
  let dx=x-center.x,dy=y-center.y;let r=Math.sqrt(dx*dx+dy*dy); if(r>radius) r=radius;
  let ang=Math.atan2(dy,dx); h=(ang*180/Math.PI+360)%360; s=Math.min(1,Math.max(0,r/radius));

  if(!pickPending){ pickPending=true; requestAnimationFrame(()=>{ drawWheel(); pickPending=false; }); }
  updatePreview();
  if(hexInput) hexInput.value=hexFromHSV();
  if(currentTarget){
    let hex=hexFromHSV();
    if (hex!==currentTarget.dataset.value){
      currentTarget.dataset.value=hex;
      currentTarget.style.background=hex;
      currentTarget.style.color=contrastText(hex);
      livePicker();
    }
  }
}

function renderFavRow(){
  const row=document.getElementById('favRow'); if(!row) return;
  row.innerHTML='';
  for(let i=0;i<9;i++){
    const b=document.createElement('button');
    b.className='fav'; b.style.background=favs[i];
    let pressTimer=null,long=false,vibrateTimer=null;
    const clear=()=>{ 
      if(pressTimer){ clearTimeout(pressTimer); pressTimer=null; }
      if(vibrateTimer){ clearTimeout(vibrateTimer); vibrateTimer=null; }
    };
    const onDown=(e)=>{ e.preventDefault(); long=false; clear();
      // Call vibrate synchronously in gesture handler - required for Firefox
      if(navigator.vibrate){ try{ navigator.vibrate(1); }catch(err){} }
      vibrateTimer=setTimeout(()=>{ if(navigator.vibrate){ try{ navigator.vibrate(50); }catch(err){} } },1000);
      pressTimer=setTimeout(()=>{ long=true;
        const hex=hexFromHSV();
        fetch('/setFav',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx:i,hex})})
          .then(()=>{ favs[i]=hex; b.style.background=hex; flashOnce(b); });
      },1000);
    };
    const onUp=()=>{ if(!pressTimer) return; clear(); if(!long){
      applyGuardUntil=Date.now()+120; clearLiveThrottle(); setFromHex(favs[i]);
      if(currentTarget){
        currentTarget.dataset.value=favs[i];
        currentTarget.style.background=favs[i];
        currentTarget.style.color=contrastText(favs[i]);
      }
      clearLiveThrottle(); live();
    }};
    b.addEventListener('mousedown',onDown); b.addEventListener('touchstart',onDown,{passive:false});
    b.addEventListener('mouseup',onUp); b.addEventListener('mouseleave',clear);
    b.addEventListener('touchend',onUp); b.addEventListener('touchcancel',clear);
    row.appendChild(b);
  }
}

function setSwatchFace(id, hex){
  const el=document.getElementById(id);
  el.dataset.value=hex; el.style.background=hex; el.style.color=contrastText(hex); el.textContent=hex;
}
function bindSwatches(){ for(let z=0;z<3;z++){ ['a','b'].forEach(k=>{
  let id='z'+z+k+'_color'; let el=document.getElementById(id);
  el.addEventListener('click',()=>openPicker(id));
});}}

function hydrateEffects(j){
  const z = j.zones;
  const ids = [['z0a','z0b'],['z1a','z1b'],['z2a','z2b']];
  for(let i=0;i<3;i++){
    document.getElementById(ids[i][0]).innerHTML = optHtml(z[i].effectA);
    document.getElementById(ids[i][1]).innerHTML = optHtml(z[i].effectB);
    document.getElementById('z'+i+'speedA').value = z[i].speedA;
    document.getElementById('z'+i+'speedB').value = z[i].speedB;
    document.getElementById('z'+i+'intA').value = z[i].intensityA;
    document.getElementById('z'+i+'intB').value = z[i].intensityB;
    setSwatchFace('z'+i+'a_color', z[i].colorA);
    setSwatchFace('z'+i+'b_color', z[i].colorB);
  }
  document.getElementById('master').value = j.master;
  document.getElementById('autoDF').checked = !!j.autoDF;
  if (j.dfThresholdV){ document.getElementById('dfThresh').value=j.dfThresholdV.toFixed(2); document.getElementById('dfThreshText').value=j.dfThresholdV.toFixed(2); document.getElementById('dfThreshLabel').innerText=j.dfThresholdV.toFixed(2); }
  document.getElementById('simVbatEn').checked=!!j.simVbatEnabled;
  if (j.simVbat){ document.getElementById('simVbat').value=j.simVbat.toFixed(2); document.getElementById('simVbatText').value=j.simVbat.toFixed(2); }
  document.getElementById('simNote').style.display = (document.getElementById('simVbatEn').checked?'block':'none');
    if ('wifiIdleAutoOff' in j) { try{ document.getElementById('wifiIdleAutoOff').checked = !!j.wifiIdleAutoOff; updateWifiIdleLabel(); }catch(e){} }
setActivePresetButton(j.activePreset);
  if (Array.isArray(j.favs) && j.favs.length === 9) { favs = j.favs.slice(0,9); renderFavRow(); }
}

function attachPicker(){
  modal=document.getElementById('pickerModal'); canvas=document.getElementById('wheel'); ctx=canvas.getContext('2d');
  preview=document.getElementById('preview'); valSlider=document.getElementById('val'); hexInput=document.getElementById('hex');
  buildWheel(); drawWheel();

  canvas.addEventListener('mousedown',e=>{pickAt(e);canvas.onmousemove=pickAt;});
  document.addEventListener('mouseup',()=>{canvas.onmousemove=null;});
  canvas.addEventListener('touchstart',pickAt,{passive:false});
  canvas.addEventListener('touchmove',pickAt,{passive:false});

  valSlider.addEventListener('input',()=>{ v=parseInt(valSlider.value)/100; updatePreview();
    if(hexInput) hexInput.value=hexFromHSV(); if(currentTarget){ let hex=hexFromHSV(); setSwatchFace(currentTarget.id, hex); livePicker(); }});
  hexInput.addEventListener('change',()=>{ let t=clampHex(hexInput.value);
    if(/^#([0-9A-Fa-f]{6})$/.test(t)){ setFromHex(t);
      if(currentTarget){ setSwatchFace(currentTarget.id, t); livePicker(); } }
    else { hexInput.value=hexFromHSV(); }});
  document.getElementById('cancel').onclick=()=>{ 
    if(currentTarget){ clearLiveThrottle(); setSwatchFace(currentTarget.id, pickerPrevColor); live(); } 
    closePicker(); 
  };
  document.getElementById('ok').onclick=()=>{ if(!currentTarget) return; const hex=hexFromHSV(); setSwatchFace(currentTarget.id, hex); closePicker(); liveThrottle(); };
  renderFavRow();
}

// ----- Presets (8) -----
let presetMeta = {active:-1, items:new Array(8).fill(null)};
function fetchPresets(){ fetch('/presets').then(r=>r.json()).then(j=>{
  presetMeta = j; renderPresets(); setActivePresetButton(j.active);
});}
function presetButtonFaceColor(idx){ const item = presetMeta.items[idx]; return (item && item.color) ? item.color : '#EEEEEE'; }
function renderPresets(){
  const grid=document.getElementById('presetsGrid'); if(!grid) return;
  grid.innerHTML='';
  for(let i=0;i<8;i++){
    const btn=document.createElement('button');
    btn.className='presetBtn';
    const face = presetButtonFaceColor(i);
    btn.style.background = face; btn.style.color = (face==='#EEEEEE')?'#111':(contrastText(face));
    btn.textContent = (i+1).toString();
    if (presetMeta.active === i) btn.classList.add('presetActive');
    let t=null, long=false, vibrateTimer=null;
    const clear=()=>{ 
      if(t){ clearTimeout(t); t=null; }
      if(vibrateTimer){ clearTimeout(vibrateTimer); vibrateTimer=null; }
    };
    const onDown=(e)=>{ e.preventDefault(); long=false; clear();
      // Call vibrate synchronously in gesture handler - required for Firefox
      if(navigator.vibrate){ try{ navigator.vibrate(1); }catch(err){} }
      vibrateTimer=setTimeout(()=>{ if(navigator.vibrate){ try{ navigator.vibrate(50); }catch(err){} } },1000);
      t=setTimeout(()=>{ long=true; doPresetSave(i, btn); }, 1000);
    };
    const onUp=()=>{ if(!t) return; clear(); if(!long) doPresetApply(i, btn); };
    btn.addEventListener('mousedown',onDown);
    btn.addEventListener('touchstart',onDown,{passive:false});
    btn.addEventListener('mouseup',onUp);
    btn.addEventListener('mouseleave',clear);
    btn.addEventListener('touchend',onUp);
    btn.addEventListener('touchcancel',clear);
    grid.appendChild(btn);
  }
}
function flashOnce(el){ el.classList.remove('flashOnce'); void el.offsetWidth; el.classList.add('flashOnce'); }
function setActivePresetButton(idx){
  const grid=document.getElementById('presetsGrid'); if(!grid) return;
  [...grid.children].forEach((b,bi)=>{ if (bi===idx) b.classList.add('presetActive'); else b.classList.remove('presetActive'); });
}
function doPresetSave(idx, btn){
  const payload = collect();
  fetch('/presetSave',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx, state:payload})})
    .then(r=>r.json()).then(j=>{
      const face = j.faceColor || payload.zones[0].colorA || '#EEEEEE';
      btn.style.background = face; btn.style.color = (face==='#EEEEEE')?'#111':contrastText(face);
      presetMeta.active = (j && typeof j.activePreset==="number") ? j.activePreset : idx; setActivePresetButton(presetMeta.active); flashOnce(btn);
    });
}
function doPresetApply(idx, btn){
  fetch('/presetApply',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx})})
    .then(r=>r.json()).then(j=>{
      hydrateEffects(j);
  
  if (typeof j.activePreset==="number"){ setActivePresetButton(j.activePreset); lastActivePreset=j.activePreset; }if (typeof j.activePreset==="number") { setActivePresetButton(j.activePreset); lastActivePreset=j.activePreset; }
      presetMeta.active = j.activePreset ?? idx;
      setActivePresetButton(presetMeta.active);
      flashOnce(btn);
      fetch('/update',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(collect())});
    });
}

document.addEventListener('input',e=>{
  if(e.target&&(e.target.tagName==='SELECT'||e.target.type==='range'||e.target.type==='checkbox')) liveThrottle();
});
document.addEventListener('change',e=>{
  if(e.target&&(e.target.tagName==='SELECT'||e.target.type==='range'||e.target.type==='checkbox')) markDirty(true);
});
document.getElementById('saveBtn').addEventListener('click',doSave);


// ---- DF threshold + simulation bindings ----
(function(){
  function clamp(val,min,max){ return Math.min(max,Math.max(min,val)); }
  const th = document.getElementById('dfThresh');
  const thT= document.getElementById('dfThreshText');
  const thL= document.getElementById('dfThreshLabel');
  const se = document.getElementById('simVbatEn');
  const sv = document.getElementById('simVbat');
  const svT= document.getElementById('simVbatText');
  const sn = document.getElementById('simNote');

  function syncThresh(fromText){
    let v = fromText ? parseFloat(thT.value) : parseFloat(th.value);
    if (isNaN(v)) v = 3.60;
    v = clamp(v, 3.55, 3.70);
    th.value = v.toFixed(2);
    thT.value = v.toFixed(2);
    thL.innerText = v.toFixed(2);
    liveThrottle();
  }
  function syncSim(fromText){
    let v = fromText ? parseFloat(svT.value) : parseFloat(sv.value);
    if (isNaN(v)) v = 3.60;
    v = clamp(v, 3.50, 4.20);
    sv.value = v.toFixed(2);
    svT.value = v.toFixed(2);
    liveThrottle();
  }
  th.addEventListener('input', ()=>syncThresh(false));
  thT.addEventListener('change', ()=>syncThresh(true));
  sv.addEventListener('input', ()=>syncSim(false));
  svT.addEventListener('change', ()=>syncSim(true));
  se.addEventListener('change', ()=>{ sn.style.display = se.checked ? 'block' : 'none'; liveThrottle(); });
})();


function updateWifiIdleLabel(){
  try{
    const cb = document.getElementById('wifiIdleAutoOff');
    const lab = document.getElementById('wifiIdleLabel');
    if (!cb || !lab) return;
    lab.textContent = cb.checked ? 'Wi‑Fi idle timer ON' : 'Wi‑Fi idle timer OFF';
  }catch(e){}
}
// Bind label updater
(function(){
  try{
    const cb = document.getElementById('wifiIdleAutoOff');
    if (cb){ cb.addEventListener('change', ()=>{ updateWifiIdleLabel(); }); }
  }catch(e){}
})();

// WiFi configuration handlers
(function(){
  const WIFI_RECONNECT_RELOAD_DELAY_MS = 3000;  // Wait 3 seconds before reload after reconnect
  
  function updateWiFiStatus(j) {
    try {
      const statusEl = document.getElementById('wifiStatus');
      const ipEl = document.getElementById('wifiIP');
      const hostnameEl = document.getElementById('wifiHostname');
      const hostnameLabelEl = document.getElementById('hostnameLabel');
      if (!statusEl || !ipEl) return;
      
      if (j.wifiStationMode) {
        if (j.wifiConnected) {
          statusEl.textContent = 'Connected to ' + (j.wifiSSID || 'home WiFi');
          statusEl.style.color = '#4CAF50';
        } else {
          statusEl.textContent = 'Station mode (disconnected)';
          statusEl.style.color = '#FF9800';
        }
      } else {
        statusEl.textContent = 'Access Point mode: ' + (j.apSSID || 'ArcReactorMK1');
        statusEl.style.color = '#2196F3';
      }
      
      ipEl.textContent = j.wifiIP || '-';
      
      // Show hostname if available (station mode)
      if (j.wifiHostname && j.wifiConnected) {
        if (hostnameEl && hostnameLabelEl) {
          hostnameEl.href = 'http://' + j.wifiHostname;
          hostnameEl.textContent = j.wifiHostname;
          hostnameEl.style.display = 'inline';
          hostnameLabelEl.style.display = 'inline';
        }
      } else {
        if (hostnameEl && hostnameLabelEl) {
          hostnameEl.style.display = 'none';
          hostnameLabelEl.style.display = 'none';
        }
      }
      
      // Populate WiFi fields
      if (j.wifiSSID !== undefined) {
        const dropdown = document.getElementById('wifiSSID');
        if (dropdown) {
          // Check if current SSID is already in dropdown
          let found = false;
          for (let i = 0; i < dropdown.options.length; i++) {
            if (dropdown.options[i].value === j.wifiSSID) {
              dropdown.selectedIndex = i;
              found = true;
              break;
            }
          }
          // If not found, add it as an option and select it
          if (!found && j.wifiSSID) {
            const opt = document.createElement('option');
            opt.value = j.wifiSSID;
            opt.textContent = j.wifiSSID + ' (connected)';
            opt.selected = true;
            dropdown.insertBefore(opt, dropdown.options[1]); // Insert after "-- Select Network --"
          }
        }
      }
      if (j.apSSID !== undefined) {
        const apSsidEl = document.getElementById('apSSID');
        if (apSsidEl && !apSsidEl.value) apSsidEl.value = j.apSSID;
      }
    } catch(e) {}
  }
  
  // Network scanning functionality
  const scanBtn = document.getElementById('scanNetworksBtn');
  const dropdown = document.getElementById('wifiSSID');
  
  if (scanBtn && dropdown) {
    scanBtn.addEventListener('click', ()=>{
      scanBtn.disabled = true;
      scanBtn.textContent = 'Scanning...';
      
      // Save current selection
      const currentValue = dropdown.value;
      
      fetch('/wifiScan')
        .then(r=>r.json())
        .then(data=>{
          dropdown.innerHTML = '<option value="">-- Select Network --</option>';
          
          // If there was a current value, add it first
          if (currentValue) {
            const opt = document.createElement('option');
            opt.value = currentValue;
            opt.textContent = currentValue + ' (current)';
            dropdown.appendChild(opt);
          }
          
          if (data.networks && data.networks.length > 0) {
            data.networks.forEach(net=>{
              // Don't add duplicate of current network
              if (net.ssid !== currentValue) {
                const opt = document.createElement('option');
                opt.value = net.ssid;
                const lockIcon = net.secure ? '🔒 ' : '';
                opt.textContent = `${lockIcon}${net.ssid} (${net.rssi} dBm)`;
                dropdown.appendChild(opt);
              }
            });
          } else if (!currentValue) {
            const opt = document.createElement('option');
            opt.value = '';
            opt.textContent = '-- No networks found --';
            dropdown.appendChild(opt);
          }
          
          // Restore selection
          if (currentValue) {
            dropdown.value = currentValue;
          }
          
          scanBtn.disabled = false;
          scanBtn.textContent = 'Scan Networks';
        })
        .catch(err=>{
          alert('Error scanning networks: ' + err);
          scanBtn.disabled = false;
          scanBtn.textContent = 'Scan Networks';
        });
    });
  }
  
  const saveBtn = document.getElementById('wifiSaveBtn');
  if (saveBtn) {
    saveBtn.addEventListener('click', ()=>{
      const ssid = document.getElementById('wifiSSID').value;
      const pass = document.getElementById('wifiPassword').value;
      const apSsid = document.getElementById('apSSID').value;
      const apPass = document.getElementById('apPassword').value;
      
      if (apPass && apPass.length > 0 && apPass.length < 8) {
        alert('AP password must be at least 8 characters or empty');
        return;
      }
      
      const data = {
        wifiSSID: ssid,
        wifiPassword: pass,
        apSSID: apSsid,
        apPassword: apPass
      };
      
      fetch('/wifiSave', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(data)})
        .then(r=>r.text())
        .then(msg=>{
          alert(msg);
        })
        .catch(err=>{
          alert('Error saving WiFi settings: ' + err);
        });
    });
  }
  
  const reconnectBtn = document.getElementById('wifiReconnectBtn');
  if (reconnectBtn) {
    reconnectBtn.addEventListener('click', ()=>{
      if (!confirm('Reconnect WiFi now? This may temporarily disconnect you.')) return;
      
      fetch('/wifiReconnect', {method:'POST'})
        .then(r=>r.text())
        .then(msg=>{
          alert(msg + ' Please wait a moment and check status.');
          setTimeout(()=>{ window.location.reload(); }, WIFI_RECONNECT_RELOAD_DELAY_MS);
        })
        .catch(err=>{
          alert('Error reconnecting: ' + err);
        });
    });
  }
  
  // Update WiFi status on initial load and periodic updates
  window.updateWiFiStatusFromJSON = updateWiFiStatus;
})();

function safeInit(){ try{ attachPicker(); bindSwatches(); fetchPresets(); }catch(e){} }
document.addEventListener('DOMContentLoaded', safeInit);

window.addEventListener('load', safeInit);


fetch('/status').then(r=>r.json()).then(j=>{
  try{var el=document.getElementById("buildTag"); if(el && j.buildTag) el.textContent=j.buildTag;}catch(e){}

  document.getElementById('batteryMB').innerText=(j.battery||0).toFixed(2);
  document.getElementById('messages').innerText=j.message||'';
  hydrateEffects(j);
  if (typeof updateWiFiStatusFromJSON === 'function') updateWiFiStatusFromJSON(j);
});
try{ if(typeof j!=='undefined' && j && typeof j.activePreset==='number'){ lastActivePreset=j.activePreset; } }catch(e){}
setInterval(()=>{
  fetch('/status').then(r=>r.json()).then(j=>{
    document.getElementById('batteryMB').innerText=(j.battery||0).toFixed(2);
    document.getElementById('messages').innerText=j.message||'';
    if (typeof j.activePreset === 'number') setActivePresetButton(j.activePreset);
    if (typeof updateWiFiStatusFromJSON === 'function') updateWiFiStatusFromJSON(j);
  });
}, 1000);
</script>

<script>
// === UI auto-sync (epoch-gated): update only after completed state changes ===
(function(){
  var lastEpoch = -1;
  var _busy = false;
  setInterval(function(){
    if (_busy) return;
    if (document.visibilityState && document.visibilityState !== 'visible') return;
    _busy = true;
    fetch('/status').then(function(r){ return r.json(); }).then(function(j){
      if (typeof j.epoch === 'number' && j.epoch !== lastEpoch){
        lastEpoch = j.epoch;
        try {
          if (typeof hydrateEffects === 'function') hydrateEffects(j);
          if (typeof j.activePreset === 'number' && typeof setActivePresetButton === 'function') {
            setActivePresetButton(j.activePreset);
          }
          var el = document.getElementById('buildTag');
          if (el && j.buildTag) el.textContent = j.buildTag;
          var b = document.getElementById('batteryMB');
          if (b && typeof j.battery === 'number') b.textContent = j.battery.toFixed(2);
          var m = document.getElementById('messages');
          if (m && typeof j.message === 'string') m.textContent = j.message;
        } catch(e){}
      }
    }).catch(function(){ /* no-op */ }).finally(function(){ _busy = false; });
  }, 300);
})();
</script>
</body>
</html>)HTML";

#endif // WEB_PAGES_H