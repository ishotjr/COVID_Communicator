#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define LILYGO_WATCH_2020_V1
#define LILYGO_WATCH_LVGL
#include <LilyGoWatch.h>

TTGOClass *ttgo;


// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  unsigned long time;
  StaticJsonDocument<200> doc;
  
  time = millis();
  Serial.print("outgoing: ");
  
  doc["time"] = time;
  //doc["sensor_a0"] = analogRead(0);
  doc["security_event"] = 1;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  
  Serial.println(time);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    Serial.println("Button clicked!");
    publishMessage();
  }
}

void setup() {
  
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
  ttgo->lvgl_begin();
  
  lv_obj_t *label;
  
  lv_obj_t *btn1 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(btn1, event_handler);
  lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, -40);
  
  label = lv_label_create(btn1, NULL);
  lv_label_set_text(label, "Security");
  

  Serial.begin(9600);
  connectAWS();
}

void loop() {

  lv_task_handler();

  // MQTT
  client.loop();

  delay(100);
}
