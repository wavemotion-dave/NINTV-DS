// =====================================================================================
// Copyright (c) 2021-2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely. 
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

#ifndef VIDEOPRODUCER_H
#define VIDEOPRODUCER_H

/**
 * This interface is implemented by any piece of hardware that renders graphic
 * output.
 */
class VideoProducer
{
    public:
        /**
         * Tells the video producer to render its output.  It should *not* flip
         * the output onto the screen as that will be done by the video bus
         * after all video producers have rendered their output to the back
         * buffer.
         *
         * This function will be called once each time the Emulator indicates
         * that it has entered vertical blank.
         */

        virtual void setPixelBuffer(UINT8* pixelBuffer, UINT32 rowSize) = 0;

        virtual void render() = 0;
};

#endif
