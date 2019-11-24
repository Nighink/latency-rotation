#include <mbed.h>
#include <string>
#include "constants.hpp"
#include "diode.hpp"

Serial pc(USBTX, USBRX); // tx, rx
Ticker photo_timer;
DigitalOut led1(LED1);
DiodeNumber diodes;

int16_t debug_num = 10000;
bool have_num = false;
int16_t next_print = 1000;

void update()
{
  int16_t number = diodes.read();
  if (number >= 0)
  {
    debug_num = number;
    have_num = true;
  }
  next_print--;
  if (next_print <= 0)
  {
    if (have_num)
    {
      pc.printf("Num: %d\r\n", debug_num);
      have_num = false;
    }
    else
    {
      pc.printf("Nothing read\r\n");
    }
    next_print = 1000;
  }
}

void command(string &cmd)
{
  if (cmd == "debug")
  {
    pc.printf("Last retrieved number %d\r\n", debug_num);
  }
  cmd.clear();
}

int main()
{
  pc.baud(115200);
  led1 = 1;
  photo_timer.attach_us(&update, DIODE_READ_US);

  pc.printf("Testing Numbers\r\n");
  pc.printf("Please change the light numbers at will\r\n");

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