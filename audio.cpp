#include <stdio.h>
#include "bass.h"
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SPECWIDTH 128 // display width (should be multiple of 4)
#define SPECHEIGHT 100  // height (changing requires palette adjustments too)
#define SERVER "192.168.0.32"
#define BUFLEN 144  //Max length of buffer
#define PORT 8888   //The port on which to send data
#define ALPHA 0.15
#define MAX(x,y) ((x > y) ? x : y)

HRECORD line_in;
HSTREAM line_out;
uint16_t refresh_rate = 25;

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(int argc, char *argv[])
{
  struct sockaddr_in si_other;
  int s, i, slen=sizeof(si_other);
  uint8_t message[BUFLEN];
  float fft[1024], fft2[1024];
  // initialize BASS
  if (!BASS_Init(-1,44100,0,NULL,NULL) || !BASS_RecordInit(1))
  {
    printf("Can't initialize device\n");
    return 0;
  }
  BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 3);

  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
      die("socket");
  }

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);

  if (inet_aton(SERVER , &si_other.sin_addr) == 0)
  {
      fprintf(stderr, "inet_aton() failed\n");
      exit(1);
  }

  line_in = BASS_RecordStart(44100, 1, 0, NULL, NULL);

  if (!line_in)
  {
    printf("Can't record channel\n");
    exit(1);
  }

  while(1)
  {
    int x, y, y_max;
    y_max = 0;
    BASS_ChannelGetData(line_in,fft,BASS_DATA_FFT2048); // get the FFT data
    for (x=0;x<SPECWIDTH;x++) {
      fft2[x + 1] = (fft[x + 1]*ALPHA) + (fft2[x + 1]*(1 - ALPHA));
      message[x] = fft2[x+1]*10*SPECHEIGHT;
    }
    //send the message
    if (sendto(s, message, sizeof(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        die("sendto()");
    }
    usleep(refresh_rate * 1000);
  }

  BASS_RecordFree();
  BASS_Free();
  close(s);
  return 0;
}
