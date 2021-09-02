
#ifndef VIDEOBUS_H
#define VIDEOBUS_H

#include "types.h"
#include "VideoProducer.h"

const INT32 MAX_VIDEO_PRODUCERS = 10;

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
        UINT8*				pixelBuffer;
        UINT32				pixelBufferSize;
        UINT32				pixelBufferRowSize;
        UINT32				pixelBufferWidth;
        UINT32				pixelBufferHeight;

    private:
        VideoProducer*        videoProducers[MAX_VIDEO_PRODUCERS];
        UINT32                videoProducerCount;

};

#endif
