#include "libutil/xml.h"
#include "libutil/stream.h"
#include "libutil/file_stream.h"
#include <list>
#include <stdio.h>
#include <string.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace {

time_t parseIsoDate(const char *s)
{
    struct tm t;
    char *r = strptime(s, "%Y-%m-%dT%H:%M:%S", &t);
    if (!r) {
        fprintf(stderr, "Don't like date >%s<\n", s);
        return 0;
    }

    t.tm_sec = 0; // For comparing with Crap Dates

    return timegm(&t);
}

time_t parseCrapDate(const char *s)
{
    struct tm t;
    memset(&t, '\0', sizeof(t));
    char *r = strptime(s, "%B %d, %Y at %H:%M", &t);
    if (!r) {
        fprintf(stderr, "Don't like date >%s<\n", s);
        return 0;
    }

    t.tm_isdst = -1; // Work it out please
    time_t tt = mktime(&t);

    // Crap Dates are Pacific time: GMT+9 in summer, GMT+8 in winter
#if 0
    struct tm tm2;
    gmtime_r(&tt, &tm2);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%F %T", &tm2);
    
    printf("%s: %d (%d) %s\n", s, t.tm_isdst, int(t.tm_gmtoff), buffer);
#endif

    return tt;
}

}

namespace wall {

class Wall: public xml::SaxParserObserver
{
    enum {
        NOENTRY,
        ENTRY,
        SUBENTRY
    } m_state;

    enum {
        NONE,
        NAME,
        DATE,
        CONTENT
    } m_item;

    unsigned int m_divs;
    unsigned int m_outOfEntry;
    unsigned int m_outOfSubEntry;
    bool m_firstContent;

    struct Entry {
        std::string name;
        time_t date;
        std::string sdate;
        std::string content;
        unsigned likes;
        std::list<Entry> comments;

        void clear() { name.clear(); date = 0; sdate.clear(); content.clear(); likes=0; comments.clear(); }
    };

    Entry m_entry;
    Entry m_subentry;

    std::list<Entry> m_entries;

public:
    Wall() : m_state(NOENTRY), m_item(NONE), m_divs(0), m_outOfEntry(300), m_outOfSubEntry(300) { m_entry.clear(); m_subentry.clear(); }

    unsigned int OnBegin(const char*);
    unsigned int OnEnd(const char*);
    unsigned int OnAttribute(const char*, const char*);
    unsigned int OnContent(const char*);

    void dump();
};

unsigned int Wall::OnBegin(const char *s)
{
    if (!strcmp(s, "div")) {
        ++m_divs;
    }
    m_firstContent = true;
    return 0;
}

unsigned int Wall::OnEnd(const char *s)
{
    if (!strcmp(s, "div")) {
        --m_divs;
        if (m_divs < m_outOfSubEntry) {
            if (!m_subentry.name.empty()) {
                boost::algorithm::trim(m_subentry.name);
                boost::algorithm::trim(m_subentry.sdate);
                boost::algorithm::trim(m_subentry.content);
                boost::algorithm::replace_all(m_subentry.content,"\n"," ");
                if (!m_subentry.date) {
                    m_subentry.date = parseCrapDate(m_subentry.sdate.c_str());
                }
                m_entry.comments.push_back(m_subentry);
                m_subentry.clear();
            }
            m_state = ENTRY;
        }
        if (m_divs < m_outOfEntry) {
            if (!m_entry.name.empty()) {
                boost::algorithm::trim(m_entry.name);
                boost::algorithm::trim(m_entry.sdate);
                boost::algorithm::trim(m_entry.content);
                boost::algorithm::replace_all(m_entry.content,"\n"," ");
                if (isalnum(m_entry.content[m_entry.content.length()-1]))
                    m_entry.content += ".";
                if (!m_entry.date) {
                    m_entry.date = parseCrapDate(m_entry.sdate.c_str());
                }
                m_entries.push_back(m_entry);
                m_entry.clear();
            }
            m_state = NOENTRY;
        }
    }
    if (m_item == DATE) {
        m_item = NONE;
    }
    if (m_item == NAME) {
        m_item = CONTENT;
    }
    return 0;
}

unsigned int Wall::OnAttribute(const char* k, const char *v)
{
    if (!strcmp(k, "class")) {
        if (strstr(v, "author")) {
            m_item = NAME;
        } else if (strstr(v, "entry-content")) {
            m_item = CONTENT;
        } else if (strstr(v, "time published")) {
            m_item = DATE;
        } else if (strstr(v, "hentry")) {
            if (m_state == NOENTRY)
            {
                m_state = ENTRY;
                m_outOfEntry = m_divs;
            } else {
                m_state = SUBENTRY;
                m_outOfSubEntry = m_divs;
            }
        } else if (!strcmp(v, "feedentry")) { // Old-style
            m_state = ENTRY;
            m_outOfEntry = m_divs;
        } else if (!strcmp(v, "comment")) { // Old-style
            m_state = SUBENTRY;
            m_outOfSubEntry = m_divs;
        } else if (!strcmp(v, "profile")) { // Old-style
            m_item = NAME;
        } else if (!strcmp(v, "time")) { // Old-style
            m_item = DATE;
        } 
    } else if (!strcmp(k, "title") && m_item == DATE) {
        if (m_state == ENTRY) {
            m_entry.date = parseIsoDate(v);
        } else if (m_state == SUBENTRY) {
            m_subentry.date = parseIsoDate(v);
        }
        m_item = NONE;
    }
    return 0;
}


unsigned int Wall::OnContent(const char *s)
{
    if (m_firstContent)
    {
        if (strspn(s, " \n\r\t") == strlen(s))
            return 0;

        m_firstContent = false;
    }
    Entry *e = NULL;
    if (m_state == ENTRY)
        e = &m_entry;
    else if (m_state == SUBENTRY)
        e = &m_subentry;
    else
        return 0;

    switch (m_item) {
    case NAME: e->name += s; break;
    case CONTENT: e->content += s; break;
    case DATE: e->sdate += s; break;
    case NONE: break;
    }
    return 0;
}

void Wall::dump()
{
    for (std::list<Entry>::const_iterator i = m_entries.begin(); i != m_entries.end(); ++i) {
        printf("%010u 0000000000 %s %s%s\n", unsigned(i->date), i->name.c_str(),
               i->date < 1192800000 ? "is " : "",
               i->content.c_str());
        for (std::list<Entry>::const_iterator j = i->comments.begin(); j != i->comments.end(); ++j) {
            printf("%010u %010u %s %s\n", unsigned(i->date), unsigned(j->date), j->name.c_str(), j->content.c_str());
        }
    }
}

}

int main(int, const char *argv[])
{
    std::unique_ptr<util::Stream> stm;
    if (util::OpenFileStream(argv[1], util::READ, &stm)) {
        perror("open");
        return 1;
    }

    wall::Wall w;

    setenv("TZ", ":America/Los_Angeles", 1);
    tzset();
    
    xml::SaxParser saxp(&w);
    if (saxp.Parse(stm.get())) {
        perror("parse");
        return 1;
    }

    w.dump();

    return 0;
}
