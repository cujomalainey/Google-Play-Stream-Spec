#if (RASPBERRY_PI)
#include <led-matrix.h>
#include <threaded-canvas-manipulator.h>
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <bass.h>

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
#if (RASPBERRY_PI)
using namespace rgb_matrix;
#endif
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
#if (RASPBERRY_PI)
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
#endif

void handle_play(char* str)
{
    DWORD r;
    BASS_StreamFree(chan); // close old stream
    if (!(chan=BASS_StreamCreateURL(str+5,0,BASS_STREAM_BLOCK|BASS_STREAM_STATUS|BASS_STREAM_AUTOFREE,NULL,(void*)r))) {
      printf("BASS Error: %d\n", BASS_ErrorGetCode());
    } else {
      BASS_ChannelPlay(chan,FALSE);
    }
}

int main(int argc, char *argv[])
{
#if (RASPBERRY_PI)
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

  ThreadedCanvasManipulator *image_gen = image_gen = new VolumeBars(canvas, REFRESH_RATE);
#endif
  // initialize BASS
  if (!BASS_Init(-1,44100,0,NULL,NULL)) {
    printf("Can't initialize device\n");
    return 0;
  }

  BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST,1); // enable playlist processing
  BASS_SetConfig(BASS_CONFIG_NET_PREBUF,0); // minimize automatic pre-buffering, so we can do it (and display it) instead

  BASS_PluginLoad("libbass_aac.so",0); // load BASS_AAC (if present) for AAC support
#if (RASPBERRY_PI)
  image_gen->Start();
#endif
  char url[1000];
  while (true)
  {
    fgets(url, sizeof(url), stdin);
    url[strcspn(url, "\n")] = 0;
    if (strncmp(url, "play", 4) == 0)
    {
      handle_play(url);
    }
  }

  BASS_Free();
#if(RASPBERRY_PI)
  // Stop image generating thread.
  delete image_gen;
  delete canvas;
#endif
  return 0;
}
