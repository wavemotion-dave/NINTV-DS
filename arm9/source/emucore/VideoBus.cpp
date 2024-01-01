// =====================================================================================
// Copyright (c) 2021-2024 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "VideoBus.h"

VideoBus::VideoBus()
  : pixelBuffer(NULL),
    pixelBufferSize(0),
    pixelBufferRowSize(0),
    pixelBufferWidth(0),
    pixelBufferHeight(0),
    videoProducerCount(0)
{
}

VideoBus::~VideoBus()
{
    if (pixelBuffer) {
        delete[] pixelBuffer;
    }

    for (UINT32 i = 0; i < videoProducerCount; i++)
        videoProducers[i]->setPixelBuffer(NULL, 0);
}

void VideoBus::addVideoProducer(VideoProducer* p)
{
    videoProducers[videoProducerCount] = p;
    videoProducers[videoProducerCount]->setPixelBuffer(pixelBuffer, pixelBufferRowSize);
    videoProducerCount++;
}

void VideoBus::removeVideoProducer(VideoProducer* p)
{
    for (UINT32 i = 0; i < videoProducerCount; i++) {
        if (videoProducers[i] == p) {
            videoProducers[i]->setPixelBuffer(NULL, 0);

            for (UINT32 j = i; j < (videoProducerCount-1); j++)
                videoProducers[j] = videoProducers[j+1];
            videoProducerCount--;
            return;
        }
    }
}

void VideoBus::removeAll()
{
    while (videoProducerCount)
        removeVideoProducer(videoProducers[0]);
}

void VideoBus::init(UINT32 width, UINT32 height)
{
    VideoBus::release();

    pixelBufferWidth = width;
    pixelBufferHeight = height;
    pixelBufferRowSize = width * sizeof(UINT8);
    pixelBufferSize = width * height * sizeof(UINT8);
    pixelBuffer = new UINT8[width * height];

    if ( pixelBuffer ) {
        memset(pixelBuffer, 0, pixelBufferSize);
    }

    for (UINT32 i = 0; i < videoProducerCount; i++)
        videoProducers[i]->setPixelBuffer(pixelBuffer, pixelBufferRowSize);
}

ITCM_CODE void VideoBus::render()
{
    //tell each of the video producers that they can now output their
    //video contents onto the video device
    for (UINT32 i = 0; i < videoProducerCount; i++)
        videoProducers[i]->render();
}

void VideoBus::release()
{
    if (pixelBuffer) {
        for (UINT32 i = 0; i < videoProducerCount; i++)
            videoProducers[i]->setPixelBuffer(NULL, 0);

        pixelBufferWidth = 0;
        pixelBufferHeight = 0;
        pixelBufferRowSize = 0;
        pixelBufferSize = 0;
        delete[] pixelBuffer;
        pixelBuffer = NULL;
    }
}
