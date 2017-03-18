#include <stdio.h>
#include <bass.h>
#include <bassmix.h>
#include <stdlib.h>
#include <math.h>

#define SPECWIDTH 64 // display width (should be multiple of 4)
#define SPECHEIGHT 32  // height (changing requires palette adjustments too)
#define BANDS 64
#define MAX(x,y) ((x > y) ? x : y)
HRECORD line_in;
HSTREAM line_out;

int main(int argc, char *argv[])
{
  // initialize BASS
  if (!BASS_Init(-1,44100,0,NULL,NULL) || !BASS_RecordInit(-1))
  {
    printf("Can't initialize device\n");
    return 0;
  }
  BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 3);

  line_in = BASS_RecordStart(44100, 2, 0, NULL, NULL);
  line_out = BASS_Mixer_StreamCreate(44100, 2, 0);

  if (!BASS_Mixer_StreamAddChannel(line_out, line_in, 0))
  {
    printf("Can't patch channels\n");
  }

  if (!BASS_ChannelPlay(line_out, false))
  {
    printf("Can't play channel\n");
  }

  BASS_PluginLoad("libbass_aac.so",0); // load BASS_AAC (if present) for AAC support
  float fft[1024];
  while(1)
  {
    int x, y, y_max;
    y_max = 0;
    BASS_ChannelGetData(chan,fft,BASS_DATA_FFT2048); // get the FFT data
    for (x=0;x<SPECWIDTH*2;x+=2) {
      y=sqrt(fft[x+1])*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
      y_max = MAX(y, y_max);
    }
    fprintf(stderr, "FFT: %d\n", y_max);
    usleep(refresh_rate * 1000);
  }

  BASS_RecordFree();
  BASS_Free();
  return 0;
}
