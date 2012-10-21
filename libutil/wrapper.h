#ifndef LIBUTIL_WRAPPER_H
#define LIBUTIL_WRAPPER_H 1

#include <assert.h>

namespace util {

template <class T>
struct WrappedType;

/** Wrap up a type so that even clients who depend on it in size,
 * don't have to see its declaration.
 *
 * See util::Condition for an example of using this template: in
 * summary, you need to use configury (or some other mechanism) to
 * work out the size of the wrapped type in advance, then instantiate
 * the template with the size in your header file, then, in your
 * implementation file, "reveal" the actual wrapped type by
 * specialising WrappedType<T> for your derived class T. In that file
 * -- and that file only -- Unwrap() can be used to obtain a reference
 * to the actual wrapped type.
 *
 * The situation gets a little uglier if the wrapped type doesn't have
 * a (useful) default constructor; see util::Mutex::Lock for how to
 * proceed in that case.
 */
template <class T, unsigned int sz>
class Wrapper
{
    union {
	char m_data[sz];
	void *m_align[1];
	unsigned long long m_align2[1];
    };

    /** Use a nested class, rather than using WrappedType<T>::type
     * directly, so that we can be sure that its destructor is called
     * "~Wrapped" -- if T1 is a typedef-name, its destructor won't be
     * called "~T1".
     */
    class Wrapped: public WrappedType<T>::type
    {
    public:
	Wrapped() {}

	template <class Arg>
	explicit Wrapped(Arg& arg) : WrappedType<T>::type(arg) {}
    };

    Wrapped *Cast() { return (Wrapped*)m_data; }

public:
    Wrapper()
    {
	assert(sizeof(Wrapped) == sz);
	new (m_data) Wrapped;
    }

    template <class Arg>
    explicit Wrapper(Arg& arg)
    {
	assert(sizeof(Wrapped) == sz);
	new (m_data) Wrapped(arg);
    }

    ~Wrapper()
     {
	 Cast()->~Wrapped();
     }

    /** Calls to Unwrap() will only compile following a definition of
     * WrappedType<T> -- not in client code.
     */
    Wrapped& Unwrap() { return *Cast(); }
};

} // namespace util

#endif
