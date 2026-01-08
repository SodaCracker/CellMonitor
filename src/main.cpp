#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// --- 配置区域 ---
const char* ssid     = "你的WiFi名字";      // 必须是2.4G WiFi
const char* password = "你的WiFi密码";
const char* targetIP = "192.168.1.100";   // 必须在路由器里给手机固定这个IP

const int pinHallSensor = 34; // 霍尔传感器接 D34
const int pinBuzzer = 26;     // 蜂鸣器接 D26

// 阈值设定：你需要先测试吸附时的数值。
// 假设没吸附是 1900，吸附了磁铁数值会剧烈变化（变大或变小取决于磁极）
// 这里假设吸附会导致数值 < 1000 或 > 2500，具体看串口监视器调试
int hallThreshold = 2500; 

// 时间设置
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8*3600, 60000); // UTC+8 (中国时间)

bool alarmTriggeredToday = false;

void setup() {
  Serial.begin(115200);
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, HIGH); // 假设低电平触发响，初始化为HIGH不响

  // 连接 WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  
  timeClient.begin();
}

void loop() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // 重置标记：每天凌晨8点重置，保证第二天还能触发
  if (currentHour == 8) {
    alarmTriggeredToday = false;
  }

  // 触发时间：1:30 且 今天没触发过
  if (currentHour == 1 && currentMinute == 30 && !alarmTriggeredToday) {
    
    Serial.println("时间到！开始检测...");
    
    // 1. Ping 手机
    bool isOnline = Ping.ping(targetIP);

    if (isOnline) {
      Serial.println("手机在线(在家)，检查磁吸...");
      
      // 2. 检查霍尔传感器
      // 读取模拟值，建议你先用 Serial.println(analogRead(pinHallSensor)) 看看吸附时的数值
      int hallValue = analogRead(pinHallSensor); 
      
      // 假设吸附会导致数值变大 (需要你自己实测校准)
      if (hallValue > hallThreshold) { 
        Serial.println("已吸附充电，无需报警。");
      } else {
        Serial.println("未吸附！开始尖叫！");
        startAlarm();
      }
    } else {
      Serial.println("手机不在线(不在家)，忽略。");
    }
    
    alarmTriggeredToday = true; // 标记已执行
  }
  
  delay(1000);
}

void startAlarm() {
  unsigned long startTime = millis();
  // 尖叫 3 分钟 (180000 ms)
  while (millis() - startTime < 180000) {
    
    // 实时检测是否吸附了
    int currentHall = analogRead(pinHallSensor);
    if (currentHall > hallThreshold) {
      digitalWrite(pinBuzzer, HIGH); // 停止叫
      Serial.println("中途吸附，停止报警！");
      return; 
    }

    // 报警音效：滴-滴-滴
    digitalWrite(pinBuzzer, LOW); // 响
    delay(200);
    digitalWrite(pinBuzzer, HIGH); // 停
    delay(200);
  }
}