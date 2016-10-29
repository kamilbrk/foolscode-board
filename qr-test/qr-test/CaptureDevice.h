#pragma once
#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string>
#include <cerrno>
#include <stdio.h>

#define CLEAR(x) memset(&x, 0, sizeof(x))

struct CaptureBuffer
{
	void *start;
	size_t length;
};
	
struct GrabFrameResponse
{
	cv::Mat frame;
	int status;
};

inline bool fileExists(const char *name)
{
	struct stat buffer;
	int status = stat(name, &buffer);
	return status == 0;
}
		
inline int xioctl(int fd, int request, void *arg)
{
	int r;
	do {
		r = ioctl(fd, request, arg);
	} while (r == -1 && EINTR == errno);
		
	return r;
}			
	
inline void errno_exit(std::string message)
{
	std::cerr << message << ": " << errno << ", " << strerror(errno) << std::endl;
	exit(EXIT_FAILURE);
}
	
const uint32_t IMG_WIDTH = 640;
const uint32_t IMG_HEIGHT = 480;
class CaptureDevice
{
public:
	CaptureDevice();
	void openDevice();
	void startCapturing();
	GrabFrameResponse grabFrame();
	void stopCapturing();
	void closeDevice();
	~CaptureDevice();
private:
	void initialise();
	void uninitialise();
private:
	const char *DEVICE_NAME = "/dev/video0";
	const uint32_t FPS = 30;
	const uint32_t TIMEOUT = 1;
	const uint32_t PIXEL_FORMAT = V4L2_PIX_FMT_MJPEG;
	const v4l2_field FIELD_FORMAT = V4L2_FIELD_INTERLACED;
	const v4l2_buf_type BUFFER_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	const v4l2_memory MEM_TYPE = V4L2_MEMORY_MMAP;
	int fd;
	bool deviceOpen = false;
	bool capturing = false;
	CaptureBuffer *buffers;
	uint32_t bufferCount;
};


