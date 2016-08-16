#include "mbed.h"

#define NUM_ROWS 4
#define NUM_COLS 4

#define NUM_LIGHTS 4
#define PWM_FREQ 120
#define BAUD 115200

Serial pc(USBTX, USBRX);
DigitalOut led1(LED1);
DigitalOut led2(LED2);

// PWM for controlling the lights' brightness
PwmOut *lights[NUM_LIGHTS];

// ADC inputs for reading the sensors
AnalogIn ain0(p15);
AnalogIn ain1(p16);
AnalogIn ain2(p17);
AnalogIn ain3(p18);

// Mux select lines
DigitalOut mux0(p14);
DigitalOut mux1(p13);
DigitalOut mux2(p12);
DigitalOut mux3(p11);

char rxBuffer[100] = {0};
int rxBufferIndex = 0;
unsigned char dataReady = 1;

unsigned const char rowMux[16][4] = {
    {0, 0, 0, 0},
    {0, 0, 0, 1},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 1, 0, 0},
    {0, 1, 0, 1},
    {0, 1, 1, 0},
    {0, 1, 1, 1},
    {1, 0, 0, 0},
    {1, 0, 0, 1},
    {1, 0, 1, 0},
    {1, 0, 1, 1},
    {1, 1, 0, 0},
    {1, 1, 0, 1},
    {1, 1, 1, 0},
    {1, 1, 1, 1}
};

int sensors[NUM_ROWS][NUM_COLS] = {0};

void setLights(int dutyCycles[NUM_LIGHTS]) {
    for (int i = 0; i < NUM_LIGHTS; i++) {
        lights[i]->write(dutyCycles[i] / 100.0);    
    }
}

void setLight(int light, int dutyCycle) {
    if (light >= 0 && light < NUM_LIGHTS) lights[light]->write(dutyCycle / 100.0);    
}

void setRowMux(int channel) {
    mux0 = rowMux[channel][3];
    mux1 = rowMux[channel][2];
    mux2 = rowMux[channel][1];
    mux3 = rowMux[channel][0];
}

void serialInterrupt() {
    char received = pc.getc();
    pc.printf("In serial interrupt\r\n");
    if (received == '\r' || received == '\n') {   
        rxBuffer[rxBufferIndex] = '\0';
        rxBufferIndex = 0;
        dataReady = 1;
    } else {
        rxBuffer[rxBufferIndex] = received; 
        rxBufferIndex++;
        if (rxBufferIndex > 100) rxBufferIndex = 100;
    }
    led1.write(!led1);
}

void setup() {
    pc.baud(BAUD);
    pc.attach(&serialInterrupt);
    
    lights[0] = new PwmOut(p26);
    lights[1] = new PwmOut(p25);
    lights[2] = new PwmOut(p24);
    lights[3] = new PwmOut(p23);
    
    for (int i = 0; i < NUM_LIGHTS; i++) {
        lights[i]->period_us(333); // 3 kHz
        lights[i]->write(0.5);   // 50% duty cycle
    }
    
    setRowMux(3);
}

void trim(int *val, int lower, int upper) {
    if (val != NULL) {
        if (*val < lower) *val = lower;
        else if (*val > upper) *val = upper;    
    }    
}

void sendSensorData() {
    //pc.printf("SENSOR_DATA_START\r\n");
    for (int i = 0; i < NUM_ROWS; i++) {
        pc.printf("%d %d %d %d %d\r\n", i, sensors[i][0], sensors[i][1], sensors[i][2], sensors[i][3]);
        //for (int j = 0; j < NUM_COLS; j++) {
        //    pc.printf("%d %d %d\r\n", i, j, sensors[i][j]);    
        //}
    }
    //pc.printf("SENSOR_DATA_END\r\n");    
}

int main() {
    int elapsedTime = 0;
    Timer timer;
    setup();
    timer.start();
    
    while (1) {
        
        
        if (dataReady) {
            led2.write(1);
            dataReady = 0;
            int light = 0, dutyCycle = 50, mux = 0;
            if (sscanf(rxBuffer, "MUX %d", &mux) == 1) {
                trim(&mux, 0, NUM_ROWS - 1);
                setRowMux(mux);
                pc.printf("Mux set to %d\r\n", mux);
            } else if (sscanf(rxBuffer, "SET_LIGHT %d %d", &light, &dutyCycle) == 2) {
                trim(&light, 0, NUM_LIGHTS - 1);
                trim(&dutyCycle, 0, 50);
                setLight(light, dutyCycle);
                pc.printf("Duty cycle on light %d set to #d\r\n", light, dutyCycle);
            } else if (sscanf(rxBuffer, "SET_LIGHTS %d", &dutyCycle) == 1) {
                trim(&dutyCycle, 0, 50);
                for (int i = 0; i < NUM_LIGHTS; i++) {
                    setLight(i, dutyCycle);    
                }
                pc.printf("Duty cycle on all lights changed to %d\r\n", dutyCycle);
            } else {
                pc.printf("Invalid command entered\r\n");    
            }
        } else {
            led2.write(0);    
        }
        
        
        
        
        // iterate through each row, set the mux, and read the columns
        for (int i = 0; i < NUM_ROWS; i++) {
            int delay = 500;
            setRowMux(i);
            wait_us(delay);
            sensors[i][0] = ain0.read_u16();
            sensors[i][1] = ain1.read_u16();
            sensors[i][2] = ain2.read_u16();
            sensors[i][3] = ain3.read_u16();
            //pc.printf("V0 = %d | V1 = %d | V2 = %d | V3 = %d\r\n", 
            //    sensors[i][0], sensors[i][1], sensors[i][2], sensors[i][3]);
        }
        
        sendSensorData();
        int currentTime = timer.read_us();
        pc.printf("Elapsed time (us): %d\r\n", currentTime - elapsedTime);
        elapsedTime = currentTime;
    }
}
