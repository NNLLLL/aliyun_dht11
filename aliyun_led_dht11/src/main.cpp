#include <Arduino.h>
#include <WiFi.h>
#include "PubSubClient.h"   ////A client library for MQTT messaging.
#include <ArduinoJson.h>
#include <DHT.h>
#define LED 2

#define DHTPIN 4  // Defines pin number to which the sensor is connected
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);  // Creats a DHT object

/* 连接WIFI SSID和密码 */
#define WIFI_SSID         "Xiaomi13"
#define WIFI_PASSWD       "nl123456789"

/* 设备的三元组信息*/
#define PRODUCT_KEY       "a1QRqPV39PT"
#define DEVICE_NAME       "LEDLED"
#define DEVICE_SECRET     "a659783c8b677d1e00ad9007f6433a36"
#define REGION_ID         "cn-shanghai"

/* 线上环境域名和端口号，不需要改 */
#define MQTT_SERVER       PRODUCT_KEY".iot-as-mqtt."REGION_ID ".aliyuncs.com"
#define MQTT_PORT         1883
#define MQTT_USRNAME      DEVICE_NAME"&"PRODUCT_KEY

#define CLIENT_ID         "a1QRqPV39PT.LEDLED|securemode=2,signmethod=hmacsha256,timestamp=1704690340626|"
#define MQTT_PASSWD       "246b9fef67547b4ada96944af6abf1b69a1f7895feff76f64b7cd8967d8164a8"

//宏定义订阅主题
#define ALINK_BODY_FORMAT         "{\"id\":\"LEDLED\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"

const char* TOPIC = "/sys/a1QRqPV39PT/LEDLED/thing/service/property/set"; 
unsigned long lastMs = 0;

WiFiClient espClient;
PubSubClient  client(espClient);

float soil_data ;  
float tep;  

//连接wifi
void wifiInit()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("WiFi not Connect");
    }
}

//mqtt连接
void mqttCheckConnect()
{
    while (!client.connected())
    {
        Serial.println("Connecting to MQTT Server ...");
        if(client.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD))
        {
          Serial.println("MQTT Connected!");
          
        }
        else{
           Serial.print("MQTT Connect err:");
            Serial.println(client.state());
            delay(5000);
          }
        
    }
}

// 上传温湿度、二氧化碳浓度
void mqttIntervalPost()
{
    char param[32];
    char jsonBuf[128];
    
    //upload humidity
    soil_data = dht.readHumidity();   
    sprintf(param, "{\"humidity\":%2f}", soil_data);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);
    boolean b = client.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
    if(b){
      Serial.println("publish Humidity success"); 
    }else{
      Serial.println("publish Humidity fail"); 
    }

    // Upload temperature
    tep =dht.readTemperature();
    sprintf(param, "{\"temperature\":%2f}",tep);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);
    boolean c = client.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
    if(c){
      Serial.println("publish Temperature success"); 
    }else{
      Serial.println("publish Temperature fail"); 
    }
}

//回调函数
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);   // 打印主题信息
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]); // 打印主题内容
  }
  Serial.println();

 DynamicJsonDocument doc(1024);  //创建了一个名为 doc 的动态 JSON 文档
 deserializeJson(doc, String((char *)payload));  //从一个名为 payload 的数据中解析 JSON 数据并将其填充到 doc 中

// DynamicJsonDocument params = doc["params"];

if(doc["params"].containsKey("powerstate"))
{
  Serial.println("GOT DENG CMD"); 
  digitalWrite(LED, doc["params"]["powerstate"]);
}

}

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  wifiInit();
  client.setServer(MQTT_SERVER, MQTT_PORT);   /* 连接MQTT服务器 */
  client.setCallback(callback);                 //设定回调方式，当ESP32收到订阅消息时会调用此方法
  digitalWrite(LED,LOW);
}

void loop()
{
      if (millis() - lastMs >= 5000)
    {
        lastMs = millis();
        mqttCheckConnect(); 
         /* 上报 */
        mqttIntervalPost();
    }
    client.loop();
    // delay(2000);
} 