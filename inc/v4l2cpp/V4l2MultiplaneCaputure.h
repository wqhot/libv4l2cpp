#ifndef _V4L2_MULTIPLANE_CAPTURE_H__
#define _V4L2_MULTIPLANE_CAPTURE_H__

#include <linux/videodev2.h>

class V4l2MultiplaneCapture
{
public:
    enum CaputureType {
        SYNC = 0,
        ASYNC,
    };
    V4l2MultiplaneCapture();
    ~V4l2MultiplaneCapture();

    int open(const char *dev_name);
    int setFormat(unsigned int format, unsigned int width, unsigned int height);
    int start(CaputureType captureType);
    void close();
    int stop();
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
    unsigned char *buffer;

    void close_f1();
    void close_f2();
};

#endif