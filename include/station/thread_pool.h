//
// Created by JiangGuoqing on 2019-06-01.
//


#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <functional>
#include <map>
#include "sys.h"
#include "glog/logging.h"
#include "error.h"
#include "queue/blockingconcurrentqueue.h"
#include "station/block_queue.h"
#include "vega_time_pnt.h"

#ifndef VEGA_THREADPOOL_H
#define VEGA_THREADPOOL_H
namespace vega {

    class Doable {
    public:
        Doable() = default;
        virtual ~Doable() = default;

        virtual void start() = 0;
    };

    using DoableSP = std::shared_ptr<Doable>;

    /**
     * Usage:
     * auto cb = std::make_shared<CallbackDoable>();
     * cb->setCallback([=]() {
     *     // here you put codes to execute while this doable is scheduled
     * });
     * station->push(cb); // push callback doable to station
     */
    class CallbackDoable : public Doable {
    public:
        CallbackDoable() = default;
        ~CallbackDoable() override = default;

        using Callback = std::function<void ()>;
        void setCallback(Callback callback) {
            callback_ = callback;
        }

        void start() override {
            if(callback_) callback_();
        }

    protected:

        Callback callback_;
    };

    class ThreadPool {
    public:
        ThreadPool(const std::string &name) {
            name_ = name;
        }
        ThreadPool() = default;
        virtual ~ThreadPool() {
            destroy();
        }

        void create(int num, bool startMonitor = false) {
            if(!pool_.empty()) {
                LOG(ERROR) << "Duplicate create thread pool";
                return;
            }

            end_ = false;
            for(auto i = 0 ; i < num ; i++) {
                createSingle(i);
            }
            if(startMonitor) {
                monitor_ = std::make_shared<std::thread>(&ThreadPool::monitor, this);
            }
        }
        void destroy() {
            end_ = true;

            DoableSP obj;
            while(q_.try_dequeue(obj)) { }

            for(auto &th : pool_) {
                th->join();
            }
            if(monitor_) {
                monitor_->join();
                monitor_.reset();
            }

            pool_.clear();
        }

        DgError put(DoableSP doable) {
            if(!end_) {
                LOG_IF_EVERY_N (ERROR, log_count_ > 0 && (int)q_.size_approx() > top_count_, log_count_)
                    << "Push ThreadPool " << name_ << " buffer " << q_.size_approx();
                if(!q_.enqueue(doable)) {
                    LOG(ERROR) << "Enqueue fail";
                    return DG_ERR_SERVICE_NOT_AVAILABLE;
                }
            } else {
                LOG(ERROR) << "Thread pool not created";
                return DG_ERR_INIT_FAIL;
            }

            return DG_OK;
        }

        inline size_t size() { return pool_.size(); }

        /**
         * Logging once of every #cnt puts if queue size exceeds #moreThan
         */
        inline void setLogging(int cnt, int moreThan) { log_count_ = cnt; top_count_ = moreThan; }

    protected:
        void createSingle(int seq) {
            th_state_[seq] = false;
            VegaTmPnt tp(std::string("ThreadTp_")+std::to_string(seq));
            th_tp_[seq] = tp;
            auto thread = std::make_shared<std::thread>(&ThreadPool::work, this, seq);
            pool_.push_back(thread);
        }

        void monitor() {
            while(!end_) {
                VegaTmPnt tp;
                tp.mark();
                for(auto seq = 0; seq < (int)pool_.size(); seq++) {
                    if(th_state_[seq]) {
                        auto du = tp - th_tp_[seq];
                        if(du > 1000) {
                            // this thread takes more than 1 s to process
                            LOG(ERROR) << "Thread " << seq << " process tm " << du << " ms";
                        }
                    }
                }
                sleep(3);
            }
        }
        void work(int seq) {
            LOGFULL << "Thread " << seq << " started";
            while(!end_) {
                DoableSP obj;
                 if(q_.wait_dequeue_timed(obj, std::chrono::milliseconds(50))) {
                     auto monitorEnabled = bool(monitor_);
                     if(monitorEnabled) {
                         th_tp_[seq].mark();
                         th_state_[seq] = true;
                     }
                     obj->start();

                     if(monitorEnabled) {
                         th_state_[seq] = false;
                     }
                 }

            }
            LOGFULL << "Thread " << seq << " end";
        }
    protected:
        bool end_ = true;
        std::vector<std::shared_ptr<std::thread>> pool_;

        // monitor in case thread dies
        std::map<int, VegaTmPnt> th_tp_;
        std::map<int, bool> th_state_;
        std::shared_ptr<std::thread> monitor_;

        moodycamel::BlockingConcurrentQueue<DoableSP> q_;
        int log_count_ = 0;
        int top_count_ = 0;
        std::string name_ = "anon";
    };

    class DoableStation {
    private:
        class EndDoable : public Doable {
        public:
            void start() override {
                // do nothing
            }
        };
    public:
        DoableStation(const std::string &name) {
            name_ = name;
            end_ = false;
            thread_ = std::make_shared<std::thread>(std::bind(&DoableStation::work, this));
            LOGFULL << "Start workstation " << name_ ;
        }
        virtual ~DoableStation() {
            if(end_)
                return;

            end_ = true;
            put(std::make_shared<EndDoable>());
            thread_->join();
        }
    public:
        /**
         * Set logging while queue size exceeds #moreThan, and print queue state
         * once for every #cnt station put
         */
        inline void setLogging(int cnt, int moreThan) { log_count_ = cnt; top_count_ = moreThan; }

        void put(DoableSP doable) {
            LOG_IF_EVERY_N (ERROR, log_count_ > 0 && top_count_ > 0 && (int)size() > top_count_, log_count_)
                << "Push station " << name_ << " buffer " << size();
            msgq_.push(doable);
        }
        inline size_t size() { return msgq_.size(); }

    protected:
        /**
         * Main thread processing
         */
        void work() {
            while(!end_) {
                auto doable = msgq_.pop();
                doable->start();
            }
        }


    private:

        BlockQueue<DoableSP> msgq_;    ///<! Blocking message queue
        std::shared_ptr<std::thread> thread_;       ///<! Workstation thread
        bool end_ = true;           ///<! Is thread ended or in ending
        std::string name_;          ///<! Station name
        int log_count_ = 0;
        int top_count_ = 0;
    };
}
#endif //VEGA_THREADPOOL_H
