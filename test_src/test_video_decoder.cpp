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
#include <sys/time.h>
#include <signal.h>
#include <vega_option.h>
#include "read_image_list.h"

using namespace vega;

std::shared_ptr<FetchFrameInterface> fetcher;
std::shared_ptr<FreeFrameInterface> freer;
#define SID 100

void fetchFrame(FrameId fid) {
    if(!fetcher) return;

    LOG(ERROR) << "Fetch " << fid;
    std::vector<std::shared_ptr<FetchFrameTask >> tasks;

    auto task = std::make_shared<FetchFrameTask >();
    task->stream_id_ = SID;
    task->frame_id_ = fid;
    task->type_ = SdkImage ::JPEG;

    tasks.push_back(task);
    auto err = fetcher->execute(tasks);
    CHECK(err == DG_OK);

}
void deleteFrame(FrameId fid) {
    if(!freer) return;

    std::vector<std::shared_ptr<FreeFrameTask>> tasks;

    auto task = std::make_shared<FreeFrameTask>();
    task->stream_id_ = SID;
    task->frame_id_ = fid;

    tasks.push_back(task);
    auto err = freer->execute(tasks);
    CHECK(err == DG_OK);
}

int main(int argc, char *argv[]) {
    if(argc < 5) {
        LOG(ERROR) << "Arg count: " << argc;
        LOG(ERROR) << "Usage: " << argv[0] << " <device_id> <image_list> <round> <fps> <jpegdir>";
        return 2;
    }

    int round = -1; // how many rounds
    int fps = 25;
    int device_id_ = 0;
    std::vector<std::string> videoList;

    device_id_ = atoi(argv[1]);
    CHECK(device_id_ >= 0) << "Device ID: " << device_id_;

    std::string respath = argv[2];
    auto vtype =ReadImageList::read_list(videoList, respath);
    CHECK(!videoList.empty());
    LOG(ERROR) << "Video list size: " << videoList.size();

    round = atoi(argv[3]);
    CHECK(round > 0) << "Invalid Round: " << round;

    fps = atoi(argv[4]);
    CHECK(fps > 0) << "Invalid FPS: " << fps;
    long intv = 1000000 / fps; // us of one frame

    std::string jpegDir;
    if(argc > 5) {
        jpegDir = argv[5];
        if(!jpegDir.empty()) {
            if(access(jpegDir.c_str(), W_OK) < 0) {
                if(mkdir(jpegDir.c_str(), 0755) < 0) {
                    CHECK(false)<< "Create dir fail: " << jpegDir;
                }
            }
        }
    }


    SDKInit("");

    freer = createFreeFrameInterface(
            device_id_, "", Model::delete_frame, nullptr,
            [&](std::vector<std::shared_ptr<FreeFrameTask >> &tasks, DgError error) {
            });
    CHECK(freer);

    fetcher = createFetchFrameInterface(
            device_id_, "", Model::fetch_frame, nullptr,
            [&](std::vector<std::shared_ptr<FetchFrameTask >> &tasks, DgError error) {
                if(error == DG_OK) {
                    std::ofstream ofs;
                    std::string path = jpegDir + "/" + std::to_string(tasks[0]->frame_id_) + ".jpg";
                    ofs.open(path, std::ios::trunc | std::ios::binary);
                    CHECK(ofs.is_open());
                    ofs.write((const char *)tasks[0]->result_.data_.get(), tasks[0]->result_.data_len_);
                    deleteFrame(tasks[0]->frame_id_);
                }
            });
    CHECK(fetcher);

    for(auto test_round = 0; test_round < round; test_round++) {
        LOG(ERROR) << "Start round " << test_round;

        zfz::Event g_evt;
        std::atomic_int rcv{0};
        auto decoder = createDecodeInterface(
                device_id_, "", Model::decode_video, nullptr,
                [&](std::vector<std::shared_ptr<DecodeTask>> &tasks, DgError error) {
                    ++rcv;
                    if(error != DG_OK) {
                        LOG(ERROR) << "Seq " << (long) tasks[0]->user_data_ << " failed: " << error;
                    }
                    if(tasks[0]->data_) {
                        delete [] tasks[0]->data_;
                    }

                    if(tasks[0]->getBool(Option::video_eos_)) {
                        g_evt.set();
                    } else if(error == DG_OK && !tasks[0]->getBool(Option::discard_frame_)) {
                        fetchFrame(tasks[0]->frame_id_);
                    }

                });
        CHECK(decoder);

        long seq = 0;
        bool needFetch = !jpegDir.empty() && test_round == 0;
        for(auto &line : videoList) {
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

            task->type_ = vtype;
            task->stream_id_ = SID;
            task->data_ = (uint8_t *)bin;
            task->data_len_ = (int)sz;
            task->user_data_ = (void *)seq;
            task->put(Option::video_eos_, false);
            task->put(Option::discard_frame_, !needFetch);
            task->put(Option::video_dec_mode_e_,1);
            //for cuda decoder
            task->put(Option::packet_index_,seq++);

            tasks.push_back(task);
            decoder->execute(tasks);

            usleep(intv);
        }

#if NEWCUDA
        {
            std::vector<std::shared_ptr<DecodeTask>> tasks;
            auto task = std::make_shared<DecodeTask>();
            task->type_ = vtype;
            task->stream_id_ = SID;
            task->user_data_ = (void *)seq++;
            task->put(Option::flush_decoder_, true);
            task->put(Option::video_eos_, false);
            task->put(Option::packet_index_,seq);
            task->put(Option::discard_frame_, true);

            tasks.push_back(task);
            decoder->execute(tasks);
            sleep(1);
        }
#endif
        {
            std::vector<std::shared_ptr<DecodeTask>> tasks;
            auto task = std::make_shared<DecodeTask>();
            task->type_ = vtype;
            task->stream_id_ = SID;
            task->user_data_ = (void *)seq++;
            task->put(Option::video_eos_, true);

            //for cuda decoder
            task->put(Option::packet_index_,seq);

            tasks.push_back(task);
            decoder->execute(tasks);

            if(g_evt.wait(60*1000) != zfz::ZFZ_EVENT_SUCCESS) {
                LOG(ERROR) << "Failed waiting";
            }
            g_evt.reset();
        }
        CHECK(rcv.load() == (long)videoList.size()+1);

        if(needFetch) {
            sleep(3); // sleep for a while to let fetcher and free done
        }
        LOG(ERROR) << "Round " << test_round << " Done";
    }

    fetcher.reset();
    freer.reset();

    SDKDestroy();
    return 0;
}
