#include <stdlib.h>
#include <wchar.h>

size_t musl_mbsrtowcs(wchar_t * ws, const char ** src, size_t wn, mbstate_t * st);

size_t musl_mbstowcs(wchar_t * ws, const char * s, size_t wn)
{
	return musl_mbsrtowcs(ws, (void*)&s, wn, 0);
}
