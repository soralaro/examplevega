//
// Created by gqjiang on 19-5-16.
//

#ifndef VEGA_OPTION_H
#define VEGA_OPTION_H

#include <string>

namespace vega {
    class VideoEncdoeUserData{
    public:
        uint8_t *sdata = nullptr;
        int sdata_len = 0;
        void * udata = nullptr;
    };

    class Option {
    public:
        ////////////////////////////////////////////////////////////////////////////////
        ////////////              Common Options for SDK Task              /////////////
        ////////////////////////////////////////////////////////////////////////////////
        /**
         * int
         * level setting to make time point tracking, see vega_time_pnt.h
         * default: VEGA_TP_LEVEL_NONE
         */
        static std::string tp_level_;
        /**
         * bool
         * force using opencv to do the preprocessing or image decoding
         * default: false
         */
        static std::string use_opencv_;
        /**
         * bool
         * force using giving roi without any modification even it's outside image
         * default: false
         */
        static std::string force_roi_;

        /**
         * bool
         * output bbox in the roi coordinate, for detect model
         * default: false
         */
        static std::string box_to_roi_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////              Model Related Options for SDK task       /////////////
        ////////////////////////////////////////////////////////////////////////////////

        /**
         * bool, for vehicle detect
         * mark if an image is a big one. This will change box filter behaviour.
         * default: false
         */
        static std::string big_image_;

        /**
         * integer, for plate rectify
         * set stream id transformed images store into, default INVALID_STREAM_ID
         */
        static std::string plate_rectify_store_stream_id_;

        /**
         * bool, for video or image decoder
         * set to true if current frame should be discarded, set to false otherwise
         * default: false
         */
        static std::string discard_frame_;
        /**
         * bool, for video decoder/encoder
         * End of Video Coding
         * default: false
         */
        static std::string video_eos_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                  HIAI Video Encoder                   /////////////
        ////////////////////////////////////////////////////////////////////////////////
        /**
          * int, for HIAI video encoder
          * set I frame interval
          * default: 16
          */
        static std::string key_frame_interval_;

        /**
         * bool, for HIAI video encoder
         * Force output I frame
         * default: false
         */
        static std::string force_i_frame_;

        /**
         * float, for HIAI video encoder
         * Set ratio of the the vide resolution to  resize  
         * default: 1.0 
         */
        static std::string video_resize_ratio_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                  JPEG Image Encoder                   /////////////
        ////////////////////////////////////////////////////////////////////////////////
        /**
          * int, for jpeg encoder
          * Set JPEG encode quality, 0~100
          * Currenly avail only on HIAI
          * default: 90
          */
        static std::string jpeg_quality_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                  CUDA Video Encoder                   /////////////
        ////////////////////////////////////////////////////////////////////////////////
        /**
         * int, for CUDA video encoder
         * The input fps for encoder
         * default: 25
         */
        static std::string enc_infps_;

        /**
        * int, for CUDA video encoder
        * The input format for encoder
        * default: SdkImage::NV12
        */
        static std::string enc_intype_;

        /**
        * int, for CUDA video encoder
        * The output format for encoder
        * default: SdkImage::H264
        */
        static std::string enc_outtype_;

        static std::string enc_src_is_host_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                  CUDA Video Decoder                   /////////////
        ////////////////////////////////////////////////////////////////////////////////

        /**
         * int, for CUDA video decoder
         * the input packet id
         * default: 0
         */
        static std::string packet_index_;

        /**
         * int, for CUDA video decoder
         * the flush decoder flag
         * default: 0
         */
        static std::string flush_decoder_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                  CUDA Image Decoder                   /////////////
        ////////////////////////////////////////////////////////////////////////////////
        /**
         * string, for CUDA image decode
         * Image decoder output type
         * Default: bgr
         * Can use nv12, bgr.
         */
        static std::string decode_output_type_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                  CUDA AIimage                         /////////////
        ////////////////////////////////////////////////////////////////////////////////

        static std::string bg_color_;                //MessyMarginDetect process area, BgColorOption can be set
        static std::string ai_image_process_type_;   //AIimageProcessType can be set, img_split or pixel_sum

        ////////////////////////////////////////////////////////////////////////////////
        ////////////              Image Transform Options       /////////////
        ////////////////////////////////////////////////////////////////////////////////

        static std::string face_align_;              ///<! bool, set to true to enable alignment1, default: false
        static std::string face_align2_;             ///<! bool, set to true to enable alignment2, default: false
        static std::string face_transform_;          ///<! bool set to true to enable transform, default: false
        static std::string transform_type_;          ///<! integer TransformType can be set
        static std::string face_store_stream_id_;    ///<! integer, set stream id transformed images store into, default INVALID_STREAM_ID


        ////////////////////////////////////////////////////////////////////////////////
        ////////////                    Options used in test               /////////////
        ////////////////////////////////////////////////////////////////////////////////
        static std::string sync_stream_;             ///<! sync stream in each inferences stags

        /**
         * string, default char, could be fp32
         * data type of image,
         */
        static std::string data_type_;

        ////////////////////////////////////////////////////////////////////////////////
        ////////////                    HISI video decoder mode            ///////////
        ///////////////////////////////////////////////////////////////////////////////
        /**
         *0: VIDEO_DEC_MODE_IPB IPB 模式,即 I、P、B 帧都解码。
         *1: VIDEO_DEC_MODE_IP IP 模式,即只解码 I 帧和 P 帧。
         *2: VIDEO_DEC_MODE_I I 模式,即只解码 I 帧。
        */
        static std::string video_dec_mode_e_;
    };

    class Command {
    public:
        static std::string sample_cmd_;
        /** Query max resolution of video decoder, return <wxh, maxStreams> */
        static std::string query_max_resolution_;
        /** Query model related information */
        static std::string query_model_info_;
        /** Query support codec ,return h264|h265|xxx*/
        static std::string query_support_codec_;
    };

    class SysAttribute {
    public:
        /** get arch string, like pascal, turing, hiai, or HI3559A ... */
        static std::string arch_;
    };
}
#endif //VEGA_OPTION_H
