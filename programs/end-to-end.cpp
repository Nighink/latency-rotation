#include "constants.hpp"
#include "diode.hpp"
#include <DevSPI.h>
#include <L6474.h>
#include <mbed.h>
#include <string>

L6474 *motor;
Ticker photo_timer;
Serial pc(USBTX, USBRX); // tx, rx
DiodeNumber diodes;
const uint16_t SAMPLES = 16560;

struct Deviation
{
  uint16_t angle;
  uint16_t timestamp;
};

Deviation deviations[SAMPLES] = {0};
uint16_t current_deviation = 0;

uint16_t current_timestamp;

inline void stop()
{
  motor->soft_stop();
  photo_timer.detach();
}

uint16_t initial_difference = 0;
int16_t debug_num = 10000;
void read_photodiodes()
{
  current_timestamp++;
  int16_t number = diodes.read();
  if (number >= 0)
  {
    debug_num = number;
    if (current_deviation < SAMPLES)
    {
      int should_number = motor->get_position() % 200;
      int is_number = number;
      deviations[current_deviation].angle =
          (should_number + 200 - is_number) % 200;
      deviations[current_deviation].timestamp = current_timestamp;
      current_deviation++;
    }
  }
  else if (current_deviation == SAMPLES)
  {
    stop();

    pc.printf("[[%d, %d]", deviations[0].timestamp, deviations[0].angle);
    for (uint16_t i = 1; i < SAMPLES; i++)
    {
      pc.printf(",[%d, %d]", deviations[i].timestamp, deviations[i].angle);
    }
    pc.printf("]\r\n");
  }
}

inline void start()
{
  current_timestamp = 0;
  current_deviation = 0;
  motor->set_home();
  photo_timer.attach_us(&read_photodiodes, DIODE_READ_US);
  motor->run(StepperMotor::FWD);
}

void read_initial_difference()
{
  int16_t number = diodes.read();
  if (number >= 0)
  {
    initial_difference = number;
    photo_timer.detach();
    start();
  }
}

void command(string &cmd)
{
  int steps;
  if (cmd == "start")
  {
    photo_timer.attach_us(&read_initial_difference, DIODE_READ_US);
  }
  else if (cmd == "stop")
  {
    stop();
  }
  else if (cmd == "debug")
  {
    pc.printf("Last retrieved number: %d\r\n", debug_num);
  }
  else if (sscanf(cmd.c_str(), "rotate %d", &steps) == 1)
  {
    motor->move(StepperMotor::FWD, steps);
    motor->wait_while_active();
    motor->set_home();
  }
  cmd.clear();
}

int main()
{
  pc.baud(115200);
  pc.printf("\r\n");

  DevSPI dev_spi(D11, D12, D13);
  motor = new L6474(D2, D8, D7, D9, D10, dev_spi);
  if (motor->init(&motor_init) != COMPONENT_OK)
  {
    exit(EXIT_FAILURE);
  }

  string cmd_buffer(50, 0);
  while (true)
  {
    if (pc.readable())
    {
      int c = pc.getc();
      switch (c)
      {
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