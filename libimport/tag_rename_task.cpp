#include "tag_rename_task.h"
#include "tags.h"
#include "libdb/recordset.h"
#include "libutil/file.h"
#include "libutil/trace.h"

namespace import {

TagRenameTask::TagRenameTask(const std::string& oldname,
			     const std::string& newname,
			     db::RecordsetPtr tags)
    : m_oldname(oldname), m_newname(newname), m_tags(tags)
{
}

util::TaskCallback TagRenameTask::Create(const std::string& oldname, 
				    const std::string& newname,
				    db::RecordsetPtr tags)
{
    TaskPtr t(new TagRenameTask(oldname, newname, tags));
    return util::Bind(t).To<&TagRenameTask::Run>();
}

unsigned int TagRenameTask::Run()
{
    util::RenameWithMkdir(m_oldname.c_str(), m_newname.c_str());
    TRACE << "Tag point 3\n";
    import::Tags tags;
    unsigned int rc = tags.Open(m_newname);
    if (rc)
	return rc;
    return tags.Write(m_tags.get());
}

} // namespace import
