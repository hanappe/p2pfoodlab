/* 

    P2P Food Lab Sensorbox

    Copyright (C) 2013  Sony Computer Science Laboratory Paris
    Author: Peter Hanappe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// USES CODE FROM v4l2grab

/***************************************************************************
 *   v4l2grab Version 0.1                                                  *
 *   Copyright (C) 2009 by Tobias Müller                                   *
 *   Tobias_Mueller@twam.info                                              *
 *                                                                         *
 *   based on V4L2 Specification, Appendix B: Video Capture Example        *
 *   (http://v4l2spec.bytesex.org/spec/capture-example.html)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>

#include <time.h>
#include <stdarg.h>
#include "json.h"
#include "logMessage.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
        void* start;
        size_t length;
};

static int fd = -1;
struct buffer* buffers = NULL;
static unsigned int n_buffers = 0;

// global settings
static unsigned int width = 640;
static unsigned int height = 480;
static unsigned char jpegQuality = 85;
static char* jpegFilename = NULL;
static const char* deviceName = "/dev/video0";
static char* configFile = "/var/p2pfoodlab/etc/config.json";

/**
   Convert from YUV422 format to RGB888. Formulae are described on http://en.wikipedia.org/wiki/YUV

   \param width width of image
   \param height height of image
   \param src source
   \param dst destination
*/
static void YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst)
{
        int line, column;
        unsigned char *py, *pu, *pv;
        unsigned char *tmp = dst;

        /* In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr. 
           Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels. */
        py = src;
        pu = src + 1;
        pv = src + 3;

#define CLIP(x) ( (x)>=0xFF ? 0xFF : ( (x) <= 0x00 ? 0x00 : (x) ) )

        for (line = 0; line < height; ++line) {
                for (column = 0; column < width; ++column) {
                        *tmp++ = CLIP((double)*py + 1.402*((double)*pv-128.0));
                        *tmp++ = CLIP((double)*py - 0.344*((double)*pu-128.0) - 0.714*((double)*pv-128.0));      
                        *tmp++ = CLIP((double)*py + 1.772*((double)*pu-128.0));

                        // increase py every time
                        py += 2;
                        // increase pu,pv every second time
                        if ((column & 1)==1) {
                                pu += 4;
                                pv += 4;
                        }
                }
        }
}

/**
   Print error message and terminate programm with EXIT_FAILURE return code.
   \param s error message to print
*/
static void errno_exit(const char* s)
{
        logMessage("grab-image", LOG_ERROR, "%s error %d, %s\n", s, errno, strerror (errno));
        exit(EXIT_FAILURE);
}

/**
   Do ioctl and retry if error was EINTR ("A signal was caught during the ioctl() operation."). Parameters are the same as on ioctl.

   \param fd file descriptor
   \param request request
   \param argp argument
   \returns result from ioctl
*/
static int xioctl(int fd, int request, void* argp)
{
        int r;

        do r = ioctl(fd, request, argp);
        while (-1 == r && EINTR == errno);

        return r;
}

/**
   Write image to jpeg file.

   \param img image to write
*/
static void jpegWrite(unsigned char* img)
{
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
	
        JSAMPROW row_pointer[1];
        FILE *outfile = fopen(jpegFilename, "wb");

        // try to open file for saving
        if (!outfile) {
                errno_exit("jpeg");
        }

        // create jpeg data
        cinfo.err = jpeg_std_error( &jerr );
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, outfile);

        // set image parameters
        cinfo.image_width = width;	
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        // set jpeg compression parameters to default
        jpeg_set_defaults(&cinfo);
        // and then adjust quality setting
        jpeg_set_quality(&cinfo, jpegQuality, TRUE);

        // start compress 
        jpeg_start_compress(&cinfo, TRUE);

        // feed data
        while (cinfo.next_scanline < cinfo.image_height) {
                row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        // finish compression
        jpeg_finish_compress(&cinfo);

        // destroy jpeg data
        jpeg_destroy_compress(&cinfo);

        // close output file
        fclose(outfile);
}

/**
   process image read
*/
static void imageProcess(const void* p)
{
        unsigned char* src = (unsigned char*)p;
        unsigned char* dst = malloc(width*height*3*sizeof(char));

        // convert from YUV422 to RGB888
        YUV422toRGB888(width,height,src,dst);

        // write jpeg
        jpegWrite(dst);
}

/**
   read single frame
*/

static int frameRead(void)
{
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                case EAGAIN:
                        return 0;

                case EIO:
                        // Could ignore EIO, see spec

                        // fall through
                default:
                        errno_exit("VIDIOC_DQBUF");
                }
        }

        assert (buf.index < n_buffers);

        imageProcess(buffers[buf.index].start);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");


        return 1;
}

/** 
    mainloop: read frames and process them
*/
static void mainLoop(void)
{
        unsigned int count;

        count = 3;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 5;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;

                                errno_exit("select");
                        }

                        if (0 == r) {
                                logMessage("grab-image", LOG_ERROR, "select timeout\n");
                                exit(EXIT_FAILURE);
                        }

                        if (frameRead())
                                break;
        
                        /* EAGAIN - continue select loop. */
                }
        }
}

/**
   stop capturing
*/
static void captureStop(void)
{
        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                errno_exit("VIDIOC_STREAMOFF");

}

/**
   start capturing
*/
static void captureStart(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        for (i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
        }
                
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");

}

static void deviceUninit(void)
{
        unsigned int i;

        for (i = 0; i < n_buffers; ++i)
                if (-1 == munmap (buffers[i].start, buffers[i].length))
                        errno_exit("munmap");

        free(buffers);
}

static void mmapInit(void)
{
        struct v4l2_requestbuffers req;

        CLEAR (req);

        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        logMessage("grab-image", LOG_ERROR, "%s does not support memory mapping\n", deviceName);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                logMessage("grab-image", LOG_ERROR, "Insufficient buffer memory on %s\n", deviceName);
                exit(EXIT_FAILURE);
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                logMessage("grab-image", LOG_ERROR, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap (NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

/**
   initialize device
*/
static void deviceInit(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        logMessage("grab-image", LOG_ERROR, "%s is no V4L2 device\n",deviceName);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                logMessage("grab-image", LOG_ERROR, "%s is no video capture device\n",deviceName);
                exit(EXIT_FAILURE);
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                logMessage("grab-image", LOG_ERROR, "%s does not support streaming i/o\n",deviceName);
                exit(EXIT_FAILURE);
        }


        /* Select video input, video standard and tune here. */
        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {        
                /* Errors ignored. */
        }

        CLEAR (fmt);

        // v4l2_format
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = width; 
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                errno_exit("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */
        if (width != fmt.fmt.pix.width) {
                width = fmt.fmt.pix.width;
                logMessage("grab-image", LOG_ERROR, "Image width set to %i by device %s.\n", width, deviceName);
        }
        if (height != fmt.fmt.pix.height) {
                height = fmt.fmt.pix.height;
                logMessage("grab-image", LOG_ERROR, "Image height set to %i by device %s.\n", height, deviceName);
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        mmapInit();
}

/**
   close device
*/
static void deviceClose(void)
{
        if (-1 == close (fd))
                errno_exit("close");
        fd = -1;
}

/**
   open device
*/
static void deviceOpen(void)
{
        struct stat st;

        // stat file
        if (-1 == stat(deviceName, &st)) {
                logMessage("grab-image", LOG_ERROR, "Cannot identify '%s': %d, %s\n", deviceName, errno, strerror (errno));
                exit(EXIT_FAILURE);
        }

        // check if its device
        if (!S_ISCHR (st.st_mode)) {
                logMessage("grab-image", LOG_ERROR, "%s is no device\n", deviceName);
                exit(EXIT_FAILURE);
        }

        // open device
        fd = open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

        // check if opening was successfull
        if (-1 == fd) {
                logMessage("grab-image", LOG_ERROR, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror (errno));
                exit(EXIT_FAILURE);
        }
}

static void usage(FILE* fp, int argc, char** argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-h | --help          Print this message\n"
                 "-v | --version       Print version\n"
                 "-o | --output        Output file\n"
                 "",
                 argv[0]);
}

static void printVersion(FILE* fp)
{
        fprintf (fp,
                 "P2P Food Lab Sensorbox\n"
                 "  Image grabber\n"
                 "  Version %d.%d.%d\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

void parseArguments(int argc, char **argv)
{
        static const char short_options [] = "hvo:c:";

        static const struct option
                long_options [] = {
                { "help",        no_argument, NULL, 'h' },
                { "version",     no_argument, NULL, 'v' },
                { "output",      required_argument, NULL, 'o' },
                { "config",      required_argument, NULL, 'c' },
                { 0, 0, 0, 0 }
        };

        for (;;) {
                int index, c = 0;
                
                c = getopt_long(argc, argv, short_options, long_options, &index);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);
                case 'v':
                        printVersion(stdout);
                        exit(EXIT_SUCCESS);
                case 'o':
                        jpegFilename = optarg;
                        break;
                case 'c':
                        configFile = optarg;
                        break;
                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }
}

typedef struct _image_size_t {
        const char* symbol;
        unsigned int width;
        unsigned int height;
} image_size_t;

static image_size_t image_sizes[] = {
        { "320x240", 320, 240 },
        { "640x480", 640, 480 },
        { "960x720", 960, 720 },
        { "1024x768", 1024, 768 },
        { "1280x720", 1280, 720 },
        { "1280x960", 1280, 960 },
        { "1920x1080", 1920, 1080 },
        { NULL, 0, 0 }};

int getImageSize(const char* symbol, unsigned int* width, unsigned int* height)
{
        for (int i = 0; image_sizes[i].symbol != 0; i++) {
                if (strcmp(image_sizes[i].symbol, symbol) == 0) {
                        *width = image_sizes[i].width;
                        *height = image_sizes[i].height;
                        return 0;
                }
        }
        return -1;
}

int main(int argc, char **argv)
{
        char buffer[512];

        //setLogFile(stderr);
        parseArguments(argc, argv);

        json_object_t config = json_load(configFile, buffer, 512);
        if (json_isnull(config)) {
                logMessage("grab-image", LOG_ERROR, "Failed to load the configuration file: %s\n", buffer); 
                exit(1);
        } 

        json_object_t camera = json_object_get(config, "camera");
        if (json_isnull(camera)) {
                logMessage("grab-image", LOG_ERROR, "Could not find the camera configuration\n"); 
                exit(1);
        }

        json_object_t device = json_object_get(camera, "device");
        if (!json_isstring(device)) {
                logMessage("grab-image", LOG_ERROR, "Invalid device configuration\n"); 
                exit(1);
        }        
        deviceName = json_string_value(device);

        json_object_t size = json_object_get(camera, "size");
        if (!json_isstring(size)) {
                logMessage("grab-image", LOG_ERROR, "Image size is not a string\n"); 
                exit(1);
        }        

        if (getImageSize(json_string_value(size), &width, &height) != 0) {
                logMessage("grab-image", LOG_ERROR, "Invalid image size: %s\n", json_string_value(size)); 
                exit(1);
        }

        if (jpegFilename == NULL) {
                struct timeval tv;
                struct tm r;
                char filename[512];

                gettimeofday(&tv, NULL);
                localtime_r(&tv.tv_sec, &r);
                snprintf(filename, 512, "/var/p2pfoodlab/photostream/%04d%02d%02d-%02d%02d%02d.jpg",
                         1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, r.tm_hour, r.tm_min, r.tm_sec);

                jpegFilename = filename;
        }

        logMessage("grab-image", LOG_INFO, "Grabbing %s image from %s\n", 
                   json_string_value(size), deviceName);

        logMessage("grab-image", LOG_INFO, "Storing image in %s\n", 
                   jpegFilename);

        // open and initialize device
        deviceOpen();
        deviceInit();

        // start capturing
        captureStart();

        // process frames
        mainLoop();

        // stop capturing
        captureStop();

        // close device
        deviceUninit();
        deviceClose();

        logMessage("grab-image", LOG_INFO, "Finished\n");

        return 0;
}
