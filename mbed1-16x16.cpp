#include "mbed.h"

#define NUM_ROWS 16
#define NUM_COLS 16

#define NUM_LIGHTS 12
#define PWM_FREQ 3000  // Default PWM frequency (Hz)
#define BAUD 921600   // Serial communication rate
#define ADC_DELAY 5000 // ADC interrupt delay (us)
#define SERIAL_SEND_DELAY 12000 // Serial send interrupt delay (us);
#define RX_BUFSIZE 2 // USB serial buffer size (NOT a string)

#define SPI_MOSI p11
#define SPI_MISO p12
#define SPI_SCK  p13
#define SPI_CS   p14

Serial pc(USBTX, USBRX);

// SPI for lights 6-11
SPI spi(SPI_MOSI, SPI_MISO, SPI_SCK);
DigitalOut cs(SPI_CS);
 
// debug LEDs
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

// PWM for controlling the lights' brightness
PwmOut *lights[6];

// ADC inputs for reading the sensors
AnalogIn ain0(p17);
AnalogIn ain1(p18);
AnalogIn ain2(p19);
AnalogIn ain3(p20);

// Mux select lines
DigitalOut muxC0(p15);
DigitalOut muxC1(p16);

DigitalOut mux0(p30);
DigitalOut mux1(p29);
DigitalOut mux2(p28);
DigitalOut mux3(p27);
volatile char start=0;

// USB data rx
char rxBuffer[RX_BUFSIZE] = {0};
int rxBufferIndex = 0;
char newflag=1;
unsigned char dataReady = 1;
volatile char okflag=0;
volatile char flag4send=0;
char sendrowindex=0;

// is PC sending a light command?
char controlLight = 0;

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


//truth table for 4:1 mux
unsigned const char colMux[4][2]={
    {0,0},
    {0,1},
    {1,0},
    {1,1}
};

// The current row of the mux
int row = 0;

// Sensor data matrix
uint16_t sensors[NUM_ROWS][NUM_COLS] = {0};
int sensorLock = 0; // synchronization

// Serial/ADC interrupt timers
Ticker adcRead;

/*******************************************************************************
 * Sets all lights' PWM channels to the desired duty cycles
 * @param dutyCycles Duty cycles (between 0 and 50) out of 100
 ******************************************************************************/
void setLights(int dutyCycles[NUM_LIGHTS]) {
    // lights on the master (this mBed)
    for (int i = 0; i < 6; i++) {
        lights[i]->write(dutyCycles[i] / 100.0);    
    }
    
    // lights on the slave
    for (int i = 6; i < NUM_LIGHTS; i++) {
        cs = 0;
        spi.write(i); // panel
        spi.write(dutyCycles[i]);
        cs = 1;
    }
}

/*******************************************************************************
 * Sets an individual light's PWM channel to the desired duty cycle
 * @param light The ID of the light (between 0 and NUM_LIGHTS - 1, inclusive)
 * @param dutyCycle Duty cycle (between 0 and 50) out of 100
 ******************************************************************************/
void setLight(int light, int dutyCycle) {
    if (light >= 0 && light < 6) lights[light]->write(dutyCycle / 100.0);
    else if (light >= 6 && light < NUM_LIGHTS) {
        cs = 0;
        spi.write(light);
        spi.write(dutyCycle);
        cs = 1;
    }
}


/*******************************************************************************
 * Sets the pressure matrix's column mux to the specified channel
 * @param channel The desired channel (between 0 and 3 inclusive)
 ******************************************************************************/
void setColMux(int channel) {
  muxC1=colMux[channel][0];
  muxC0=colMux[channel][1];
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
       
    if (received == '>' && start == 0) { // system start
        start = 1;
    } else if (received == 's') { // control a light panel4
        rxBufferIndex = 0;
        controlLight = 1;
    } else {
        if (controlLight) {
            rxBuffer[rxBufferIndex] = received; 
            rxBufferIndex++;
            if (rxBufferIndex >= RX_BUFSIZE) {
                rxBufferIndex = 0;
                controlLight = 0;
                dataReady = 1;
            }
        } else {
            rxBufferIndex = 0;    
        }
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
        for(int k=0; k<4;k++){
        sensors[row][k] = ain0.read_u16();
    
        sensors[row][k+4] = ain1.read_u16();
        
        sensors[row][k+4+4] = ain2.read_u16();
      
        sensors[row][k+4+4] = ain3.read_u16();
        setColMux(k);
        }
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
    
    // Setup the spi for 8 bit data, high steady state clock,
    // second edge capture, with a 1MHz clock rate
    cs = 1;
    spi.format(8,3);
    spi.frequency(1000000);
    
    // set up the lights on this mBed
    // light initialization on slave mBed handled by its own setup code
    lights[0] = new PwmOut(p26);
    lights[1] = new PwmOut(p25);
    lights[2] = new PwmOut(p24);
    lights[3] = new PwmOut(p23);
    lights[4] = new PwmOut(p22);
    lights[5] = new PwmOut(p21);
    
    for (int i = 0; i < 6; i++) {
        lights[i]->period_us(333); // 3 kHz
        lights[i]->write(0.5);   // 50% duty cycle
    }
    
    row = 0;
    setRowMux(row);
   led3.write(0);
   while(start==0);
    
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
            char light2 = rxBuffer[0], DC = rxBuffer[1];
            setLight((int)light2, (int)DC);
        } else {
            led2.write(0);    
        }
        
        
        //when the flag is set, send two rows over to the pc
        if(flag4send){
       
            
      
            pc.putc('s');
            wait_us(100);
            //send two rows
            for(char sendrow=0;sendrow<2;sendrow++){
                //send all the columns
                for(char sendcol=0;sendcol<NUM_COLS;sendcol++){
                    //send the top 8 bits
                    temp=sensors[sendrowindex][sendcol]>>8;
                    pc.putc(temp);

                    wait_us(100);
                    //send the bottom 8 bits
                    temp=sensors[sendrowindex][sendcol];
                    pc.putc(temp);
                    wait_us(100);
                }
                sendrowindex++;
                //reset row index once the max is reached
                if(sendrowindex>=NUM_ROWS)sendrowindex=0;
             }
             flag4send=0;
        }   
   }  
}
