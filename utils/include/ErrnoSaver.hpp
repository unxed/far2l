#pragma once
#include <errno.h>

class ErrnoSaver
{
	int _errno;

public:
	inline ErrnoSaver(int err) : _errno(err) {}

#ifdef __MINGW32__
	inline ErrnoSaver() : _errno(/*errno*/ 0) {}
	inline ~ErrnoSaver() { /*errno = _errno;*/ }
#else
	inline ErrnoSaver() : _errno(errno) {}
	inline ~ErrnoSaver() { errno = _errno; }
#endif

	inline void Set(int err) { _errno = err; }

#ifdef __MINGW32__
	inline void Set() { _errno = 0; }
#else
	inline void Set() { _errno = errno; }
#endif

	inline int Get() const { return _errno; };

	inline bool IsAccessDenied() const
	{
		return _errno == EPERM
			|| _errno == EACCES
#ifdef ENOTCAPABLE
			|| _errno == ENOTCAPABLE
#endif
			;
	}
};
