#include <stdlib.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

#include "v4l2_driver.h"
#include "socket_t.h"

extern int IMAGE_WIDTH;
extern int IMAGE_HEIGHT;

/* miscellanous */
int thread_exit_sig = 0;

#define PROTOCOL_ITSFRAME 2

typedef struct {
    uint8_t protocol;
} message;

void print_help() {
  printf("Usage: simple_cam <width> <height> <device>\n");
  printf("Example: simple_cam 640 480 /dev/video0\n");
}

static void *v4l2_streaming(int video_fd, socket_p _socket) {

  fd_set fds;
  struct v4l2_buffer buf;
  while (!thread_exit_sig) {
    int ret;
    FD_ZERO(&fds);
    FD_SET(video_fd, &fds);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    ret = select(video_fd + 1, &fds, NULL, NULL, &tv);
    if (-1 == ret) {
      fprintf(stderr, "select error\n");
      return NULL;
    } else if (0 == ret) {
      fprintf(stderr, "timeout waiting for frame\n");
      continue;
    }
    if (FD_ISSET(video_fd, &fds)) {
      memset(&buf, 0, sizeof(buf));
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      if (-1 == ioctl(video_fd, VIDIOC_DQBUF, &buf)) {
        fprintf(stderr, "VIDIOC_DQBUF failure\n");
        return NULL;
      }
#ifdef DEBUG
      printf("deque buffer %d\n", buf.index);
#endif

      /*if (handler)
        (*handler)(v4l2_ubuffers[buf.index].start,
                   v4l2_ubuffers[buf.index].length);*/
    uint8_t _;
    socket_recv(_socket, &_, 1);
    message m = { PROTOCOL_ITSFRAME };
    socket_send(_socket, &m.protocol, sizeof(uint8_t));
    socket_send(_socket, (uint8_t*)&v4l2_ubuffers[buf.index].length, sizeof(unsigned int));
    socket_send(_socket, v4l2_ubuffers[buf.index].start, v4l2_ubuffers[buf.index].length);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(video_fd, VIDIOC_QBUF, &buf)) {
        fprintf(stderr, "VIDIOC_QBUF failure\n");
        return NULL;
    }
#ifdef DEBUG
      printf("queue buffer %d\n", buf.index);
#endif
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
    const char *device = "/dev/video0";

    if (argc == 2 && (strchr(argv[1], 'h') != NULL)) {
        print_help();
        exit(0);
    }

    if (argc > 2) {
        IMAGE_WIDTH = atoi(argv[1]);
        IMAGE_HEIGHT = atoi(argv[2]);
    }

    if (argc > 3) {
        device = argv[3];
    }

    int video_fildes = v4l2_open(device);
    if (video_fildes == -1) {
        fprintf(stderr, "can't open %s\n", device);
        exit(-1);
    }

    if (v4l2_querycap(video_fildes, device) == -1) {
        perror("v4l2_querycap");
        goto exit_;
    }

    // most of devices support YUYV422 packed.
    if (v4l2_sfmt(video_fildes, V4L2_PIX_FMT_YUYV) == -1) {
        perror("v4l2_sfmt");
        goto exit_;
    }

    if (v4l2_gfmt(video_fildes) == -1) {
        perror("v4l2_gfmt");
        goto exit_;
    }

    if (v4l2_sfps(video_fildes, 30) == -1) { // no fatal error
        perror("v4l2_sfps");
    }

    if (v4l2_mmap(video_fildes) == -1) {
        perror("v4l2_mmap");
        goto exit_;
    }

    if (v4l2_streamon(video_fildes) == -1) {
        perror("v4l2_streamon");
        goto exit_;
    }

    socket_p _socket = socket_new(PROTO_TCP);
    if(_socket == NULL) {
        perror("socket");
        return EXIT_FAILURE;
    }
    if(!socket_bind(_socket, "0.0.0.0", 22122)){
        perror("bind");
        return EXIT_FAILURE;
    }
    if(!socket_listen(_socket, 1)) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("Awaiting for client to connect\n");
    socket_p _client = socket_accept(_socket);
    if(_client == NULL) {
        perror("accept");
        return EXIT_FAILURE;
    }

    // START
    v4l2_streaming(video_fildes, _client);
    // END

    if (v4l2_streamoff(video_fildes) == -1) {
        perror("v4l2_streamoff");
        goto exit_;
    }

    if (v4l2_munmap() == -1) {
        perror("v4l2_munmap");
        goto exit_;
    }


exit_:
  if (v4l2_close(video_fildes) == -1) {
    perror("v4l2_close");
  };
  return 0;
}