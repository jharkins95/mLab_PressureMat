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
		
		//wait for the character to tell pc to start reading
		while (rec != 's') ReadFile(serialh, &rec, 1, NULL, NULL);
		
		for (char i = 0; i < 2; i++) {
			//receicve the data for each element
			for (char j = 0; j < 4; j++) {
				while (!ReadFile(serialh, &rec, 1, NULL, NULL));
				sensordata[recrow][j] = rec;
				sensordata[recrow][j] = sensordata[recrow][j] << 8;
				//tell mbed that message was received
				
				while (!ReadFile(serialh, &rec, 1, NULL, NULL));
				sensordata[recrow][j] = rec + sensordata[recrow][j];
				
			}
			recrow++;
			if (recrow >= 4) recrow = 0;
		}



		//fill up text file
		for (char i = 0; i < 2; i++) {
			//write the two new lines of data
			datalog <<scan<<"\t\t"<< writerow<< "\t\t" << sensordata[writerow][0] << "\t\t" << sensordata[writerow][1] << "\t\t" << sensordata[writerow][2] << "\t\t" << sensordata[writerow][3] << '\n';
			linecounter++;
			writerow++;
			if (writerow >= 4) {
				writerow = 0;
				scan++;
			}
		}
		if (linecounter%10000==0) {
			datalog.close();
			datalog.clear();
			//restart to write to the top
			datalog.open("MatData.txt");
		}

	}














}