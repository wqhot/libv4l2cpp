#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <errno.h>
#include <thread>
#include "v4l2cpp/V4l2MultiplaneCaputure.h"

V4l2MultiplaneCapture::V4l2MultiplaneCapture() : fd(-1)
{
}

V4l2MultiplaneCapture::~V4l2MultiplaneCapture()
{
    close();
}

int V4l2MultiplaneCapture::setFormat(unsigned int format, unsigned int width, unsigned int height)
{
    if (fd < 0)
    {
        return -1;
    }
    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // 修改
    fmt.fmt.pix_mp.width = width;
    fmt.fmt.pix_mp.height = height;
    fmt.fmt.pix_mp.pixelformat = format;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        printf("Set format fail\n");
        close_f1();
        return -1;
    }

    printf("width = %d\n", fmt.fmt.pix_mp.width);
    printf("height = %d\n", fmt.fmt.pix_mp.height);
    printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);
    return 0;
}

int V4l2MultiplaneCapture::start(unsigned char *buffer, unsigned int reqCount)
{
    req.count = reqCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (buffer == nullptr)
    {
        printf("buffer is null\n");
        return -1;
    }

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        printf("Reqbufs fail\n");
        close_f1();
        return -1;
    }

    printf("buffer number: %d\n", req.count);

    num_planes = fmt.fmt.pix_mp.num_planes;

    buffers = (buffer_t *)malloc(req.count * sizeof(*buffers));

    for (int i = 0; i < req.count; i++)
    {
        memset(&buf, 0, sizeof(buf));
        planes_buffer = (v4l2_plane *)calloc(num_planes, sizeof(*planes_buffer));
        plane_start = (plane_start_t *)calloc(num_planes, sizeof(*plane_start));
        memset(planes_buffer, 0, sizeof(*planes_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes_buffer;
        buf.length = num_planes;
        buf.index = i;
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
        {
            printf("Querybuf fail\n");
            req.count = i;
            close_f2();
            return -1;
        }

        (buffers + i)->planes_buffer = planes_buffer;
        (buffers + i)->plane_start = plane_start;
        for (int j = 0; j < num_planes; j++)
        {
            printf("plane[%d]: length = %d\n", j, (planes_buffer + j)->length);
            printf("plane[%d]: offset = %d\n", j, (planes_buffer + j)->m.mem_offset);
            (plane_start + j)->start = mmap(NULL /* start anywhere */,
                                            (planes_buffer + j)->length,
                                            PROT_READ | PROT_WRITE /* required */,
                                            MAP_SHARED /* recommended */,
                                            fd,
                                            (planes_buffer + j)->m.mem_offset);
            if (MAP_FAILED == (plane_start + j)->start)
            {
                printf("mmap failed\n");
                req.count = i;
                close_f2();
                return -1;
            }
        }
    }

    for (int i = 0; i < req.count; ++i)
    {
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = num_planes;
        buf.index = i;
        buf.m.planes = (buffers + i)->planes_buffer;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
            printf("VIDIOC_QBUF failed\n");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
        printf("VIDIOC_STREAMON failed\n");

    int num = 0;
    struct v4l2_plane *tmp_plane;
    tmp_plane = (v4l2_plane *)calloc(num_planes, sizeof(*tmp_plane));
    struct timeval tv;

    auto while_fun = [&]()
    {
        fd_set fds;
        while (fd >= 0)
        {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            int r = select(fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == r)
            {
                if (EINTR == errno)
                    continue;
                printf("select err\n");
            }
            if (0 == r)
            {
                fprintf(stderr, "select timeout\n");
                return -1;
            }

            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.m.planes = tmp_plane;
            buf.length = num_planes;
            if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
                printf("dqbuf fail\n");

            std::unique_lock<std::mutex> lock(mtx);
            lock.lock();
            size_t buffer_size_tmp = 0;
            for (int j = 0; j < num_planes; j++)
            {
                printf("plane[%d] start = %p, bytesused = %d\n", j, ((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused);
                // fwrite(((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused, 1, file_fd);
                memcpy(buffer + buffer_size_tmp, ((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused);
                buffer_size_tmp += (tmp_plane + j)->bytesused;
            }
            cond.notify_one();
            lock.unlock();

            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
                printf("failture VIDIOC_QBUF\n");
        }
    };
    std::thread th(while_fun);
    th.detach();
    return 0;
}

void V4l2MultiplaneCapture::waitForFrame()
{
    std::unique_lock<std::mutex> lock(mtx);
    cond.wait(lock);
    lock.unlock();
}

size_t V4l2MultiplaneCapture::getBufferSize()
{
    if (fd < 0)
    {
        return 0;
    }
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    size_t length = 0;
    if (0 != ioctl(fd, VIDIOC_G_FMT, &fmt))
    {
        return 0;
        // printf("width = %d\n", fmt.fmt.pix_mp.width);
        // printf("height = %d\n", fmt.fmt.pix_mp.height);
        // printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);
    }

    for (int i = 0; i < fmt.fmt.pix_mp.num_planes; i++)
    {
        length += fmt.fmt.pix_mp.plane_fmt[i].sizeimage;
    }
    return length;
}

int V4l2MultiplaneCapture::open(const char *dev_name)
{
    int ret = -1, i, j, r;

    fd = ::open(dev_name, O_RDWR);

    if (fd < 0)
    {
        printf("open device: %s fail\n", dev_name);
        return -1;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        printf("Get video capability error!\n");
        close_f1();
        return -1;
    }

    if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
    {
        printf("Video device not support capture!\n");
        close_f1();
        return -1;
    }

    printf("Support capture!\n");

    // memset(&fmt, 0, sizeof(struct v4l2_format));
    // fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
    //	printf("Set format fail\n");
    //	goto err;
    // }

    // printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);

    return 0;
}

void V4l2MultiplaneCapture::close_f2()
{
    for (int i = 0; i < req.count; i++)
    {
        for (int j = 0; j < num_planes; j++)
        {
            if (MAP_FAILED != ((buffers + i)->plane_start + j)->start)
            {
                if (-1 == munmap(((buffers + i)->plane_start + j)->start, ((buffers + i)->planes_buffer + j)->length))
                    printf("munmap error\n");
            }
        }
    }

    for (int i = 0; i < req.count; i++)
    {
        free((buffers + i)->planes_buffer);
        free((buffers + i)->plane_start);
    }

    free(buffers);
    close_f1();
}

void V4l2MultiplaneCapture::close_f1()
{
    ::close(fd);
    fd = -1;
}
void V4l2MultiplaneCapture::close()
{
    if (fd < 0)
    {
        return;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
        printf("VIDIOC_STREAMOFF fail\n");
    close_f2();
}