#ifndef DG_WORK_STATION_H_
#define DG_WORK_STATION_H_

#include "station/work_msg.h"
#include "station/block_queue.h"

#include <string>
#include <thread>

namespace vega
{

    /**
     * A work node inside a work flow.
     *
     * This node will start a thread, maintain a concurrent blockig Msg queue.
     *
     * When thread starts to run, it will wait until message coming from queue,
     * process them, then deliver the result to next station.
     *
     * virtual function procMsg will be called to process message and
     * get the next message to be sent to next station.
     *
     * The station must be started explicitly by calling start()
     */
    class WorkStation {
    public:
        WorkStation(const std::string &name);
        virtual ~WorkStation();
    public:
        void start();
        void stop();
        bool started();

        void connectTo(WorkStation *station);

        bool sendMsg(Msg *msg);
        bool sendMsg(int msgId);
        bool sendSyncMsg(Msg *msg);
        bool sendSyncMsg(int msgId);

    protected:
        /**
         * Process an incoming message, and return a new message to be sent
         * to next station.
         *
         * If return nullptr, no message will be delivered to next station.
         *
         * The input msg pointer can be changed, this feature is used to
         * reuse the same message:
         * - process coming msg
         * - save msg to a temp var like old
         * - set msg to nullptr(note that msg is a reference to Msg *)
         * - return old
         * In this processing, coming msg will be avoided to be deleted(also,
         * event, if it takes, will not be set). Instead it will be treat as
         * a new message to be sent to next station.
         */
        virtual Msg * procMsg(Msg *&msg) = 0;

        /**
         * Cleanup message after processing and before deleting
         */
        virtual void cleanupMsg(Msg *msg);
        virtual void onStart();
        virtual void onStop();
        /**
         * Main thread processing
         */
        void work();

        /**
         * Send to next station
         */
        virtual void sendToNextStation(Msg *msg);

    private:

        BlockQueue<Msg *> msgq_;    ///<! Blocking message queue
        std::thread *thread_;       ///<! Workstation thread
        bool end_ = true;           ///<! Is thread ended or in ending
        WorkStation *next_;         ///<! Next station of this one
        std::string name_;          ///<! Station name
    }; // class WorkStation

    ////////////////////////////////////////////////////////////////////////

    inline bool WorkStation::started() {
        return thread_ != nullptr;
    }

    inline bool WorkStation::sendMsg(int msgId) {
        return sendMsg(new Msg(msgId));
    }

    inline bool WorkStation::sendSyncMsg(int msgId) {
        return sendSyncMsg(new Msg(msgId));
    }
    /**
     * Connect this station to next one
     */
    inline void WorkStation::connectTo(WorkStation *station) {
        next_ = station;
    }
} // namespace vega
#endif /* DG_WORK_STATION_H_ */
