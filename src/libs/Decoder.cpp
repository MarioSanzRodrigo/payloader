#include "Decoder.h"
#include <sys/time.h>

namespace payloader {

DEFINE_LOGGER(Decoder, "Decoder");

Decoder::Decoder() {
    sink_ = NULL;
    vDecoder_ = NULL;
    vDecoderContext_ = NULL;
    vFrame_ = NULL;
    aDecoder_ = NULL;
    aDecoderContext_ = NULL;
    aFrame_ = NULL;
}

Decoder::~Decoder() {
    if (vDecoderContext_ != NULL)
        avcodec_close(vDecoderContext_);
    if (vFrame_ != NULL)
        av_frame_free(&vFrame_);
    if (aDecoderContext_ != NULL)
        avcodec_close(aDecoderContext_);
    if (aFrame_ != NULL)
        av_frame_free(&aFrame_);
}

int Decoder::init(AVCodecContext *pCodecCtx) {

	avcodec_register_all();

	if (pCodecCtx->codec_id) {
		vDecoder_ = avcodec_find_decoder(pCodecCtx->codec_id);
        if (!vDecoder_) {
            ELOG_DEBUG("Error getting video decoder");
            return -1;
        }else{
              ELOG_DEBUG("Decoder finded ID: %d", pCodecCtx->codec_id );
        }

        vDecoderContext_ = avcodec_alloc_context3(vDecoder_);
        if (!vDecoderContext_) {
            ELOG_DEBUG("Error getting allocating decoder context");
            return -1;
        }else{
              ELOG_DEBUG("Context finded");
        }

        // Copy context
        if(avcodec_copy_context(vDecoderContext_, pCodecCtx) != 0) {
          fprintf(stderr, "Couldn't copy codec context");
          return -1; // Error copying codec context
        }else{
            ELOG_DEBUG("Context copied");
        }

       // vDecoderContext_->pix_fmt = videoInfo.pix_fmt;

        if (avcodec_open2(vDecoderContext_, vDecoder_, NULL) < 0) {
            ELOG_DEBUG("Error opening video decoder");
            return -1;
        }else{
              ELOG_DEBUG("Decoder opened");
        }

        // vDecoderContext_->width = videoInfo.width;
        // vDecoderContext_->height = videoInfo.height;

        vFrame_ = av_frame_alloc();
        if (!vFrame_) {
            ELOG_DEBUG("Error allocating video frame");
            return -1;
        }else{
              ELOG_DEBUG("Video frame alocated: %d and %d ",  vDecoderContext_->width,  vDecoderContext_->height );
        }
	}

	// if (audioInfo.enabled) {
	// 	aDecoder_ = avcodec_find_decoder(audioInfo.codec);
 //        if (!aDecoder_) {
 //            ELOG_DEBUG("Error getting audio decoder");
 //            return -1;
 //        }

 //        aDecoderContext_ = avcodec_alloc_context3(aDecoder_);
 //        if (!aDecoderContext_) {
 //            ELOG_DEBUG("Error getting allocating decoder context");
 //            return -1;
 //        }

 //        aDecoderContext_->sample_fmt = AV_SAMPLE_FMT_S16;
 //        aDecoderContext_->sample_rate = 48000;
 //        aDecoderContext_->channels = 2;

 //        if (avcodec_open2(aDecoderContext_, aDecoder_, NULL) < 0) {
 //            ELOG_DEBUG("Error opening audio decoder");
 //            return -1;
 //        }
 //        aFrame_ = av_frame_alloc();
 //        if (!aFrame_) {
 //            ELOG_DEBUG("Error allocating video frame");
 //            return -1;
 //        }
	// }

    ELOG_DEBUG("Codec init finished")
    return true;

}

void Decoder::setSink(FrameReceiver* receiver) {
	sink_ = receiver;
    return;
}

void Decoder::receivePacket(AVPacket& packet, AVMediaType type) {

    int frameFinished;
    int aux;

	ELOG_DEBUG("Received packet %d, type %d", packet.size, (int)type);

    if (type == AVMEDIA_TYPE_VIDEO) {
        if (vDecoder_ == 0 || vDecoderContext_ == 0) {
            ELOG_DEBUG("Init Codec First because..." );
        }else{
              ELOG_DEBUG("Video frame alocated in ReceivedPacket as: %d and %d ",  vDecoderContext_->width,  vDecoderContext_->height );
        }

      
        ELOG_DEBUG("Trying to decode packet of %d bytes", packet.size);

            aux = avcodec_decode_video2(vDecoderContext_, vFrame_, &frameFinished, &packet);
            if( aux < 0){
                ELOG_DEBUG("Error");
            }else{
                ELOG_DEBUG("ready");
            }

            ELOG_DEBUG("Has decoded frame %d", vDecoderContext_->frame_number);

            if(frameFinished) {

                if (sink_ != NULL) {
                    sink_->receiveFrame(vFrame_, AVMEDIA_TYPE_VIDEO);
                }

            }

        

    } else if (type == AVMEDIA_TYPE_AUDIO) {
        if (aDecoder_ == 0 || aDecoderContext_ == 0) {
          //  ELOG_DEBUG("Init Codec First AUDIO");
            return;
        }

        int ret;
        
        ret = avcodec_send_packet(aDecoderContext_, &packet);
        if (ret < 0) {
            ELOG_DEBUG("Error submitting the packet to the decoder");
            return;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(aDecoderContext_, aFrame_);

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            else if (ret < 0) {
                ELOG_DEBUG("Error during decoding");
                return;
            }
            /* the stream parameters may change at any time, check that they are
             * what we expect */
            // if (av_get_channel_layout_nb_channels(aFrame_->channel_layout) != 2 ||
            //     aFrame_->format != AV_SAMPLE_FMT_S16P) {
            //     ELOG_DEBUG("Unsupported frame parameters");
            //     return;
            // }
            sink_->receiveFrame(aFrame_, AVMEDIA_TYPE_AUDIO);
        }
    }

}



}	// Namespace payloader