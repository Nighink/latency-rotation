#include <mbed.h>
#include <string>
#include "diode.hpp"

Serial pc(USBTX, USBRX); // tx, rx

DigitalOut led1(LED1);

Diode diodes[] = {
    Diode(A0, 0),
    Diode(A1, 1),
    Diode(A2, 2),
    Diode(A3, 3),
};

void command(string &cmd) {
  if (cmd == "") {
    for (uint8_t i = 0; i < 4; i++) {
      diodes[i].load_calibration();
      diodes[i].print_loaded_calibration(pc);
    }
    led1 = !led1;
  } else if (cmd == "test1") {
    HAL_StatusTypeDef status;
    uint32_t address = 0x08080000 + 10 * 2 * sizeof(uint32_t);
    uint32_t data = 0x57F9;
    HAL_FLASHEx_DATAEEPROM_Unlock(); // Unprotect the EEPROM to allow writing
    status =
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, address, data);
    HAL_FLASHEx_DATAEEPROM_Lock(); // Reprotect the EEPROM
    pc.printf("Writing: %d\r\n", data);
    pc.printf("Write result: %d\r\n", status);
    __IO uint32_t *data_read = reinterpret_cast<__IO uint32_t *>(address);
    pc.printf("Read: %d\r\n", *data_read);
  }
  cmd.clear();
}

int main() {
  pc.baud(115200);
  led1 = 1;
  pc.printf("\r\n");

  for (uint8_t i = 0; i < 4; i++) {
    diodes[i].load_calibration();
    diodes[i].print_loaded_calibration(pc);
  }

  string cmd_buffer(50, 0);
  while (true) {
    if (pc.readable()) {
      int c = pc.getc();
      switch (c) {
      case EOF:
        break;
      case '\r':
        break;
      case '\n':
        command(cmd_buffer);
        break;
      default:
        cmd_buffer.push_back(c);
        break;
      }
    }
  }
}