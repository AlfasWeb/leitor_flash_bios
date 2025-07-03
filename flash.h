#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>
#include <SPI.h>

#define PIN_CS    5
#define PIN_MOSI  23
#define PIN_MISO  19
#define PIN_SCK   18

#define FLASH_SECTOR_SIZE    4096
#define FLASH_PAGE_SIZE      256

void iniciarSPIFlash();
void identificarJEDEC();
void enableWrite();
void waitBusy();
void eraseSector(uint32_t addr);
void writePage(uint32_t addr, const uint8_t* data, size_t len);
void readData(uint32_t addr, uint8_t* buffer, size_t len);
void limparFlashParcial(uint32_t addrIni, uint32_t addrFim);

#endif
