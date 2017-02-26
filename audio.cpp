#include <stdio.h>
#include <bass.h>

#define SPECWIDTH 64 // display width (should be multiple of 4)
#define SPECHEIGHT 32  // height (changing requires palette adjustments too)
#define BANDS 64

#define MAX(x,y) ((x > y) ? x : y)

DWORD chan;

int main(int argc, char *argv[])
{
  // initialize BASS
  if (!BASS_Init(-1,44100,0,NULL,NULL)) {
    printf("Can't initialize device\n");
    return 0;
  }

  BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST,1); // enable playlist processing
  BASS_SetConfig(BASS_CONFIG_NET_PREBUF,0); // minimize automatic pre-buffering, so we can do it (and display it) instead

  BASS_PluginLoad("libbass_aac.so",0); // load BASS_AAC (if present) for AAC support

  BASS_Free();
  return 0;
}
