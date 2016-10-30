#include <iostream>
#include <iomanip>
#include <time.h>
#include <vector>
#include <algorithm>
#include "BlackDef.h"
#include "BlackADC.h"
#include "skip32.hh"

using namespace std;

float calculateResistance(const float &volts);
void takeCleanAirReading(BlackLib::BlackADC &adc);
void takeUserReading(BlackLib::BlackADC &adc);
void encryptAndEncode(int plain);

float controlVolts = 0;
float controlRes = 0;
float readingVolts = 0;
float readingRes = 0;
float readingRatio = 0;

vector<float> readings;
int readingCount = 0;
double timeDiff, prevTimeDiff;
time_t begTime, endTime;

skip32 *skip;
const std::vector<uint8_t> SKIP_KEY = { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' };
 
int main(int argc, char *argv[])
{
	BlackLib::BlackADC adc = BlackLib::BlackADC(BlackLib::AIN3);
	skip = new skip32(SKIP_KEY);
	cout << std::fixed;
	cout << std::setprecision(3);

	while (true)
	{
		cout << "Press 'Enter' to take a reading." << endl;
		cin.get();
		takeCleanAirReading(adc);
		cout << endl;
		takeUserReading(adc);
		cout << endl;
		encryptAndEncode(readingRatio * 1000);
	}

	delete skip;
}

void encryptAndEncode(int plain)
{
	plain = plain << 22;
	int crypt;
	skip->block_encrypt(&plain, &crypt);
	stringstream ss;
	ss << hex << crypt;
	cout << "Encoded Hex String: " << ss.str() << endl;
}

void takeUserReading(BlackLib::BlackADC &adc)
{
	readingVolts = 0;
	readingRes = 0;
	readings.clear();
	cout << "Press 'Enter' to take a reading when you're ready." << endl;
	cin.get();
	cout << "Taking breath reading for 5 seconds." << endl;
	time(&begTime);
	do 
	{
		readings.push_back(adc.getConvertedValue(BlackLib::dap3));
		time(&endTime);
		timeDiff = difftime(endTime, begTime);
		if (timeDiff != prevTimeDiff) cout << timeDiff << endl;
		prevTimeDiff = timeDiff;
	} while (timeDiff < 5);

	sort(readings.begin(), readings.end());
	int take = readings.size() * 0.2;
	for (auto it = readings.end() - take; it != readings.end(); ++it)
		readingVolts += *it;

	readingVolts = readingVolts / take;
	readingRes = calculateResistance(readingVolts);
	readingRatio = readingRes / controlRes;
	cout << "User Breath Reading Rs/R0: " << readingRatio << endl;
}

void takeCleanAirReading(BlackLib::BlackADC &adc)
{
	controlVolts = 0;
	controlRes = 0;
	readingCount = 0;
	cout << "Taking \"clean\" air reading for 2 seconds." << endl;
	time(&begTime);
	do 
	{
		controlVolts += adc.getConvertedValue(BlackLib::dap3);
		readingCount++;
		time(&endTime);
		timeDiff = difftime(endTime, begTime);
		if (timeDiff != prevTimeDiff) cout << timeDiff << endl;
		prevTimeDiff = timeDiff;
	} while (timeDiff < 2);

	controlVolts = controlVolts / readingCount;
	controlRes = calculateResistance(controlVolts);
	cout << "Average Clean Air Volts: " << controlVolts << endl;
	cout << "Clean Air Resistance: " << controlRes << endl;
}

float calculateResistance(const float &volts)
{
	return (5 - volts) / volts;
}