#ifndef TAG_RENAME_TASK_H
#define TAG_RENAME_TASK_H

#include "libutil/task.h"
#include "libdb/db.h"
#include <string>
#include <map>

namespace import {

class TagRenameTask: public util::Task
{
    std::string m_oldname;
    std::string m_newname;
    db::RecordsetPtr m_tags;

    TagRenameTask(const std::string& oldname, const std::string& newname,
		  db::RecordsetPtr tags)
	: m_oldname(oldname), m_newname(newname), m_tags(tags) {}

public:
    static util::TaskPtr Create(const std::string& oldname, 
				const std::string& newname, 
				db::RecordsetPtr tags);
    virtual void Run();
};

}; // namespace import

#endif
