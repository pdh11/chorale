#include "tag_rename_task.h"
#include "tags.h"
#include "libdb/recordset.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/bind.h"

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
    TRACE << "Tag point 3\n";
    unsigned int rc = util::RenameWithMkdir(m_oldname.c_str(),
					    m_newname.c_str());
    if (rc)
    {
	TRACE << "Rename failed!\n";
	FireError(rc);
	return rc;
    }
    import::TagWriter tags;
    rc = tags.Init(m_newname);
    if (!rc)
	rc = tags.Write(m_tags.get());
    if (rc)
	FireError(rc);
    return rc;
}

} // namespace import
