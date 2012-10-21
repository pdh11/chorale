#include "schema.h"
#include "libutil/trace.h"
#include <assert.h>
#include <stdint.h>

namespace mediadb {

static void AppendUTF8(std::string& s, unsigned int ch)
{
    if (ch == 0)
	s += '?';
    else if (ch < 0x80)
	s += (char)ch;
    else if (ch < 0x800)
    {
	s += (char)(0xC0 + (ch>>6));
	s += (char)(0x80 + (ch & 0x3F));
    }
    else if (ch < 0x10000)
    {
	s += (char)(0xE0 + (ch>>12));
	s += (char)(0x80 + ((ch>>6) & 0x3F));
	s += (char)(0x80 + (ch & 0x3F));
   } 
    else if (ch < 0x200000)
    {
	s += (char)(0xF0 + (ch>>18));
	s += (char)(0x80 + ((ch>>12) & 0x3F));
	s += (char)(0x80 + ((ch>>6) & 0x3F));
	s += (char)(0x80 + (ch & 0x3F));
    }
    else if (ch < 0x04000000)
    {
	s += (char)(0xF8 + (ch>>24));
	s += (char)(0x80 + ((ch>>18) & 0x3F));
	s += (char)(0x80 + ((ch>>12) & 0x3F));
	s += (char)(0x80 + ((ch>>6) & 0x3F));
	s += (char)(0x80 + (ch & 0x3F));
    }
    else if (ch < 0x80000000)
    {
	s += (char)(0xFC + (ch>>30));
	s += (char)(0x80 + ((ch>>24) & 0x3F));
	s += (char)(0x80 + ((ch>>18) & 0x3F));
	s += (char)(0x80 + ((ch>>12) & 0x3F));
	s += (char)(0x80 + ((ch>>6) & 0x3F));
	s += (char)(0x80 + (ch & 0x3F));
    }
}

std::string VectorToChildren(const std::vector<unsigned int>& v)
{
    if (v.size() == 0)
	return std::string();

    std::string result;
    result.reserve(v.size() * 3);

    AppendUTF8(result, (unsigned int)v.size());
    for (unsigned int i=0; i<v.size(); ++i)
    {
	AppendUTF8(result, v[i]);
    }
    return result;
}

static uint32_t GetUTF8Char(const char **pptr)
{
    unsigned char ch = (unsigned char)**pptr;
    if (!ch)
	return ch;
    else if (ch < 0x80)
    {
	(*pptr)++;
	return ch;
    }
    else if (ch < 0xC0) // %10xxxxxx
    {
	// UTF-8 error
	TRACE << "UTF-8 error\n";
	(*pptr)++;
	return 0;
    }
    else if (ch < 0xE0) // %110xxxxx
    {
	(*pptr)++;
	unsigned char ch2 = (unsigned char)**pptr;
	(*pptr)++;
	return (ch2 & 0x3Fu) + ((ch & 0x1Fu) << 6);
    }
    else if (ch < 0xF0) // %1110xxxx
    {
	(*pptr)++;
	unsigned char ch2 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch3 = (unsigned char)**pptr;
	(*pptr)++;
	return (ch3 & 0x3Fu) + ((ch2 & 0x3Fu)<<6) + ((ch & 0xFu) << 12);
    }
    else if (ch < 0xF8) // %11110xxx
    {
	(*pptr)++;
	unsigned char ch2 = (unsigned char)**pptr;
	 (*pptr)++;
	unsigned char ch3 = (unsigned char)**pptr;
	  (*pptr)++;
	unsigned char ch4 = (unsigned char)**pptr;
	   (*pptr)++;
	return (ch4 & 0x3Fu) + ((ch3 & 0x3Fu)<<6) + ((ch2 & 0x3Fu)<<12) + ((ch & 7u) << 18);
    }
    else if (ch < 0xFC) // %111110xx
    {
	(*pptr)++;
	unsigned char ch2 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch3 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch4 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch5 = (unsigned char)**pptr;
	(*pptr)++;
	return (ch5 & 0x3Fu)
	    + ((ch4 & 0x3Fu)<<6) 
	    + ((ch3 & 0x3Fu)<<12)
	    + ((ch2 & 0x3Fu)<<18)
	    + ((ch & 3u) << 24);
    }
    else if (ch < 0xFE) // %1111110x
    {
	(*pptr)++;
	unsigned char ch2 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch3 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch4 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch5 = (unsigned char)**pptr;
	(*pptr)++;
	unsigned char ch6 = (unsigned char)**pptr;
	(*pptr)++;
	return (ch6 & 0x3Fu)
	    + ((ch5 & 0x3Fu)<<6) 
	    + ((ch4 & 0x3Fu)<<12)
	    + ((ch3 & 0x3Fu)<<18)
	    + ((ch2 & 0x3Fu)<<24)
	    + ((ch & 1u) << 30);
    }

    TRACE << "UTF-8 error 2\n";
    (*pptr)++;
    return 0;
}

void ChildrenToVector(const std::string& children_field,
		      std::vector<unsigned int> *vec_out)
{
    vec_out->clear();

    const char *ptr = children_field.c_str();

    if (!ptr || !*ptr)
	return; // No children

    unsigned int n = GetUTF8Char(&ptr);
    if (n == 0 || n > 20000) // Sanity check
	return;

    vec_out->resize(n);

    for (unsigned int i=0; i<n; ++i)
	(*vec_out)[i] = GetUTF8Char(&ptr);
}

} // namespace mediadb

#ifdef TEST

static void Test(const std::vector<unsigned int>& in)
{
    std::string s = mediadb::VectorToChildren(in);
    std::vector<unsigned int> out;
    mediadb::ChildrenToVector(s, &out);
    
    assert(in == out);
}

int main()
{
    std::vector<unsigned int> cv;

    Test(cv);

    cv.push_back(111);
    cv.push_back(9);
    cv.push_back(91);
    Test(cv);

    cv.clear();
    cv.push_back(130);
    Test(cv);

    cv.push_back(10000);
    cv.push_back(100000);
    cv.push_back(1000000);
    cv.push_back(10000000);
    cv.push_back(  0x400000);
    cv.push_back( 0x4000000);
    cv.push_back(0x40000000);
    Test(cv);

    return 0;
}

#endif
