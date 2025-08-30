#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h> // 使用标准的WebServer库
#include <WiFiManager.h>
#include <ESP32Servo.h> // 舵机库
#include <SPIFFS.h> // 用于文件系统
#include <WiFiUdp.h> // 用于NTP客户端
#include <NTPClient.h> // NTP客户端库
#include <time.h> // 用于时间结构体和格式化
#include <sys/time.h> // 用于设置时区

// 定义舵机连接的GPIO引脚
#define SERVO_PIN 13

// 舵机对象
Servo myservo;

// Web服务器对象，监听80端口
WebServer httpServer(80);

// WiFi管理器对象
WiFiManager wm;

// 全局状态变量
bool isLightOn = false; // 灯光状态，默认为关
bool isAutoResetEnabled = false; // 自动复位功能是否启用，默认为禁用
int autoResetAngle = 45; // 自动复位角度，默认为45度
int onAngle = 90; // 开灯角度，默认为90度
int offAngle = 0; // 关灯角度，默认为0度

// 用于触发WiFi配网模式的标志位
bool shouldEnterConfigMode = false;

// 记录ESP32启动时的毫秒数
unsigned long bootMillis = 0;

// NTP客户端
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600); // NTP服务器，8小时时区偏移 (UTC+8)

// 存储开机时的实际日期时间
struct tm bootTimeInfo;
bool ntpSynced = false; // 标志NTP是否已同步

// 将毫秒数转换为 HH:MM:SS 格式的字符串
String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  seconds %= 60;
  minutes %= 60;
  
  char buf[32];
  sprintf(buf, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buf);
}

// 舵机控制函数
// 参数：angle - 目标角度 (0-180)
void setServoAngle(int angle) {
  // 确保角度在有效范围内
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  myservo.write(angle); // 设置舵机角度
  Serial.printf("舵机旋转到角度: %d\n", angle);
}

// 处理根路径请求，发送index.html
void handleRoot() {
  httpServer.send(200, "text/html", SPIFFS.open("/index.html").readString());
}

// 处理CSS文件请求
void handleCSS() {
  httpServer.send(200, "text/css", SPIFFS.open("/style.css").readString());
}

// 处理JavaScript文件请求
void handleJS() {
  httpServer.send(200, "application/javascript", SPIFFS.open("/script.js").readString());
}

// 处理开灯请求
void handleTurnLightOn() {
  if (httpServer.hasArg("angle")) {
    int angle = httpServer.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      setServoAngle(angle);
      isLightOn = true;
      // 如果启用了自动复位，则在短时间后复位舵机
      if (isAutoResetEnabled) {
        delay(500); // 等待500毫秒
        setServoAngle(autoResetAngle); // 舵机复位到设定角度
      }
      httpServer.send(200, "text/plain", "ON");
    } else {
      httpServer.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    httpServer.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理关灯请求
void handleTurnLightOff() {
  if (httpServer.hasArg("angle")) {
    int angle = httpServer.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      setServoAngle(angle);
      isLightOn = false;
      // 如果启用了自动复位，则在短时间后复位舵机
      if (isAutoResetEnabled) {
        delay(500); // 等待500毫秒
        setServoAngle(autoResetAngle); // 舵机复位到设定角度
      }
      httpServer.send(200, "text/plain", "OFF");
    } else {
      httpServer.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    httpServer.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理设置开灯角度的请求
void handleSetOnAngle() {
  if (httpServer.hasArg("angle")) {
    int angle = httpServer.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      onAngle = angle;
      httpServer.send(200, "text/plain", "OK");
    } else {
      httpServer.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    httpServer.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理设置关灯角度的请求
void handleSetOffAngle() {
  if (httpServer.hasArg("angle")) {
    int angle = httpServer.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      offAngle = angle;
      httpServer.send(200, "text/plain", "OK");
    } else {
      httpServer.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    httpServer.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理切换自动复位状态的请求
void handleToggleAutoReset() {
  if (httpServer.hasArg("enable")) {
    String enableStr = httpServer.arg("enable");
    isAutoResetEnabled = (enableStr == "true"); // 更新自动复位状态
    httpServer.send(200, "text/plain", isAutoResetEnabled ? "Enabled" : "Disabled");
  } else {
    httpServer.send(400, "text/plain", "Missing enable parameter.");
  }
}

// 处理设置自动复位角度的请求
void handleSetAutoResetAngle() {
  if (httpServer.hasArg("angle")) {
    int angle = httpServer.arg("angle").toInt();
    // 后端数据校验
    if (angle >= 0 && angle <= 180) {
      autoResetAngle = angle; // 更新自动复位角度
      httpServer.send(200, "text/plain", "OK");
    } else {
      httpServer.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    httpServer.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理获取当前状态的请求 (用于网页初始化)
void handleGetStatus() {
  String json = "{";
  json += "\"isLightOn\":" + String(isLightOn ? "true" : "false") + ",";
  json += "\"isAutoResetEnabled\":" + String(isAutoResetEnabled ? "true" : "false") + ",";
  json += "\"autoResetAngle\":" + String(autoResetAngle) + ",";
  json += "\"onAngle\":" + String(onAngle) + ",";
  json += "\"offAngle\":" + String(offAngle);
  json += "}";
  httpServer.send(200, "application/json", json);
}

// 处理获取IP地址的请求
void handleGetIP() {
  httpServer.send(200, "text/plain", WiFi.localIP().toString());
}

// 处理断开WiFi并进入配网模式的请求
void handleDisconnectAndConfigureWifi() {
  Serial.println("接收到断开WiFi并进入配网模式的请求...");
  httpServer.send(200, "text/plain", "OK"); // 立即响应客户端，避免超时
  shouldEnterConfigMode = true; // 设置标志位，在loop中处理
  Serial.println("已设置shouldEnterConfigMode为true。");
}

// 处理获取时间信息的请求
void handleTimeInfo() {
  unsigned long currentUptimeMillis = millis() - bootMillis;
  String uptimeStr = formatUptime(currentUptimeMillis);

  char bootTimeBuffer[64];
  if (ntpSynced) {
    strftime(bootTimeBuffer, sizeof(bootTimeBuffer), "%Y-%m-%d %H:%M:%S", &bootTimeInfo);
  } else {
    strcpy(bootTimeBuffer, "NTP未同步");
  }
  String bootTimeStr = String(bootTimeBuffer);

  String json = "{";
  json += "\"uptime\":\"" + uptimeStr + "\",";
  json += "\"bootTime\":\"" + bootTimeStr + "\"";
  json += "}";
  httpServer.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200); // 初始化串口通信
  bootMillis = millis(); // 记录开机时的毫秒数

  // 初始化舵机
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // 标准舵机通常为50Hz
  myservo.attach(SERVO_PIN, 500, 2500); // 将舵机连接到指定引脚，并设置PWM脉冲范围 (微秒)

  // WiFiManager配置
  wm.autoConnect("ESP32_SmartSwitch_AP"); // 自动连接WiFi，如果失败则创建AP
  Serial.println("WiFi连接成功！");
  Serial.print("ESP32 IP地址: ");
  Serial.println(WiFi.localIP());

  // 配置NTP时间同步
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // 设置时区和NTP服务器
  Serial.println("等待NTP时间同步...");
  time_t now = 0;
  struct tm timeinfo;
  int retryCount = 0;
  while (!getLocalTime(&timeinfo) && retryCount < 10) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }
  if (retryCount < 10) {
    Serial.println("\nNTP时间同步成功！");
    getLocalTime(&bootTimeInfo); // 记录开机时的NTP同步时间
    ntpSynced = true;
    char bootTimeBuffer[64];
    strftime(bootTimeBuffer, sizeof(bootTimeBuffer), "%Y-%m-%d %H:%M:%S", &bootTimeInfo);
    Serial.printf("开机时间: %s\n", bootTimeBuffer);
  } else {
    Serial.println("\nNTP时间同步失败！将使用相对时间。");
  }

  // 初始化SPIFFS文件系统
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS挂载失败！");
    return;
  }
  Serial.println("SPIFFS挂载成功！");

  // 配置Web服务器路由
  httpServer.on("/", HTTP_GET, handleRoot); // 根路径
  httpServer.on("/style.css", HTTP_GET, handleCSS); // CSS文件
  httpServer.on("/script.js", HTTP_GET, handleJS); // JavaScript文件

  httpServer.on("/turnLightOn", HTTP_GET, handleTurnLightOn); // 开灯
  httpServer.on("/turnLightOff", HTTP_GET, handleTurnLightOff); // 关灯
  httpServer.on("/setOnAngle", HTTP_GET, handleSetOnAngle); // 设置开灯角度
  httpServer.on("/setOffAngle", HTTP_GET, handleSetOffAngle); // 设置关灯角度
  httpServer.on("/toggleAutoReset", HTTP_GET, handleToggleAutoReset); // 切换自动复位状态
  httpServer.on("/setAutoResetAngle", HTTP_GET, handleSetAutoResetAngle); // 设置自动复位角度
  httpServer.on("/status", HTTP_GET, handleGetStatus); // 获取当前状态
  httpServer.on("/ip", HTTP_GET, handleGetIP); // 获取IP地址
  httpServer.on("/disconnectAndConfigureWifi", HTTP_GET, handleDisconnectAndConfigureWifi); // 断开WiFi并进入配网模式
  httpServer.on("/timeinfo", HTTP_GET, handleTimeInfo); // 获取运行时间和开机时间

  // 启动Web服务器
  httpServer.begin();
  Serial.println("Web服务器已启动！");

  // 初始舵机状态设置为关灯角度
  setServoAngle(offAngle);
}

void loop() {
  httpServer.handleClient(); // 处理客户端请求

  if (shouldEnterConfigMode) {
    Serial.println("断开当前WiFi连接...");
    WiFi.disconnect(true); // 断开WiFi并清除保存的凭据
    
    Serial.println("重置WiFiManager设置...");
    wm.resetSettings(); // 重置WiFiManager的配置

    Serial.println("启动WiFiManager配网门户...");
    // 启动配网门户，这将阻塞直到配置完成或超时
    wm.startConfigPortal("ESP32_SmartSwitch_AP"); 
    
    Serial.println("配网门户已退出，尝试重新连接WiFi...");
    // 重新连接WiFi（如果用户在门户中配置了新的凭据）
    // wm.autoConnect() 会在内部处理连接逻辑

    shouldEnterConfigMode = false; // 重置标志位
  }
}
