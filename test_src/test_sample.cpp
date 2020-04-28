#include <zfz/zfz_event.hpp>
#include "dg_types.h"
#include "error.h"
#include "interface_base.h"
#include "model_define.h"
#include "vega_interface.h"
#include "vega_option.h"
#include "vega_time_pnt.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;
using namespace vega;

#define STREAM 1
int device_id_;
const int stream_id_ = STREAM;
FrameId frame_id_;
zfz::Event task_done_;
// interfaces
std::shared_ptr<DecodeInterface> decoder_;
std::shared_ptr<FreeFrameInterface> free_frame_;
std::shared_ptr<DetectInterface> detector_;
std::shared_ptr<FetchFrameInterface> fetch_frame_;
std::string getHostModelPath() {
    auto path = std::getenv("VEGA_HOST_MODEL_PATH");
    if(!path || strlen(path) == 0) {
        CHECK(false) << "env VEGA_HOST_MODEL_PATH " << "Must be set";
    }
    return path;
}

void SendDecoder(string source_image) {
    std::vector<std::shared_ptr<DecodeTask>> tasks;
    /**
      * Every task defined with comments to show how to prepare a task
      * check hiai_interface.h
      */
    auto task = std::make_shared<DecodeTask>();

    /////////////////////////////////////////////////////////////
    FILE* fp = fopen(source_image.c_str(), "rb");
    CHECK(fp) << "File not exist: " << source_image;

    fseek(fp,0L,SEEK_END);
    auto sz = ftell(fp);
    fseek(fp,0L,SEEK_SET);

    auto bin = new char[sz];
    fread(bin, 1, sz, fp);
    fclose(fp);
    /////////////////////////////////////////////////////////////
    task->stream_id_ = stream_id_;
    task->data_ = (uint8_t *)bin;
    task->data_len_ = (int)sz;
    task->type_ = SdkImage::JPEG;
    task->user_data_ = &task_done_;
    ///////////////////////////////////////////////////////////
    tasks.push_back(task);
    /**
     * execute, and wait for callback
     */
    decoder_->execute(tasks);
}

void onDecoder(std::vector<std::shared_ptr<DecodeTask>> &tasks, DgError error) {
    CHECK(error == DG_OK);
    LOGFULL << "Decoder done";
    std::vector<std::shared_ptr<DetectTask>> dtasks;
    for(auto &task : tasks) {
        delete task->data_;
        frame_id_ = task->frame_id_;
        CHECK(task->stream_id_ == STREAM);
    }
    task_done_.set();
}

void SendDetector(const int stream_id, FrameId frame_id) {
    std::vector<std::shared_ptr<DetectTask>> dtasks;
    auto dtask = std::make_shared<DetectTask>();
    dtask->stream_id_ = stream_id;
    dtask->frame_id_ = frame_id;
    dtask->user_data_ = &task_done_;
    dtasks.push_back(dtask);
    detector_->execute(dtasks);
}

void onDetector(std::vector<std::shared_ptr<DetectTask>> &tasks, DgError error) {
    CHECK(error == DG_OK);
    LOGFULL << "Detector done";
    std::vector<std::shared_ptr<FreeFrameTask>> ftasks;
    for(auto &task : tasks) {
        int box_id = 0;
        for (auto &box : task->result_) {
            LOG(ERROR) << "box_id:" << box_id << " box_type:" << (int)box.type_
            << " box_confidence:" << box.confidence_ << " box_ROI:" << box.rect_.x << ","
            << box.rect_.y << "," << box.rect_.width << "," << box.rect_.height << std::endl;
        }
    }
    task_done_.set();
}

void FetchFrame(StreamId sid, FrameId fid,int quality=90) {
    LOG(ERROR)<<"fetch_frame_";
    std::vector<std::shared_ptr<FetchFrameTask>> tasks;

    auto task = std::make_shared<FetchFrameTask>();
    task->stream_id_ = sid;
    task->frame_id_ = fid;
    task->user_data_= nullptr;
    task->type_ = SdkImage ::JPEG;
    task->put(Option::jpeg_quality_,quality);
    task->roi_.x=0;
    task->roi_.y=0;
    task->roi_.width=100;
    task->roi_.height=100;
    tasks.push_back(task);
    auto err = fetch_frame_->execute(tasks);
    CHECK(err == DG_OK);
}

void onFetchFrame(std::vector<std::shared_ptr<FetchFrameTask>> &tasks, DgError error) {
    CHECK(error == DG_OK);
    std::ofstream ofs;
    std::string path;
    path="./result/";
    path+= std::to_string(tasks[0]->frame_id_) + ".jpg";
    ofs.open(path, std::ios::trunc | std::ios::binary);
    CHECK(ofs.is_open());
    ofs.write((const char *) tasks[0]->result_.data_.get(), tasks[0]->result_.data_len_);
    ofs.close();
    task_done_.set();
}

void SendFreeFrame(const int stream_id, FrameId frame_id) {
    std::vector<std::shared_ptr<FreeFrameTask>> ftasks;
    auto ftask = std::make_shared<FreeFrameTask>();
    ftask->stream_id_ = stream_id;
    ftask->frame_id_ = frame_id;
    ftask->user_data_ = &task_done_;
    ftasks.push_back(ftask);
    free_frame_->execute(ftasks);
}

void onFreeFrame(std::vector<std::shared_ptr<FreeFrameTask>> &tasks, DgError error) {
    CHECK(error == DG_OK);
    task_done_.set();
}

void create() {
    SDKInit("");
    decoder_ = createDecodeInterface(device_id_, "", Model::decode_frame, nullptr, onDecoder);
    free_frame_ = createFreeFrameInterface(device_id_, "", Model::delete_frame, nullptr, onFreeFrame);
    //detector_ = createDetectInterface(device_id_, getHostModelPath() + "/" + "FaceDetector", "", nullptr, onDetector);
    fetch_frame_ = createFetchFrameInterface(device_id_,"",Model::fetch_frame,nullptr,onFetchFrame);
}

void destroy() {
    decoder_.reset();
    free_frame_.reset();
//    detector_.reset();
    fetch_frame_.reset();
    SDKDestroy();
}

int main(int argc, char *argv[]) {
    device_id_ = atoi(argv[1]);
    string source_image = argv[2];
    create();
    
    // Every frame
    {
        SendDecoder(source_image);
        task_done_.wait();
        task_done_.reset();
      //  SendDetector(stream_id_, frame_id_);
     //   task_done_.wait();
     //   task_done_.reset();
        FetchFrame(stream_id_, frame_id_,100);
        task_done_.wait();
        task_done_.reset();
        SendFreeFrame(stream_id_, frame_id_);
        task_done_.wait();
        task_done_.reset();
    }
    
    destroy();
    return 0;
}
