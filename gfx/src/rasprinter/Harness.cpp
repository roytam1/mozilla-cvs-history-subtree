#include "HPPrintAPI.h"
#include "HPLinuxPrintServices.h"


DRIVER_ERROR PrintJob(unsigned int NumPages,
		      unsigned int PageHeight,
		      unsigned int PageWidth,
		      uint8_t *img )
{
  
  BYTE raster[14400]; // should be plenty big enough
  int k=0;

  HPLinuxSS* pSys = new HPLinuxSS("/dev/usb/lp0");
  PrintContext* pPC;
  
  if (pSys==NULL) 
  {
    printf("pSys Instantiation error.  Aborting...\n");
    return SYSTEM_ERROR;
  }
  
  DRIVER_ERROR err = pSys->constructor_error;
  if (err != NO_ERROR) 
  {
    printf("pSys constructor error.  Aborting...\n");
    delete pSys;
    return err;
  }  

  pPC = new PrintContext(pSys, PageWidth);
  
  if (pPC == NULL) { 
    printf("pPC Instantiation error.  Aborting...\n");
    delete pSys;
    return SYSTEM_ERROR; 
  } 

  //pPC->SelectDevice(DJ9xx);

  err = pPC->constructor_error; 
  
  if (err != NO_ERROR) { 
    printf("pPC constructor error.  Aborting...\n");
    delete pPC;
    delete pSys; 
    return err; 
  }  

  Job* pJob = new Job(pPC);
  if (pJob == NULL) {
    printf("pJob Instantiation error.  Aborting...\n");
    delete pPC; 
    delete pSys;          
    return SYSTEM_ERROR; 
  } 

  err = pJob->constructor_error; 
  if (err != NO_ERROR) {
    printf("pJob constructor error.  Aborting...\n");
    delete pJob;
    delete pPC; 
    delete pSys;               
    return SYSTEM_ERROR; 
  }  

  // JOB LOOP 
  for (int i=0; i < NumPages; i++) {
    // PAGE LOOP 
    for (int j=0; j < PageHeight; j++) {
//      err = pJob->SendRasters( img + j * PageWidth * 3 );

      // convert PPMs wierd byte order to RGB
      for (k=0; k < PageWidth*3; k+=3)
      {
        raster[k+0] = img[(j * PageWidth * 3) + k+1];
        raster[k+1] = img[(j * PageWidth * 3) + k+2];
        raster[k+2] = img[(j * PageWidth * 3) + k+0];
      }
      err = pJob->SendRasters( raster );
      
      if (err != NO_ERROR) {
	delete pJob; 
	delete pPC; 
	delete pSys; 
	return err; 
      }                        
    }

    err=pJob->NewPage();

    if (err != NO_ERROR) 
      return err; 
  }  

  delete pJob;
  delete pPC; 
  delete pSys;  
  return NO_ERROR; 
} 

extern uint8_t* ppm_read(char* file_name, int* width, int* height, int* stride)
{
  FILE* f;
  char line[16];
  int size, levels;
  uint8_t* img;

  if ((f = fopen(file_name, "rb")) == NULL)
    return (NULL);

  do {
    if (!fgets(line, sizeof(line), f) || line[0] != '#' && strcmp(line, "P6\n"))
      return (NULL);
  } while (line[0] == '#');

  do {
    if (!fgets(line, sizeof(line), f) || line[0] != '#' && sscanf(line, "%d %d %d\n", width, height, &levels) != 2)
      return (NULL);
  } while (line[0] == '#');

  *stride = *width * 3;
  size = (*stride) * (*height);
  
  if ((img = (uint8_t*) malloc(size)) == NULL)
    return (NULL);
  
  if (fread(img, size, 1, f) != 1)
    return (NULL);

  fclose(f);
  return (img);
}

/*
DRIVER_ERROR PrintPCL(char *name_p)
{ 
  HPLinuxSS* pSys = new HPLinuxSS("/dev/usb/lp0");
  PrintContext* pPC;
  
  if (pSys==NULL) 
  {
    printf("pSys Instantiation error.  Aborting...\n");
    return SYSTEM_ERROR;
  }
  
  DRIVER_ERROR err = pSys->constructor_error;
  if (err != NO_ERROR) 
  {
    printf("pSys constructor error.  Aborting...\n");
    delete pSys;
    return err;
  }  

  pPC = new PrintContext(pSys, 8 * 300);
  
  if (pPC == NULL) { 
    printf("pPC Instantiation error.  Aborting...\n");
    delete pSys;
    return SYSTEM_ERROR; 
  } 

  //pPC->SelectDevice(DJ9xx);

  err = pPC->constructor_error; 
  
  if (err != NO_ERROR) { 
    printf("pPC constructor error.  Aborting...\n");
    delete pPC;
    delete pSys; 
    return err; 
  }  

  FILE* fin = fopen(name_p, "rb");
  if (fin == NULL) {
    delete pPC;
    delete pSys;
    return (SYSTEM_ERROR);
  }

  fseek(fin, 0, SEEK_END);
  int size = ftell(fin);
  fseek(fin, 0, SEEK_SET);
  BYTE *data_p = new BYTE[size];
  fread(data_p, size, 1, fin);
  fclose(fin);

  int offset = 0, len;
  err = NO_ERROR;
  while (offset < size && err == NO_ERROR) {
    len = size - offset;
    if (len > 0x7fff)
      len = 0x7fff;
    err = pPC->SendPrinterReadyData(data_p + offset, len);
    offset += len;
  }

  delete[] data_p;

  delete pSys;
  delete pPC; 
  return err; 
} 
*/

int main(int argc, char *argv_p[])
{
  int width, height, stride;
  uint8_t *img;

#if 1
  img = ppm_read(argv_p[1], &width, &height, &stride);

  if (!img) {
    perror("ppm_read()");
    return (0);
  }

  printf("w = %d, h = %d, s = %d\n", width, height, stride);

  DRIVER_ERROR err = PrintJob(1, height, width, img);
  printf("\n--------------\nPrintJob err = %d\n", err);
#else
  PrintPCL(argv_p[1]);
#endif
}

