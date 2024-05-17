#ifndef TAG_RENAME_TASK_H
#define TAG_RENAME_TASK_H

#include "libutil/task.h"
#include "libutil/task_queue.h"
#include "libutil/counted_pointer.h"
#include "libdb/db.h"
#include <string>
#include <map>

namespace import {

class TagRenameTask final: public util::Task
{
    std::string m_oldname;
    std::string m_newname;
    db::RecordsetPtr m_tags;

    typedef util::CountedPointer<TagRenameTask> TaskPtr;

    TagRenameTask(const std::string& oldname, const std::string& newname,
		  db::RecordsetPtr tags);
public:
    static util::TaskCallback Create(const std::string& oldname, 
				     const std::string& newname, 
				     db::RecordsetPtr tags);
    unsigned int Run() override;
};

} // namespace import

#endif
