#include "config.h"
#include "encoding_task.h"
#include "tag_rename_task.h"

namespace import {

EncodingTask::~EncodingTask()
{
//    TRACE << "~EncodingTask\n";
}

/** Rename and tag.
 * To get the threading right, there are three cases:
 *
 *  - Called before encoding starts: tag during original write, always write
 *    to new filename
 *  - Called during encoding: wait until encoding ends, then rename and tag
 *    (in this task; a second task might end up running before we've finished)
 *  - Called after encoding finishes: a new task must be started.
 *
 * When deciding what to do here we must mutex ourselves from the task doing
 * the actual encoding.
 */
void EncodingTask::RenameAndTag(const std::string& new_filename,
				db::RecordsetPtr tags, util::TaskQueue *queue)
{
    boost::mutex::scoped_lock lock(m_rename_mutex);
    if (m_rename_stage == LATE)
    {
	queue->PushTask(TagRenameTask::Create(m_output_filename, new_filename,
					      tags));
	return;
    }
    m_rename_filename = new_filename;
    m_rename_tags = tags;
}

} // namespace import
