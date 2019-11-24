#include "constants.hpp"
#include "diode.hpp"
#include <DevSPI.h>
#include <L6474.h>
#include <cstring>
#include <mbed.h>
#include <string>

Ticker calibration_timer;
Serial pc(USBTX, USBRX); // tx, rx

DigitalOut led1(LED1);

Diode diodes[] = {
    Diode(A0, 0),
    Diode(A1, 1),
    Diode(A2, 2),
    Diode(A3, 3),
};

int current_read[] = {0, 0, 0, 0};
uint16_t read_count = 0;
bool gathering = false;
const uint32_t FRAME_DURATION_APPROX_MS = 11;
const uint32_t FRAMES_PER_CALIBRATION = 20;

void set_waiting_for_next_light_flank(){
  for(uint8_t d = 0; d < 4; d++) {
      diodes[d].next_calibration();
    }
}

void calibrate() {
  for (uint8_t i = 0; i < 4; i++) {
    diodes[i].calibrate(current_read[i]);
  }
  read_count++;
  if (read_count >= FRAMES_PER_CALIBRATION * FRAME_DURATION_APPROX_MS * 1000 /
                        DIODE_READ_US) {
    calibration_timer.detach();
    gathering = false;
    pc.printf("ok\r\n");
  }
}

void calibrate_black() {
  for (uint8_t i = 0; i < 4; i++) {
    diodes[i].calibrate_black();
  }
  read_count++;
  if (read_count >= FRAMES_PER_CALIBRATION * 2 * FRAME_DURATION_APPROX_MS *
                        1000 / DIODE_READ_US) {
    calibration_timer.detach();
    gathering = false;
    pc.printf("ok\r\n");
  }
}

bool PRINT_JSON = false;

void command(string &cmd) {
  if (cmd == "end") {
    if (PRINT_JSON) {
      pc.printf("\r\n[");
      diodes[0].print_calibration(pc);
      for (uint8_t i = 1; i < 4; i++) {
        pc.printf("\r\n,");
        diodes[i].print_calibration(pc);
      }
      pc.printf("]\r\n");
    } else {
      for (uint8_t i = 0; i < 4; i++) {
        diodes[i].print_calibration_plain(pc);
      }
    }
  } else if (cmd == "black") {
    read_count = 0;
    calibration_timer.attach_us(&calibrate_black, DIODE_READ_US);
  } else if (cmd == "save") {
    for (uint8_t i = 0; i < 4; i++) {
      diodes[i].save_calibration();
    }
    pc.printf("ok\r\n");
  } else if (cmd == "hello") {
    pc.printf("ok\n");
  } else if (sscanf(cmd.c_str(), "%d-%d-%d-%d", &current_read[0],
                    &current_read[1], &current_read[2],
                    &current_read[3]) == 4) {
    read_count = 0;
    set_waiting_for_next_light_flank();
    calibration_timer.attach_us(&calibrate, DIODE_READ_US);
  } else if (cmd == "print_json") {
    PRINT_JSON = true;
  } else if (cmd == "print_plain") {
    PRINT_JSON = false;
  } else if (cmd == "reset") {
    for (uint8_t i = 0; i < 4; i++) {
      diodes[i].calibrate_reset();
    }
    pc.printf("ok\r\n");
  } else if (cmd == "print_saved_calibration") {
    for (uint8_t i = 0; i < 4; i++) {
      diodes[i].print_loaded_calibration(pc);
    }
    pc.printf("ok\r\n");
  } else if (cmd == "print_black_calibration") {
    for (uint8_t i = 0; i < 4; i++) {
      pc.printf("Black %d: %d\r\n", i, diodes[i].m_calibration[0]);
    }
  } else {
    pc.printf("Unknown msg %s\r\n", cmd.c_str());
  }
  cmd.clear();
}

int main() {
  pc.baud(115200);
  led1 = 1;
  pc.printf("\r\n");
  pc.printf("init\r\n");

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