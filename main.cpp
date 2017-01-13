#if (RASPBERRY_PI)
#include <led-matrix.h>
#include <threaded-canvas-manipulator.h>
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <bass.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#define SPECWIDTH 64 // display width (should be multiple of 4)
#define SPECHEIGHT 32  // height (changing requires palette adjustments too)
#define BANDS 64

#define CHAIN 4
#define ROWS 32
#define REFRESH_RATE 30
#define THREAD_CHECK_DELAY 500

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

volatile bool playing = false;
volatile bool alive = true;

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

uint16_t refresh_rate;

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
      usleep(refresh_rate * 1000);
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

void *threadFunc(void *arg)
{
  while(alive)
  {
    if (playing && BASS_StreamGetFilePosition(chan, BASS_FILEPOS_CURRENT) == BASS_StreamGetFilePosition(chan, BASS_FILEPOS_SIZE))
    {
      printf("finished\n");
      fflush(stdout);
      playing = false;
    }
    usleep(THREAD_CHECK_DELAY * 1000);
  }
}

void handle_play(char* str)
{
    DWORD r;
    playing = false;
    BASS_StreamFree(chan); // close old stream
    if (!(chan=BASS_StreamCreateURL(str,0,BASS_STREAM_BLOCK|BASS_STREAM_STATUS|BASS_STREAM_AUTOFREE,NULL,(void*)r))) {
      printf("BASS Error: %d\n", BASS_ErrorGetCode());
    } else {
      BASS_ChannelPlay(chan,FALSE);
      playing = true;
    }
}

void handle_volume(char* str)
{
  uint8_t vol = atoi(str);
  BASS_SetVolume(vol/100.0);
}

// http://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void reset_colors()
{
  memcpy(palette, main_palette, sizeof(main_palette));
  refresh_rate = REFRESH_RATE;
}

void handle_color(char* str)
{
  char** tokens = str_split(str, ' ');
  uint8_t color[4];
  if (tokens)
  {
      int i;
      for (i = 0; *(tokens + i); i++)
      {
          color[i] = atoi(*(tokens + i));
          free(*(tokens + i));
      }
      free(tokens);
  }
  uint8_t upper, lower;
  if (color[0] < 0 || color[0] >= 32)
  {
    return;
  }
  else if (color[0] == 0)
  {
    lower = 0;
    upper = 11;
  }
  else if (color[0] == 1)
  {
    lower = 11;
    upper = 21;
  }
  else if (color[0] == 2)
  {
    lower = 21;
    upper = 27;
  }
  else if (color[0] == 3)
  {
    lower = 27;
    upper = 32;
  }
  for (uint8_t i = lower; i < upper; i++)
  {
    palette[i].rgbRed = color[1];
    palette[i].rgbGreen = color[2];
    palette[i].rgbBlue = color[3];
  }
}

void handle_refresh(char* str)
{
  refresh_rate = atoi(str);
}

int main(int argc, char *argv[])
{
#if (RASPBERRY_PI)
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
  pthread_t pth;
  pthread_create(&pth, NULL, threadFunc, NULL);
  while (true)
  {
    fgets(url, sizeof(url), stdin);
    url[strcspn(url, "\n")] = 0;
    if (strncmp(url, "play ", 5) == 0)
    {
      handle_play(url+5);
    }
    else if (strncmp(url, "pause ", 6) == 0)
    {
      BASS_Pause();
    }
    else if (strncmp(url, "unpause ", 8) == 0)
    {
      BASS_Start();
    }
    else if (strncmp(url, "color ", 6) == 0)
    {
      handle_color(url+6);
    }
    else if (strncmp(url, "volume ", 7) == 0)
    {
      handle_volume(url+7);
    }
    else if (strncmp(url, "reset ", 6) == 0)
    {
      reset_colors();
    }
    else if (strncmp(url, "refresh ", 8) == 0)
    {
      handle_refresh(url+8);
    }
    else if (strncmp(url, "stop ", 5) == 0)
    {
      BASS_Stop();
      alive = false;
      return 0;
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
