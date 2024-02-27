/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
**
** test V4L2 capture device
**
** -------------------------------------------------------------------------*/

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fstream>
#include <thread>

#include "v4l2cpp/V4l2MultiplaneCapture.h"

void callback_func(unsigned char *buffer, unsigned int bufferLength, void *user_data)
{
	V4l2MultiplaneCapture *cap_p = (V4l2MultiplaneCapture *)user_data;
	printf("new frame, length = %d\n", bufferLength);
	// 此处会阻塞收视频线程
}

/* ---------------------------------------------------------------------------
**  main
** -------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	const char *in_devname = argv[1];

	V4l2MultiplaneCapture cap;
	cap.open(in_devname);
	cap.setFormat(V4L2_PIX_FMT_NV12, 1920, 1080);
	size_t l = cap.getBufferSize();
	cap.callbackRegister(callback_func, &cap);
	cap.start(5);

	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	return 0;
}
