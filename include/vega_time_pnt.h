//
// Created by gqjiang on 2019/8/9.
//

#ifndef VEGA_TIME_PNT_H
#define VEGA_TIME_PNT_H

#include <memory>
#include <chrono>
#include <mutex>
#include <glog/logging.h>

namespace vega {

    /**
     * Vega time point. It records a time point with a name.
     */
    class VegaTmPnt {
    public:
        VegaTmPnt () = default;
        VegaTmPnt (const std::string & pnt) { mark(pnt); }
        VegaTmPnt (std::string &&pnt) { pnt_ = std::move(pnt); mark();  }
        VegaTmPnt (const std::string &pnt, long time) {
            pnt_ = pnt;
            time_ = std::chrono::high_resolution_clock ::time_point(std::chrono::high_resolution_clock::duration(time));
        }
        VegaTmPnt (const std::string &pnt, std::chrono::high_resolution_clock ::time_point time) {
            pnt_ = pnt;
            time_ = time;
        }
        VegaTmPnt (const VegaTmPnt & tp) { pnt_ = tp.pnt_; time_ = tp.time_; }
        VegaTmPnt (VegaTmPnt && tp) { pnt_ = std::move(tp.pnt_); time_ = tp.time_; }
        VegaTmPnt & operator = (const VegaTmPnt &tp) { pnt_ = tp.pnt_; time_ = tp.time_; return *this; }

        inline void mark() { time_ = std::chrono::high_resolution_clock::now(); }
        inline void mark(const std::string &pnt) { pnt_ = pnt; mark(); }

        inline const std::string & where() const { return pnt_; }
        inline const std::chrono::high_resolution_clock ::time_point & when() const { return time_; }

        inline double from(const VegaTmPnt &another) const {
            auto span = std::chrono::duration_cast<std::chrono::duration<double>>(when() - another.when());
            return span.count() * 1000;
        }

        void removeTp() {
            pnt_ = "";
        }
    protected:
        std::string pnt_;
        std::chrono::high_resolution_clock::time_point time_;

    };

    /**
     * Calculate duration between two time point.
     * @param end End time point
     * @param start Start time point
     * @return Duration with unit microsecond
     */
    inline double operator - (const VegaTmPnt & end, const VegaTmPnt & start) { return end.from(start); }

    class VegaTmRecord {
    public:
        explicit VegaTmRecord(const std::string &prefix) : start_("start") {
            prefix_ = prefix;
        }
        ~VegaTmRecord() {
            VegaTmPnt stop("stop");
            LOG(INFO) << prefix_ << " takes " << stop-start_ << " ms";
        }
        VegaTmPnt start_;
        std::string prefix_;
    };

/**
 * Do not mark time point
 */
#define VEGA_TP_LEVEL_NONE 0
/**
 * Only set a time point when device engine recv request(rx) or send response(done)
 */
#define VEGA_TP_LEVEL_OUTLINE 1
/**
 * Mark model PreProc/Infer/PostProc
 */
#define VEGA_TP_LEVEL_SIMPLE 2
/**
 * More detailed time point, please do not try this on releasing.
 */
#define VEGA_TP_LEVEL_DETAIL 3

    /**
     * Time points group sharing a prefix, all time points appended into this group should not
     * be greater than a specified level.
     *
     * For instance, if we add a time point "start", and prefix is "model",
     * the time point from outside of this group will be "model.start".
     */
    class VegaTpGrp {
    public:
        VegaTpGrp() = default;
        VegaTpGrp(int level) { setLevel(level); }
        VegaTpGrp(VegaTpGrp && grp) { level_ = grp.level_; tps_ = std::move(grp.tps_); }

        inline void setPrefix(const std::string &prefix) { prefix_ = prefix + "."; }
        /**
         * set a time point level. All time point less than or equal to this level will be
         * allowed to pushing into this group.
         * @param level
         */
        inline void setLevel(int level) { level_ = level; }
        inline bool allowed(int level) { return level <= level_; }
        inline void append(const VegaTpGrp &grp) {
            for(auto & tp : grp.tps_) {
                tps_.emplace_back(grp.prefix_ + tp.where(), tp.when());
            }
        }
        inline void append(const std::string &pnt, int level) { if(allowed(level)) tps_.emplace_back(pnt); }
        inline void restore(const std::string &pnt, long time) { tps_.emplace_back(pnt, time); }

        inline VegaTmPnt & operator [] (int idx) { return tps_[idx]; }
        inline bool empty() { return tps_.empty(); }
        inline size_t size() { return tps_.size(); }

        // total duration of this group(last - first)
        inline double duration() { return tps_.size() < 2 ? 0 : tps_[tps_.size()-1] - tps_[0]; }

        void dump() {
            LOG(ERROR) << "Total: " << duration() << " ms";
            for(auto i = 0u; i < tps_.size() - 1; i++) {
                LOG(ERROR) << prefix_ + tps_[i].where() << ": " << tps_[i+1] - tps_[i] << " ms";
            }
        }
    protected:
        std::string prefix_;
        std::vector<VegaTmPnt> tps_;
        int level_ = 0;
    };

    using VegaTpGrpSP = std::shared_ptr<VegaTpGrp>;

    /**
     * Accumulate time point groups and calculate min/max/average durations
     */
    class VegaTpAccumulator {
    public:
        explicit VegaTpAccumulator() = default;
        ~VegaTpAccumulator() {
            acc_.clear();
            max_.clear();
            min_.clear();
        }

        void push(VegaTpGrpSP &tpg) {
            std::unique_lock<std::mutex> lock(mtx_);
            accumulate(tpg);
        }
        void dump() {
            dump(LOG(ERROR));
        }
        void dump(std::ostream &os) {
            std::vector<double> ret;
            avg(ret);

            os << "Avg:" << std::endl;
            for(auto i = 0u; i < hdrs_.size(); i++) {
                os << "\t" << hdrs_[i] << ": " << ret[i] << std::endl;
            }

            os << "Max:" << std::endl;
            for(auto i = 0u; i < hdrs_.size(); i++) {
                os << "\t" << hdrs_[i] << ": " << max_[i] << std::endl;
            }

            os << "Min:" << std::endl;
            for(auto i = 0u; i < hdrs_.size(); i++) {
                os << "\t" << hdrs_[i] << ": " << min_[i] << std::endl;
            }

            os << std::endl;
        }
        // use auto ret = std::move(xxx->avg());
        void avg(std::vector<double> &v) {
            v.clear();
            std::unique_lock<std::mutex> lock(mtx_);
            for(auto p : acc_) {
                v.push_back(p / cnter_);
            }

        }

        void hdrs(std::vector<std::string> &vhdr) {
            vhdr = hdrs_;
        }
        void max(std::vector<double> &vmax) {
            vmax = max_;
        }
        void min(std::vector<double> &vmin) {
            vmin = min_;
        }
    protected:
        void accumulate(const VegaTpGrpSP &tpg) {
            if(tpg->size() < 2) {
                return;
            }

            if(acc_.empty()) {
                // first pos always store total duration
                hdrs_.push_back("total");
                acc_.push_back(tpg->duration());

                // store each gap
                for(auto i = 0u; i < tpg->size()-1; i++) {
                    auto &start = (*tpg)[i];
                    auto &stop = (*tpg)[i+1];
                    hdrs_.push_back(start.where());
                    acc_.push_back(stop - start);
                }

                cnter_ = 1;

                max_ = acc_;
                min_ = acc_;
            } else if(tpg->size() == hdrs_.size()) {
                // must be same tpg
                for(auto i = 0u; i < tpg->size()-1; i++) {
                    if(hdrs_[i+1] != (*tpg)[i].where()) {
                        return;
                    }
                }

                auto dur = tpg->duration();
                acc_[0] += dur;
                if(max_[0] < dur) max_[0] = dur;
                if(min_[0] > dur) min_[0] = dur;

                for(auto i = 0u; i < tpg->size() - 1; i++) {
                    auto &start = (*tpg)[i];
                    auto &stop = (*tpg)[i+1];

                    dur = stop - start;
                    acc_[i+1] += dur;
                    if(max_[i+1] < dur) max_[i+1] = dur;
                    if(min_[i+1] > dur) min_[i+1] = dur;
                }

                ++cnter_;
            }
        }
    protected:
        std::mutex mtx_;

        long cnter_ = 0;
        std::vector<double> acc_;
        std::vector<double> max_;
        std::vector<double> min_;

        std::vector<std::string> hdrs_;
    };
}
#endif //VEGA_TIME_PNT_H
