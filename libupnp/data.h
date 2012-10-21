#ifndef LIBUPNP_DATA_H
#define LIBUPNP_DATA_H

namespace upnp {

struct EnumeratedString
{
    unsigned int n;
    const char *const *alternatives;

    /** Returns index into array, or n if not found. Case insensitive.
     *
     * NB "alternatives" must be sorted for this to work
     */
    unsigned int Find(const char *key) const;
};

/** Compact form of SCPD suitable for marshalling of SOAP.
 */
struct Data
{
    /* Values of 'types' */
    enum {
	BOOL = 'A',
	I8,
	UI8,
	I16,
	UI16,
	I32,
	UI32,
	STRING,
	ENUM
    };

    EnumeratedString params;

    /** Types of each state variable (indexed by param number) */
    const unsigned char *param_types;

    EnumeratedString actions;

    const unsigned char *const *action_args;
    const unsigned char *const *action_results;

    const EnumeratedString *enums;
};

} // namespace upnp

#endif
