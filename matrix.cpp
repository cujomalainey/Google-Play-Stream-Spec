#include <led-matrix.h>
#include <threaded-canvas-manipulator.h>

#define CHAIN 4
#define ROWS 32
#define REFRESH_RATE 30

const RGB main_palette[] = {
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

RGB palette[ROWS];

#pragma pack(1)
typedef struct {
  BYTE rgbRed,rgbGreen,rgbBlue;
} RGB;
#pragma pack()

using namespace rgb_matrix;

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
      for (x=0;x<SPECWIDTH*2;x+=2) {
        y=sqrt(fft[x+1])*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
        if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
        y2 = SPECHEIGHT + 1;
        y1 = (y1+y)/2;
        while (--y2>y1) drawBarRow(x, y2, {0,0,0});
        while (--y1>=0) drawBarRow(x, y1, palette[y1]);
        y1=y;
        y2 = SPECHEIGHT + 1;
        while (--y2>y) drawBarRow(x+1, y2, {0,0,0});
        while (--y>=0) drawBarRow(x+1, y, palette[y]); // draw level
      }
      usleep(refresh_rate * 1000);
    }
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

  reset_colors();

  // Need to be root for this
  GPIO io;
  if (!io.Init())
    return 1;

  RGBMatrix *matrix = new RGBMatrix(&io, ROWS, CHAIN);

  Canvas *canvas = matrix;

  ThreadedCanvasManipulator *image_gen = image_gen = new VolumeBars(canvas, REFRESH_RATE);

  image_gen->Start();

  // Stop image generating thread.
  delete image_gen;
  delete canvas;
}