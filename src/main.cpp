#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h> // 使用标准的WebServer库
#include <WiFiManager.h>
#include <ESP32Servo.h> // 舵机库
#include <SPIFFS.h> // 用于文件系统

// 定义舵机连接的GPIO引脚
#define SERVO_PIN 13

// 舵机对象
Servo myservo;

// Web服务器对象，监听80端口
WebServer server(80);

// WiFi管理器对象
WiFiManager wm;

// 全局状态变量
bool isLightOn = false; // 灯光状态，默认为关
bool isAutoResetEnabled = false; // 自动复位功能是否启用，默认为禁用
int autoResetAngle = 45; // 自动复位角度，默认为45度
int onAngle = 90; // 开灯角度，默认为90度
int offAngle = 0; // 关灯角度，默认为0度

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
  server.send(200, "text/html", SPIFFS.open("/index.html").readString());
}

// 处理CSS文件请求
void handleCSS() {
  server.send(200, "text/css", SPIFFS.open("/style.css").readString());
}

// 处理JavaScript文件请求
void handleJS() {
  server.send(200, "application/javascript", SPIFFS.open("/script.js").readString());
}

// 处理开灯请求
void handleTurnLightOn() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      setServoAngle(angle);
      isLightOn = true;
      // 如果启用了自动复位，则在短时间后复位舵机
      if (isAutoResetEnabled) {
        delay(500); // 等待500毫秒
        setServoAngle(autoResetAngle); // 舵机复位到设定角度
      }
      server.send(200, "text/plain", "ON");
    } else {
      server.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    server.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理关灯请求
void handleTurnLightOff() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      setServoAngle(angle);
      isLightOn = false;
      // 如果启用了自动复位，则在短时间后复位舵机
      if (isAutoResetEnabled) {
        delay(500); // 等待500毫秒
        setServoAngle(autoResetAngle); // 舵机复位到设定角度
      }
      server.send(200, "text/plain", "OFF");
    } else {
      server.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    server.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理设置开灯角度的请求
void handleSetOnAngle() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      onAngle = angle;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    server.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理设置关灯角度的请求
void handleSetOffAngle() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      offAngle = angle;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    server.send(400, "text/plain", "Missing angle parameter.");
  }
}

// 处理切换自动复位状态的请求
void handleToggleAutoReset() {
  if (server.hasArg("enable")) {
    String enableStr = server.arg("enable");
    isAutoResetEnabled = (enableStr == "true"); // 更新自动复位状态
    server.send(200, "text/plain", isAutoResetEnabled ? "Enabled" : "Disabled");
  } else {
    server.send(400, "text/plain", "Missing enable parameter.");
  }
}

// 处理设置自动复位角度的请求
void handleSetAutoResetAngle() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    // 后端数据校验
    if (angle >= 0 && angle <= 180) {
      autoResetAngle = angle; // 更新自动复位角度
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid angle. Must be between 0 and 180.");
    }
  } else {
    server.send(400, "text/plain", "Missing angle parameter.");
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
  server.send(200, "application/json", json);
}

// 处理获取IP地址的请求
void handleGetIP() {
  server.send(200, "text/plain", WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200); // 初始化串口通信

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

  // 初始化SPIFFS文件系统
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS挂载失败！");
    return;
  }
  Serial.println("SPIFFS挂载成功！");

  // 配置Web服务器路由
  server.on("/", HTTP_GET, handleRoot); // 根路径
  server.on("/style.css", HTTP_GET, handleCSS); // CSS文件
  server.on("/script.js", HTTP_GET, handleJS); // JavaScript文件

  server.on("/turnLightOn", HTTP_GET, handleTurnLightOn); // 开灯
  server.on("/turnLightOff", HTTP_GET, handleTurnLightOff); // 关灯
  server.on("/setOnAngle", HTTP_GET, handleSetOnAngle); // 设置开灯角度
  server.on("/setOffAngle", HTTP_GET, handleSetOffAngle); // 设置关灯角度
  server.on("/toggleAutoReset", HTTP_GET, handleToggleAutoReset); // 切换自动复位状态
  server.on("/setAutoResetAngle", HTTP_GET, handleSetAutoResetAngle); // 设置自动复位角度
  server.on("/status", HTTP_GET, handleGetStatus); // 获取当前状态
  server.on("/ip", HTTP_GET, handleGetIP); // 获取IP地址

  // 启动Web服务器
  server.begin();
  Serial.println("Web服务器已启动！");

  // 初始舵机状态设置为关灯角度
  setServoAngle(offAngle);
}

void loop() {
  server.handleClient(); // 处理客户端请求
}
