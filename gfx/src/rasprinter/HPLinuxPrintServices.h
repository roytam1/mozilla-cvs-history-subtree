// HPLinuxPrintServices.h

typedef unsigned char uint8_t;

#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>

#define IOCNR_GET_DEVICE_ID	1
#define IOCNR_GET_STATUS	2
#define LPGETSTATUS         0x060b
#define LPIOC_GET_DEVICE_ID(len) _IOC(_IOC_READ, 'P', IOCNR_GET_DEVICE_ID, len)	/* get device_id string */
#define LPIOC_GET_STATUS	 _IOC(_IOC_READ, 'P', IOCNR_GET_STATUS, 1)

#include "HPPrintAPI.h"
#include <string.h>

class HPLinuxSS : public SystemServices
{
protected:
  int file;
//  int file2;
  int count;

  // Buffer to xK blocks - based on USB class driver buffer size
  static const int iBuffSize = 8192;
  BYTE bSendBuffer[iBuffSize];
  int iCurrBuffSize;
  BOOL FlushNow;
  BOOL AbortNow;
  BOOL CancelJob;
  char devpath[256];
  
// track these for debug messages only...
  DISPLAY_STATUS status;
  BYTE status_byte;

public:
  HPLinuxSS(char *device_p) {
    status = (DISPLAY_STATUS) -1;
    strcpy(devpath,device_p);

    count = 0;
    iCurrBuffSize = 0;
    FlushNow = FALSE;
    AbortNow = FALSE;
    CancelJob = FALSE;

//    file2 = open("out.prn", O_WRONLY | O_CREAT, 0);

    if ((file = open(devpath, O_WRONLY | O_NONBLOCK, 0)) < 0) {
      printf("Error Opening devpath '%s'\n",devpath);
      constructor_error = IO_ERROR;
      return;
    }

    IOMode.bUSB = TRUE;
    constructor_error = InitDeviceComm();

#ifdef DEBUG_TLOGUE
    printf("DevID = ");
    if(IOMode.bDevID) printf("TRUE\n");
    else printf("FALSE\n");

    printf("Status = ");
    if(IOMode.bStatus) printf("TRUE\n");
    else printf("FALSE\n");
#endif

  }
  
  virtual ~HPLinuxSS() {


//    close(file2);

    if (!AbortNow)
      FlushIO();

    close(file);

  }

  void DisplayPrinterStatus(DISPLAY_STATUS new_status) {
    if (status != new_status) {
      printf("DisplayPrinterStatus = %d\n", new_status);
      status = new_status;
    }
  }

  DRIVER_ERROR ReadDeviceID(BYTE *id_p, int id_len) {
    if (ioctl(file, LPIOC_GET_DEVICE_ID(id_len), (void*) id_p) < 0)
    {
      printf("ReadDeviceID Error\n");
      return IO_ERROR;
    }
    return NO_ERROR;
  }

  void GetStatusInfo(BYTE *status_p) {
    /* check kernel release */
    struct utsname name;
    uname(&name);

    if (strncmp(name.release, "2.2.", 4) != 0)
      ioctl(file, LPGETSTATUS, (void*) status_p);
    else
      ioctl(file, LPIOC_GET_STATUS, (void*) status_p);

    if(status_byte != *status_p) {

#ifdef DEBUG_TLOGUE
      printf("GetStatusInfo() byte = 0x%x\n", *status_p);
#endif

      status_byte = *status_p;
    }
  }

  DRIVER_ERROR BusyWait(DWORD msec) {
    usleep(msec*1000);  // convert milli to microseconds
    if(CancelJob==TRUE) return JOB_CANCELED;
    return (NO_ERROR);
  }

  BYTE *AllocMem(int size) {
    return (BYTE*) malloc(size);
  }
  
  void FreeMem(BYTE *mem_p) {
    free(mem_p);
  }

  BOOL PrinterIsAlive() {
    return (TRUE);
  }

  DRIVER_ERROR FlushIO() {

//    printf("Flushing IO...\n");
    DRIVER_ERROR err = NO_ERROR;
    BYTE tmp = 0;
    WORD i = 0;

    FlushNow = TRUE;

    if((err = ToDevice(&tmp,&i)) != NO_ERROR)
    {
      printf("FlushIO Error [%d]\n",err);
    }

    return err;
  }

  DRIVER_ERROR AbortIO() {

    printf("Aborting IO...\n");
    DRIVER_ERROR err = NO_ERROR;

    AbortNow = TRUE;

    return err;
  }


  DRIVER_ERROR ToDevice(const BYTE *buffer_p, WORD *count_p) {

    int wlen = 0;
    DRIVER_ERROR err = NO_ERROR;
    WORD startcount = *count_p;

// DEBUGGING:  Stub out I/O here...
// *count_p = 0; return NO_ERROR;

    if (*count_p <= iBuffSize-iCurrBuffSize)
    {
      memcpy((void*)(bSendBuffer+iCurrBuffSize),(void*)buffer_p,*count_p);
      iCurrBuffSize += *count_p;
      *count_p = 0;
    }
    else
    {
      memcpy((void*)(bSendBuffer+iCurrBuffSize),(void*)buffer_p,
             iBuffSize-iCurrBuffSize);
      *count_p -= (iBuffSize-iCurrBuffSize);
      iCurrBuffSize = iBuffSize;
    }

    if( FlushNow == TRUE )
    {
      while ( (wlen = write(file, (void*)bSendBuffer, iCurrBuffSize)) != iCurrBuffSize)
      {
        if (wlen >= 0) iCurrBuffSize -= wlen;
        if( (err = BusyWait(10)) == JOB_CANCELED ) break;
      }
      FlushNow = FALSE;
    }
    else if( iBuffSize == iCurrBuffSize )
    {   
      wlen = write(file, (void*)bSendBuffer, iCurrBuffSize);

      if (wlen < 0)
          return NO_ERROR;
      
      if (wlen < iCurrBuffSize) // should never happen with this USB driver
      {
        // Hmm. we could still have a problem if USB does not flush
        // properly at job's end if the code somehow did get in here.
//        printf("              Incomplete Send\n");

        BYTE tmpbuff[iBuffSize];
        memcpy ((void*)tmpbuff,(void*)(bSendBuffer+wlen),iCurrBuffSize-wlen);
        memcpy ((void*)bSendBuffer,(void*)tmpbuff,iCurrBuffSize-wlen);

        iCurrBuffSize -= wlen;
      }
      else
      {
        iCurrBuffSize = 0;

        // we save an iteration or more of the ::send loop by dumping the rest
        // of the buffer to this recursed ToDevice call.
        if((*count_p > 0) && (wlen >= 0))
        {
          err = ToDevice((const BYTE*)(buffer_p+startcount-(*count_p)),count_p);
        }
      }
    }

      return err;
  }


  DRIVER_ERROR FromDevice(char *buffer_p, WORD *count_p) {
    count_p = 0;
    return (NO_ERROR);
  }

  BOOL YieldToSystem() {
    return (FALSE);
  }

  BYTE GetRandomNumber() {
    return rand();
  }

  DWORD GetSystemTickCount() {
    return (0);
  }

  float power(float x, float y) {
    return pow(x,y);
  }

};
