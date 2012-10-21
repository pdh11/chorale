#ifndef ENCODING_TASK_H
#define ENCODING_TASK_H

#include "libutil/task.h"
#include <string>
#include <map>
#include "libutil/stream.h"
#include "libdb/db.h"

namespace import {

/** A generic encoding task.
 *
 * Subclasses EncodingTaskFlac and EncodingTaskMP3 do all the work.
 */
class EncodingTask: public util::Task
{
protected:
    util::StreamPtr m_input_stream;
    size_t m_input_size;
    std::string m_output_filename;

    enum {
	EARLY,
	LATE
    } m_rename_stage;

    boost::mutex m_rename_mutex;
    std::string m_rename_filename;
    db::RecordsetPtr m_rename_tags;

    explicit EncodingTask(const std::string& output_filename)
	: m_output_filename(output_filename), m_rename_stage(EARLY) {}

    ~EncodingTask();

public:

    /** Stream is read once sequentially until EOF. Stream must be set before
     * Run() is called, i.e. before task is queued anywhere.
     */
    void SetInputStream(util::StreamPtr stm, size_t size) 
    {
	m_input_stream = stm; 
	m_input_size = size; 
    }

    /** Called from UI thread once tags decided */
    void RenameAndTag(const std::string& new_filename,
		      db::RecordsetPtr tags, util::TaskQueue *queue);
};

typedef boost::intrusive_ptr<EncodingTask> EncodingTaskPtr;

}; // namespace import

#endif
