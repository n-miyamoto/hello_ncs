// Copyright 2017 Intel Corporation. 
// The source code, information and material ("Material") contained herein is  
// owned by Intel Corporation or its suppliers or licensors, and title to such  
// Material remains with Intel Corporation or its suppliers or licensors.  
// The Material contains proprietary information of Intel or its suppliers and  
// licensors. The Material is protected by worldwide copyright laws and treaty  
// provisions.  
// No part of the Material may be used, copied, reproduced, modified, published,  
// uploaded, posted, transmitted, distributed or disclosed in any way without  
// Intel's prior express written permission. No license under any patent,  
// copyright or other intellectual property rights in the Material is granted to  
// or conferred upon you, either expressly, by implication, inducement, estoppel  
// or otherwise.  
// Any license under such intellectual property rights must be express and  
// approved by Intel in writing. 


#include <stdio.h>
#include "stdafx.h"
#include "mvnc.h"
#include <stdlib.h>

#define NAME_SIZE 100
#define GRAPH_FILE_NAME "graph"


void* LoadFile(const char *filename, unsigned int *len) {
  void *p=NULL;

  unsigned int filesize;
  //open the file
  FILE *fpr;
  errno_t error;
  error = fopen_s(&fpr,filename ,"rb");
  if (error != 0) {
    
    printf("faild to open file.\n");
    return p;
  }
  
  if (fpr == NULL){
    printf("error rb");
    return p;
  }
  // read the file length
  fseek(fpr, 0L, SEEK_END);
  filesize = ftell(fpr);
  fseek(fpr, 0L, SEEK_SET);

  p = malloc(sizeof(char) * filesize);
  if (p == NULL) return p;
  *len = filesize;
  fread(p, sizeof(char), filesize, fpr);
  return p;
}


void UnloadFile(void* p) {
  if (p != NULL) { 
    free(p);
    p = NULL;
  }
  return;
}

int main(int argc, char** argv)
{
    mvncStatus retCode;
    void *deviceHandle;
    char devName[NAME_SIZE];
    retCode = mvncGetDeviceName(0, devName, NAME_SIZE);
    if (retCode != MVNC_OK)
    {   // failed to get device name, maybe none plugged in.
        printf("Error - No NCS devices found.\n");
	      printf("    mvncStatus value: %d\n", retCode);
		return 0;
    }

    // Try to open the NCS device via the device name
    retCode = mvncOpenDevice(devName, &deviceHandle);
    if (retCode != MVNC_OK)
    {   // failed to open the device.  
        printf("Error - Could not open NCS device.\n");
		    printf("    mvncStatus value: %d\n", retCode);
        return 0;
    }
    
    // deviceHandle is ready to use now.  
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Hello NCS! Device opened normally.\n");


    // Now read in a graph file so graphFileBuf will point to the 
    // bytes of the file in a memory buffer and graphFileLen will be
    // set to the number of bytes in the graph file and memory buffer.
    unsigned int graphFileLen;
    void* graphFileBuf = LoadFile(GRAPH_FILE_NAME, &graphFileLen);
    if (graphFileBuf == NULL) {
      printf("Failed to load fraph\n");
      return 0;
    }

    // Allocate the graph
    void* graphHandle;
    retCode = mvncAllocateGraph(deviceHandle, &graphHandle, graphFileBuf, graphFileLen);
    if (retCode != MVNC_OK)
    {   // Error allocating graph
      printf("Could not allocate graph for file: %s\n", GRAPH_FILE_NAME);
    }
    else
    {   // Successfully allocated graph. Now graphHandle is ready to go.  
        // Use graphHandle for other API calls and call mvncDeallocateGraph
        // when done with it.
      printf("Successfully allocated graph for %s\n", GRAPH_FILE_NAME);

      retCode = mvncDeallocateGraph(graphHandle);
      graphHandle = NULL;
    }

    UnloadFile(graphFileBuf);

    retCode = mvncCloseDevice(deviceHandle);
    deviceHandle = NULL;
    if (retCode != MVNC_OK)
    {
        printf("Error - Could not close NCS device.\n");
    		printf("    mvncStatus value: %d\n", retCode);
		    return 0;
    }
    printf("Goodbye NCS!  Device Closed normally.\n");
    printf("NCS device working.\n");
    return 0;
}
