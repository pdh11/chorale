#include "tag_rename_task.h"
#include "tags.h"
#include "libutil/file.h"
#include "libutil/trace.h"

namespace import {

util::TaskPtr TagRenameTask::Create(const std::string& oldname, 
				    const std::string& newname,
				    db::RecordsetPtr tags)
{
    return util::TaskPtr(new TagRenameTask(oldname, newname, tags));
}

void TagRenameTask::Run()
{
    util::RenameWithMkdir(m_oldname.c_str(), m_newname.c_str());
    TRACE << "Tag point 3\n";
    WriteTags(m_newname, m_tags);
}

}; // namespace import
