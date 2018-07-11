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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "fp16.h"


#define NAME_SIZE 100
//#define GRAPH_FILE_NAME "inceptionv3.graph"
#define GRAPH_FILE_NAME "ssd.graph"

typedef unsigned short half;
const int networkDim = 300;
float networkMean[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};

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

// Reads an image file from disk (8 bit per channel RGB .jpg or .png or other formats 
// supported by stbi_load.)  Resizes it, subtracts the mean from each channel, and then 
// converts to an array of half precision floats that is suitable to pass to mvncLoadTensor.  
// The returned array will contain 3 floats for each pixel in the image the first float 
// for a pixel is it's the Blue channel value the next is Green and then Red.  The array 
// contains the pixel values in row major order.
// Param path is a pointer to a null terminated string that must be set to the path of the 
//            to read before calling.
// Param reqsize must be set to the width and height that the image will be resized to.  
//               Its assumed width and height are the same size.
// Param mean must be set to point to an array of 3 floating point numbers.  The three
//            numbers are the mean values for the blue, green, and red channels in that order.
//            each B, G, and R value from the image will have this value subtracted from it.
// Returns a pointer to a buffer that is allocated internally via malloc.  this buffer contains
//         the 16 bit float values that can be passed to mvncLoadTensor().  The returned buffer 
//         will contain reqSize*reqSize*3 half floats.
half *LoadImage(const char *path, int reqSize, float *mean)
{
	int width, height, cp, i;
	unsigned char *img, *imgresized;
	float *imgfp32;
	half *imgfp16;

	img = stbi_load(path, &width, &height, &cp, 3);
	if(!img)
	{
		printf("Error - the image file %s could not be loaded\n", path);
		return NULL;
	}
	imgresized = (unsigned char*) malloc(3*reqSize*reqSize);
	if(!imgresized)
	{
		free(img);
		perror("malloc");
		return NULL;
	}
	stbir_resize_uint8(img, width, height, 0, imgresized, reqSize, reqSize, 0, 3);
	free(img);
	imgfp32 = (float*) malloc(sizeof(*imgfp32) * reqSize * reqSize * 3);
	if(!imgfp32)
	{
		free(imgresized);
		perror("malloc");
		return NULL;
	}
	for(i = 0; i < reqSize * reqSize * 3; i++)
		imgfp32[i] = imgresized[i];
	free(imgresized);
	imgfp16 = (half*) malloc(sizeof(*imgfp16) * reqSize * reqSize * 3);
	if(!imgfp16)
	{
		free(imgfp32);
		perror("malloc");
		return NULL;
	}
	for(i = 0; i < reqSize*reqSize; i++)
	{
		float blue, green, red;
                blue = imgfp32[3*i+2];
                green = imgfp32[3*i+1];
                red = imgfp32[3*i+0];

                imgfp32[3*i+0] = blue-mean[0];
                imgfp32[3*i+1] = green-mean[1]; 
                imgfp32[3*i+2] = red-mean[2];

                // uncomment to see what values are getting passed to mvncLoadTensor() before conversion to half float
                //printf("Blue: %f, Grean: %f,  Red: %f \n", imgfp32[3*i+0], imgfp32[3*i+1], imgfp32[3*i+2]);
	}
	floattofp16((unsigned char *)imgfp16, imgfp32, 3*reqSize*reqSize);
	free(imgfp32);
	return imgfp16;
}
/*
half* LoadImage(const char* str, int networkDim, float* networkMean) {
  half* img;
  size_t sz = 3 * networkDim * networkDim * sizeof(*img);
  img = (half*)malloc(sz);
  memset(img, 0, sz);
  //TODO: load image from file or camera.
  
  return img;
} 
*/

void UnloadImage(half* img) {
  if (img != NULL) {
    free(img);
  }
  return;
}

int main(int argc, char** argv)
{
    mvncStatus retCode;
    void *deviceHandle;
    char devName[NAME_SIZE];
    unsigned int graphFileLen;
    void* graphFileBuf = NULL;
    half* imageBufFp16 = NULL;
        
    // Calculate the length of the buffer that contains the half precision floats.
    // 3 channels * width * height * sizeof a 16-bit float 
    unsigned int lenBufFp16 = 3*networkDim*networkDim*sizeof(*imageBufFp16);


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
    graphFileBuf = LoadFile(GRAPH_FILE_NAME, &graphFileLen);
    if (graphFileBuf == NULL) {
      printf("Failed to load graph file\n");
      goto close_device;
    }

    // Allocate the graph
    void* graphHandle;
    retCode = mvncAllocateGraph(deviceHandle, &graphHandle, graphFileBuf, graphFileLen);
    UnloadFile(graphFileBuf);
    if (retCode != MVNC_OK)
    {   // Error allocating graph
      printf("Could not allocate graph for file: %s\n", GRAPH_FILE_NAME);
      goto close_device;
    }
    printf("Successfuly allocate graph.\n");

    // Load an image from disk.
    // LoadImage will read image from disk, convert channels to floats.
    // Subtract network mean for each value in each channel. Then convert
    // floats to half precision floats.
    // Return pointer to the buffer of half precision floats. 
    imageBufFp16 = LoadImage("image.jpg", networkDim, networkMean);
    
    // Start the inference with mvncLoadTensor()
    retCode = mvncLoadTensor(graphHandle, imageBufFp16, lenBufFp16, NULL);
    UnloadImage(imageBufFp16);
    if (retCode != MVNC_OK){
      printf("failed to load tensor.\n");
      goto close_device;
    }
    printf("Successfuly load tensor. \n");

    //TODO : get result from graph

    //Deallocate graph and remove handle.
    retCode = mvncDeallocateGraph(graphHandle);
    graphHandle = NULL;


    //close device
close_device:
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
