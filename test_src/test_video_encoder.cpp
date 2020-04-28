//
// Created by guli on 2019/8/26.
//

#include "vega_interface.h"
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>
#include <zfz/zfz_event.hpp>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "station/thread_pool.h"

int ROUND = -1;
int device_id_ = 0;
int test_round = 0;
vega::SdkImage h26x_type = vega::SdkImage::H264;
std::string output_path_;
std::string output_filename;
std::vector<std::string> h264_list_;
vega::DoableStation  s_enc_video("EncVideo"), s_free_frame("FreeFrame");

std::shared_ptr<vega::DecodeInterface> decoder;
std::shared_ptr<vega::FetchFrameInterface > fetcher;
std::shared_ptr<vega::FreeFrameInterface > freer;
std::shared_ptr<vega::EncodeInterface> encoder;

using namespace vega;

static std::string & ltrim(std::string & str)
{
    auto it2 =  std::find_if( str.begin() , str.end() , [](char ch){ return !std::isspace<char>(ch , std::locale::classic() ) ; } );
    str.erase( str.begin() , it2);
    return str;
}

static std::string & rtrim(std::string & str)
{
    auto it1 =  std::find_if( str.rbegin() , str.rend() , [](char ch){ return !std::isspace<char>(ch , std::locale::classic() ) ; } );
    str.erase( it1.base() , str.end() );
    return str;
}

static std::string &trim(std::string &str) {
    return ltrim(rtrim(str));
}

static bool endwith(const std::string &str, const std::string &substr) {
    if(str.length() < substr.length()) return false;

    return str.substr(str.length() - substr.length()) == substr;
}

void prepare(const std::string &outPath) {
    auto resPath = outPath;
    auto pos = resPath.rfind('/');
    if(pos == std::string::npos) {
        resPath = "./";
    } else {
        resPath = resPath.substr(0, pos+1);
    }
    resPath += "codec";

    if(access(resPath.c_str(), W_OK) < 0) {
        if(mkdir(resPath.c_str(), 0755) < 0) {
            CHECK(false)<< "Create dir fail: " << resPath;
        }
    }

    output_path_ = resPath;

    LOGFULL << "Output to " << output_path_;
}

void read_list(const std::string &vdpath) {

    std::ifstream fp(vdpath);
    CHECK(fp) << "Image list open fail: " << vdpath;
    std::string line;

    while(std::getline(fp, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        bool esc = false;
        if(line[0] == '"') {
            auto p = line.find('"', 1);
            CHECK(p != line.npos) << "line start with \" but not end with it: " << line;
            for(int i = 1; i < (int)p; i++) {
                if(line[i] == ',') line[i] = '?';
            }
            esc = true;
        }
        std::vector<std::string> vec;
        boost::split(vec, line, boost::is_any_of(","));

        for(auto &s : vec) {
            trim(s);
        }

        CHECK(!vec.empty());

        if(esc) {
            for(auto &c : vec[0]) {
                if (c == '?') c = ',';
            }
            for(auto it = vec[0].begin(); it != vec[0].end(); ) {
                if(*it == '"') {
                    it = vec[0].erase(it);
                } else {
                    it ++;
                }
            }
        }

        if(endwith(vec[0], ".jpg")) {
            h264_list_.push_back(vec[0]);
        } else {
            LOG(WARNING) << "Only supported h264, Skip" << vec[0];
        }
    }

    LOGFULL << "h264 " << h264_list_.size();
}

#define SID 1
void sendEncode(FrameId fid, bool eos);
void sendFetch(FrameId fid , bool eos);
void sendFree(FrameId fid, bool eos);
void writeH26x(DG_U8 * data, int len);
void writeSingleH264(DG_U8 *data , int len);
zfz::Event g_evt;

void init()
{
    int id = 1;
    decoder = createDecodeInterface(
            device_id_, "", Model::decode_frame, nullptr,
            [=](std::vector<std::shared_ptr<DecodeTask>> &tasks, DgError error) {
                LOGFULL << "Decoder " << id << " done";
                CHECK(error == DG_OK);
                auto evt = (zfz::Event *)tasks[0]->user_data_;
                evt->set();

                auto fid = tasks[0]->frame_id_;
                auto eos = tasks[0]->getBool("eos");
                auto doable = std::make_shared<CallbackDoable>();
                doable->setCallback([=](void) { sendEncode(fid, eos); /*sendFetch(fid,eos);*/ });
                s_enc_video.put(doable);
            });
    CHECK(decoder);

    fetcher = createFetchFrameInterface(
            device_id_,"",Model::fetch_frame,nullptr,
            [=](std::vector<std::shared_ptr<FetchFrameTask>> &tasks,DgError error){
                LOGFULL << "Fetcher " << id << " done";
                uint8_t *frame_data = tasks[0]->result_.data_.get();
                int frame_size = tasks[0]->result_.data_len_;

                FILE *fp = fopen("/home/vse/encode-output/test.yuv" , "ab");
                fwrite(frame_data,frame_size,1,fp);
                fclose(fp);
            });

    freer = createFreeFrameInterface(
            device_id_, "", Model::delete_frame, nullptr,
            [=](std::vector<std::shared_ptr<FreeFrameTask >> &tasks, DgError error) {
                CHECK(error == DG_OK);
                auto evt = (zfz::Event *)tasks[0]->user_data_;
                CHECK(evt);
                evt->set();
            });
    CHECK(freer);

    encoder = createEncodeInterface(
            device_id_, "", Model::encode_video, nullptr,
            [=](std::vector<std::shared_ptr<EncodeTask>> &tasks, DgError error) {
                LOGFULL << "Encode " << id << " done";
                CHECK(error == DG_OK);
#if !NEWCUDA
                auto evt = (zfz::Event *)tasks[0]->user_data_;
                CHECK(evt);
                evt->set();
#endif

                auto fid = tasks[0]->frame_id_;
                auto eos = tasks[0]->getBool("eos");

                if(fid % 100 == 0) {
                    LOG(ERROR) << "Encode: " << fid;
                }
                auto data = tasks[0]->result_.data_.get();
                auto len = tasks[0]->result_.data_len_;

                auto doable = std::make_shared<CallbackDoable>();
		doable->setCallback([=](void) {
				writeH26x(data, len);
				//writeSingleH264(data,len);
				sendFree(fid, eos);
				});
                s_free_frame.put(doable);
            });
    CHECK(encoder);

}

void sendJpg() {
#if 1
    for(auto i = 0u; i < h264_list_.size(); i++) {
#else
    for(auto i = 0u; i < h264_list_.size(); i = (i+1) % h264_list_.size()) {
#endif
        auto &line = h264_list_[i];
        zfz::Event event;
        std::vector<std::shared_ptr<DecodeTask>> tasks;
        auto task = std::make_shared<DecodeTask>();
        /////////////////////////////////////////////////////////////
        FILE* fp = fopen(line.c_str(), "rb");
        CHECK(fp) << "File not exist: " << line;

        fseek(fp,0L,SEEK_END);
        auto sz = ftell(fp);
        fseek(fp,0L,SEEK_SET);

        auto bin = new char[sz];
        fread(bin, 1, sz, fp);
        fclose(fp);
        /////////////////////////////////////////////////////////////

        task->type_ = vega::SdkImage::JPEG;
        task->stream_id_ = SID;
        task->data_ = (uint8_t *)bin;
        task->data_len_ = (int)sz;
        LOGFULL << "Decode File " << line;
        task->user_data_ = &event;

#if NEWCUDA
        static const std::string sdecode_out_type = "nv12";
        task->put("decode_output_type_",sdecode_out_type);
#endif

#if 1
        if(i == h264_list_.size()-1) {
            task->put("eos", true);
            LOG(ERROR) << "Send last jpeg";
        } else {
            task->put("eos", false);
        }
#else
            task->put("eos", false);
#endif
        tasks.push_back(task);
        decoder->execute(tasks);
        event.wait();
        event.reset();

#if NEWCUDA
        usleep(40000);
#endif

#if 0
        if(i == h264_list_.size()-1) {
            LOG(ERROR) << "Pause 10s";
            sleep(10);
            LOG(ERROR) << "Resume";
        }
#endif
    }
}

void writeH26x(DG_U8 * data, int len) {
    if(test_round == 0) {
        FILE* h26x_file = fopen(output_filename.c_str(), "ab");
        fwrite(data, 1, (size_t)len, h26x_file);
        fclose(h26x_file);
    }
}


void writeSingleH264(DG_U8 * data , int len){
    if(test_round == 0){
        static int index = 0;
        char file_path[256];
        sprintf(file_path,"/home/vse/encode-output/%d.h264",index++);
        FILE* h264_file = fopen(file_path,"wb");
        fwrite(data,1,(size_t)len,h264_file);
        fclose(h264_file);
    }
}

void sendFetch(FrameId fid , bool eos){
    std::vector<std::shared_ptr<FetchFrameTask >> tasks;
    std::shared_ptr<FetchFrameTask > task = std::make_shared<FetchFrameTask >();
    task->stream_id_ = SID;
    task->frame_id_ = fid;
    task->type_ = SdkImage ::NV12;
    task->put("video_eos",eos);
    tasks.push_back(task);
    fetcher->execute(tasks);
}

void sendEncode(FrameId fid, bool eos) {
    zfz::Event event;
    std::vector<std::shared_ptr<EncodeTask>> encode_tasks;
    auto task = std::make_shared<EncodeTask>();
    task->type_ = h26x_type;       //H265 or H264
    task->stream_id_ = SID;
    task->frame_id_ = fid;
    task->put("key_frame_interval", 3); //I frame interval,if not set,default is 16
    task->put("video_resize_ratio", 0.5f); //only effect in  hiai ,  ration of vidoe resolution to resize,if not set, default is 1.0; 
    task->put("video_eos", eos);
    task->put("eos", eos);

    static int nCount = 0;
    LOG(INFO) << "====> send encoder count " << nCount++;

    if(fid == 0) {
        task->put("force_i_frame", true);
    } else {
        task->put("force_i_frame", false);
    }

#if NEWCUDA
    //for cuda encode
    task->put("encoder_fps" , 25);
    task->put("encoder_intype", (int)vega::SdkImage::NV12);
    task->put("encoder_outtype", (int)vega::SdkImage::H264);
#endif

    task->user_data_ = &event;
    encode_tasks.push_back(task);
    encoder->execute(encode_tasks);

#if !NEWCUDA
    event.wait();
    event.reset();
#endif
}

void sendFree(FrameId fid, bool eos) {
    //LOG(ERROR) << "Free " << fid;
    std::vector<std::shared_ptr<FreeFrameTask>> tasks;
    zfz::Event event;
    auto task = std::make_shared<FreeFrameTask>();
    task->type_ = vega::SdkImage::JPEG;
    task->stream_id_ = SID;
    task->frame_id_ = fid;
    task->user_data_ = &event;

    tasks.push_back(task);
    freer->execute(tasks);
    event.wait();
    event.reset();

    if(eos) {
        LOG(ERROR) << "Test done";
        g_evt.set();
    }
}

int main(int argc, char *argv[]) {
    if(argc < 6) {
        LOG(ERROR) << "Arg count: " << argc;
        LOG(ERROR) << "Usage: " << argv[0] << " <device_id> <image_list> <output_path> <h264 or h265> <round>";
        return 2;
    }

    device_id_ = atoi(argv[1]);
    CHECK(device_id_ >= 0) << "Device ID: " << device_id_;

    std::string respath = argv[2];
    read_list(respath);
    CHECK(!h264_list_.empty());

    std::string outpath = argv[3];
    prepare(outpath);
    std::string str_h26x_type=argv[4];
    if(str_h26x_type.compare("h265")==0)
    {
	    h26x_type=vega::SdkImage::H265;
    }
    ROUND = atoi(argv[5]);
    CHECK(ROUND > 0) << "Invalid Round: " << ROUND;

    SDKInit("");
    init();

    output_filename = outpath+ "/" + "video_encoder.";
    if(h26x_type==SdkImage::H265)
    {
	    output_filename +="h265";
    }
    else
    {
	    output_filename +="h264";
    }
    FILE* h26x_file = fopen(output_filename.c_str(), "wb");
    fclose(h26x_file);
    LOG(ERROR) << "Output file: " << output_filename;

    for(test_round = 0; test_round < ROUND; test_round++) {
        LOG(ERROR) << "Start round " << test_round;
        sendJpg();
        auto ret = g_evt.wait(40 * 1000);
        // hiai encoder has an unsolved problem on destroy
        // we check here so that the test won't be stuck
        // and device will not be occupied
        CHECK(ret == zfz::ZFZ_EVENT_SUCCESS) << " Please retry your ci test";
        g_evt.reset();
        LOG(ERROR) << "Round " << test_round << " Done";
        //sleep(10);
    }

    decoder.reset();
    freer.reset();
    encoder.reset();
    fetcher.reset();
    SDKDestroy();
    return 0;
}
