/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2MmapDevice.h
** 
** V4L2 source using mmap API
**
** -------------------------------------------------------------------------*/


#ifndef V4L2_MMAP_DEVICE
#define V4L2_MMAP_DEVICE
 
#include "v4l2cpp/V4l2Device.h"

#define V4L2MMAP_NBBUFFER 10

class V4l2MmapDevice : public V4l2Device
{	
	protected:	
		size_t writeInternal(char* buffer, size_t bufferSize);
		bool   startPartialWrite();
		size_t writePartialInternal(char*, size_t);
		bool   endPartialWrite();
		size_t readInternal(char* buffer, size_t bufferSize);
			
	public:
		V4l2MmapDevice(const V4L2DeviceParameters & params, v4l2_buf_type deviceType);		
		virtual ~V4l2MmapDevice();

		virtual bool init(unsigned int mandatoryCapabilities);
		virtual bool isReady() { return  ((m_fd != -1)&& (n_buffers != 0)); }
		virtual bool start();
		virtual bool stop();
	
	protected:
		unsigned int  n_buffers;
	
		struct buffer 
		{
			void *                  start;
			size_t                  length;
		};

		struct plane_start 
		{
			void * start;
		};

		struct buffer_mplane
		{
			struct plane_start* plane_start;
			struct v4l2_plane* planes_buffer;
		};
		buffer m_buffer[V4L2MMAP_NBBUFFER];
		buffer_mplane m_buffer_mplane[V4L2MMAP_NBBUFFER];
		struct v4l2_plane  *m_planes_buffer;
		struct plane_start *m_plane_start;
};

#endif

