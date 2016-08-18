#include "mbed.h"
#include <SPISlave.h>
char ELbuffer[3];
char bufcount=0;
float PWM;
SPISlave mbed2(p11,p12,p13,p14);
PwmOut EL6(p21);
PwmOut EL7(p22);
PwmOut EL8(p23);
PwmOut EL9(p24);
PwmOut EL10(p25);
PwmOut EL11(p26);


int main(){
    //format SPI
    mbed2.format(8,0);
    mbed2.frequency(30000000);
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
                            
    
 while(1){
    
    if(mbed2.receive()){
        //read the data from the 
        ELbuffer[bufcount]=mbed2.read();
        bufcount++;
        if(bufcount==1 && ELbuffer[0]!=0xFF) bufcount=0;
        else if(bufcount==3){
            bufcount=0;
            PWM=ELbuffer[2]/(100.0);
            //logic for determining which EL pannel to light up
            if(ELbuffer[1]==6)  EL6.write(PWM);
            else if(ELbuffer[1]==7)EL7.write(PWM);
            else if(ELbuffer[1]==8)EL8.write(PWM);
            else if(ELbuffer[1]==9)EL9.write(PWM);
            else if(ELbuffer[1]==10)EL10.write(PWM);
            else if(ELbuffer[1]==11)EL11.write(PWM);
            
        }
    
    }   
  }  
}
