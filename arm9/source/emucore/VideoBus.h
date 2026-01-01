// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef VIDEOBUS_H
#define VIDEOBUS_H

#include "types.h"
#include "VideoProducer.h"

#define MAX_VIDEO_PRODUCERS 2

class VideoBus
{
    public:
        VideoBus();
        virtual ~VideoBus();

        void addVideoProducer(VideoProducer* ic);
        void removeVideoProducer(VideoProducer* ic);
        void removeAll();

        virtual void init(UINT32 width, UINT32 height);
        virtual void render();
        virtual void release();

    protected:
        UINT8*              pixelBuffer;
        UINT32              pixelBufferSize;
        UINT32              pixelBufferRowSize;
        UINT32              pixelBufferWidth;
        UINT32              pixelBufferHeight;

    private:
        VideoProducer*        videoProducers[MAX_VIDEO_PRODUCERS];
        UINT32                videoProducerCount;
};

#endif
