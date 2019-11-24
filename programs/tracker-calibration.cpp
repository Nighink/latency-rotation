#include "constants.hpp"
#include <DevSPI.h>
#include <L6474.h>
#include <mbed.h>
#include <string>

L6474 *motor;
Serial pc(USBTX, USBRX); // tx, rx

void command(string &cmd) {
  int steps;
  if (cmd == "stop") {
    motor->soft_stop();
    motor->wait_while_active();
  } else if (cmd == "go") {
    motor->enable();
    motor->run(StepperMotor::FWD);
  } else if (sscanf(cmd.c_str(), "rotate %d", &steps) == 1) {
    motor->move(StepperMotor::FWD, steps);
    motor->wait_while_active();
    motor->set_home();
  } else if (cmd == "end") {
    motor->disable();
  }
  cmd.clear();
}

int main() {
  pc.baud(115200);
  pc.printf("\r\n");
  /* Initializing SPI bus. */
  DevSPI dev_spi(D11, D12, D13);

  /* Initializing Motor Control Component. */
  motor = new L6474(D2, D8, D7, D9, D10, dev_spi);
  if (motor->init(&motor_init) != COMPONENT_OK) {
    exit(EXIT_FAILURE);
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
