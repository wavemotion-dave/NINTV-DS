
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
