#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <led-matrix.h>
#include <threaded-canvas-manipulator.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define ROWS 32
#define CHAIN 4
#define REFRESH_RATE 30
#define BANDS 128
#define SPECWIDTH BANDS
#define SPECHEIGHT 32
#define MAX(x,y) ((x > y) ? x : y)
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data
#define BYTE uint8_t

#pragma pack(1)
typedef struct {
  BYTE rgbRed,rgbGreen,rgbBlue;
} RGB;
#pragma pack()

const RGB palette[] = {
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {0, 200, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {150, 150, 0},
  {250, 100, 0},
  {250, 100, 0},
  {250, 100, 0},
  {250, 100, 0},
  {250, 100, 0},
  {250, 100, 0},
  {200, 0, 0},
  {200, 0, 0},
  {200, 0, 0},
  {200, 0, 0},
  {200, 0, 0}
};

using namespace rgb_matrix;

void die(char *s)
{
    perror(s);
    exit(1);
}

class VolumeBars : public ThreadedCanvasManipulator {
public:
  VolumeBars(Canvas *m, int delay_ms=50)
    : ThreadedCanvasManipulator(m), delay_ms_(delay_ms),
      numBars_(BANDS), t_(0) {
  }

  void Run() {
    struct sockaddr_in si_me;
    int x, y, y2, recv_len, s;
    height_ = canvas()->height();
    barWidth_ = 2; //width/numBars_;
      //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
    // Start the loop
    while (running()) {
      uint8_t fft[144];
      //try to receive some data, this is a blocking call
      if ((recv_len = recvfrom(s, fft, BUFLEN, 0, NULL, NULL)) == -1)
      {
          die("recvfrom()");
      }

      //print details of the client/peer and the data received
      for (x=0;x<SPECWIDTH;x++) {
        y = fft[x] - 1;
        y2 = SPECHEIGHT;
        if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
        while (--y2>y) drawBarRow(x, y2, {0,0,0});
        while (--y>=0) drawBarRow(x, y, palette[y]);
      }
    }
    close(s);
  }

private:
  void drawBarRow(int bar, uint8_t y, RGB color) {
    canvas()->SetPixel(bar, height_-1-y, color.rgbRed, color.rgbGreen, color.rgbBlue);
  }

  int delay_ms_;
  int numBars_;
  int barWidth_;
  int height_;
  int heightGreen_;
  int heightYellow_;
  int heightOrange_;
  int heightRed_;
  int t_;
};

int main(int argc, char *argv[])
{
  if (getuid() != 0) {
    fprintf(stderr, "Must run as root to be able to access /dev/mem\n"
            "Prepend 'sudo' to the command:\n\tsudo %s ...\n", argv[0]);
    return 1;
  }

  // Need to be root for this
  GPIO io;
  if (!io.Init())
    return 1;

  RGBMatrix *matrix = new RGBMatrix(&io, ROWS, CHAIN);

  Canvas *canvas = matrix;

  ThreadedCanvasManipulator *image_gen  = new VolumeBars(canvas, REFRESH_RATE);

  image_gen->Start();

  while(1){}
  // Stop image generating thread.
  delete image_gen;
  delete canvas;
}
