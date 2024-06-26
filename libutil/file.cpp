#include "file.h"
#include "trace.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <assert.h>

namespace util {

unsigned int MkdirParents(const char *leafname)
{
    const char *rslash = strrchr(leafname, '/');
    if (!rslash)
    {
	TRACE << "MkdirParents(" << leafname << ") no slash\n";
	return ENOTDIR;
    }
    std::string parent(leafname, rslash);
    if (DirExists(parent.c_str()))
	return 0;

    unsigned int rc = MkdirParents(parent.c_str());
    if (rc)
	return rc;

    rc = Mkdir(parent.c_str());

    /* This is completely racy with other threads doing the same thing, so
     * EEXIST is a non-problem here.
     */
    if (rc == EEXIST)
	rc = 0;

    if (rc)
    {
	TRACE << "mkdir(" << parent << ") failed, " << rc << "\n";
    }
    return rc;
}

unsigned int RenameWithMkdir(const char *oldname, const char *newname)
{
    unsigned int rc = MkdirParents(newname);
    if (rc)
    {
	TRACE << "MkdirParents failed: " << rc << "\n";
	return rc;
    }
//    TRACE << "Rename " << oldname << " to " << newname << "\n";
    
    int rc2 = ::rename(oldname, newname);
    if (rc2 < 0)
    {
	TRACE << "Rename failed: " << errno << "\n";
	return errno;
    }
    return 0;
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
	    unsigned int ucs4 = 0xFF00u + ((unsigned int)s[i] - 0x0020u);
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

bool IsInRoot(const char *root, const char *path)
{
    if (!root || !path)
	return false;
    if (root[0] != '/' || path[0] != '/')
	return false;

    size_t rootlen = strlen(root);
    if (root[rootlen-1] == '/')
	--rootlen;

    if (strncmp(root, path, rootlen) != 0)
	return false;

    if (path[rootlen] && path[rootlen] != '/')
	return false;

    return true;
}

std::string PathToURL(const std::string& path)
{
    /* RFC3986 specifies we percent-encode almost everything */
    std::string result = "file://";
    result.reserve(path.length()*2);

    for (unsigned int i=0; i<path.length(); ++i)
    {
	char c = path[i];
	if ((c>='A' && c<='Z') || (c>='a' && c<='z') // alpha

            || (c>='0' && c<='9') // digit
            || (c=='/' || c=='-' || c == '.' || c == '_' || c =='~') // unreserved or /
            || (c=='!' || c=='$' || c == '&' || c == '\'' || c == '('
		 || c == ')' || c == '*' || c == '+' || c == ';' || c == '=') // sub-delims
            || (c==':' || c == '@'))
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
    if (strncasecmp(url.c_str(), "file:///", 8) != 0)
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
		char uc;
		c = url[i+1];
		uc = (char)((c & 15) << 4);
		if (!isdigit(c))
		    uc = (char)(uc + 0x90); // A = 0x41 A&15=1
		c = url[i+2];
		uc = (char)(uc + (c & 15));
		if (!isdigit(c))
		    uc = (char)(uc + 0x9);
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

static const struct {
    const char *from;
    const char *to;
    const char *rel;
} tests[] = {
    { "/", "/", "/" },
    { "/foo",     "/bar", "bar" },
    { "/foo/woo", "/bar", "../bar" },
    { "/foo",     "/bar/boo", "bar/boo" },
    { "/media/mp3audio/New Albums/foo", "/media/mp3audio/Artists/Elbow/foo", "../Artists/Elbow/foo" },

#if 0
    { "C:/foo/woo", "C:/bar", "../bar" },
    { "C:/foo", "D:/bar", "D:/bar" },

    { "//server1/share1/foo/bar", "//server1/share1/boo/wah", "../boo/wah" },
    { "//server1/share1/foo/bar", "//server2/share2/boo/wah", "//server2/share2/boo/wah" },
#endif
};

static void TestRelativePaths()
{
    for (unsigned int i=0; i<(sizeof(tests)/sizeof(*tests)); ++i)
    {
	const char *from = tests[i].from;
	const char *to   = tests[i].to;
	const char *rel  = tests[i].rel;

	std::string rel2 = util::MakeRelativePath(from, to);
	if (rel2 != rel)
	{
	    TRACE << "From '" << from << "' to '" << to << "' expected '"
		  << rel << "' got '" << rel2 << "'\n";
	}

	assert(util::MakeRelativePath(from, to) == rel);
	assert(util::MakeAbsolutePath(from, rel) == to);
    }
}

int main()
{
    assert(util::PathToURL("/") == "file:///");
    assert(util::PathToURL("/foo/") == "file:///foo/");
    assert(util::PathToURL("/who?.mp3") == "file:///who%3F.mp3");

    assert(util::URLToPath("file:///") == "/");
    assert(util::URLToPath("file:///foo/") == "/foo/");
    assert(util::URLToPath("file:///who%3F.mp3") == "/who?.mp3");

    assert(util::posix::GetDirName("foo/bar\\wurdle") == "foo");

    assert(util::posix::GetLeafName("foo/bar\\wurdle") == "bar\\wurdle");

    assert(util::posix::StripExtension("ILOVEYOU.TXT.VBS") == "ILOVEYOU.TXT");
    assert(util::posix::StripExtension("foo.txt\\bar") == "foo");
    assert(util::posix::StripExtension("foo/bar.txt") == "foo/bar");

    assert(util::posix::GetExtension("ILOVEYOU.TXT.VBS") == "VBS");
    assert(util::posix::GetExtension("foo.txt\\bar") == "txt\\bar");
    assert(util::posix::GetExtension("foo/bar.txt") == "txt");
    assert(util::posix::GetExtension("foo.bar/txt") == "");

    TestRelativePaths();

    assert(util::IsInRoot("/zootle/", "/zootle/frink"));
    assert(util::IsInRoot("/zootle", "/zootle/frink"));
    assert(!util::IsInRoot("/zootle", "/zootleb/frink"));
    assert(!util::IsInRoot("/zootle/", "/zootleb/"));
    assert(!util::IsInRoot("/zootle/", "/zootleb"));
    assert(util::IsInRoot("/zootle/", "/zootle"));
    assert(util::IsInRoot("/zootle/", "/zootle/"));
    assert(util::IsInRoot("/zootle", "/zootle"));
    assert(util::IsInRoot("/zootle", "/zootle/"));
    assert(util::IsInRoot("/", "/zootle"));
    assert(util::IsInRoot("/", "/"));
    assert(!util::IsInRoot("/zootle", "/"));
    assert(!util::IsInRoot("/zootle/frink", "/zootle"));
    assert(util::IsInRoot("/zootle/frink", "/zootle/frink"));

    return 0;
}

#endif
