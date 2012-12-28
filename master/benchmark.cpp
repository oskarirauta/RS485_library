#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
  
#include "RS485.h"
  
#define TESTFILES 3

RS485 rs485;

char dataPattern[] = "Praesent dapibus, neque id cursus faucibus, tortor neque egestas augue, eu vulputate magna eros eu erat. Aliquam erat volutpat. Nam dui mi, tincidunt quis, accumsan porttitor, facilisis luctus, metus. "; 
char fName[16];
unsigned char str[64];
int c;

int file_exists(char *fname) {
  struct stat st;
  return (stat (fname, &st) == 0);
}

void makeFiles(void) {
  unsigned int b;
  c = 7;
  FILE *oF;
  
  for ( b = 1; b < TESTFILES+1; b++ ) {
    sprintf(fName, "sendfile%d.bin", b);
  	if (!file_exists(fName)) { // Don't overwrite, maybe user wants to use his/her own files..
      if ( (oF = fopen(fName, "wb")) == NULL ) {
        printf("ERROR: Cannot write file sendfile1.bin\r\n");
        exit(-1);
      }
      fwrite (dataPattern, c, strlen(dataPattern), oF);
      fclose(oF);
      printf("Created file %s with %ld bytes\r\n", fName, c*strlen(dataPattern));
    }
    c += (35 *b); 
  }
  printf("-----------------------------\r\n\n");
}

  
void downloadTest() {
  uint32 fSize;
  unsigned char b, r;  
  for ( r = 1; r < 3; r++ ) {
    c = 5;
    for ( b = 0; b < TESTFILES; b++ ) {
      fSize = strlen(dataPattern) * floor(c / 6);
      if (c == 5)
        fSize = strlen(dataPattern) * c;
      printf("\nDownloading file with blockSize %d and filesize %d bytes.\r\n", r == 0 ? 64 : (r * 31 * 132), fSize);
      sprintf((char *)str, "ATDOWNLOAD,0,%d,%d", r * 31, fSize);
      if ( !rs485.receiveFile(str, (char *)"./recvFile") )
        printf("Was not able to request file %d on round %d.\r\n",b,r);
      else
        printf("Test file received\r\n");

      c += 512;
    }
  }  
  printf("-----------------------------\r\n\n");
}
  
void uploadTest() {
  uint32 fSize;
  unsigned char b, r;  
  struct stat st;

  for ( r = 1; r < 3; r++ ) {
    c = 5;
    for ( b = 1; b < TESTFILES + 1; b++ ) {
      sprintf(fName, "sendfile%d.bin", b);
      stat(fName, &st);
      fSize = st.st_size;
      printf("\nUploading file %s with blockSize %d and filesize %d bytes.\r\n", fName, r == 0 ? 64 : (r * 31 * 132), fSize);

      sprintf((char *)str, "ATUPLOAD,0,%d,%d", r * 31, fSize);
      if ( !rs485.sendFile(str, fName) )
        printf("Was not able to send file %s on round %d.\r\n",fName,r);
      else
        printf("File %s was sent.\r\n", fName);
      c += 512;
    }
  }
  printf("-----------------------------\r\n\n");
}
 
int main() {
  makeFiles();
  rs485.begin((char *)"/dev/tty.usbserial-FTVTL64A");
  
    if ( rs485.call(2) ) {
      printf("Device #2 answered.\r\n");
  	  printf("-----------------------------\r\n\n");
      if ( rs485.getStatus() )
        printf("Status is OK.\r\n");
      else
        printf("Status query resulted: \"%s\"\r\n", rs485.reply());
      
      if ( !rs485.getAnswer((unsigned char *)"ATTEST") )
        printf("Device #2 didn't answer to test command ATTEST.\r\n");
      else
        printf("Device #2 answered to ATTEST with: \"%s\"\r\n", rs485.reply());

      printf("Starting to request files from device.\r\n");
      downloadTest();

      printf("Starting to send files to device.\r\n");
      uploadTest();
      
      printf("Disconnecting device.\r\n");
      rs485.disconnect();
    } else 
      printf("Device #2 did not answer when called..\r\n");
  
  return 0;
}
