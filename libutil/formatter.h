#ifndef LIBUTIL_FORMAT_H
#define LIBUTIL_FORMAT_H 1

#include <map>
#include <string>

namespace util {

/** A class for doing "printf-like" string formatting of fields.
 *
 * For example:
 *    Formatter f;
 *    f.AddField('v', "Carrot");
 *    s = f.Format("Today's vegetable is %v");
 *
 * Formatter understands field widths ("%8v") and zero-padding ("%08v"), but
 * not precision.
 */
class Formatter
{
    std::map<char, std::string> m_map;

public:
    Formatter();
    ~Formatter();

    void AddField(char c, const std::string& s);

    std::string Format(const std::string& template_string);
};

} // namespace util

#endif
