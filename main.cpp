// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bass.h>

#include <algorithm>

#define SPECWIDTH 64 // display width (should be multiple of 4)
#define SPECHEIGHT 32  // height (changing requires palette adjustments too)
#define BANDS 64

#define CHAIN 4
#define ROWS 32
#define REFRESH_RATE 30

#define MAX(x,y) ((x > y) ? x : y)

#pragma pack(1)
typedef struct {
  BYTE rgbRed,rgbGreen,rgbBlue;
} RGB;
#pragma pack()

using namespace rgb_matrix;

DWORD chan;

RGB palette[] = {
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

class VolumeBars : public ThreadedCanvasManipulator {
public:
  VolumeBars(Canvas *m, int delay_ms=50)
    : ThreadedCanvasManipulator(m), delay_ms_(delay_ms),
      numBars_(BANDS), t_(0) {
  }

  void Run() {
    int x,y,y1,y2;
    y1 = 0;
    height_ = canvas()->height();
    barWidth_ = 2; //width/numBars_;

    // Start the loop
    while (running()) {
      float fft[1024];
      BASS_ChannelGetData(chan,fft,BASS_DATA_FFT2048); // get the FFT data
      for (x=0;x<SPECWIDTH;x++) {
        y=sqrt(fft[x+1])*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
        // printf("x: %d, y: %d, y1: %d, y2: %d\n", x, y, y1, y2);
        if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
        y2 = SPECHEIGHT + 1;
        while (--y2>MAX(y,y1)) drawBarRow(x, y2, {0,0,0});
        if (x && (y1=(y+y1)/2)) // interpolate from previous to make the display smoother
          while (--y1>=0) drawBarRow(x, y1, palette[y1]);
        y1=y;
        while (--y>=0) drawBarRow(x, y, palette[y]); // draw level
      }
      usleep(delay_ms_ * 1000);
    }
  }

private:
  void drawBarRow(int bar, uint8_t y, RGB color) {
    for (uint8_t x=bar*barWidth_; x<(bar+1)*barWidth_; ++x) {
      canvas()->SetPixel(x, height_-1-y, color.rgbRed, color.rgbGreen, color.rgbBlue);
    }
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


  // Initialize GPIO pins. This might fail when we don't have permissions.
  GPIO io;
  if (!io.Init())
    return 1;

  // The matrix, our 'frame buffer' and display updater.
  RGBMatrix *matrix = new RGBMatrix(&io, ROWS, CHAIN);

  Canvas *canvas = matrix;

  // The ThreadedCanvasManipulator objects are filling
  // the matrix continuously.
  ThreadedCanvasManipulator *image_gen = image_gen = new VolumeBars(canvas, REFRESH_RATE);

  // initialize BASS
  if (!BASS_Init(-1,44100,0,NULL,NULL)) {
    printf("Can't initialize device\n");
    return 0;
  }

  BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST,1); // enable playlist processing
  BASS_SetConfig(BASS_CONFIG_NET_PREBUF,0); // minimize automatic pre-buffering, so we can do it (and display it) instead

  BASS_PluginLoad("libbass_aac.so",0); // load BASS_AAC (if present) for AAC support

  // Image generating demo is crated. Now start the thread.
  image_gen->Start();

  char url[1000];
  DWORD r;
  while (true)
  {
    fgets(url, sizeof(url), stdin);
    url[strcspn(url, "\n")] = 0;
    BASS_StreamFree(chan); // close old stream
    if (!(chan=BASS_StreamCreateURL(url,0,BASS_STREAM_BLOCK|BASS_STREAM_STATUS|BASS_STREAM_AUTOFREE,NULL,(void*)r))) {
      printf("BASS Error: %d\n", BASS_ErrorGetCode());
      break; //TODO add proper error handling here
    } else {
      BASS_ChannelPlay(chan,FALSE);
    }
  }

  BASS_Free();

  // Stop image generating thread.
  delete image_gen;
  delete canvas;

  return 0;
}
