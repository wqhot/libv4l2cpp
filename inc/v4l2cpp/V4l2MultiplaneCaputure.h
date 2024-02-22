#ifndef _V4L2_MULTIPLANE_CAPTURE_H__
#define _V4L2_MULTIPLANE_CAPTURE_H__

#include <linux/videodev2.h>
#include <mutex>
#include <condition_variable>

class V4l2MultiplaneCapture
{
public:
    V4l2MultiplaneCapture();
    ~V4l2MultiplaneCapture();

    int open(const char *dev_name);
    int setFormat(unsigned int format, unsigned int width, unsigned int height);
    size_t getBufferSize();
    int start(unsigned char* buffer, unsigned int reqCount);
    void close();
    void waitForFrame();
private:
    struct plane_start_t
    {
        void *start;
    };

    struct buffer_t
    {
        struct plane_start_t *plane_start;
        struct v4l2_plane *planes_buffer;
    };

    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane *planes_buffer;
    struct plane_start_t *plane_start;
    struct buffer_t *buffers;
    enum v4l2_buf_type type;
    int fd;
    int num_planes;
    std::mutex mtx;
    std::condition_variable cond;

    void close_f1();
    void close_f2();
};

#endif