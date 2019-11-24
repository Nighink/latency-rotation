/*
 * Wanted to try if I can make the light patterns
 * more easy to distinguish. Looks like the problem
 * I had with the calibration before was fixed with a restart
 * so this here is not needed.
 */
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

const uint16_t SAMPLES = 400;
uint16_t readings[4][SAMPLES] = {{0}, {0}, {0}, {0}};
uint16_t last_reading[4] = {0};
uint16_t current_sample = 0;
uint16_t sample_counter = 0;
AnalogIn pins[] = {AnalogIn(A0), AnalogIn(A1), AnalogIn(A2), AnalogIn(A3)};
uint16_t black_max[4] = {0};

void (*complete_function)();

void print_samples() {
  pc.printf("{");
  bool first_d = true;
  for (uint8_t d = 0; d < 4; d++) {
    if (!first_d) {
      pc.printf(",");
    } else {
      first_d = false;
    }
    pc.printf("\"d%d\":[", d);
    bool first_s = true;
    for (uint16_t s = 1; s < SAMPLES; s++) {
      if (!first_s) {
        pc.printf(",");
      } else {
        first_s = false;
      }
      pc.printf("%d", readings[d][(current_sample + s + 1) % SAMPLES]);
    }
    pc.printf("]\r\n");
  }
  pc.printf("}\r\n");
}

void black() {
  printf("{");
  bool first = true;
  for (uint8_t d = 0; d < 4; d++) {
    if(first){
      first = false;
    } else{
      printf(",");
    }
    uint16_t b_min = -1, b_max = 0;
    uint32_t acc = 0;
    for (uint16_t s = 1; s < SAMPLES; s++) {
      auto val = readings[d][(current_sample + s + 1) % SAMPLES];
      b_min = min(b_min, val);
      b_max = max(b_max, val);
      black_max[d] = b_max; // save as calibration
      acc += val;
    }
    printf("\"d%d\": {\"min\": %d, \"max\": %d, \"avg\": %d}\r\n", d, b_min, b_max,
           static_cast<uint16_t>(acc / static_cast<uint32_t>(SAMPLES)));
    black_max[d] = static_cast<uint16_t>(static_cast<float>(b_max)*1.15); // save as calibration
  }
  printf("}\r\n");
}

void calibrate() {
  current_sample = (current_sample + 1) % SAMPLES;
  for (uint8_t d = 0; d < 4; d++) {
    uint16_t brightness = pins[d].read_u16();
    readings[d][current_sample] = (last_reading[d] + brightness) / 2;
    last_reading[d] = brightness;

    if(readings[d][current_sample] < black_max[d]){
      
    }
  }
  sample_counter++;
  if (sample_counter > SAMPLES * 2) {
    calibration_timer.detach();
    complete_function();
  }
}

void command(string &cmd) {
  led1 = !led1;
  if (cmd == "print_samples") {
    complete_function = print_samples;
    calibration_timer.attach_us(calibrate, DIODE_READ_US);
    sample_counter = 0;
  } else if (cmd == "black") {
    complete_function = black;
    for(uint8_t d = 0; d < 4; d++){
      black_max[d] = 0;
    }
    calibration_timer.attach_us(calibrate, DIODE_READ_US);
    sample_counter = 0;
  } else {
    pc.printf("Unknown msg %s\r\n", cmd.c_str());
  }
  cmd.clear();
}

int main() {
  pc.baud(115200);
  led1 = 1;
  pc.printf("\r\n");

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