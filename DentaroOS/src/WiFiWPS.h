#ifndef _WIFIWPS_H_
#define _WIFIWPS_H_

#include <esp_wps.h>
#include <Preferences.h>
#include <WiFi.h>
#include "MenuCallBack.h"
#include "Header.h"

// #include <stdint.h>
// #include <stdbool.h>
// #include "esp_err.h"
#include "esp_wifi_crypto_types.h"


class WiFiWPS : public MenuCallBack
{
  static esp_wps_config_t wps_config;
  static bool closing;
public:
  #define ESP_WPS_MODE      WPS_TYPE_PBC
  #define ESP_MANUFACTURER  "ESPRESSIF"
  #define ESP_MODEL_NUMBER  "ESP32"
  #define ESP_MODEL_NAME    "ESPRESSIF IOT"
  #define ESP_DEVICE_NAME   "ESP STATION"

  void wpsInitConfig(){
    wps_config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
    wps_config.wps_type = ESP_WPS_MODE;
    strcpy(wps_config.factory_info.manufacturer, ESP_MANUFACTURER);
    strcpy(wps_config.factory_info.model_number, ESP_MODEL_NUMBER);
    strcpy(wps_config.factory_info.model_name, ESP_MODEL_NAME);
    strcpy(wps_config.factory_info.device_name, ESP_DEVICE_NAME);
  }

  static String wpspin2string(uint8_t a[]){
    char wps_pin[9];
    for(int i=0;i<8;i++){
      wps_pin[i] = a[i];
    }
    wps_pin[8] = '\0';
    return (String)wps_pin;
  }

  static void WiFiEvent(WiFiEvent_t event, system_event_info_t info){
    if (closing) return;
    lcd.setTextFont(0);
    lcd.setTextFont(2);
    switch(event){
      case SYSTEM_EVENT_STA_START:
        lcd.println("Station Mode Started");
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        lcd.println("\r\nConnected to :" + WiFi.SSID());
        lcd.print("Got IP: ");
        lcd.println(WiFi.localIP());
        save();
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        lcd.println("Disconnected from station, attempting reconnection");
        WiFi.reconnect();
        break;
      case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        lcd.println("WPS Successfull, stopping WPS and connecting to: " + String(WiFi.SSID()));
        esp_wifi_wps_disable();
        delay(10);
        WiFi.begin();
        break;
      case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        lcd.println("WPS Failed, retrying");
        esp_wifi_wps_disable();
        esp_wifi_wps_enable(&wps_config);
        esp_wifi_wps_start(0);
        break;
      case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        lcd.println("WPS Timedout, retrying");
        esp_wifi_wps_disable();
        esp_wifi_wps_enable(&wps_config);
        esp_wifi_wps_start(0);
        break;
      case SYSTEM_EVENT_STA_WPS_ER_PIN:
        lcd.println("WPS_PIN = " + wpspin2string(info.sta_er_pin.pin_code));
        break;
      default:
        break;
    }
  }

  wifi_event_id_t onevent = 0;
  bool setup() {
    Header.draw();
    lcd.setTextFont(0);
    lcd.setTextColor(0xFFFF);
    for (int i = 1; i < 16; ++i) {
      lcd.drawFastHLine(0, 10 + i, lcd.width(), i << 1);
    }
    lcd.drawString("WiFi WPS Client (button mode)", 10, 10, &fonts::Font2);

    closing = false;
    WiFi.disconnect(true);
    delay(10);
    lcd.setCursor(0,50);
    lcd.setTextFont(2);
    lcd.println("Starting WPS");
    onevent = WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);

    wpsInitConfig();
    esp_wifi_wps_disable();
    esp_wifi_wps_enable(&wps_config);
    esp_wifi_wps_start(0);

    return true;
  }

  bool loop()
  {
    Header.draw();
    lcd.setTextFont(0);
    lcd.setTextColor(0xFFFF);
    delay(100);
    return true;
  }

  void close()
  {
    closing = true;
    WiFi.removeEvent(onevent);
    delay(100);
    esp_wifi_wps_disable();
    delay(100);
  }
private:
  static void save()
  {
    Preferences preferences;
    preferences.begin("wifi-config");
    preferences.putString("WIFI_SSID", WiFi.SSID());
    preferences.putString("WIFI_PASSWD", WiFi.psk());
    preferences.end();
  }
};
esp_wps_config_t WiFiWPS::wps_config;
bool WiFiWPS::closing;
#endif
