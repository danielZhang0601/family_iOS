#include "sys/sys_time.h"
#include <assert.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

unsigned int sys_get_tick_count()
{
#ifdef WIN32
	return GetTickCount();
#else
	struct timespec tp;
#ifdef __RJ_IOS__
	//http://lists.freedesktop.org/archives/spice-devel/2010-September/001231.html
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tp.tv_sec = tv.tv_sec;
	tp.tv_nsec = tv.tv_usec*1000;
#else
	if(!clock_gettime(CLOCK_MONOTONIC, &tp) < 0){assert(false);}
#endif
	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
#endif
}

