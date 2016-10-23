#include "mbed.h"
#include <SPISlave.h>

#define SPI_MOSI p11
#define SPI_MISO p12
#define SPI_SCK  p13
#define SPI_CS   p14

char ELbuffer[3];
char bufcount=0;
float PWM;

SPISlave mbed2(SPI_MOSI,SPI_MISO,SPI_SCK,SPI_CS);
Serial pc(USBTX, USBRX);

PwmOut EL6(p26);
PwmOut EL7(p25);
PwmOut EL8(p24);
PwmOut EL9(p23);
PwmOut EL10(p22);
PwmOut EL11(p21);

DigitalOut led1(LED1);


int main(){
    led1 = 0;
    //format PWM, 333us period=3kHz
    EL6.period_us(333);
    EL7.period_us(333);
    EL8.period_us(333);
    EL9.period_us(333);
    EL10.period_us(333);
    EL11.period_us(333);
    //set duty cycles to 0 just to be sure
    EL6.write(0);
    EL7.write(0);
    EL8.write(0);
    EL9.write(0);
    EL10.write(0);
    EL11.write(0);
    
    // Setup the spi for 8 bit data, high steady state clock,
    // second edge capture, with a 1MHz clock rate
    mbed2.format(16,3);
    mbed2.frequency(1000000);
                            
    
 while(1){
    
    if(mbed2.receive()){
        //read the data from the 
        uint16_t received = mbed2.read();
        uint8_t channel = received >> 8;
        uint8_t dutyCycle = (received & 0x00FF);
        switch (channel) {
            case 6: EL6.write(dutyCycle / 100.0); break;
            case 7: EL7.write(dutyCycle / 100.0); break;
            case 8: EL8.write(dutyCycle / 100.0); break;
            case 9: EL9.write(dutyCycle / 100.0); break;
            case 10: EL10.write(dutyCycle / 100.0); break;
            case 11: EL11.write(dutyCycle / 100.0); break;
        }    
        pc.printf("channel %d, duty cycle %d\r\n", channel, dutyCycle);
    
    }   
  }  
}
