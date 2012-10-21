#ifndef RIPPING_TASK_H
#define RIPPING_TASK_H

#include "libutil/task.h"
#include "audio_cd.h"
#include "encoding_task.h"

namespace import {

class RippingTask: public util::Task
{
    AudioCDPtr m_cd;
    unsigned int m_track;
    std::string m_filename;
    EncodingTaskPtr m_etp1;
    EncodingTaskPtr m_etp2;
    util::TaskQueue *m_encode_queue;
    util::TaskQueue *m_disk_queue;

    RippingTask(AudioCDPtr, unsigned int, const std::string&,
		EncodingTaskPtr, EncodingTaskPtr, util::TaskQueue*,
		util::TaskQueue*);
    ~RippingTask();

public:
    typedef boost::intrusive_ptr<RippingTask> RippingTaskPtr;

    static RippingTaskPtr Create(AudioCDPtr cd, unsigned int track,
				 const std::string& filename,
				 EncodingTaskPtr etp1, EncodingTaskPtr etp2,
				 util::TaskQueue *encode_queue, 
				 util::TaskQueue *disk_queue);
    
    unsigned int Run();
};

typedef boost::intrusive_ptr<RippingTask> RippingTaskPtr;

} // namespace import

#endif
