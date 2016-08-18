#include "mbed.h"

#define NUM_ROWS 4
#define NUM_COLS 4

#define NUM_LIGHTS 4
#define PWM_FREQ 3000  // Default PWM frequency (Hz)
#define BAUD 115200    // Serial communication rate
#define ADC_DELAY 1000 // ADC interrupt delay (us)
#define SERIAL_SEND_DELAY 1000 // Serial send interrupt delay (us);

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

// USB data rx
char rxBuffer[100] = {0};
int rxBufferIndex = 0;
unsigned char dataReady = 1;
char okflag=0;
char flag4send=0;
char sendrowindex=0;
// Truth table for 16:1 row mux
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

// The current row of the mux
int row = 0;

// Sensor data matrix
int sensors[NUM_ROWS][NUM_COLS] = {0};
int sensorLock = 0; // synchronization

// Serial/ADC interrupt timers
Ticker adcRead;

/*******************************************************************************
 * Sets all lights' PWM channels to the desired duty cycles
 * @param dutyCycles Duty cycles (between 0 and 50) out of 100
 ******************************************************************************/
void setLights(int dutyCycles[NUM_LIGHTS]) {
    for (int i = 0; i < NUM_LIGHTS; i++) {
        lights[i]->write(dutyCycles[i] / 100.0);    
    }
}

/*******************************************************************************
 * Sets an individual light's PWM channel to the desired duty cycle
 * @param light The ID of the light (between 0 and NUM_LIGHTS - 1, inclusive)
 * @param dutyCycle Duty cycle (between 0 and 50) out of 100
 ******************************************************************************/
void setLight(int light, int dutyCycle) {
    if (light >= 0 && light < NUM_LIGHTS) lights[light]->write(dutyCycle / 100.0);    
}

/*******************************************************************************
 * Sets the pressure matrix's row mux to the specified channel
 * @param channel The desired channel (between 0 and NUM_ROWS - 1, inclusive)
 ******************************************************************************/
void setRowMux(int channel) {
    mux0 = rowMux[channel][3];
    mux1 = rowMux[channel][2];
    mux2 = rowMux[channel][1];
    mux3 = rowMux[channel][0];
}

/*******************************************************************************
 * Callback function called when the serial port receives data.
 * If the received character is a newline, instruct the main loop that data
 * is ready to be read from rxBuffer by setting dataReady to 1.
 * Otherwise, add the received character to rxBuffer.
 ******************************************************************************/
void serialInterrupt() {
    char received = pc.getc();
    pc.printf("In serial interrupt\r\n");
    if (received == '\r' || received == '\n') {   
        rxBuffer[rxBufferIndex] = '\0';
        rxBufferIndex = 0;
        dataReady = 1;
    } 
    else if(received=='!') okflag=1;
    else {
        rxBuffer[rxBufferIndex] = received; 
        rxBufferIndex++;
        if (rxBufferIndex > 100) rxBufferIndex = 100;
    }
    led1.write(!led1);
}

/*******************************************************************************
 * Trims val to be between lower and upper, inclusive.
 * @param val The value to be trimmed
 * @param lower The lower bound
 * @param upper The upper bound
 ******************************************************************************/
void trim(int *val, int lower, int upper) {
    if (val != NULL) {
        if (*val < lower) *val = lower;
        else if (*val > upper) *val = upper;    
    }    
}

/*******************************************************************************
 * Callback function.
 * Sends the sensor matrix data over the serial port to the PC.
 ******************************************************************************/


/*******************************************************************************
 * Callback function.
 * Read the four ADC channels in parallel, save the data into the sensor matrix,
 * and set the muxes for the next reading.
 ******************************************************************************/
void readAdcs() {
   
        sensors[row][0] = ain0.read_u16();
        sensors[row][1] = ain1.read_u16();
        sensors[row][2] = ain2.read_u16();
        sensors[row][3] = ain3.read_u16();
        row++;
        if(row!=0 && row%2==0) flag4send=1;
        if (row >= NUM_ROWS) row = 0;
        setRowMux(row);
    
}

/*******************************************************************************
 * Sets up the serial port for communication and PWM for the lights, and
 * initializes the device.
 ******************************************************************************/
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
    
    row = 0;
    setRowMux(row);
    
   
    adcRead.attach_us(&readAdcs, ADC_DELAY);
}

/*******************************************************************************
 * Main loop.
 * Initialize the system and process commands when they are received via the
 * serial port.
 ******************************************************************************/
int main() {
    char temp;
    setup();
    
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
        
        
        //when the flag is set, send two rows over to the pc
        if(flag4send){
            //send start character 
            pc.putc('s');
            //send two rows
            for(char sendrow=0;sendrow<2;sendrow++){
                //send all the columns
                for(char sendcol=0;sendcol<4;sendcol++){
                    //send the top 8 bits
                    temp=sensors[sendrowindex][sendcol]>>8;
                    pc.putc(temp);
                    while(okflag==0);
                    okflag=0;
                    //send the bottom 8 bits
                    temp=sensors[sendrowindex][sendcol];
                    pc.putc(temp);
                    while(okflag==0);
                    okflag=0;
                }
                sendrowindex++;
                //reset row index once the max is reached
                if(sendrowindex>=NUM_ROWS)sendrowindex=0;
             }
             flag4send=0;
        }   
   }  
}
