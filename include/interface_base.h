//
// Created by JiangGuoqing on 2019/3/25.
//

#ifndef VEGA_INTERFACE_BASE_H
#define VEGA_INTERFACE_BASE_H

#include <functional>
#include "dg_types.h"
#include <codecvt>
#include <locale>
#include <bitset>

namespace vega {

    /**
     * SdkTask is a template overriding of SdkTaskBase. It defines
     * an extra result data the task produces.
     *
     * SdkTask can be overrided to take more data that beyond the capability
     * of put/get functions SdkTaskBase provides. For example:
     *
     * \code{.cpp}
     * class MyTask : public SdkTask<TYPE> {
     * public:
     *     std::map<int, std::string> kv_;
     * }
     * \endcode
     *
     * In execution by interface iface(let's say Executable<TYPE>), you may:
     *
     * \code{.cpp}
     * std::vector<SdkTask<TYPE>> tasks;
     * auto myTask = std::make_shared<MyTask>();
     * // fill myTask->kv_ and other fields
     * tasks.push_back(myTask)
     * iface->execute(tasks);
     * \endcode
     *
     * When task is done, in async callback
     *
     * \code{.cpp}
     * void callback(std::vector<SdkTask<TYPE>> &tasks, DgError error) {
     *     auto myTask = std::dynamic_pointer_cast<MyTask>(tasks[0]);
     *     // deal with my task
     * }
     * \endcode
     *
     * @tparam _Tp Task result type
     */
    template <typename _Tp>
    class SdkTask : public SdkTaskBase {
    public:
        SdkTask() = default;
        ~SdkTask() override = default;
    public:
        using type = _Tp;
        /**
         * Task result
         */
        _Tp result_;
    };

    /**
     * This indicate the output template is useless(which means SdkTask::result_ should be ignored)
     */
    using DummyTask = SdkTask<int>;

    /**
     * Executable is the base of interface.
     * @tparam _Task Task type defined by SdkTask<>
     */
    template <typename _Task>
    class Executable {
    public:
        Executable() = default;
        virtual ~Executable() = default;

    public:
        /**
         * Async callback function.
         * This is called while async execute ends. tasks is the input task group, error is
         * the processing result.
         * Note that error is set to DG_OK only when all tasks in input group are ok.
         */
        using AsyncCallback = std::function<void(std::vector<std::shared_ptr<_Task>> &tasks, DgError error)> ;
        using TaskType = _Task;
    public:
        /**
         * Get maximum batch size
         */
        virtual int getBatchSize() {
            return 0;
        }

        /**
         * See Command::xxx in vega_option.h
         */
        virtual DgError sendCommand(const std::string &cmd, const std::string &param, std::map<std::string, std::string> &result) = 0;
        /**
         * Execute a serial tasks. When tasks are processed, async callback will be called.
         *
         * condition: tasks.size() <= getBatchSize().
         *
         * @param tasks  input tasks
         * @return #DG_OK if execution is sent(for async interface) or done(for sync interface)
         */
        virtual DgError execute(std::vector<std::shared_ptr<_Task>> &tasks) {
            VEGA_UNUSED(tasks);
            CHECK(false) << "You need to override execute";
            return DG_ERR_OP_REJECT_INPUT;
        }

        virtual DgError execute(std::string &tasks) {
            VEGA_UNUSED(tasks);
            CHECK(false) << "You need to override execute";
            return DG_ERR_OP_REJECT_INPUT;
        }
    };


#if !CUDA || NEWCUDA // old cuda has its own task definition
    /**
     * Input: stream_id_, data_, data_len_, with type_ of image or video.
     *
     *        If an decoded frame is taken, for example, type_ is set to bgr packed, nv12, ...,
     *        stride_ and size_ must be set.
     *
     *        The type of frame decoded from data_ is platform dependent. But you should
     *        not care about this, you just use it by returned stream_id_ and frame_id_.
     *
     *        frame_id_, roi_ and landmark_ will be ignored.
     *
     * HIAI:
     *        In video decoding, if a stream is stopped, you need send a frame
     *        with Option::video_eos_ = true, and WITHOUT any video data. It will
     *        cancel any frame on this stream in decoding(error = DG_ERR_CANCEL),
     *        and send eos to VDEC to end a stream.
     * Note:
     *        Frames that already decoded and responded will be remained in matrix pool.
     *
     * Option:
     *     - Option::discard_frame_: set to true if video frame should be discarded
     *     - Option::video_eos_: set to true if video frame is end frame of video coding
     * CUDA:
     *     - Option::packet_index_: video decode packet id
     *     - Option::flush_decoder_: flag to flush video decoder
     *     - Option::decode_output_type_: string of Output frame type of image decoder
     *
     * Output: Result code, frame_id_ in SdkTaskBase
     */
    using DecodeTask = DummyTask;
    using DecodeInterface = Executable<DecodeTask>;

    /**
     * Input: stream_id_; type_ must be set for video stream, or set to any value
     *        except h264/h265.
     */
    using RemoveStreamTask = DummyTask ;
    /**
     * Remove a stream and all saved frames.
     */
    using RemoveStreamInterface = Executable<RemoveStreamTask>;



    typedef struct {
        std::shared_ptr<uint8_t> data_; ///<! frame data
        int data_len_;  ///<! length of data_ in bytes

        cv::Size size_; ///<! size of frame
        cv::Size stride_; ///<! stride of frame in bytes
    } FrameData;
    /**
     * Input: stream_id_, frame_id_, type_
     *
     *        type_ is the type of frame you expected. If it is different from the
     *        frame type saved in device, interface will try to transform for you,
     *        but it may not be supported, in which case an error will be returned.
     *
     *        set type_ to SdkImage::IMAGE if you want to keep the original format.
     *
     *        If type_ is set to SdkImage::JPEG, roi_ can be set to crop target
     *        image and encode to jpeg. Cropping on other image is still not
     *        supported.
     *
     * Output: FrameData, type_
     */
    using FetchFrameTask = SdkTask<FrameData>;
    using FetchFrameInterface = Executable<FetchFrameTask>;

    typedef struct {
        std::shared_ptr<uint8_t> data_; ///<! frame data
        int data_len_;  ///<! length of data_ in bytes
        cv::Size size_; ///<! size of frame
        cv::Size stride_; ///<! stride of frame in bytes
        int spspps_header_len = 0; ///<! Length of video spspps header
    } VideoData;

    /**
      * Input: stream_id_, frame_id_, type_
      *       type_ is the type of frame you expected. Only H264(H264_MAIN) and H265(H265_MAIN)
      *
      * HIAI:
      *     To end a stream encoding, you should send last frame with Option::video_eos_ = true.
      *     SDK will wait for few seconds for encoding frames done. But it is still possible
      *     that encoding does not finish after waiting. In this situation, all frame, including
      *     last one with eos may be aborted(error = DG_ERR_ABORTED). Please note that this means
      *     frame encoding fails, but stream ending succeeds.
      *
      * Option:
      * HIAI:
      *     - Option::key_frame_interval_: set I frame interval(1~65534)
      *     - Option::force_i_frame_: set to true if want to force output I frame
      * CUDA:
      *     - Option::enc_infps_: FPS number
      *     - Option::enc_intype_: SdkImage value, input image format
      *     - Option::enc_outtype_: SdkImage value, output video format
      *
      * COMMON:
      *     - Option::video_eos_: set to true if video frame is end frame of video coding
      *
      * Output: Result code, frame_id_ in SdkTaskBase
      */
    using EncodeTask = SdkTask<VideoData>;
    using EncodeInterface = Executable<EncodeTask>;

    /**
     * Input: stream_id_, frame_id_
     * Output: Result code
     */
    using FreeFrameTask = DummyTask;
    using FreeFrameInterface = Executable<FreeFrameTask>;


    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ if you need.
     *
     * Option:
     *     Option::box_to_roi_: set to true if output box related to roi,
     *                          set to false if output box related to source frame,
     *                          default false
     *
     *     for Vehicle detect, set Option::big_image_ to true for big image threshold filtering
     *
     * Output: BBoxes
     */
    using DetectTask = SdkTask<std::vector<BBox>>;
    using DetectInterface = Executable<DetectTask>;


    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ if you need.
     *
     * Output: Attributes
     */
    using ClassifierTask = SdkTask<std::vector<ClassifyAttribute>>;
    using ClassifierInterface = Executable<ClassifierTask>;

    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ and/or landmark_ if you need.
     *
     * Output: floats vector. For face alignment, its 72 landmark points(144 floats)
     */
    using DataFlowTask = SdkTask<std::vector<float>>;
    using DataFlowInterface = Executable<DataFlowTask>;

    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ if you need.
     *
     * Output: VehicleBrand
     */
    using VehicleBrandTask = SdkTask<VehicleBrand>;
    using VehicleBrandInterface = Executable<VehicleBrandTask>;

    typedef struct {
        std::vector<Landmark> landmarks_;   ///<! landmarks for align1 or align2
        std::vector<float> landmark_score_; ///<! currently not set
        StreamId stream_id_; ///<! Saved transformed image stream id if transform is enabled
        FrameId frame_id_;   ///<! Saved transformed image frame id if transform is enabled
    } FaceAlignTransform;
    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        Face align transform task supports dynamic options:
     *           Option::face_align_:
     *                 set to true to enable alignment(align1 at least, align2 depends on face_align2_)
     *           Option::face_align2_:
     *                 set to true to enable alignment 2
     *           Option::face_transform_:
     *                 set to true to enable face transform
     *           Option::face_store_stream_id_:
     *                 if face_transform_ is true, set this to the stream id of transform output image
     *        You need ONLY set these options to task[0] of batch:
     *           task->put(Option::face_align_, true);
     *           task->put(Option::face_transform_, true);
     *           task->put(Option::face_store_stream_id_, 1100);
     *
     *        If alignment is disabled and transform is enabled, you must set a landmark to each task,
     *        otherwise set the face box is enough.
     *
     *        If transform is enabled, the transformed image will be saved in device
     *
     * Output: FaceAlignTransform
     */
    using FaceAlignTransformTask = SdkTask<FaceAlignTransform>;
    using FaceAlignTransformInterface = Executable<FaceAlignTransformTask>;

    typedef struct {
        std::vector<Landmark> landmarks_;   ///<! landmarks for align1 or align2
        float roll;
        float pitch;
        float yaw;
    } LandmarkPose;
    using LandmarkPoseTask = SdkTask<LandmarkPose>;
    using LandmarkPoseInterface = Executable<LandmarkPoseTask>;


    typedef struct {
        std::vector<Landmark> landmarks_;   ///<! landmarks for align1 or align2
        std::vector<cv::Rect> rois_; ///<! empty if single line plate, 2 rois for upper and lower plate of double line plate
        bool doubleline_;
        StreamId stream_id_; ///<! Saved transformed image stream id
        FrameId frame_id_;   ///<! Saved transformed image frame id
    } PlateRectify;
    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     * Options:
     *       Option::plate_rectify_store_stream_id_:
     *                 set this to the stream id of transform output image
     *
     * Output: PlateRectify
     */
    using PlateRectifyTask = SdkTask<PlateRectify>;
    using PlateRectifyInterface = Executable<PlateRectifyTask>;

    typedef struct
    {
        std::vector<Landmark> landmarks_; ///<! landmarks for align1 or align2
        cv::Rect roi_;      ///<! rectified roi in source image, CROP_RESIZE for plate quality
        bool substandard_;   ///<! = true if plate is under minSize
    } PlateQualityRectify;
    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *
     * Output: PlateQualityRectify
     */
    using PlateQualityRectifyTask = SdkTask<PlateQualityRectify>;
    using PlateQualityRectifyInterface = Executable<PlateQualityRectifyTask>;

    class PlateChar {
    public:
        PlateChar() = default;
        PlateChar(wchar_t c, Confidence confidence) {
            ch = c;
            score = confidence;
        }
        wchar_t ch = 0;
        Confidence score = 0;

        /*
         * OLD_FILTER_RULE
        */
        /**********************************************************************************/
        bool valid() {
            return ch != 0;
        }

        inline bool OisChinese() {
            const std::wstring ch_list = L"京津沪渝冀豫云辽黑湘皖闽鲁新苏浙赣鄂桂甘晋蒙陕吉贵粤青藏川宁琼使领试学临时警港挂澳海口";
            if(ch_list.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }
        inline bool OisProvice() {
            const static std::wstring province = L"京津沪渝冀豫云辽黑湘皖闽鲁新苏浙赣鄂桂甘晋蒙陕吉贵粤青藏川宁琼";
            if(province.find(ch) != std::wstring::npos) {
                return true;
            }
            return false;
        }
        /**********************************************************************************/
    };
    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        Usually, input of plate char is the output of plate rectify image. If plate is
     *        single line, no roi is required, and if it's double line, upper or lower plate
     *        roi should be provided. PlateChar is only designed to recognize a single
     *        line, call twice if plate is double line one.
     *
     *        Even though, PlateChar can take any size of input image.
     *
     * Output: std::vector<PlateChar>
     *
     */
    using PlateCharTask = SdkTask<std::vector<PlateChar>>;
    using PlateCharInterface = Executable<PlateCharTask>;

    /**
     * Plate recognition task
     *
     * Input: Detected plate image
     *
     *        If you use frame saved in device, set stream_id_ & frame_id_ and roi_ of plate.
     *        Note that roi must be based on the full image
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *  Output:
     *        PlateRecogData
     */
    class PlateRecogData {
    public:
        /**
         * Plate category;
         * -1 to indicate this plate is invalid, or category can not be produced, or rule file is not provided
         * default 0 to indicate category is not set
         */
        int category = 0;
        ClassifyAttribute color; ///<! plate color
        std::wstring wide_literal; ///<! Wide string of each char
        std::vector<Confidence> literal_confidence; ///<! confidence for each wchar_t of wide_literal, include L'|'
        bool is_double_line_plate; ///<! single or double line

        BBox box;
    public:
        /**
         * get narrow string of wide_literal
         */
        std::string rawString() {
            return ws2s(wide_literal);
        }

    public:
        void clearSeperator() {
            if(is_double_line_plate) {
                auto pos = wide_literal.find(L'|');
                if(pos != std::wstring::npos) {
                    wide_literal.erase(pos, 1);
                    literal_confidence.erase(literal_confidence.begin() + pos);
                }
            }
        }

    protected:

        using convert_typeX = std::codecvt_utf8<wchar_t>;

        static std::wstring s2ws(const std::string& str)
        {
            std::wstring_convert<convert_typeX, wchar_t> converterX;

            return converterX.from_bytes(str);

        }

        static std::string ws2s(const std::wstring& wstr)
        {
            std::wstring_convert<convert_typeX, wchar_t> converterX;

            return converterX.to_bytes(wstr);

        }
    };
    using PlateRecogTask = SdkTask<PlateRecogData>;
    using PlateRecogInterface = Executable<PlateRecogTask>;

    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ if you need.
     *
     * Output: KeyPoint vector.
     */
    using KeyPointTask = SdkTask<std::vector<KeyPoint>>;
    using KeyPointInterface = Executable<KeyPointTask>;

     /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ if you need.
     *
     *       Option::bg_color_ and Option::ai_image_process_type_ can be set.
     *
     * Output: cv::rect vector or pixel sum
     */
    using AIimageTask = SdkTask<AIimageData>;
    using AIimageInterface = Executable<AIimageTask>;

    /**
     * Input: If you use frame saved in device, set stream_id_ & frame_id_.
     *
     *        If you provide your own image, set data_, data_len_ and type_. type_ must be
     *        a decoded image like BGRPacked, NV12..., JPEG or H264... is not supported.
     *        In this case, image must comply the requirement of stride
     *        target device requests. For hiai, width must be 16 BYTES aligned, height must
     *        be 2 aligned.
     *
     *        In both above situation, set roi_ if you need.
     *
     * Output: PlateQualityData
     */
    typedef struct
    {
        cv::Rect rectified_roi; //rectified plate roi in source image
        ClassifyAttribute quality;
    }  PlateQualityData;

    using PlateQualityTask = SdkTask<PlateQualityData>;
    using PlateQualityInterface = Executable<PlateQualityTask>;

    /*
     * Vega 2.0 model output,used to replace all model output in vega 1.0
     */
    class AbstractTagItem {
    public:
        AbstractTagItem() = default;
        virtual ~AbstractTagItem() = default;

    public:
        enum class Type {
            CONFIDENCE,
            CONFIDENCES,
            JUDGMENT,
            BBOX,
            KEYPOINTS,
            INDEXS,
            FEATURE,
            RAWDATA,
        };

        virtual TagId               getTagNameID() = 0;
        virtual std::bitset<16>    &getValidType() = 0;
        virtual FrameId             getFrameId() = 0;
        virtual StreamId            getStreamId() = 0;
        virtual float               getConfidence() = 0;
        virtual std::vector<float> &getConfidences() = 0;
        virtual bool                getJudgment() = 0;
        virtual BBoxf              &getBbox() = 0;
        virtual std::vector<float> &getKeypoints() = 0;
        virtual std::vector<int>   &getIndexs() = 0;
        virtual std::vector<float> &getFeature() = 0;
        virtual std::vector<float> &getRawData() = 0;
    };

    using ModelOutputSP  = std::shared_ptr<AbstractTagItem>;
    using ModelOutputSPV = std::vector<ModelOutputSP>;
    using ModelTask      = SdkTask<ModelOutputSPV>;
    using ModelInterface = Executable<ModelTask>;

    typedef struct {
        std::vector<Landmark> landmarks_;   ///<! landmarks for align1 or align2
        std::vector<float> landmark_score_; ///<! currently not set
        StreamId stream_id_; ///<! Saved transformed image stream id if transform is enabled
        FrameId frame_id_;   ///<! Saved transformed image frame id if transform is enabled
    } ImgTransform;
    using ImgTransformTask = SdkTask<ImgTransform>;
    using ImgTransformInterface = Executable<ImgTransformTask>;

#endif
}
#endif //VEGA_INTERFACE_BASE_H
