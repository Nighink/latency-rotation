#include <DevSPI.h>
#include <L6474.h>
#include <mbed.h>

Ticker flipper;
Serial pc(USBTX, USBRX); // tx, rx

DigitalOut led1(LED1);

void flip() { led1 = !led1; }

int main() {
  pc.baud(115200);
  led1 = 1;
  flipper.attach(&flip, 2.0); // the address of the function to be attached
  // (flip) and the interval (2 seconds)
  // put your setup code here, to run once:

  while (1) {
  }
}