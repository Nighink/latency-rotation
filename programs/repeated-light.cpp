#include <DevSPI.h>
#include <L6474.h>
#include <mbed.h>
#include "constants.hpp"

// Ticker flipper;
Ticker read_ticker;
Serial pc(USBTX, USBRX); // tx, rx
Timer read_timer;

AnalogIn ain[] = {
    AnalogIn(A0),
    AnalogIn(A1),
    AnalogIn(A2),
    AnalogIn(A3)};
DigitalOut led1(LED1);

const unsigned int SAMPLES = 400;
unsigned short data[SAMPLES * 4];
unsigned int data_i = 0;

void flip() { led1 = !led1; }

void read_diode()
{
  if (data_i < SAMPLES)
  {
    for (unsigned short i = 0; i < 4; i++)
    {
      data[data_i * 4 + i] = ain[i].read_u16();
    }
    data_i++;
  }
  else
  {
    pc.printf("[");
    bool first = true;
    for (unsigned int i = 0; i < SAMPLES; i++)
    {
      if (!first)
      {
        pc.printf(",");
      }
      else
      {
        first = false;
      }
      pc.printf("[%d,%d,%d,%d]", data[i * 4 + 0], data[i * 4 + 1],
                data[i * 4 + 2], data[i * 4 + 3]);
    }
    pc.printf("]\n");
    read_ticker.detach();
  }
}

int main()
{
  pc.baud(115200);
  led1 = 1;

  char read_buffer[32];
  while (1)
  {
    if (pc.gets(read_buffer, 31) > 0 && strncmp(read_buffer, "go", 2) == 0)
    {

      data_i = 0;
      read_ticker.attach_us(&read_diode, DIODE_READ_US);
    }
  }
}