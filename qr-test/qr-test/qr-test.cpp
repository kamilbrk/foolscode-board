#include <iostream>
#include "CaptureDevice.h"
#include <unistd.h>

#define MAX_TIMEOUTS 5

using namespace std;

int main(int argc, char *argv[])
{
	CaptureDevice* camera = new CaptureDevice;
	camera->openDevice();
	GrabFrameResponse gfResp;
	auto timeouts = MAX_TIMEOUTS;
	
	camera->startCapturing();
	
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
		}
		gfResp.frame.release();
		usleep(75000);
	}
	
	camera->stopCapturing();
	camera->closeDevice();
	std::cout << "The capture device timed out " << MAX_TIMEOUTS << " times. Program Exiting." << std::endl;
	exit(EXIT_FAILURE);
}