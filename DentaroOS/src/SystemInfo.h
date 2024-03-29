#ifndef _SYSTEMINFO_H_
#define _SYSTEMINFO_H_

#include <nvs.h>
#include "MenuCallBack.h"
#include "Header.h"

class SystemInfo : public MenuCallBack
{
public:
  bool setup()
  {
    lcd_version = lcd_ver();
    lcd.begin();
    btnDrawer.setText(1, "Page");
    btnDrawer.setText(2, "Page");
    page = 0;
    drawPage(page);
    return true;
  }

  bool loop()
  {
    Header.draw();
    if (cmd == M5TreeView::eCmd::NEXT || cmd == M5TreeView::eCmd::PREV || cmd == M5TreeView::eCmd::ENTER) {
      page = (1+page) % pageCount;
      drawPage(page);
    }
    delay(10);
    return true;
  }

private:
  bool lcd_version;
  int page;
  const int pageCount = 2;
  void title(const char* title) {
    int16_t y = lcd.getCursorY() + 10;
    for (int i = 1; i < 16; ++i) {
      lcd.drawFastHLine(0, y + i, lcd.width(), i << 1);
    }
    lcd.drawString(title, 10, y, &fonts::Font2);
    lcd.setCursor(0, y + 14);
  }
  void print(const char* title) {
    int16_t y = lcd.getCursorY() + 3;
    lcd.drawRightString(title, 110, y, &fonts::Font2);
    lcd.drawString(":", 110, y, &fonts::Font2);
    lcd.setCursor(120, y);
  }
  template <typename T>
  void print(const char* title, const char* data, T str) {
    print(title);
    lcd.printf(data, str);
    lcd.setCursor(0, lcd.getCursorY() + 8);
  }
  void drawPage(int page) {
    lcd.fillRect(0, 10, lcd.width(), lcd.height() - 30, 0);
    lcd.setTextSize(1);
    lcd.setTextColor(0xffff);
    lcd.setCursor(0,0);
    lcd.setTextFont(1);
    switch (page) {
    case 0:
      title("System Information");
      uint64_t chipid;
      chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
      esp_chip_info_t chip_info;
      esp_chip_info(&chip_info);
      print("ESP32 Chip ID",   "%04X",  (uint16_t)(chipid>>32));//print High 2 bytes
      print("Chip Revision",   "%d",    ESP.getChipRevision());
      print("Number of Core",  "%d",    chip_info.cores);
      print("CPU Frequency",   "%3d MHz", ESP.getCpuFreqMHz());  
      print("Flash Frequency", "%3d MHz", ESP.getFlashChipSpeed() / 1000000);
      print("Flash Chip Size", "%d Byte", ESP.getFlashChipSize());
      print("ESP-IDF version", "%s",    esp_get_idf_version());
      print("LCD Type",        "%s",    lcd_version ? "IPS" : "TN");
      // print("IMU Type",        "%s",    get_imu_type().c_str());

      title("Mac Address");
      uint8_t mac[6];
      esp_base_mac_addr_get(mac);            print("Base Mac Address");printMac(mac);
      //esp_efuse_mac_get_default(mac);        print("Default");printMac(mac);
      esp_read_mac(mac, ESP_MAC_WIFI_STA);   print("Wi-Fi Station");printMac(mac);
      esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);print("Wi-Fi Soft AP");printMac(mac);
      esp_read_mac(mac, ESP_MAC_BT);         print("Bluetooth");printMac(mac);
      esp_read_mac(mac, ESP_MAC_ETH);        print("Ethernet");printMac(mac);
      break;

    case 1:
      title("Memory");
      print("Total Heap Size"  , "%7d Byte", ESP.getHeapSize());
      print("Total PSRAM Size" , "%7d Byte", ESP.getPsramSize());
      print("Free Heap Size"   , "%7d Byte", esp_get_free_heap_size());
      print("Minimum Free Heap", "%7d Byte", esp_get_minimum_free_heap_size());

      title("NVS (Preferences)");
      nvs_stats_t nvs_stats;
      nvs_get_stats(NULL, &nvs_stats);
      print("Used Entries"   ,"%4d", nvs_stats.used_entries   );
      print("Free Entries"   ,"%4d", nvs_stats.free_entries   );
      print("Total Entries"  ,"%4d", nvs_stats.total_entries  );
      print("Namespace Count","%4d", nvs_stats.namespace_count);

      title("WiFi");
      print("IP Addr"); lcd.println(WiFi.localIP().toString());
      print("Subnet");  lcd.println(WiFi.subnetMask().toString());
      print("SSID");    lcd.println(WiFi.SSID());
      break;
    }
  }
  void printMac(uint8_t* mac) {
    lcd.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

  bool lcd_ver() const {
    bool res = 0;
    pinMode(22, INPUT_PULLDOWN);//33がボタンと被るので、22に撤退
    delay(1);
    if (digitalRead(22)) {
      res = 1;
    } 
    pinMode(22, OUTPUT);
    return res;
  }

  String get_imu_type() const {
    String res;
    uint8_t tmp;
    if (M5.I2C.readByte(0x68, 0x75, &tmp)) {
      switch (tmp) {
      case 0x19: res = "MPU-6886"; break;
      case 0x68: res = "MPU-6050"; break;
      case 0x71: res = "MPU-9250"; break;
      default:   res = "MPU-????"; break;
      }
    } else if (M5.I2C.readByte(0x6C, 0x30, &tmp)) {
      switch (tmp) {
      case 0x18: res = "SH200Q";
      default:   res = "SH????";
      }
    } else {
      res = "not found";
    }

    if (M5.I2C.readByte(0x0E, 0x07, &tmp)) {
      res += " + MAG3110";
    }
    if (M5.I2C.readByte(0x10, 0x40, &tmp)) {
      res += " + BMM150";
    }
    return res;
  }
};

#endif
