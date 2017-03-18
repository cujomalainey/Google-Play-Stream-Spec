#include <stdio.h>
#include <bass.h>
#include <bassmix.h>

#define SPECWIDTH 64 // display width (should be multiple of 4)
#define SPECHEIGHT 32  // height (changing requires palette adjustments too)
#define BANDS 64

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

  while(1){}

  BASS_RecordFree();
  BASS_Free();
  return 0;
}
