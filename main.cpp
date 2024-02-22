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

#include "v4l2cpp/logger.h"
#include "v4l2cpp/V4l2MultiplaneCaputure.h"

int stop=0;

/* ---------------------------------------------------------------------------
**  SIGINT handler
** -------------------------------------------------------------------------*/
void sighandler(int)
{ 
       printf("SIGINT\n");
       stop =1;
}

/* ---------------------------------------------------------------------------
**  main
** -------------------------------------------------------------------------*/
int main(int argc, char* argv[]) 
{	
	int verbose = 0;
	const char *in_devname = argv[1];	

	V4l2MultiplaneCapture cap;
	cap.open(in_devname);
	cap.setFormat(V4L2_PIX_FMT_BGR24, 1920, 1080);
	cap.start(V4l2MultiplaneCapture::SYNC);

	while(1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	return 0;
}
