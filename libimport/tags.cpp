#include "tags.h"
#include "config.h"
#include "tags_flac.h"
#include "tags_mp3.h"

#undef CTIME

#if HAVE_TAGLIB

template<>
const util::ChooseByExtension<import::TagReaderBase>::ExtensionMap
util::ChooseByExtension<import::TagReaderBase>::sm_map[] = {
    { "mp3",  &Factory<import::mp3::TagReader> },
    { "mp2",  &Factory<import::mp3::TagReader> },
    { "flac", &Factory<import::TagReaderBase>  },
};

template<>
const util::ChooseByExtension<import::TagWriterBase>::ExtensionMap
util::ChooseByExtension<import::TagWriterBase>::sm_map[] = {
    { "mp3",  &Factory<import::mp3::TagWriter> },
    { "mp2",  &Factory<import::mp3::TagWriter> },
    { "flac", &Factory<import::flac::TagWriter>  },
};

namespace import {

/* TagReader */


TagReader::TagReader()
{
}

TagReader::~TagReader()
{
}

unsigned TagReader::Init(const std::string& filename)
{
    return m_chooser.Init(filename);
}

unsigned TagReader::Read(db::Recordset *rs)
{
    if (!m_chooser.IsValid())
	return EINVAL;
    return m_chooser->Read(m_chooser.GetFilename(), rs);
}


/* TagWriter */


TagWriter::TagWriter()
{
}

TagWriter::~TagWriter()
{
}

unsigned TagWriter::Init(const std::string& filename)
{
    return m_chooser.Init(filename);
}

unsigned TagWriter::Write(const db::Recordset *rs)
{
    if (!m_chooser.IsValid())
	return EINVAL;
    return m_chooser->Write(m_chooser.GetFilename(), rs);
}

} // namespace import

#endif // HAVE_TAGLIB
