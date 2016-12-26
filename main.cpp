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
        printf("I constructed\n");
  }

  void Run() {
    printf("I started\n");
    int numBars_ = BANDS;
    int x,y,y1,y2;
    const int width = canvas()->width();
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

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s <options> -D <demo-nr> [optional parameter]\n",
          progname);
  fprintf(stderr, "Options:\n"
          "\t-r <rows>     : Display rows. 16 for 16x32, 32 for 32x32. "
          "Default: 32\n"
          "\t-c <chained>  : Daisy-chained boards. Default: 1.\n"
          "\t-L            : 'Large' display, composed out of 4 times 32x32\n"
          "\t-p <pwm-bits> : Bits used for PWM. Something between 1..11\n"
          "\t-l            : Don't do luminance correction (CIE1931)\n"
          "\t-D <demo-nr>  : Always needs to be set\n"
          "\t-d            : run as daemon. Use this when starting in\n"
          "\t                /etc/init.d, but also when running without\n"
          "\t                terminal (e.g. cron).\n"
          "\t-t <seconds>  : Run for these number of seconds, then exit.\n"
          "\t       (if neither -d nor -t are supplied, waits for <RETURN>)\n"
          "\t-w <count>    : Wait states (to throttle I/O speed)\n");
  fprintf(stderr, "Example:\n\t%s -t 10 -D 1 runtext.ppm\n"
          "Scrolls the runtext for 10 seconds\n", progname);
  return 1;
}

int main(int argc, char *argv[]) {
  bool as_daemon = false;
  int runtime_seconds = -1;
  int demo = -1;
  int rows = 32;
  int chain = 1;
  int scroll_ms = 30;
  int pwm_bits = -1;
  bool large_display = false;
  bool do_luminance_correct = true;
  uint8_t w = 0; // Use default # of write cycles

  const char *demo_parameter = NULL;

  int opt;
  while ((opt = getopt(argc, argv, "dl:t:r:p:c:m:w:L")) != -1) {
    switch (opt) {
    case 'd':
      as_daemon = true;
      break;

    case 't':
      runtime_seconds = atoi(optarg);
      break;

    case 'r':
      rows = atoi(optarg);
      break;

    case 'c':
      chain = atoi(optarg);
      break;

    case 'm':
      scroll_ms = atoi(optarg);
      break;

    case 'p':
      pwm_bits = atoi(optarg);
      break;

    case 'l':
      do_luminance_correct = !do_luminance_correct;
      break;

    case 'L':
      // The 'large' display assumes a chain of four displays with 32x32
      chain = 4;
      rows = 32;
      large_display = true;
      break;

    case 'w':
      w = atoi(optarg);
      break;

    default: /* '?' */
      return usage(argv[0]);
    }
  }

  if (optind < argc) {
    demo_parameter = argv[optind];
  }

  if (getuid() != 0) {
    fprintf(stderr, "Must run as root to be able to access /dev/mem\n"
            "Prepend 'sudo' to the command:\n\tsudo %s ...\n", argv[0]);
    return 1;
  }

  if (rows != 16 && rows != 32) {
    fprintf(stderr, "Rows can either be 16 or 32\n");
    return 1;
  }

  if (chain < 1) {
    fprintf(stderr, "Chain outside usable range\n");
    return 1;
  }
  if (chain > 8) {
    fprintf(stderr, "That is a long chain. Expect some flicker.\n");
  }

  // Initialize GPIO pins. This might fail when we don't have permissions.
  GPIO io;
  if (!io.Init())
    return 1;
  if(w) io.writeCycles = w;

  // The matrix, our 'frame buffer' and display updater.
  RGBMatrix *matrix = new RGBMatrix(&io, rows, chain);
  matrix->set_luminance_correct(do_luminance_correct);
  if (pwm_bits >= 0 && !matrix->SetPWMBits(pwm_bits)) {
    fprintf(stderr, "Invalid range of pwm-bits\n");
    return 1;
  }

  Canvas *canvas = matrix;

  // The ThreadedCanvasManipulator objects are filling
  // the matrix continuously.
  ThreadedCanvasManipulator *image_gen = image_gen = new VolumeBars(canvas, scroll_ms);

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
