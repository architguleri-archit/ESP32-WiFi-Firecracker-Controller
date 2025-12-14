#include <WiFi.h>
#include <WebServer.h>

// ==== Relay Pins ====
#define RELAY1 25
#define RELAY2 26
#define RELAY3 27
#define RELAY4 14
#define ARM_LED 2  // Green LED

WebServer server(80);

// ==== Wi-Fi AP ====
const char* ssid = "BoltBunny Firing Panel"; 
const char* password = "boltbunny";

// ==== System State ====
volatile bool armed = false;
volatile bool fastBlink = false;

// ==== Relay Control ====
void relayControl(int relay, bool state) {
  digitalWrite(relay, state ? LOW : HIGH); // Active LOW
  Serial.print("Relay "); Serial.print(relay);
  Serial.println(state ? " ON" : " OFF");
}

// ==== HTML Page ====
void handleRoot() {
  String html = R"rawliteral(
<!doctype html>
<html>
<head>
<title>BoltBunny Firing Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {
  font-family: Arial; 
  margin:0; padding:18px; 
  color:#111; 
  display:flex; flex-direction:column; align-items:center;
  background: #f5f5f5;
  background: linear-gradient(135deg, #f9f9f9 25%, #e9e9e9 25%, #e9e9e9 50%, #f9f9f9 50%, #f9f9f9 75%, #e9e9e9 75%, #e9e9e9);
  background-size: 40px 40px;
}

h1 { 
  font-size:32px; 
  margin-bottom:25px; 
  text-align:center; 
  text-shadow: 1px 1px 3px rgba(0,0,0,0.25);
  background: linear-gradient(90deg, #00BFFF, #1E90FF);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.logo { width:100px; height:100px; margin-bottom:15px;}

.top-row {
  width:90%;
  display:flex; 
  justify-content:space-between; /* LED left, POWER button right */
  align-items:center; 
  margin-bottom:25px;
}

.arm-indicator { 
  width:18px; height:18px; border-radius:50%; 
  background:red; 
  margin-right:10px;
  transition: background 0.2s, box-shadow 0.2s; 
  box-shadow: 0 0 5px rgba(0,0,0,0.3);
}

.arm-button { 
  padding:14px 28px; 
  border:1px solid #d3d3d3; 
  border-radius:30px; 
  background: linear-gradient(to bottom, #ffffff 0%, #dcdcdc 100%); 
  cursor:pointer; 
  font-weight:700; 
  font-size:20px; 
  transition:all 0.15s ease; 
  box-shadow: 0 3px 6px rgba(0,0,0,0.2);
}

.arm-button.active, .fire-btn.active, .fire-all.active { 
  transform:scale(0.95); 
  box-shadow:0 5px 12px rgba(0,0,0,0.4);
}

.relay-row { 
  width:90%; 
  display:flex; 
  align-items:center; 
  justify-content:space-between; 
  margin-bottom:20px;
}

.relay-number { font-size:24px; width:20px; }

.toggle { 
  position:relative; 
  width:60px; 
  height:32px; 
  background:#ccc; 
  border-radius:20px; 
  cursor:pointer; 
  transition:0.2s; 
  box-shadow: inset 0 2px 4px rgba(0,0,0,0.2);
}

.toggle::before { 
  content:''; 
  position:absolute; top:2px; left:2px; width:28px; height:28px; 
  background:white; 
  border-radius:50%; 
  transition:0.2s; 
  box-shadow: 0 2px 3px rgba(0,0,0,0.3);
}

.toggle.on { background:#28a745; }
.toggle.on::before { left:30px; }

.fire-btn { 
  padding:12px 26px; border:none; border-radius:25px; 
  background: linear-gradient(to bottom, #ff4d4d 0%, #cc0000 100%); 
  color:#fff; font-weight:700; cursor:pointer; font-size:18px; 
  transition:all 0.15s ease; box-shadow: 0 3px 8px rgba(0,0,0,0.3);
}

.fire-btn.active { 
  background: linear-gradient(to bottom, #ff1a1a 0%, #b30000 100%); 
  box-shadow:0 5px 10px rgba(0,0,0,0.5);
}

.fire-all { 
  padding:14px 32px; border:none; border-radius:30px; 
  background: linear-gradient(to bottom, #4da6ff 0%, #005cb3 100%); 
  color:#fff; font-weight:700; cursor:pointer; 
  margin-top:20px; font-size:20px; transition:all 0.15s ease; 
  box-shadow: 0 3px 8px rgba(0,0,0,0.3);
}

.fire-all.active { 
  background: linear-gradient(to bottom, #3399ff 0%, #003d80 100%); 
  box-shadow: 0 5px 10px rgba(0,0,0,0.5);
}
</style>
</head>
<body>
<img src="https://i.postimg.cc/Y9NLnTsB/500x500Bolt-Bunny-Logo.png" class="logo" alt="Bolt Bunny Logo">
<h1>BoltBunny Firing Panel</h1>

<div class="top-row">
  <div style="display:flex;align-items:center;">
    <div id="armLed" class="arm-indicator"></div>
    <span id="armLabel">DISARMED</span>
  </div>
  <button id="armBtn" class="arm-button">POWER</button>
</div>

<div class="relay-row"><span class="relay-number">1</span><div id="t1" class="toggle"></div><button id="f1" class="fire-btn"><span>FIRE</span></button></div>
<div class="relay-row"><span class="relay-number">2</span><div id="t2" class="toggle"></div><button id="f2" class="fire-btn"><span>FIRE</span></button></div>
<div class="relay-row"><span class="relay-number">3</span><div id="t3" class="toggle"></div><button id="f3" class="fire-btn"><span>FIRE</span></button></div>
<div class="relay-row"><span class="relay-number">4</span><div id="t4" class="toggle"></div><button id="f4" class="fire-btn"><span>FIRE</span></button></div>

<button id="fireAll" class="fire-all">FIRE ALL</button>

<script>
let armed=false;
const armBtn=document.getElementById('armBtn');
const armLed=document.getElementById('armLed');
const armLabel=document.getElementById('armLabel');

armBtn.onclick = () => {
  armBtn.classList.add('active');           
  setTimeout(() => armBtn.classList.remove('active'), 150);
  armed = !armed;                           

  // ✅ Update label and color immediately
  if (armed) {
    armLabel.textContent = "ARMED";
    armLed.style.background = "lime";
    armLed.style.boxShadow = "0 0 12px lime";
  } else {
    armLabel.textContent = "DISARMED";
    armLed.style.background = "red";
    armLed.style.boxShadow = "0 0 8px rgba(255,0,0,0.5)";
  }

  fetch(armed ? '/armOn' : '/armOff').catch(()=>{}); 
};

let blinkState = false;
let fastBlink = false;
let lastBlinkTime = 0;

function getBlinkInterval() { return fastBlink ? 150 : 500; }

setInterval(() => {
  const now = Date.now();
  if (armed) {
    if(now - lastBlinkTime > getBlinkInterval()) {
      blinkState = !blinkState;
      armLed.style.background = blinkState ? 'lime' : '#004d00';
      armLed.style.boxShadow = blinkState ? '0 0 12px lime' : '0 0 0px rgba(0,0,0,0)';
      lastBlinkTime = now;
    }
  } else {
    armLed.style.background = 'red';
    armLed.style.boxShadow = '0 0 8px rgba(255,0,0,0.5)';
    blinkState = false;
  }
}, 50);

function setupToggle(toggleId, fireBtnId, relayNum){
  const toggle = document.getElementById(toggleId);
  const fireBtn = document.getElementById(fireBtnId);
  toggle.onclick = ()=> toggle.classList.toggle('on');
  function fireAction(on){
    if(on && armed && toggle.classList.contains('on')) fetch('/r'+relayNum+'on').catch(()=>{}); 
    else fetch('/r'+relayNum+'off').catch(()=>{}); 
  }
  fireBtn.addEventListener('mousedown', ()=>{ if(toggle.classList.contains('on')){ fireBtn.classList.add('active'); fireAction(true); fastBlink = true; } });
  fireBtn.addEventListener('mouseup', ()=>{ if(toggle.classList.contains('on')){ fireBtn.classList.remove('active'); fireAction(false); fastBlink = false; } });
  fireBtn.addEventListener('touchstart', (e)=>{ e.preventDefault(); if(toggle.classList.contains('on')){ fireBtn.classList.add('active'); fireAction(true); fastBlink = true; } });
  fireBtn.addEventListener('touchend', (e)=>{ e.preventDefault(); if(toggle.classList.contains('on')){ fireBtn.classList.remove('active'); fireAction(false); fastBlink = false; } });
}

setupToggle('t1','f1',1);
setupToggle('t2','f2',2);
setupToggle('t3','f3',3);
setupToggle('t4','f4',4);

const fireAll=document.getElementById('fireAll');
function fireAllAction(on){
  ['t1','t2','t3','t4'].forEach((tid,i)=>{
    const t = document.getElementById(tid);
    const f = document.getElementById('f'+(i+1));
    if(t.classList.contains('on')){
      fetch('/r'+(i+1)+(on?'on':'off')).catch(()=>{});
      if(on) f.classList.add('active'); else f.classList.remove('active');
    }
  });
  fastBlink = on;
}
fireAll.addEventListener('mousedown', ()=>{ fireAll.classList.add('active'); if(armed) fireAllAction(true); });
fireAll.addEventListener('mouseup', ()=>{ fireAll.classList.remove('active'); if(armed) fireAllAction(false); });
fireAll.addEventListener('touchstart',(e)=>{ e.preventDefault(); fireAll.classList.add('active'); if(armed) fireAllAction(true); });
fireAll.addEventListener('touchend',(e)=>{ e.preventDefault(); fireAll.classList.remove('active'); if(armed) fireAllAction(false); });
</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(ARM_LED, OUTPUT);

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);
  digitalWrite(ARM_LED, LOW);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);

  server.on("/armOn", []() { armed = true; server.send(200, "text/plain", "ARMED"); });
  server.on("/armOff", []() {
    armed = false;
    relayControl(RELAY1, false);
    relayControl(RELAY2, false);
    relayControl(RELAY3, false);
    relayControl(RELAY4, false);
    server.send(200, "text/plain", "DISARMED");
  });

  server.on("/r1on", []() { if (armed) relayControl(RELAY1, true); server.send(200, "text/plain", "R1 ON"); });
  server.on("/r1off", []() { relayControl(RELAY1, false); server.send(200, "text/plain", "R1 OFF"); });
  server.on("/r2on", []() { if (armed) relayControl(RELAY2, true); server.send(200, "text/plain", "R2 ON"); });
  server.on("/r2off", []() { relayControl(RELAY2, false); server.send(200, "text/plain", "R2 OFF"); });
  server.on("/r3on", []() { if (armed) relayControl(RELAY3, true); server.send(200, "text/plain", "R3 ON"); });
  server.on("/r3off", []() { relayControl(RELAY3, false); server.send(200, "text/plain", "R3 OFF"); });
  server.on("/r4on", []() { if (armed) relayControl(RELAY4, true); server.send(200, "text/plain", "R4 ON"); });
  server.on("/r4off", []() { relayControl(RELAY4, false); server.send(200, "text/plain", "R4 OFF"); });

  server.begin();
}

// ==== Loop ====
void loop() {
  server.handleClient();
  if (armed) {
    digitalWrite(ARM_LED, HIGH);
  } else {
    digitalWrite(ARM_LED, LOW);
  }
}






