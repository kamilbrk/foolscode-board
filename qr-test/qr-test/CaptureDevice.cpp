#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "CaptureDevice.h"


CaptureDevice::CaptureDevice()
{
}

void CaptureDevice::openDevice()
{
	struct stat statBuffer;
	int status = stat(DEVICE_NAME, &statBuffer);
	
	// Check that the device exists.
	if (status == -1)
	{
		std::cerr << "Error opening" << DEVICE_NAME 
				  << ": " << errno << ", " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	
	// Check if the device is of the right type, i.e. Character Special File
	if (!S_ISCHR(statBuffer.st_mode))
	{
		std::cerr << DEVICE_NAME << " is not a capture device." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	fd = open(DEVICE_NAME, O_RDWR | O_NONBLOCK, 0);
	
	// Check that the device was opened successfully.
	if (fd == -1)
	{
		std::cerr << "Cannot Open " << DEVICE_NAME 
				  << ": " << errno << ", " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	
	initialise();
	deviceOpen = true;
}

void CaptureDevice::initialise()
{
	struct v4l2_capability cap;
	struct v4l2_format format;
	struct v4l2_streamparm frameint;
	struct v4l2_requestbuffers req;
	
	// Check that the device is compatible 
	if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		if (errno == EINVAL)
		{
			std::cerr << DEVICE_NAME << " is not a V4L2 device." << std::endl;
			exit(EXIT_FAILURE);
		}
		else errno_exit("VIDIOC_QUERYCAP Error");
	}
	
	// Check that the device can be used to capture video.
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		std::cerr << DEVICE_NAME << " is not a video capture device." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	// Check that the device supports streaming.
	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		std::cerr << DEVICE_NAME << " does not support streaming." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	// Set the video format required.
	CLEAR(format);
	format.type = BUFFER_TYPE;
	format.fmt.pix.width = IMG_WIDTH;
	format.fmt.pix.height = IMG_HEIGHT;
	format.fmt.pix.pixelformat = PIXEL_FORMAT;
	format.fmt.pix.field = FIELD_FORMAT;
	if (xioctl(fd, VIDIOC_S_FMT, &format) == -1)
		errno_exit("Error setting video format");
	
	// Confirm the values were accepted.
	if (format.fmt.pix.pixelformat != PIXEL_FORMAT)
	{
		std::cerr << "Pixel format not accepted." << std::endl;
		exit(EXIT_FAILURE);
	}
	else if (format.fmt.pix.width != IMG_WIDTH)
	{
		std::cerr << "Width not accepted." << std::endl;
		exit(EXIT_FAILURE);	
	}
	else if (format.fmt.pix.height != IMG_HEIGHT)
	{
		std::cerr << "Height not accepted." << std::endl;
		exit(EXIT_FAILURE);	
	}
	
	// Set the time interval as required.
	CLEAR(frameint);
	frameint.type = BUFFER_TYPE;
	frameint.parm.capture.timeperframe.numerator = 1;
	frameint.parm.capture.timeperframe.denominator = FPS;
	if (xioctl(fd, VIDIOC_S_PARM, &frameint) == -1)
		errno_exit("Error setting frame interval");
	
	// Allocate initialise memory map buffers
	CLEAR(req);
	req.count = 4; // Why 4?
	req.type = BUFFER_TYPE;
	req.memory = MEM_TYPE;
	if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1)
	{
		if (errno == EINVAL)
		{
			std::cerr << DEVICE_NAME << " does not support memory mapping." << std::endl;
			exit(EXIT_FAILURE);
		}
		else errno_exit("VIDIOC_REQBUFS Error");
	}	
	
	if (req.count < 2)
	{
		std::cerr << "Insufficient memory on " << DEVICE_NAME << std::endl;
		exit(EXIT_FAILURE);
	}
	
	buffers = static_cast<CaptureBuffer*>(calloc(req.count, sizeof(CaptureBuffer)));
	bufferCount = req.count;
	
	if (!buffers)
	{
		std::cerr << "Out of memory. Cannot allocate capture buffers." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	for (int i = 0; i < req.count; i++)
	{
		struct v4l2_buffer buf;
		
		CLEAR(buf);
		buf.type = BUFFER_TYPE;
		buf.memory = MEM_TYPE;
		buf.index = i;
		if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
			errno_exit("VIDIOC_QUERYBUF Error");
		
		buffers[i].length = buf.length;
		buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		
		if (buffers[i].start == MAP_FAILED)
			errno_exit("MMAP Failed");
	}
}

void CaptureDevice::uninitialise()
{
	for (int i = 0; i < bufferCount; i++)
	{
		if (munmap(buffers[i].start, buffers[i].length) == -1)
			errno_exit("MUNMAP Error");
	}
	
	free(buffers);
}

GrabFrameResponse CaptureDevice::grabFrame()
{
	GrabFrameResponse resp;
	int sResp;
	fd_set fds;
	timeval tv;
	
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	
	sResp = select(fd + 1, &fds, nullptr, nullptr, &tv);
	
	// Return 0 if calling function should retry
	// Return -1 if the device timed out
	if (sResp == -1)
	{
		if (errno == EINTR)
		{
			resp.status = 0;
			return resp;
		}
		else 
			errno_exit("Capture Device 'select' Error");
	}
	else if (sResp == 0)
	{
		resp.status = -1;
		return resp;
	}
		
	
	v4l2_buffer buf;
	CLEAR(buf);
	
	buf.type = BUFFER_TYPE;
	buf.memory = MEM_TYPE;
	
	if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1)
	{
		switch (errno)
		{
		case EAGAIN:
			resp.status = 0;
			return resp;
		default:
			errno_exit("VIDIOC_DQBUF Error");
		}
	}
	
	assert(buf.index < bufferCount);
	
	cv::Mat imgbuf(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3, buffers[buf.index].start);
	resp.frame = cv::imdecode(imgbuf, CV_LOAD_IMAGE_GRAYSCALE);
	resp.status = 1;
	imgbuf.release();
	
	if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
		errno_exit("VIDIOC_QBUF Error");
	
	return resp;
}

void CaptureDevice::startCapturing()
{
	enum v4l2_buf_type type = BUFFER_TYPE;
	for (int i = 0; i < bufferCount; i++)
	{
		struct v4l2_buffer buf;
		
		CLEAR(buf);
		buf.type = BUFFER_TYPE;
		buf.memory = MEM_TYPE;
		buf.index = i;
		if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
			errno_exit("VIDIOC_QBUF Error");
	}
	
	if (xioctl(fd, VIDIOC_STREAMON, &type) == -1)
		errno_exit("VIDIOC_STREAMON Error");
	
	capturing = true;
}

void CaptureDevice::stopCapturing()
{
	enum v4l2_buf_type type = BUFFER_TYPE;
	
	if (xioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
		errno_exit("VIDIOC_STREAMOFF Error");
	capturing = false;
}

void CaptureDevice::closeDevice()
{
	uninitialise();
	if (close(fd) == -1)
		errno_exit("Closing Capture Device Error");
	deviceOpen = false;
}

CaptureDevice::~CaptureDevice()
{
	if (capturing) stopCapturing();
	if (deviceOpen) closeDevice();
}
