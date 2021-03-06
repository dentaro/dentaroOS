#ifndef _BINARYVIEWER_H_
#define _BINARYVIEWER_H_

#include <SD.h>
#include <esp_flash_partitions.h>
#include "MenuCallBack.h"
#include "Header.h"

class BinaryViewer : public MenuCallBack
{
public:
  bool setup() 
  {
    for (int i = 0; i < DISPLAYLINES; ++i) {
      drawData(i, i * 16);
    }
    drawAddress();
    lcd.setCursor(224,40);
    lcd.printf("size:% 9d", dataSize);
    btnDrawer.setText(0, "Back / Up");
    btnDrawer.setText(2, "Down");
    return true;
  }

  bool loop()
  {
    int move = (cmd == M5TreeView::eCmd::NEXT && dataSize/16 > firstdisplayline+DISPLAYLINES ) ? 1
             : (cmd == M5TreeView::eCmd::PREV && 0 < firstdisplayline) ? -1 : 0;

    if (0 != move) {
      firstdisplayline += move;
#ifdef ARDUINO_ODROID_ESP32
      for (int i = 0; i < DISPLAYLINES; ++i) {
        drawData(i, (firstdisplayline + i) * 16);
      }
#else
      setupScrollArea(hHeader, hFooter);
      scroll(hHeader + (firstdisplayline % DISPLAYLINES) * 10);
      if (move == 1) {
        drawData(((firstdisplayline-1) % DISPLAYLINES), (firstdisplayline-1+ DISPLAYLINES) * 16);
      } else {
        drawData((firstdisplayline % DISPLAYLINES), firstdisplayline * 16);
      }
#endif
      drawAddress();
    } else {
      delay(10);
      Header.draw();
    }
    return true;
  }
  void close() {
    scroll(0);
  }

protected:
  int dataSize;
  const int DISPLAYLINES = 17;
  int firstdisplayline = 0;
  int hHeader = 50;
  int hFooter = 20;

  virtual int getData(int pos, uint8_t* buf) { return false; }
private:
  void setupScrollArea(uint16_t tfa, uint16_t bfa) {
    static constexpr uint32_t VSCRDEF = 0x33;
    lcd.startWrite();
    lcd.writecommand(VSCRDEF); // Vertical scroll definition
    lcd.writedata(tfa >> 8);           // Top Fixed Area line count
    lcd.writedata(tfa);
    lcd.writedata((lcd.height()-tfa-bfa)>>8);  // Vertical Scrolling Area line count
    lcd.writedata(lcd.height()-tfa-bfa);
    lcd.writedata(bfa >> 8);           // Bottom Fixed Area line count
    lcd.writedata(bfa);
    lcd.endWrite();
  }
  void scroll(uint16_t vsp) {
    static constexpr uint32_t VSCRSADD = 0x37;
    lcd.startWrite();
    lcd.writecommand(VSCRSADD); // Vertical scrolling pointer
    lcd.writedata(vsp>>8);
    lcd.writedata(vsp);
    lcd.endWrite();
  }
  void drawAddress()
  {
    lcd.setCursor(0,40);
    lcd.setTextColor(0xFFE0, 0);
    lcd.printf("0x%08X", firstdisplayline * 16);
  }
  bool drawData(int line_y, int pos)
  {
    int y = line_y * 10 + hHeader;
    uint8_t buf[16] = {0};
    int len = getData(pos, buf);
    if (len < 16) {
      lcd.fillRect(len * 14, y, lcd.width(), 10, 0);
    }
    lcd.drawFastVLine(110, y, 10, 0x8410);
    lcd.drawFastVLine(222, y, 10, 0x8410);
    ++y;
    lcd.setTextFont(1);
    lcd.setTextSize(1);
    lcd.setTextColor(0xFFFF, 0);
    for (int x = 0; x < len; ++x) {
      lcd.setCursor(x * 14, y);
      lcd.printf("%02X", buf[x]);
      if (buf[x] > 0x20) {
        lcd.setCursor(224 + x * 6, y);
        lcd.write(buf[x]);
      } else {
        lcd.fillRect(224 + x * 6, y, 12, 8, 0);
      }
    }
    return true;
  }
};

class BinaryViewerFS : public BinaryViewer
{
  File file;
public:
  bool setup() 
  {
    MenuItemFS* mi = (MenuItemFS*)menuItem;
    if (mi->path == "") return false;
    FS& fs(mi->getFS());
    file = fs.open(mi->path);
    if (file.isDirectory()) {
      return false;
    }
    dataSize = file.size();
    lcd.setTextColor(0xFFFF);
    for (int i = 1; i < 16; ++i) {
      lcd.drawFastHLine(0, 10 + i, lcd.width(), (&fs == &SD) ? (i << 1) : (i << 6));
    }
    lcd.drawString("Binary Viewer " + String((&fs == &SD) ? "(SDCard)" : "(SPIFFS)"), 10, 10, &fonts::Font2);

    lcd.drawString(mi->path, 0, 30, &fonts::Font2);

    return BinaryViewer::setup();
  }

  void close() {
    BinaryViewer::close();
    file.close();
  }
  virtual int getData(int pos, uint8_t* buf) {
    if (!file.seek(pos)) return false;
    return file.read(buf, 16);
  }
};

class BinaryViewerFlash : public BinaryViewer
{
  esp_partition_t partition;
public:
  bool setup() 
  {
    switch (menuItem->tag) {
    case 0:
      partition.address = ESP_BOOTLOADER_OFFSET;
      partition.size = CONFIG_PARTITION_TABLE_OFFSET - ESP_BOOTLOADER_OFFSET;
      strcpy(partition.label, "2nd boot loader");
      partition.encrypted = false;
      break;
    case 1:
      partition.address = CONFIG_PARTITION_TABLE_OFFSET;
      partition.size = 4096;
      strcpy(partition.label, "partition table");
      partition.encrypted = false;
      break;

    default:
      const esp_partition_t* p
             (esp_partition_find_first( (esp_partition_type_t)(menuItem->tag >> 8)
                                      , (esp_partition_subtype_t)(menuItem->tag & 0xFF)
                                      , NULL)
             );
      if (p == NULL) return false;
      partition = *p;
    }

    dataSize = partition.size;

    lcd.setTextColor(0xFFFF);
    for (int i = 1; i < 16; ++i) {
      lcd.drawFastHLine(0, 10 + i, lcd.width(), (i << 12) | (i << 6));
    }
    lcd.drawString("Binary Viewer (FLASH)", 10, 10, &fonts::Font2);

    lcd.drawString("0x" + String(partition.address, HEX) + ":" + partition.label + (partition.encrypted ? " encrypted" : ""), 0, 30, &fonts::Font2);

    return BinaryViewer::setup();
  }

  virtual int getData(int pos, uint8_t* buf) {
    if (pos >= partition.size) return 0;
    if (!ESP.flashRead(partition.address + pos, (uint32_t*)buf, 16)) return 0;
    return 16;
  }
};
#endif
