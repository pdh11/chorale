#include "file.h"
#include "trace.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <assert.h>

namespace util {

void MkdirParents(const char *leafname)
{
    const char *rslash = strrchr(leafname, '/');
    if (!rslash)
	return;
    std::string parent(leafname, rslash);
    struct stat st;
    if (stat(parent.c_str(), &st) < 0 && errno == ENOENT)
    {
	MkdirParents(parent.c_str());
//	TRACE << "mkdir " << parent << "\n";
	mkdir(parent.c_str(), 0775);
    }
}

void RenameWithMkdir(const char *oldname, const char *newname)
{
    MkdirParents(newname);
//    TRACE << "Rename " << oldname << " to " << newname << "\n";
    rename(oldname, newname);
}

std::string ProtectLeafname(const std::string& s)
{
    std::string out;
    out.reserve(s.length());

    for (unsigned int i=0; i<s.length(); ++i)
    {
	// Convert characters which FAT doesn't like, to UTF-8
	if (s[i] == '/' || s[i] == ':' || s[i] == '\\' || s[i] == '?'
	    || s[i] == '\"' || s[i] == '|' || s[i] == '<' || s[i] == '>'
	    || s[i] == '*')
	{
	    unsigned int ucs4 = 0xFF00 + (s[i] - 0x0020);
	    // 3-byte UTF-8
	    out += (char)(0xE0 + (ucs4 >> 12));
	    out += (char)(0x80 + ((ucs4 >> 6) & 0x3F));
	    out += (char)(0x80 + (ucs4 & 0x3F));
	}
	else if (s[i])
	    out += s[i];
    }
    return out;
}

std::string GetDirName(const char *filename)
{
    const char *rslash = strrchr(filename, '/');
    if (rslash)
	return std::string(filename, rslash);
    return std::string();
}

std::string GetLeafName(const char *filename)
{
    const char *rslash = strrchr(filename, '/');
    if (rslash)
	return std::string(rslash+1);
    return filename;
}

std::string GetExtension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot)
	return "";
    const char *slash = strrchr(filename, '/');
    if (slash && slash > dot)
	return "";
    return std::string(dot+1);
}

std::string StripExtension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot)
	return filename;
    const char *slash = strrchr(filename, '/');
    if (slash && slash > dot)
	return filename;
    return std::string(filename, dot);
}

static void Split(const std::string& in, std::string *head, std::string *tail)
{
    std::string::size_type slash = in.find('/');
    if (slash == std::string::npos)
    {
	*head = in;
	*tail = std::string();
	return;
    }
    *head = std::string(in, 0, slash);
    *tail = std::string(in, slash+1);

//    TRACE << in << " -> " << *head << ", " << *tail << " (" << slash << ")\n";
}

std::string MakeRelativePath(const std::string& from, const std::string& to)
{
    // Find common root
    std::string remain1 = from, remain2 = to;
    for (;;)
    {
	std::string head1, tail1, head2, tail2;
	Split(remain1, &head1, &tail1);
	Split(remain2, &head2, &tail2);

	if (head1 != head2 || tail1.empty() || tail2.empty())
	    break;
	remain1 = tail1;
	remain2 = tail2;
    }

    // Add '..'s
    std::string thelink = remain2;
    for (;;)
    {
	std::string head, tail;
	Split(remain1, &head, &tail);
	if (tail.empty())
	    break;
	remain1 = tail;
	thelink = "../" + thelink;
    }
    return thelink;
}

unsigned MakeRelativeLink(const std::string& from, const std::string& to)
{
    if (from.empty() || to.empty() || from[0] != '/' || to[0] != '/')
	return EINVAL;

    std::string thelink = MakeRelativePath(from, to);

//    TRACE << "from=" << from << " to=" << to << " link=" << thelink << "\n";

    int rc = symlink(thelink.c_str(), from.c_str());
    if (rc < 0)
	return errno;
    return 0;
}

std::string MakeAbsolutePath(const std::string& from, const std::string& rel)
{
    if (rel.empty() || rel[0] == '/')
	return rel;

    std::string remainfrom = GetDirName(from.c_str());
    std::string remainrel = rel;

    for (;;) {
	std::string head, tail;
	Split(remainrel, &head, &tail);

	if (head != "..")
	    break;

	remainrel = tail;
	remainfrom = GetDirName(remainfrom.c_str());
    }
	
    std::string result = remainfrom + "/" + remainrel;

//    TRACE << "map(" << from << "," << rel << ")=" << result << "\n";
    return result;
}

std::string Canonicalise(const std::string& path)
{
    char *cpath = canonicalize_file_name(path.c_str());
    if (!cpath)
	return path;

    std::string s(cpath);
    free(cpath);
    return s;
}

std::string PathToURL(const std::string& path)
{
    /* RFC3986 specifies we percent-encode almost everything */
    std::string result = "file://";
    result.reserve(path.length()*2);

    for (unsigned int i=0; i<path.length(); ++i)
    {
	char c = path[i];
	if ((c>='A' && c<='Z') || (c>='a' && c<='z'))
	    result += c; // alpha
	else if (c>='0' && c<='9')
	    result += c; // digit
	else if (c=='/' || c=='-' || c == '.' || c == '_' || c =='~')
	    result += c; // unreserved or /
	else if (c=='!' || c=='$' || c == '&' || c == '\'' || c == '('
		 || c == ')' || c == '*' || c == '+' || c == ';' || c == '=')
	    result += c; // sub-delims
	else if (c==':' || c == '@')
	    result += c; // remaining cases of pchar
	else
	{
	    result += '%';
	    
	    static const char uchex[] = "0123456789ABCDEF";
	    result += uchex[((unsigned char)c) >> 4];
	    result += uchex[c & 15];
	}
    }
    return result;
}

std::string URLToPath(const std::string& url)
{
    if (strncasecmp(url.c_str(), "file:///", 8))
	return std::string();

    std::string result;
    for (unsigned int i=7; i<url.length(); ++i)
    {
	char c = url[i];
	if (c != '%')
	    result += c;
	else
	{
	    if (isxdigit(url[i+1])
		&& isxdigit(url[i+2]))
	    {
		unsigned char uc;
		c = url[i+1];
		uc = (c & 15) << 4;
		if (!isdigit(c))
		    uc += 0x90; // A = 0x41 A&15=1
		c = url[i+2];
		uc += (c & 15);
		if (!isdigit(c))
		    uc += 0x9;
		result += (char)uc;
		i += 2;
	    }
	    else
	    {
		// Invalid '%': we're being gamed
		return std::string();
	    }
	}
    }
    
//    TRACE << "U2P(" << url << ")=" << result << "\n";

    return result;
}

} // namespace util

#ifdef TEST

int main()
{
    assert(util::PathToURL("/") == "file:///");
    assert(util::PathToURL("/foo/") == "file:///foo/");
    assert(util::PathToURL("/who?.mp3") == "file:///who%3F.mp3");

    assert(util::URLToPath("file:///") == "/");
    assert(util::URLToPath("file:///foo/") == "/foo/");
    assert(util::URLToPath("file:///who%3F.mp3") == "/who?.mp3");

    return 0;
}

#endif
