#ifndef WCWIDTH_H_INCLUDED
#define WCWIDTH_H_INCLUDED

#include <stdlib.h>

__BEGIN_DECLS

// Modified for moot project - does not return negative number for non-printable
// characters.
int wcwidth(wchar_t ucs);

__END_DECLS

#endif