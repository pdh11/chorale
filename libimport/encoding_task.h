#ifndef ENCODING_TASK_H
#define ENCODING_TASK_H

#include "libdb/db.h"
#include "libutil/task.h"
#include "libutil/mutex.h"
#include "libutil/counted_pointer.h"
#include <string>
#include <memory>
#include <boost/scoped_array.hpp>

namespace util { class Stream; }
namespace util { class TaskQueue; }

namespace import {

class AudioEncoder;

/** A generic encoding task.
 *
 * The AudioEncoder class does the real work
 */
class EncodingTask final: public util::Task
{
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;
    std::string m_output_filename;
    AudioEncoder *m_encoder;
    boost::scoped_array<short> m_buffer;

    enum {
	EARLY,
	LATE
    } m_rename_stage;

    enum {
	STARTING,
	RUNNING,
	DONE
    } m_state;

    std::unique_ptr<util::Stream> m_output_stream;
    std::unique_ptr<util::Stream> m_buffer_stream;
    std::unique_ptr<util::Stream> m_input_stream;
    size_t m_input_size;
    size_t m_input_done;

    util::Mutex m_rename_mutex;
    std::string m_rename_filename;
    db::RecordsetPtr m_rename_tags;

    unsigned int OnReadable();

public:
    EncodingTask(const std::string& task_name,
		 util::TaskQueue *cpu_queue,
		 util::TaskQueue *disk_queue,
		 const std::string& output_filename,
		 AudioEncoder* (*encoder_factory)());
    ~EncodingTask();

    /** Stream is read once sequentially until EOF. Stream must be set before
     * Run() is called, i.e. before task is queued anywhere.
     */
    void SetInputStream(std::unique_ptr<util::Stream>& stm, size_t size);

    unsigned int Run() override;

    /** Called from UI thread once tags decided */
    void RenameAndTag(const std::string& new_filename, db::RecordsetPtr tags);
};

typedef util::CountedPointer<EncodingTask> EncodingTaskPtr;

} // namespace import

#endif
