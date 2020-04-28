#ifndef DG_WORK_MSG_H_
#define DG_WORK_MSG_H_

#include "zfz/zfz_event.hpp"

namespace vega
{
    /**
     * Message which may take some given data, and support event set on delete
     */
    class Msg {
    public:
        /**
         * This message id is reserved for ending processing thread
         */
        static const int MSG_ID_END = -1;
    public:
        Msg(int id) ;
        virtual ~Msg() ;
        Msg(const Msg &) = delete; // hidden
        Msg & operator = (const Msg &) = delete; // hidden

    public:
        int id() ;

        /**
         * attach some data to this message,
         * the size only matters if you require
         */
        bool attach(void *data, int size = 0) ;
        bool attached();
        int size();
        /**
         * Take data out of message
         */
        void *take();
        /**
         * Get data but keep it in this message
         */
        void *get();
        /**
         * Set an event to this message, so that the event will
         * be automatically set on delete
         *
         * This is used on message synchronized processing(caller
         * wait this event until message processed and deleted and
         * event is set then)
         */
        void sync(zfz::Event *event);
    private:
        int id_;                // message id, you should not define an id <= 0
        int size_;              // data_ size, matters only if you need
        void *data_;            // attached data block
        zfz::Event *event_;     // event used to sync with message sender
    }; // class Msg

    ////////////////////////////////////////////////////////////////////////////
    inline Msg::Msg(int id) {
        id_ = id;
        size_ = 0;
        data_ = nullptr;
        event_ = nullptr;
    }
    inline Msg::~Msg() {
        if (event_) {
            event_->set();
        }
    }

    inline int Msg::id() {
        return id_;
    }

    inline bool Msg::attach(void *data, int size) {
        if (attached())
            return false;

        data_ = data;
        size_ = size;
        return true;
    }
    inline bool Msg::attached() {
        return (data_ != nullptr);
    }
    inline int Msg::size() {
        return size_;
    }
    inline void *Msg::take() {
        void * data = data_;
        data_ = nullptr;
        size_ = 0;

        return data;
    }
    inline void *Msg::get() {
        return data_;
    }
    inline void Msg::sync(zfz::Event *event) {
        event_ = event;
    }
} // namespace vega
#endif /* DG_WORK_MSG_H_ */
