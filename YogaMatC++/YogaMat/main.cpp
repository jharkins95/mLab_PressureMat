#include<io.h>
#include<stdio.h>
#include <iostream>
#include <string>
#include <cmath>
#include <math.h>
#include <sstream>
#include <fstream>
#include <windows.h>
#include <conio.h>

#define NUMROWS 4
#define NUMCOL 4
#define ELEMS 4.0
char result;
int EL1 = 0, EL2 = 0;
void sendlight(char light, char PWM);
long scan=1;
using namespace std;
// serial handle 
HANDLE serialh;
char rec;
char okaychar = '!';  
uint16_t sensordata[4][4];
int recrow = 0, reccol = 0, writerow = 0, writecol = 0;
long linecounter = 0;
ofstream datalog("MatData.txt");

int main() {
	//open Com
	serialh = CreateFile("\\\\.\\COM11", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	// Set params
	DCB serialparams = { 0 };
	serialparams.DCBlength = sizeof(serialparams);
	GetCommState(serialh, &serialparams);
	serialparams.BaudRate = 921600;
	serialparams.ByteSize = 8;
	serialparams.StopBits = 1;
	serialparams.Parity = 0;
	SetCommState(serialh, &serialparams);
	char start = '>';
	WriteFile(serialh, &start, 1, NULL, NULL);
	while (1) {
		EL1 = 0;
		EL2 = 0;

		//wait for the character to tell pc to start reading
		while (rec != 's') ReadFile(serialh, &rec, 1, NULL, NULL);

		for (char i = 0; i < 2; i++) {
			//receicve the data for each element
			for (char j = 0; j < NUMCOL; j++) {

				while (!ReadFile(serialh, &rec, 1, NULL, NULL));
				sensordata[recrow][j] = rec;
				sensordata[recrow][j] = sensordata[recrow][j] << 8;
				//tell mbed that message was received

				while (!ReadFile(serialh, &rec, 1, NULL, NULL));
				sensordata[recrow][j] = rec + sensordata[recrow][j];

			}
			recrow++;
			if (recrow >= NUMROWS) recrow = 0;
		}


		//fill up text file
		for (char i = 0; i < 2; i++) {
			
			//write the two new lines of data
			datalog << scan << "\t\t" << writerow << "\t\t";
			for (char k = 0; k < NUMCOL; k++) {
				result = (int)((0.000719*(float)sensordata[writerow][k]) + 2.84);
				if (k < 2) EL1 = EL1 + result;
				else if (k >=2 && k<4) EL2 = EL2 + result;
				datalog << sensordata[writerow][k] << "\t\t";
			}
			linecounter++;
			datalog << '\n';
			writerow++;
			
		}
		if (writerow >= NUMROWS) {
			writerow = 0;
			scan++;
		}
		EL1 = EL1 / 4;
		EL2 = EL2 / 4;
		if (writerow == 2) {
			sendlight(0, EL1);
			sendlight(1, EL2);
		}
		else if (writerow == 0) {
			sendlight(2, EL1);
			sendlight(3, EL2);
		}

		if (linecounter % 10000 == 0) {
			datalog.close();
			datalog.clear();
			//restart to write to the top
			datalog.open("MatData.txt");
		}
	}
	



}




void sendlight(char light,char PWM)
{
	char first = 's';
	char space = ' ';
	WriteFile(serialh, &first, 1, NULL, NULL);
	for (int l = 0; l<10; l++);
	WriteFile(serialh, &light, 1, NULL, NULL);
	for (int l = 0; l<10; l++);
	WriteFile(serialh, &PWM, 1, NULL, NULL);
	for (int l = 0; l<10; l++);
	
}