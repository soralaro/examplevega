//
// Created by JiangGuoqing on 2018/10/10.
//

#ifndef DG_TYPES_H_
#define DG_TYPES_H_

#include <locale>
#include <codecvt>
#include <map>
#include "error.h"
#include "sys.h"
#include "opencv2/opencv.hpp"
#include "glog/logging.h"

#include <memory>
#include "model_define.h"
#include "vega_time_pnt.h"

namespace vega{

    typedef unsigned char       DG_U8;
    typedef char                DG_S8;
    typedef unsigned short      DG_U16;
    typedef unsigned int        DG_U32;
    typedef int                 DG_S32;
    typedef float               DG_F32;
    typedef unsigned long long  DG_U64;

    using RawStream = std::shared_ptr<DG_U8>;

    typedef DG_U64              DG_PADDR;

    typedef void *              DG_HANDLE;
    typedef void                DG_VOID;
    typedef void *              DG_PTR;


    typedef DG_F32              Landmark;
    typedef cv::Point2f         LandmarkPoint;
    typedef DG_F32              Feature;
    typedef DG_F32              Confidence;

    typedef DG_U64              DG_ID;

    typedef DG_ID               TrackId;
    typedef DG_ID               ReferenceId;
    typedef DG_ID               FrameId;
    typedef DG_ID               VideoId;
    typedef VideoId             StreamId;
    typedef DG_ID               TaskId;

    typedef DG_U32              TagId;
    const TagId INVALID_TAG_ID = 0;

const StreamId INVALID_STREAM_ID = 0xFFFFFFFF;

    /**
     * Memory blob
     */
    typedef struct {
        DG_U8 * ptr     = nullptr;
        DG_U32  size_   = 0;
    } MemBlock;

    /**
     * KeyPoint
     */
    typedef struct  KeyPoint {
        cv::Point2f    pnt;
        Confidence     score;
    } KeyPoint;

    struct RawBuffer {
        uint32_t len_of_byte;  // size length
        std::shared_ptr<uint8_t> data;
    };

    enum class VegaDataType {
        CHAR = sizeof(char),
        FLOAT = sizeof(float)
    };

#define VEGA_DATA_SIZE(dataType) ((int)(dataType))
    ///////////////////////////////////////////////////////////////////////////////////
    ////////                      DgImage Class Definition                    /////////
    ///////////////////////////////////////////////////////////////////////////////////

    enum class DgImageType
    {
        DG_IMAGE_YUV_SP420      = 0,
        DG_IMAGE_YUV_I420       = 1,
        DG_IMAGE_BGR_PACKAGE    = 2,
        DG_IMAGE_BGR_PLANAR     = 3,
        DG_IMAGE_RGB_PACKAGE    = 4,
        DG_IMAGE_RGB_PLANAR     = 5,
        DG_IMAGE_YUV_GRAY       = 6,

        DG_IMAGE_TYPE_INVALID

    };

    class DgImage {
    public:
        DgImage() : frame_(0), type_(DgImageType::DG_IMAGE_TYPE_INVALID) {

        }
        DgImage(const cv::Mat &mat, DgImageType type = DgImageType::DG_IMAGE_BGR_PACKAGE) {
            cv::Size size(mat.cols, mat.rows);
            create(mat.data, size, type);
        }
        virtual ~DgImage() = default;

        FrameId         frame_;     ///<! Frame id in video or picture stream
        DgImageType     type_;      ///<! image type
        cv::Rect        roi_;       ///<! Region of interest
        cv::Size        size_;      ///<! current size
        cv::Size        orig_size_; ///<! original size, in case this image is resized, here record the original size
        MemBlock        blob_;      ///<! image data

    public:
        inline DG_U8 *data() {
            return blob_.ptr;
        }
        inline DG_F32 ratio() {
            return (DG_F32)size_.width / size_.height;
        }
        inline cv::Rect rect() {
            return cv::Rect(cv::Point(0, 0), size_);
        }
        inline cv::Rect roi() {
            if(!hasRoi()) {
                return rect();
            }
            return roi_;
        }
        inline void setRoi(const cv::Rect &roi, bool force = false) {
            auto newRoi = roi;
            if(!force) {
                newRoi = roi & rect();
                if(roi != newRoi) {
                    LOG(ERROR) << "Roi("<< roi.x << " " << roi.y << " " << roi.width << " " << roi.height
                               << ") out of range, sz(" << size_.width << " " << size_.height << ")";
                }
            }

            if(newRoi == rect()) {
                roi_ = cv::Rect(0, 0, 0, 0);
                return;
            }

            roi_ = newRoi;
        }
        inline bool hasRoi() {
            return roi_.area() > 0;
        }
        /** transfer to mat */
        virtual cv::Mat mat() {
            CHECK(type_ == DgImageType::DG_IMAGE_BGR_PACKAGE);
            return cv::Mat(size_, CV_8UC3, blob_.ptr);
        }
        void clear() {
            frame_ = 0;
            type_ = DgImageType ::DG_IMAGE_TYPE_INVALID;
            roi_ = cv::Rect();
            size_ = cv::Size();
            orig_size_ = size_;
            blob_.size_ = 0;
            blob_.ptr = nullptr;
        }
        void create(void *data, const cv::Size &size, DgImageType type) {
            create(data, size, size, type);
        }
        void create(void *data, const cv::Size &origSize, const cv::Size &size, DgImageType type) {
            type_ = type;
            blob_.ptr = (DG_U8 *)data;
            switch(type) {
                case DgImageType::DG_IMAGE_YUV_SP420:
                    blob_.size_ = DG_U32(3 * size.area()/2);
                    break;
                case DgImageType::DG_IMAGE_BGR_PACKAGE:
                case DgImageType::DG_IMAGE_BGR_PLANAR:
                case DgImageType::DG_IMAGE_RGB_PACKAGE:
                case DgImageType::DG_IMAGE_RGB_PLANAR:
                    blob_.size_ = DG_U32(3 * size.area());
                    break;
                default:
                    CHECK(false) << "image not support " << int(type);
                    break;
            }
            size_ = size;
            orig_size_ = origSize;
        }
    } ;

    /**
     * Shortcut to use image as shared ptr
     */
    using DgImageSP = std::shared_ptr<DgImage>;

#if CUDA
    /**
     * for BGR, use DgCudaImage(GpuMat, DgImageType::DG_IMAGE_BGR_PACKED) to
     * create a DgImage, and use getGpuMat() if you want to get matrix.
     *
     * For NV12, use DgCudaImage(void *, size) to create a DgImage, and
     * MUST not call getGpuMat(), you need use data() and size_ to get
     * image data and size(width/height)
     */
    class DgCudaImage : public DgImage {
    public:
        DgCudaImage() = delete;
        virtual ~DgCudaImage() = default;

        /**
         * Temp constructor specified for cuda/interfaces, you should not call this unless
         * adapting those files under cuda/interface
         * @param mat CPU BGR Packed image
         */
        DgCudaImage(cv::Mat &mat) {
            gpu_mat_.upload(mat);
            DgImage::create(gpu_mat_.data, gpu_mat_.size(), DgImageType::DG_IMAGE_BGR_PACKAGE);
        }
        /**
         * Construct a GPU mat, it can be BGR or NV12, only
         * NV12 must be stride aligned
         *
         * Use getGpuMat() retrieve if you need
         */
        DgCudaImage(const cv::cuda::GpuMat & gpu_mat, DgImageType type) {
            DgImage::create(gpu_mat.data, gpu_mat.size(), type);
            gpu_mat_ = gpu_mat;
        }
        /**
         * Construct an NV12 with cuda memory
         * MUST NOT use getGpuMat() to retrive
         */
        DgCudaImage(void *nv12, const cv::Size &size, DgImageType type = DgImageType::DG_IMAGE_YUV_SP420) {
            DgImage::create(nv12, size, type);
        }
        /**
         * Get GpuMat constructed by DgCudaImage(GpuMat, type)
         */
        inline cv::cuda::GpuMat getGpuMat() {
            CHECK(gpu_mat_.size().area() > 0);
            return gpu_mat_;
        }

    protected:
        cv::cuda::GpuMat gpu_mat_;
    };

    using DgCudaImageSP = std::shared_ptr<DgCudaImage>;
#endif


    ///////////////////////////////////////////////////////////////////////////////////
    ////////                         BBox Class Definition                    /////////
    ///////////////////////////////////////////////////////////////////////////////////

#if DG_UUID
    enum class DetectType : TagId
    {
        DETECT_TYPE_UNDEFINED = 0,

        DETECT_TYPE_FACE        = 2000001001,
        DETECT_TYPE_VEHICLE     = 3001001,
        DETECT_TYPE_PEDESTRIAN  = 3001002,
        DETECT_TYPE_BYCICLE     = 3001003,
        DETECT_TYPE_NONMOTOR    = 3001004, // tricycle
        // todo:
        DETECT_TYPE_PLATE       = 5,
    };
#else
    enum class DetectType
    {
        DETECT_TYPE_UNDEFINED = -1,

        DETECT_TYPE_FACE        = 0,
        DETECT_TYPE_VEHICLE     = 1,
        /**
         * for models not distinguish tri-bycicle and bycicle, this for both, otherwise for tri-bycicle only
         */
#if HISI
        DETECT_TYPE_PEDESTRIAN  = 2,
        DETECT_TYPE_BYCICLE     = 3,
        DETECT_TYPE_NONMOTOR    = 4,
#else
        DETECT_TYPE_NONMOTOR    = 2,
        DETECT_TYPE_PEDESTRIAN  = 3,
        DETECT_TYPE_BYCICLE     = 4,
#endif
        DETECT_TYPE_PLATE       = 5,

        DETECT_TYPE_WINDOW      = 6,

        DETECT_TYPE_HEAD_SHOULDER = 7,
        
        DETECT_TYPE_MARKER_GLOBAL        = 20,
        DETECT_TYPE_MARKER_MOT           = 21,
        DETECT_TYPE_MARKER_SUNVISOR      = 22,
        DETECT_TYPE_MARKER_OTHER         = 23,
        DETECT_TYPE_MARKER_BELT          = 24,
        DETECT_TYPE_MARKER_ACCESSORIES   = 25,
        DETECT_TYPE_MARKER_TISSUEBOX     = 26,

        DETECT_TYPE_SKY_LIGHT = 31,
        DETECT_TYPE_ANTENNA = 32,
        DETECT_TYPE_ROOF_LUGGAGE_RACK = 33,

        DETECT_TYPE_TEMPLICENSE = 34,
        DETECT_TYPE_RED_ROPE    = 35,

        DETECT_TYPE_CROSSWALK = 36,

        DETECT_TYPE_PART_COVER = 50,
        DETECT_TYPE_ALL_COVER = 51,
        DETECT_TYPE_NORMAL_PLATE = 52,
        DETECT_TYPE_NO_PLATE = 53,
        DETECT_TYPE_DIRTY_COVER = 54,
        DETECT_TYPE_OTHER_COVER = 55,
        DETECT_TYPE_BLUR_PLATE = 56,
        DETECT_TYPE_LIGHT_PLATE = 57,

        DETECT_TYPE_RISK_MARK = 40,
        DETECT_TYPE_DANGER_SIGN = 41,
        DETECT_TYPE_TRAFFIC_LIGHTS = 42,
        DETECT_TYPE_ROADCONSTRUCT = 43,
        DETECT_TYPE_SLAGHEAD = 44,
        DETECT_TYPE_TRAFFICSIGN = 45,

        DETECT_TYPE_POI_WATER = 60,
        DETECT_TYPE_POI_PHONE = 61,
        DETECT_TYPE_POI_PALM = 62,
        DETECT_TYPE_POI_FACE = 63,

        DETECT_TYPE_MAX,
    };
#endif

    template <typename _Tp>
    class BBox_ {
    public:
        BBox_() = default;
        BBox_(DetectType type, cv::Rect_<_Tp> &rect, Confidence confidence) :
            type_((decltype(type_))type), rect_(rect), confidence_(confidence) {
        }

        inline bool is(DetectType type) const { return type_ == ((decltype(type_)) type); }
#if DG_UUID
        TagId           type_ = INVALID_TAG_ID;
#else
        DetectType      type_ = DetectType::DETECT_TYPE_UNDEFINED;
#endif
        cv::Rect_<_Tp>  rect_;
        Confidence      confidence_ = 0.0f;
    };

    typedef BBox_<int>      BBoxi;
    typedef BBox_<float>    BBoxf;
    typedef BBox_<double>   BBoxd;
    typedef BBoxi           BBox;

    ///////////////////////////////////////////////////////////////////////////////////
    ////////                        Range Class Definition                    /////////
    ///////////////////////////////////////////////////////////////////////////////////
    template <typename _Tp>
    class Range_ {
    public:
        Range_() = default;
        Range_(_Tp start, _Tp end) : start_(start), end_(end) {

        }

        bool in(const _Tp &val) const {
            return val >= std::min(start_, end_) && val <= std::max(start_, end_);
        }
        _Tp start_ = 0;
        _Tp end_   = 0;
    };

    typedef Range_<int> Rangei;
    typedef Range_<float> Rangef;
    typedef Range_<double> Ranged;
    typedef Rangei Range;

    ///////////////////////////////////////////////////////////////////////////////////
    ////////                   Transaction Related Data Types                 /////////
    ///////////////////////////////////////////////////////////////////////////////////


    /**
     * Region of interest, a closed shape defined by points
     */
    using InterestRegion = std::vector<cv::Point>;
    using InterestRegion2f = std::vector<cv::Point2f>;

    ///////////////////////////   Tracking   //////////////////////////////////////////

    enum class DgTrackSpeed
    {
        DG_TRACK_SPEED_FAST = 0, // tracking fast speed
        DG_TRACK_SPEED_MED, // tracking middle speed
        DG_TRACK_SPEED_SLOW, // tracking slow speed
        DG_TRACK_SPEED_UNKNOWN // tracking unknown speed
    };

    enum class DgTrackStatus
    {
        DG_TRACK_STATUS_DETECTED = 0, // first detected input
        DG_TRACK_STATUS_TRACKED,      // tracked
        DG_TRACK_STATUS_MISSED,       // temporal missing
    };
    /**
     * Track direction
     */
    typedef struct
    {
        int up_; // tracking up and down direction, < 0: up, > 0: down, 0: stay still
        int left_; // tracking left and right direction
    } DgTrackDirection;

    typedef struct {
        std::vector<BBox>   boxes_;
        StreamId            streamid_;
        FrameId             frameid_;
    }DetObject;

    typedef struct {
        BBox box_;
        TrackId             track_id_;
        ReferenceId         ref_id_ = 0;
        DgTrackSpeed        speed_;
        DgTrackDirection    direction_;
        DgTrackStatus       status_;
        DG_F32              rect_quality_ = 0.0f;
    } TrackObject ;

    typedef struct {
        StreamId                    stream_id_;
        FrameId                     frame_id_;
        std::vector<TrackObject>    objects_;
        std::vector<TrackId>        kill_id_; // tracking droped id
    } DgTrack;


    ///////////////////////////   Analysis   /////////////////////////////////
    typedef struct {
        DG_F32 is_face;
        DG_F32 blur;
        DG_F32 yaw;
        DG_F32 pitch;
        DG_F32 roll;
        DG_F32 border;
    } FaceQualityFactors;


    typedef struct {
        DG_F32 local;
    } VehicleQuality;

    /////////////////////////       Face       ///////////////////////////////////
    enum class FaceImageType{
        MONITORING_SCENE = 0,
        FACE_IN_CAR = 1,
        PEDESTRIAN = 2,
        LICENSE = 3
    };



    /////////////////////////       Plate       ///////////////////////////////////
#if DG_UUID
    enum class PlateColor : TagId{
        LP_COLOUR_UNKNOWN = 401001001,
        LP_COLOUR_BLUE = 401001002,
        LP_COLOUR_YELLOW = 401001003,
        LP_COLOUR_WHITE = 401001004,
        LP_COLOUR_BLACK = 401001005,
        LP_COLOUR_GREEN = 401001006,
        LP_COLOUR_YELLOWGREEN = 401001007,
        LP_COLOUR_HALFGREEN = 401001008,
    };
#else
    enum class PlateColor{
        LP_COLOUR_UNKNOWN = 0,
        LP_COLOUR_BLUE,
        LP_COLOUR_YELLOW,
        LP_COLOUR_WHITE,
        LP_COLOUR_BLACK,
        LP_COLOUR_GREEN,
        LP_COLOUR_YELLOWGREEN,
        LP_COLOUR_HALFGREEN,
    };
#endif

    enum class PlateCategory{
        LP_UNKNOWN = 0,
        LP_BLUE = 1,
        LP_BLACK = 2,
        LP_SINGLE_YELLOW = 3,
        LP_DOUBLE_YELLOW = 4,
        LP_POLICE = 5,
        LP_WUJIN = 6,
        LP_SINGLE_JUNPAI = 8,
        LP_DOUBLE_JUNPAI = 9,
        LP_AMBASSY = 10,
        LP_YUEGANG = 11,
        LP_AGRICULTURE = 12,
        LP_SMALL_NEW_ENERGY = 13,
        LP_BIG_NEW_ENERGY = 14,
        LP_NONVEHICLE = 15,
        LP_GUA = 17,
        LP_COUSULATE = 18,
        LP_YUEAO = 19,
        LP_BIG_VEHICLE = 20,
        LP_JIAXIAO = 1000,

        LP_POSTPONE = 3000,
        LP_WUPAI,
        LP_HONGKONG,

    };

    enum class PlateFilterSetFunc {
        ENABLE_NONE                 = 0,
        CORRECT_FILTER_ENABLE       = 0X01,
        RULEMATCH_FILTER_ENABLE     = 0X02,
        LOCATION_FILTER_ENABLE      = 0X04,
        SECURITY_FILTER_ENABLE      = 0X08,
        PETRO_STATION_CONF_ENABLE   = 0X10,
        OLD_FILTER_RULE_ENABLE      = 0X20,
    };

    inline PlateFilterSetFunc operator|(PlateFilterSetFunc a, PlateFilterSetFunc b)
    {return static_cast<PlateFilterSetFunc>(static_cast<int>(a) | static_cast<int>(b));}

    inline PlateFilterSetFunc operator&(PlateFilterSetFunc a, PlateFilterSetFunc b)
    {return static_cast<PlateFilterSetFunc>(static_cast<int>(a) & static_cast<int>(b));}

    typedef struct {
        int   TextPattern;                                              // value: 1, 2, 3
        float TextConfidence;                                           // Filter plates below this confidence level
    } PetroStationConf;

    const wchar_t PLATE_SPLITTER = L'|';

    typedef struct {
        PlateColor color_;
        Confidence color_confidence_;
        PlateCategory category_;
        /**
         * for double line, an extra L"|" will be used to split between upper and lower line
         */
        std::wstring literal_;
        std::vector<Confidence> literal_confidence_;
        Confidence average_confidence_;
        Confidence province_confidence_; // 1.0f if no province
        BBox bbox_;


        bool double_line_ = false;
#if 0
        inline bool isDoubleLine() {
            return literal_.find(PLATE_SPLITTER) != std::wstring::npos;
        }
#endif

        std::wstring getString() {
            auto pos = literal_.find(PLATE_SPLITTER);
            if (pos == std::wstring::npos) {
                return literal_;
            } else {
                auto cp = literal_;
                cp.erase(pos, 1);
                return cp;
            }
        }

    } PlateInfo;

    typedef struct {
        std::vector<std::pair<int, float>> pose; // front, end
        std::vector<std::pair<int, float>> type; // car, bus, etc.
        std::vector<std::pair<int, float>> make; // Benz, BMW, Audi, etc.
        std::vector<std::pair<int, float>> model; // A6, A8, etc
        std::vector<std::pair<int, float>> year; // 2012, 2009, etc
        std::vector<std::pair<int, float>> color; // black, white, etc
    } VehicleBrand;


    typedef struct {
        std::pair<int, float> pose_; // front, end
        std::pair<int, float> type_; // car, bus, etc.
        std::pair<int, float> make_; // Benz, BMW, Audi, etc.
        std::pair<int, float> model_; // A6, A8, etc
        std::pair<int, float> year_; // 2012, 2009, etc
    }VehicleBrand_Ret;

    typedef struct {
        bool isSmoke = false;
        float smokeConf = 0;
        bool isPhone = false;
        float phoneConf = 0;
    }PedPhoneSmoke;


    class ClassifyAttribute {
    public:
        int idx;
        std::string name;
        float confidence;
        TagId mappingId;
        bool trueValue; // passed threshold or not
    };
    ////////////////////////////////////////////////////////////////////////////////
    /**
     * for an image which you do not know its format, just give image
     * which will be decoded by OpenCV
     */
    enum class SdkImage { BGR, ARGB, NV12, NV21, H264, H265, MPEG4, VC1, VP8, VP9, JPEG, PNG, IMAGE, GRAY, YUV420P};

    /**
     * Task input data definition.
     *
     * Use type_/data_/data_len_ to describe an input image, stride_ and size_ must be present if
     * this image is an decoded frame like nv12 or bgr.., or your may set stream_id_/frame_id_
     * instead to refer to a frame inside vega image pool.
     *
     * roi_ is optional to describe an interested area as a box.
     * landmark_ is optional to describe a serial floating points to describe landmarks.
     *
     * user_data_ is reserved for caller, vega will keep it without changing.
     *
     * error_ is set to DG_ON_GOING on creation and will be set to a code other than DG_ON_GOING
     * to indicate processing result.
     *
     * A serial put/get interface allow caller to fill options with variable data types. These
     * options will be used by interface to adjust functionality. Each interface may declares
     * its supported options, see interface declaration and vega_option.h.
     *
     * You also may define options that not defined in vega_option.h to save some data inside
     * task and retrieve them when task is done.
     */
    class SdkTaskBase {
    public:
        SdkTaskBase() = default;
        virtual ~SdkTaskBase() = default;

    public:
        /**
         * Put value into map
         */
        void put(const std::string &key, bool value) {
            std::string v = value ? "1" : "0";
            values_[key] = v;
        }
        void put(const std::string &key, const std::string &value) {
            values_[key] = value;
        }
        void put(const std::string &key, int value) {
            values_[key] = std::to_string(value);
        }
        void put(const std::string &key, long int value) {
            values_[key] = std::to_string(value);
        }
        void put(const std::string &key, float value) {
            values_[key] = std::to_string(value);
        }
        void put (const std::string &key, StreamId value){
            values_[key] = std::to_string(value);
        }
        void put(const std::string &key, wchar_t value) {
            std::wstring wstr;
            wstr += value;
            values_[key] = ws2s(wstr);
        }

        void put(const std::string &key, const std::vector<float> &value) {
            vf_values_[key] = value;
        }

        /**
         * Get property with default value if not exist
         */
        bool getBool(const std::string &key, bool def) {
            auto str = find(key);
            if(str.empty()) return def;
            if(str == "1") return true;
            if(str == "0") return false;
            CHECK(false) << "invalid bool key " << key << " value " << str;
            return false;
        }

        std::string getString(const std::string &key, const std::string &def) {
            auto str = find(key);
            if(str.empty()) return def;
            return str;
        }
        int getInteger(const std::string &key, int def) {
            auto str = find(key);
            if(str.empty()) return def;
            return atoi(str.c_str());
        }
        long int getLongInt(const std::string &key, int def) {
            auto str = find(key);
            if(str.empty()) return def;
            return atol(str.c_str());
        }
        StreamId getLLInt(const std::string &key, int def){
            auto str = find(key);
            if(str.empty()) return def;
            return atoll(str.c_str());
        }

        float getFloat(const std::string &key, float def) {
            auto str = find(key);
            if(str.empty()) return def;
            return atof(str.c_str());
        }
        wchar_t getWChar(const std::string &key, wchar_t def) {
            auto str = find(key);
            if(str.empty()) return def;
            auto wstr = s2ws(str);
            CHECK(wstr.size() == 1) << "invalid wchar " << str;
            return wstr[0];
        }



        /**
         * Get property without default value
         * if key not exist, dead
         */
        bool getBool(const std::string &key) {
            auto str = get(key);
            if(str == "1") return true;
            if(str == "0") return false;
            CHECK(false) << "invalid bool " << str;
            return false;
        }
        std::string getString(const std::string &key) {
            return get(key);
        }
        int getInteger(const std::string &key) {
            return atoi(get(key).c_str());
        }
        float getFloat(const std::string &key) {
            return atof(get(key).c_str());
        }
        wchar_t getWChar(const std::string &key) {
            auto str = get(key);
            auto wstr = s2ws(str);
            CHECK(wstr.size() == 1) << "invalid wchar " << str;
            return wstr[0];
        }

        void getVecFloat(const std::string &key, std::vector<float> &vec) {
            vec.clear();
            auto it = vf_values_.find(key);
            if(it != vf_values_.end()) {
                vec = it->second;
            }
        }

        const std::map<std::string, std::string>& options() {
            return values_;
        }
        void dumpFrom(std::map<std::string, std::string> &mapOut) {
            values_ = mapOut;
        }

    protected:
        std::string &get(const std::string &key) {
            auto it = values_.find(key);
            CHECK(it != values_.end()) << "Key [" << key << "] must be set";
            return it->second;
        }
        std::string find(const std::string &key) {
            auto it = values_.find(key);
            if(it != values_.end()) return it->second;
            return "";
        }

        using convert_typeX = std::codecvt_utf8<wchar_t>;

        std::wstring s2ws(const std::string& str)
        {
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            return converterX.from_bytes(str);
        }

        std::string ws2s(const std::wstring& wstr)
        {
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            return converterX.to_bytes(wstr);
        }
    public:
        SdkImage type_ = SdkImage :: NV12;
        /**
         * Task image data
         * the memory depends on platform and application
         * For HIAI or CUDA: use CPU memory
         */
        uint8_t *data_ = nullptr;
        /**
         * bytes of data_
         */
        int data_len_ = 0;

        /**
         * size of padded image in bytes, MUST be set if size_ is not empty
         */
        cv::Size stride_;
        /**
         * size of image, in pixel
         * Must be set while data_ is not nullptr, and image is a decoded image
         */
        cv::Size size_;
        /**
         * Region of Interest, optional
         */
        cv::Rect roi_;
        /**
         * Landmark, optional
         */
        std::vector<Landmark> landmark_;

        /**
         * image stream id, optional
         * For video in HIAI, stream_id_ should be set between [0, 15], and we
         * suggest that you set it beyond that range for image streams.
         */
        StreamId stream_id_ = INVALID_STREAM_ID;
        /**
         * image frame id, optional
         */
        FrameId frame_id_ = 0;

        /**
         * User provided data
         */
        void *user_data_ = nullptr;

        /**
         * Task status
         * Initially set to DG_ON_GOING, while task is done, error_ will be set
         * to DG_OK if succeeds, DG_ERR_XXX if error occurs
         */
        DgError error_ = DG_ON_GOING;

        VegaTpGrpSP tp_grp_;
    protected:
        std::map<std::string, std::string> values_;
        std::map<std::string, std::vector<float>> vf_values_;
    };

    using ModelDecryptor = int (*)(const void *src, size_t src_len, void *dst, size_t *dst_len);

    typedef void *              VegaStream;
#define VEGA_INVALID_STREAM     (VegaStream *)nullptr

    enum class AIimageBgColorOption {
        BLACK = 0X01,
        WHITE = 0X02,
    };
    inline AIimageBgColorOption operator|(AIimageBgColorOption a, AIimageBgColorOption b)
    {return static_cast<AIimageBgColorOption>(static_cast<int>(a) | static_cast<int>(b));}

    inline AIimageBgColorOption operator&(AIimageBgColorOption a, AIimageBgColorOption b)
    {return static_cast<AIimageBgColorOption>(static_cast<int>(a) & static_cast<int>(b));}

    enum class AIimageProcessType {
        img_split = 0,
        pixel_sum = 1,
    };

    typedef struct AIimageData {
        std::vector<cv::Rect> rects;
        float pixel_sum;
    } AIimageData;

    enum class TransformType {
        WARP_AFFINE = 1,
    };

}; // end namespace vega

#endif //DG_TYPES_H_
