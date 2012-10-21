#ifndef TV_EPG_H
#define TV_EPG_H 1

namespace db { class Database; }

namespace util { class Scheduler; }

namespace tv {

class Recordings;

namespace dvb {

class Frontend;
class Channels;
class Service;

/** EPG implementation for DVB (where the EPG is transmitted on-air)
 */
class EPG
{
    db::Database *m_db;
    
public:
    EPG();
    ~EPG();

    unsigned int Init(Frontend*, Channels*, util::Scheduler*,
		      const char *db_filename, Service*);

    db::Database *GetDatabase() { return m_db; }
};

} // namespace dvb

} // namespace tv

#endif

