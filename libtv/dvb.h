#ifndef TV_DVB_H
#define TV_DVB_H 1

#include <string>
#include <vector>
#include <memory>
#include "libutil/attributes.h"

namespace util { class Stream; }

/** Classes for TV (and radio) support
 */
namespace tv {

/** Classes for DVB (Freeview) support
 */
namespace dvb {

struct Channel {
    std::string name;
    unsigned int frequency;
    unsigned int inversion;
    unsigned int bandwidth;
    unsigned int coderatehp;
    unsigned int coderatelp;
    unsigned int constellation;
    unsigned int transmissionmode;
    unsigned int guardinterval;
    unsigned int hierarchy;
    unsigned int videopid;
    unsigned int audiopid;
    unsigned int service_id;
};

class Channels
{
public:
    typedef std::vector<Channel> vec_t;

private:
    vec_t m_vec;

public:
    Channels();

    unsigned int Load(const char *configfile);

    typedef vec_t::const_iterator const_iterator;

    const_iterator begin() { return m_vec.begin(); }
    const_iterator end() { return m_vec.end(); }
    
    size_t Count() const { return m_vec.size(); }
};

/** A frontend is a DVB tuner; it tunes to a particular multiplex
 */
class Frontend
{
    int m_fd;
    unsigned int m_hz;

public:
    Frontend();
    ~Frontend();
    
    unsigned int Open(unsigned int adaptor, unsigned int device)
	ATTRIBUTE_WARNUNUSED;

    void Close();

    /** Tunes to a multiplex -- all channels on the same frequency are
     * available.
     */
    unsigned int Tune(const Channel&);

    void GetState(unsigned int *signalpercent, unsigned int *snrpercent,
		  bool *tuned);

    /** Access an audio channel from a multiplex (Tune() must have been called)
     */
    std::auto_ptr<util::Stream> GetStream(const Channel&);
};

} // namespace dvb

} // namespace tv

#endif
