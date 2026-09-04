#include "web_control.h"

#include <WiFi.h>
#include <esp_mac.h>
#include "build_info.h"
#include "config.h"
#include "cruise.h"
#include "motor.h"
#include "pcb_config.h"
#include "sensors.h"

namespace {
const char WEB_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>刀盾设备控制</title><style>
:root{--bg:#0b1020;--panel:#151d31;--line:#293552;--text:#edf3ff;--muted:#91a1bf;--blue:#4f8cff;--red:#ff5d6c;--green:#32d296}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}body{margin:0;min-height:100vh;background:linear-gradient(160deg,#090d18,#111a31);color:var(--text);font-family:system-ui,"Microsoft YaHei",sans-serif}.wrap{width:min(720px,100%);margin:auto;padding:18px}h1{margin:0;font-size:22px}.sub{color:var(--muted);font-size:13px;margin:5px 0 16px}.panel{background:#151d31f0;border:1px solid var(--line);border-radius:18px;padding:16px;margin-bottom:14px;box-shadow:0 14px 36px #0005}.status{display:flex;justify-content:space-between;gap:10px;align-items:center}.pill{padding:6px 11px;border-radius:999px;background:#202b45;color:var(--muted);font-size:12px}.pill.on{background:#153b32;color:#77e7bd}.compass{position:relative;width:220px;height:220px;margin:12px auto 6px;border-radius:50%;border:3px solid #60749c;background:radial-gradient(circle,#1b2844 0 47%,#10182b 48% 100%);box-shadow:inset 0 0 30px #0008}.cardinal{position:absolute;font-weight:800;color:#c9d8f5}.n{top:8px;left:50%;transform:translateX(-50%);color:var(--red)}.s{bottom:8px;left:50%;transform:translateX(-50%)}.w{left:12px;top:50%;transform:translateY(-50%)}.e{right:12px;top:50%;transform:translateY(-50%)}.needle,.target-needle{position:absolute;left:calc(50% - 3px);bottom:50%;width:6px;height:78px;border-radius:6px;transform-origin:50% 100%;transition:transform .28s ease}.needle{background:linear-gradient(var(--red) 0 76%,#f7f9ff 76%);z-index:3}.target-needle{left:calc(50% - 2px);width:4px;height:91px;background:var(--green);opacity:.75;z-index:2}.hub{position:absolute;left:50%;top:50%;width:18px;height:18px;border-radius:50%;background:#f2f6ff;border:4px solid #273552;transform:translate(-50%,-50%);z-index:4}.heading{text-align:center;font-size:28px;font-weight:750}.heading small{font-size:13px;color:var(--muted)}.legend{text-align:center;color:var(--muted);font-size:12px;margin-top:5px}.speed{display:grid;grid-template-columns:auto 1fr auto;gap:10px;align-items:center;margin:12px 2px 4px;color:var(--muted);font-size:13px}.speed input{width:100%;accent-color:var(--blue)}.speed output{min-width:34px;text-align:right;color:var(--text);font-variant-numeric:tabular-nums}.dpad{display:grid;grid-template-columns:repeat(3,86px);grid-template-rows:repeat(3,70px);gap:9px;justify-content:center;margin:10px 0}button{border:1px solid #385079;background:#223352;color:var(--text);border-radius:14px;font-size:17px;font-weight:650;cursor:pointer;touch-action:none;user-select:none;box-shadow:0 5px 12px #0004}button:active,.manual.active{transform:translateY(2px);background:#315b9f;box-shadow:none}.stop,.danger{background:#612a39;border-color:#a7465b}.empty{visibility:hidden}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px}.actions button{min-height:50px;font-size:15px}.primary{background:#2359a8;border-color:var(--blue)}.turn{background:#234438;border-color:#34785f}.note{color:var(--muted);font-size:12px;line-height:1.55}.toast{position:fixed;left:50%;bottom:24px;transform:translate(-50%,20px);padding:10px 16px;background:#050914e8;border:1px solid #465574;border-radius:10px;opacity:0;pointer-events:none;transition:.2s}.toast.show{opacity:1;transform:translate(-50%,0)}@media(max-width:420px){.wrap{padding:12px}.compass{width:200px;height:200px}.dpad{grid-template-columns:repeat(3,78px)}}
</style></head><body><main class="wrap"><h1>刀盾设备控制</h1><div class="sub" id="deviceName">正在连接设备…</div>
<section class="panel"><div class="status"><strong>当前前进方向</strong><span class="pill" id="runState">等待状态</span></div><div class="compass"><span class="cardinal n">N</span><span class="cardinal e">E</span><span class="cardinal s">S</span><span class="cardinal w">W</span><div class="target-needle" id="targetNeedle"></div><div class="needle" id="needle"></div><div class="hub"></div></div><div class="heading"><span id="yaw">--.-</span>° <small id="direction">--</small></div><div class="legend">绿色指针为巡航目标：<span id="target">--.-°</span></div></section>
<section class="panel"><div class="status"><strong>手动方向</strong><span class="pill">转向固定 70% PWM</span></div><label class="speed"><span>前进/后退 PWM</span><input id="manualPwm" type="range" min="0" max="255" value="255"><output id="manualPwmValue">255</output></label><div class="dpad"><button class="empty"></button><button class="manual" data-dir="forward">▲<br>前进</button><button class="empty"></button><button class="manual" data-dir="left">◀ 左转</button><button class="stop" id="stopBtn">■ 停止</button><button class="manual" data-dir="right">右转 ▶</button><button class="empty"></button><button class="manual" data-dir="backward">后退<br>▼</button><button class="empty"></button></div><p class="note">前进和后退使用滑块的原始 PWM 值；左右转固定为 PWM 179。</p></section>
<section class="panel"><div class="status"><strong>巡航控制</strong><span class="pill">每次调整15°</span></div><div class="actions" style="margin-top:12px"><button class="primary" onclick="setCruise(1)">开始巡航</button><button class="danger" onclick="setCruise(0)">停止巡航</button><button class="turn" id="cruiseLeft" onclick="turnCruise('left')">巡航左转</button><button class="turn" id="cruiseRight" onclick="turnCruise('right')">巡航右转</button></div><p class="note">巡航转向只修改目标航向，由设备自行纠偏。</p></section>
</main><div class="toast" id="toast"></div><script>
const $=id=>document.getElementById(id);let holding=null,keepTimer=null,needleAngle=null,targetAngle=null;
function toast(s){$('toast').textContent=s;$('toast').classList.add('show');setTimeout(()=>$('toast').classList.remove('show'),1500)}
async function post(url){try{const r=await fetch(url,{method:'POST',cache:'no-store'});return await r.json()}catch(e){return{ok:false,error:'连接失败'}}}
async function motion(dir){const pwm=$('manualPwm').value;const r=await post('/api/motion?dir='+encodeURIComponent(dir)+'&pwm='+pwm);if(!r.ok)toast(r.error||'控制失败')}
function endHold(sendStop=true){if(keepTimer){clearInterval(keepTimer);keepTimer=null}document.querySelectorAll('.manual.active').forEach(x=>x.classList.remove('active'));if(holding&&sendStop)motion('stop');holding=null}
function beginHold(dir,el,e){e.preventDefault();endHold(false);holding=dir;el.classList.add('active');motion(dir);keepTimer=setInterval(()=>motion(dir),250)}
document.querySelectorAll('.manual').forEach(b=>{b.onpointerdown=e=>{try{b.setPointerCapture(e.pointerId)}catch(_){}beginHold(b.dataset.dir,b,e)};b.onpointerup=()=>endHold();b.onpointercancel=()=>endHold();b.oncontextmenu=e=>e.preventDefault()});
$('manualPwm').oninput=e=>$('manualPwmValue').textContent=e.target.value;
$('stopBtn').onclick=()=>{endHold(false);motion('stop')};window.addEventListener('blur',()=>endHold());document.addEventListener('visibilitychange',()=>{if(document.hidden)endHold()});
async function setCruise(enable){endHold(false);const r=await post('/api/cruise?enable='+enable);toast(r.ok?(enable?'巡航已开始':'巡航已停止'):(r.error||'操作失败'));refresh()}
async function turnCruise(side){const r=await post('/api/cruise/turn?side='+side);toast(r.ok?('目标航向 '+Number(r.target).toFixed(1)+'°'):(r.error||'请先开始巡航'));refresh()}
function dirName(y){const n=['北','东北','东','东南','南','西南','西','西北'];return n[Math.round(((y%360)+360)%360/45)%8]}
function nearestAngle(previous,next){next=((next%360)+360)%360;if(previous===null)return next;const base=((previous%360)+360)%360;let delta=next-base;if(delta>180)delta-=360;else if(delta<-180)delta+=360;return previous+delta}
async function refresh(){try{const r=await fetch('/api/status',{cache:'no-store'}),s=await r.json();const y=Number(s.yaw)||0,t=Number(s.target)||0;needleAngle=nearestAngle(needleAngle,y);$('needle').style.transform='rotate('+needleAngle+'deg)';if(s.cruise){targetAngle=nearestAngle(targetAngle,t);$('targetNeedle').style.transform='rotate('+targetAngle+'deg)';$('targetNeedle').style.display='block'}else{$('targetNeedle').style.display='none';targetAngle=null}$('yaw').textContent=y.toFixed(1);$('direction').textContent=dirName(y);$('target').textContent=s.cruise?t.toFixed(1)+'°':'未巡航';$('runState').textContent=s.state;$('runState').classList.toggle('on',s.cruise);$('deviceName').textContent=s.ap+' · '+s.pcb+' · 固件 '+s.build;$('cruiseLeft').disabled=!s.cruise;$('cruiseRight').disabled=!s.cruise}catch(e){$('runState').textContent='连接中断'}}
refresh();setInterval(refresh,350);
</script></body></html>
)HTML";
}

WebControlClass WebControl;

bool WebControlClass::begin() {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char name[33];
    snprintf(name, sizeof(name), "DaoDun-WiFi-%02X%02X%02X", mac[3], mac[4], mac[5]);
    apName_ = name;

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(apName_.c_str(), nullptr, WEB_AP_CHANNEL, false,
                     WEB_AP_MAX_CONNECTIONS)) return false;

    server_.on("/", HTTP_GET, [this]() { handleRoot(); });
    server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/api/motion", HTTP_POST, [this]() { handleMotion(); });
    server_.on("/api/cruise", HTTP_POST, [this]() { handleCruise(); });
    server_.on("/api/cruise/turn", HTTP_POST, [this]() { handleCruiseTurn(); });
    server_.onNotFound([this]() {
        server_.send(404, "application/json", "{\"ok\":false,\"error\":\"not found\"}");
    });
    server_.begin();
    started_ = true;
    return true;
}

void WebControlClass::loop() {
    if (!started_) return;
    server_.handleClient();
    if (manualMotion_ != ManualMotion::STOP
        && millis() - lastManualCommandMs_ > WEB_MANUAL_WATCHDOG_MS) {
        stopManualMotion();
    }
}

void WebControlClass::handleRoot() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
}

void WebControlClass::handleStatus() {
    bool cruise = cruiseIsEnabled();
    float yaw = sensorsYaw();
    String json;
    json.reserve(180);
    json = F("{\"ok\":true,\"yaw\":");
    json += String(yaw, 1);
    json += F(",\"target\":");
    json += String(cruiseGetTargetYaw(), 1);
    json += F(",\"cruise\":");
    json += cruise ? F("true") : F("false");
    json += F(",\"state\":\"");
    json += cruise ? "巡航" : manualMotionName();
    json += F("\",\"ap\":\"");
    json += apName_;
    json += F("\",\"pcb\":\"");
    json += pcbGetVersion() == PcbVersion::NEW_PCB ? F("新版PCB") : F("旧版PCB");
    json += F("\",\"build\":\"");
    json += FIRMWARE_BUILD_TIME;
    json += F("\"}");
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json; charset=utf-8", json);
}

void WebControlClass::handleMotion() {
    String direction = server_.arg("dir");
    if (server_.hasArg("pwm")) {
        long value = server_.arg("pwm").toInt();
        if (value < 0) value = 0;
        if (value > PWM_MAX) value = PWM_MAX;
        manualPwm_ = (uint8_t)value;
    }
    ManualMotion motion = ManualMotion::STOP;
    if (direction == "forward") motion = ManualMotion::FORWARD;
    else if (direction == "backward") motion = ManualMotion::BACKWARD;
    else if (direction == "left") motion = ManualMotion::LEFT;
    else if (direction == "right") motion = ManualMotion::RIGHT;
    else if (direction != "stop") {
        sendResult(false, "invalid direction", 400);
        return;
    }
    applyManualMotion(motion);
    sendResult(true);
}

void WebControlClass::handleCruise() {
    if (!server_.hasArg("enable")) {
        sendResult(false, "missing enable", 400);
        return;
    }
    stopManualMotion();
    cruiseEnable(server_.arg("enable").toInt() != 0, sensorsYaw());
    sendResult(true);
}

void WebControlClass::handleCruiseTurn() {
    if (!cruiseIsEnabled()) {
        sendResult(false, "cruise is not running", 409);
        return;
    }
    String side = server_.arg("side");
    float target = cruiseGetTargetYaw();
    if (side == "left") target += WEB_CRUISE_TURN_STEP_DEG;
    else if (side == "right") target -= WEB_CRUISE_TURN_STEP_DEG;
    else {
        sendResult(false, "invalid turn", 400);
        return;
    }
    cruiseSetTargetYaw(target);
    String json = F("{\"ok\":true,\"target\":");
    json += String(cruiseGetTargetYaw(), 1);
    json += '}';
    server_.send(200, "application/json", json);
}

void WebControlClass::sendResult(bool ok, const char* error, int statusCode) {
    if (ok) {
        server_.send(statusCode, "application/json", "{\"ok\":true}");
        return;
    }
    String json = F("{\"ok\":false,\"error\":\"");
    json += error ? error : "error";
    json += F("\"}");
    server_.send(statusCode, "application/json", json);
}

void WebControlClass::stopManualMotion() {
    if (manualMotion_ != ManualMotion::STOP) motorsStop();
    manualMotion_ = ManualMotion::STOP;
    lastManualCommandMs_ = millis();
}

void WebControlClass::applyManualMotion(ManualMotion motion) {
    if (cruiseIsEnabled()) cruiseEnable(false, sensorsYaw());
    switch (motion) {
        case ManualMotion::FORWARD:
            setLeftMotor(MOTOR_FORWARD, manualPwm_);
            setRightMotor(MOTOR_FORWARD, manualPwm_);
            break;
        case ManualMotion::BACKWARD:
            setLeftMotor(MOTOR_REVERSE, manualPwm_);
            setRightMotor(MOTOR_REVERSE, manualPwm_);
            break;
        case ManualMotion::LEFT:
            setLeftMotor(MOTOR_REVERSE, WEB_TURN_MOTOR_PWM);
            setRightMotor(MOTOR_FORWARD, WEB_TURN_MOTOR_PWM);
            break;
        case ManualMotion::RIGHT:
            setLeftMotor(MOTOR_FORWARD, WEB_TURN_MOTOR_PWM);
            setRightMotor(MOTOR_REVERSE, WEB_TURN_MOTOR_PWM);
            break;
        default:
            motorsStop();
            break;
    }
    manualMotion_ = motion;
    lastManualCommandMs_ = millis();
}

const char* WebControlClass::manualMotionName() const {
    switch (manualMotion_) {
        case ManualMotion::FORWARD: return "前进";
        case ManualMotion::BACKWARD: return "后退";
        case ManualMotion::LEFT: return "左转";
        case ManualMotion::RIGHT: return "右转";
        default: return "停止";
    }
}
