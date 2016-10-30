#include <iostream>
#include <unistd.h>
#include "BlackDef.h"
#include "BlackI2C.h"
#include "CaptureDevice.h"
#include "quirc.h"

#define MAX_TIMEOUTS 5
#define LCD_DISPLAYON 0x04
#define LCD_FUNCTIONSET 0x20
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTDECREMENT 0x00

using namespace std;

BlackLib::BlackI2C* i2c;

void sendCommand(uint8_t cmd);

int main(int argc, char *argv[])
{
	CaptureDevice* camera = new CaptureDevice;

	struct quirc *qr;

	qr = quirc_new();
	if (!qr)
		errno_exit("Failed to allocate memory");

	if (quirc_resize(qr, 640, 480) < 0) 
		errno_exit("Failed to allocate video memory");

	camera->openDevice();

	GrabFrameResponse gfResp;
	auto timeouts = MAX_TIMEOUTS;
	int w, h; // pointless shite

	while (timeouts > 0)
	{
		gfResp = camera->grabFrame();
		if (gfResp.status == -1)
		{
			std::cerr << "Capture device timeout out - " << timeouts << " attempts remaining" << std::endl;
			timeouts--;
			continue;
		}
		else if (gfResp.status == 1)
		{
			cv::imwrite("/home/root/capture.jpg", gfResp.frame);
			uint8_t* image = quirc_begin(qr, &w, &h);
			memcpy(image, gfResp.frame.ptr(0), gfResp.frame.rows * gfResp.frame.cols * sizeof(uint8_t));
		
			if (quirc_count(qr) > 0)
			{
				struct quirc_code code;
				struct quirc_data data;
				quirc_decode_error_t err;

				quirc_extract(qr, 0, &code);
	
				 /* Decoding stage */
				err = quirc_decode(&code, &data);

				cout << data.payload << endl;
			}
		}
		gfResp.frame.release();
		usleep(75000);
	}
	quirc_destroy(qr);
	camera->stopCapturing();
	camera->closeDevice();
	exit(EXIT_FAILURE);
}

void sendCommand(uint8_t cmd)
{
	unsigned char packet[] = { 0x80, cmd };
	i2c->writeBlock(0x7c >> 1, packet, 2);
}