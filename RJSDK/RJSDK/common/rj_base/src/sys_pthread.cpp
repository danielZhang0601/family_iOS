#include "sys/sys_pthread.h"
#include "sys/sys_mem.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>

typedef struct sys_thread_t
{
	HANDLE              thread;
	void                *p_param;
	int                 *p_run;
    sys_start_routine   user_routine;
}sys_thread_t;

typedef struct sys_mutex_t
{
	CRITICAL_SECTION    mutex;
}sys_mutex_t;

typedef struct sys_cond_t
{
    union{
        CONDITION_VARIABLE      cond_var;
        struct {
            unsigned int        waiters_count;
            CRITICAL_SECTION    waiters_count_lock;
            HANDLE              signal_event;
            HANDLE              broadcast_event;
        } fallback;
    }cond;
}sys_cond_t;

#else

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

typedef struct sys_thread_t
{
	pthread_t         thread;
	void              *p_param;
	int               *p_run;
	sys_start_routine user_routine;
}sys_thread_t;

typedef struct sys_mutex_t
{
	pthread_mutex_t mutex;
}sys_mutex_t;

typedef struct sys_cond_t
{
    pthread_cond_t  cond;
}sys_cond_t;

#undef NANOSEC
#define NANOSEC ((uint64) 1e9)

#endif


///	@struct sys_atomic_t
///	@brief	原子变量
typedef struct sys_atomic_t
{
    long volatile   atomic_count;           ///< 这个值必须位于结构体首位
}sys_atomic_t;


#ifdef WIN32

static DWORD WINAPI internal_thread_routing(void *p_param)
{
    static sys_atomic_t s_thread_num = {0};

    sys_atomic_inc(&s_thread_num);
    int num = sys_atomic_get_value(&s_thread_num);

    printf("thread start,tid=%d,pid=%d.total=%d\n", sys_get_tid(), sys_get_pid(), num);

	sys_thread_t *p_thread = reinterpret_cast<sys_thread_t *>(p_param);
	p_thread->user_routine(p_thread->p_param, p_thread->p_run);

    sys_atomic_dec(&s_thread_num);
    num = sys_atomic_get_value(&s_thread_num);

    printf("thread stop,tid=%d,pid=%d.total=%d\n", sys_get_tid(), sys_get_pid(), num);
	return 0;
}

sys_thread_t* sys_thread_create(sys_start_routine start_routine, void *p_param, int *p_run)
{
	if(NULL == start_routine || NULL == p_run)
	{
		assert(false);
		return NULL;
	}

	sys_thread_t *p_thread = (sys_thread_t*)sys_malloc(sizeof(sys_thread_t));
    assert(NULL != p_thread);

	p_thread->p_run = p_run;
	p_thread->p_param = p_param;
	p_thread->user_routine = start_routine;
	*p_run = 1;
	
    p_thread->thread = CreateThread(0, 0, internal_thread_routing, (void *)p_thread, 0, NULL);
	if(NULL == p_thread->thread)
	{
		sys_free(p_thread);
		return NULL;
	}

	return p_thread;
}

void sys_thread_destroy(sys_thread_t *p_thread, int *p_run)
{
    assert(NULL != p_thread);
    assert(NULL != p_run);

	assert(p_run == p_thread->p_run);
	*p_run = 0;

	WaitForSingleObject(p_thread->thread, INFINITE);
	CloseHandle(p_thread->thread);
	
    sys_free(p_thread);
    p_thread  = NULL;
}

void sys_sleep(unsigned int millon_seconds)
{
    if (10 >= millon_seconds)
    {
        Sleep(10);
    }
    else
    {
        Sleep(millon_seconds);
    }
}

int sys_get_pid()
{
    return ::GetProcessIdOfThread(GetCurrentThread());
}

int sys_get_tid()
{
    return ::GetCurrentThreadId();
}

sys_mutex_t* sys_mutex_create()
{
	sys_mutex_t *pMutex = (sys_mutex_t*)sys_malloc(sizeof(sys_mutex_t));
    assert(NULL != pMutex);

	InitializeCriticalSection(&(pMutex->mutex));
	return pMutex;
}

void sys_mutex_destroy(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

	DeleteCriticalSection(&(p_mutex->mutex));
	
    sys_free(p_mutex);
}

#ifdef NDEBUG
void sys_mutex_lock(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

    EnterCriticalSection(&(p_mutex->mutex));
}
#endif

void sys_my_mutex_lock(sys_mutex_t *p_mutex, const char *p_file, const char *p_func, int line)
{
    int count = 0;
    while(true)
    {
        if(0 == sys_mutex_trylock(p_mutex))
        {
            break;
        }

        count += 1;
        if( (count >= 300) && (0 == (count % 100)) )
        {
            printf("lock error.count=%d.(%s,%s,%d)\n", count, p_file, p_func, line);
        }

        sys_sleep(10);
    }
}

int sys_mutex_trylock(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

	if(TryEnterCriticalSection(&(p_mutex->mutex)))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

void sys_mutex_unlock(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

	LeaveCriticalSection(&(p_mutex->mutex));
}


/* This condition variable implementation is based on the SetEvent solution
 * (section 3.2) at http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * We could not use the SignalObjectAndWait solution (section 3.4) because
 * it want the 2nd argument (type sys_mutex_t) of sys_cond_wait() and
 * sys_cond_timedwait() to be HANDLEs, but we use CRITICAL_SECTIONs.
 */

static int sys_cond_fallback_init(sys_cond_t *p_cond)
{
    int err;
    
    /* Initialize the count to 0. */
    p_cond->cond.fallback.waiters_count = 0;

    InitializeCriticalSection(&p_cond->cond.fallback.waiters_count_lock);

    /* Create an auto-reset event. */
    p_cond->cond.fallback.signal_event = CreateEvent(NULL,  /* no security */
                                            FALSE, /* auto-reset event */
                                            FALSE, /* non-signaled initially */
                                            NULL); /* unnamed */
    if (!p_cond->cond.fallback.signal_event) {
        err = GetLastError();
        goto error2;
    }

    /* Create a manual-reset event. */
    p_cond->cond.fallback.broadcast_event = CreateEvent(NULL,  /* no security */
                                               TRUE,  /* manual-reset */
                                               FALSE, /* non-signaled */
                                               NULL); /* unnamed */
    if (!p_cond->cond.fallback.broadcast_event) {
        err = GetLastError();
        goto error;
    }

    return 0;

error:
    CloseHandle(p_cond->cond.fallback.signal_event);
error2:
    DeleteCriticalSection(&p_cond->cond.fallback.waiters_count_lock);
    return err;
}

sys_cond_t* sys_cond_create() 
{
    sys_cond_t *p_cond = (sys_cond_t*)sys_malloc(sizeof(sys_cond_t));
    assert(NULL != p_cond);

    memset(p_cond, 0, sizeof(*p_cond));

    if(0 == sys_cond_fallback_init(p_cond))
    {
        return p_cond;
    }
    else
    {
        sys_free(p_cond);
        assert(false);
        return NULL;
    }
}

void sys_cond_destroy(sys_cond_t* p_cond) {
    assert(NULL != p_cond);

    if (!CloseHandle(p_cond->cond.fallback.broadcast_event))
    {
         assert(false);
    }

    if (!CloseHandle(p_cond->cond.fallback.signal_event))
    {
         assert(false);
    }

    DeleteCriticalSection(&p_cond->cond.fallback.waiters_count_lock);

    sys_free(p_cond);
}

void sys_cond_signal(sys_cond_t* p_cond)
{
    assert(NULL != p_cond);

    int have_waiters;

    /* Avoid race conditions. */
    EnterCriticalSection(&p_cond->cond.fallback.waiters_count_lock);
    have_waiters = p_cond->cond.fallback.waiters_count > 0;
    LeaveCriticalSection(&p_cond->cond.fallback.waiters_count_lock);

    if (have_waiters)
    {
        SetEvent(p_cond->cond.fallback.signal_event);
    }
}

void sys_cond_broadcast(sys_cond_t* p_cond)
{
    assert(NULL != p_cond);

    int have_waiters;

    /* Avoid race conditions. */
    EnterCriticalSection(&p_cond->cond.fallback.waiters_count_lock);
    have_waiters = p_cond->cond.fallback.waiters_count > 0;
    LeaveCriticalSection(&p_cond->cond.fallback.waiters_count_lock);

    if (have_waiters)
    {
        SetEvent(p_cond->cond.fallback.broadcast_event);
    }
}

static int sys_cond_wait_helper(sys_cond_t *p_cond, sys_mutex_t *p_mutex, DWORD dwMilliseconds) 
{
    DWORD result;
    int last_waiter;
    HANDLE handles[2] = {
        p_cond->cond.fallback.signal_event,
        p_cond->cond.fallback.broadcast_event
    };

    /* Avoid race conditions. */
    EnterCriticalSection(&p_cond->cond.fallback.waiters_count_lock);
    p_cond->cond.fallback.waiters_count++;
    LeaveCriticalSection(&p_cond->cond.fallback.waiters_count_lock);

    /* It's ok to release the <mutex> here since Win32 manual-reset events */
    /* maintain state when used with <SetEvent>. This avoids the "lost wakeup" */
    /* bug. */
    sys_mutex_unlock(p_mutex);

    /* Wait for either event to become signaled due to <sys_cond_signal> being */
    /* called or <sys_cond_broadcast> being called. */
    result = WaitForMultipleObjects(2, handles, FALSE, dwMilliseconds);

    EnterCriticalSection(&p_cond->cond.fallback.waiters_count_lock);
    p_cond->cond.fallback.waiters_count--;
    last_waiter = result == WAIT_OBJECT_0 + 1
        && p_cond->cond.fallback.waiters_count == 0;
    LeaveCriticalSection(&p_cond->cond.fallback.waiters_count_lock);

    /* Some thread called <pthread_cond_broadcast>. */
    if (last_waiter) {
        /* We're the last waiter to be notified or to stop waiting, so reset the */
        /* the manual-reset event. */
        ResetEvent(p_cond->cond.fallback.broadcast_event);
    }

    /* Reacquire the <mutex>. */
    sys_mutex_lock(p_mutex);

    if (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0 + 1)
        return 0;

    if (result == WAIT_TIMEOUT)
        return ETIMEDOUT;

    assert(false);
    return -1; /* Satisfy the compiler. */
}

void sys_cond_wait(sys_cond_t *p_cond, sys_mutex_t *p_mutex) 
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    if (sys_cond_wait_helper(p_cond, p_mutex, INFINITE))
    {
        assert(false);
    }
}

int sys_cond_timedwait(sys_cond_t *p_cond, sys_mutex_t *p_mutex, uint64 timeout)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    return sys_cond_wait_helper(p_cond, p_mutex, (DWORD)(timeout / 1e6));
}

sys_atomic_t* sys_atomic_create()
{
    // 原子变量地址必须为对齐的地址
    sys_atomic_t *ptr = (sys_atomic_t*)_aligned_malloc(sizeof(sys_atomic_t), sizeof(void*));
    assert(NULL != ptr);

    if(NULL != ptr)
    {
        memset(ptr, 0, sizeof(sys_atomic_t));
    }

    return ptr;
}

void sys_atomic_destroy(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    _aligned_free(p_atomic);
    //sys_free(p_atomic);
}

void sys_atomic_set_zero(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    InterlockedExchange(&p_atomic->atomic_count, 0);
}

int sys_atomic_set_value(sys_atomic_t *p_atomic, int value)
{
    assert(NULL != p_atomic);
    return InterlockedExchange(&p_atomic->atomic_count, value);
}


int sys_atomic_get_value(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    return InterlockedExchangeAdd(&p_atomic->atomic_count, 0);
}


int sys_atomic_inc(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    return InterlockedExchangeAdd(&p_atomic->atomic_count, 1);
}

int sys_atomic_dec(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    return InterlockedExchangeAdd(&p_atomic->atomic_count, -1);
}

int sys_atomic_zero(sys_atomic_t * p_atomic)
{
    assert(NULL != p_atomic);

    if (0 == InterlockedCompareExchange(&p_atomic->atomic_count, 0, 0))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void sys_atomic_lock(sys_atomic_t *p_atomic)
{
    //LONGLONG Old;

    //do {
    //    Old = *Destination;
    //} while (InterlockedCompareExchange64(Destination,
    //    Old & Value,
    //    Old) != Old);

    //return Old;

    assert(NULL != p_atomic);

    while(0 != InterlockedCompareExchange(&(p_atomic->atomic_count), 1, 0));
}

int sys_atomic_try_lock(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);

    if(0 != InterlockedCompareExchange(&(p_atomic->atomic_count), 1, 0))
    {
        // 没有锁成功
        return 1;
    }
    else
    {
        // 锁成功
        return 0;
    }

    return 0;
}

void sys_atomic_unlock(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    InterlockedExchange(&(p_atomic->atomic_count), 0);
}

#else

static void* internal_thread_routing(void * p_param)
{
    static sys_atomic_t s_thread_num = {0};

    sys_atomic_inc(&s_thread_num);
    int num = sys_atomic_get_value(&s_thread_num);
    printf("thread start,tid=%d,pid=%d.total=%d\n", sys_get_tid(), sys_get_pid(), num);

	sys_thread_t *p_thread = reinterpret_cast<sys_thread_t *>(p_param);
	p_thread->user_routine(p_thread->p_param, p_thread->p_run);

    sys_atomic_dec(&s_thread_num);
    num = sys_atomic_get_value(&s_thread_num);
    printf("thread stop,tid=%d,pid=%d.total=%d\n", sys_get_tid(), sys_get_pid(), num);

	return 0;
}

sys_thread_t* sys_thread_create(sys_start_routine start_routine, void *p_param, int *p_run)
{
	if(NULL == start_routine || NULL == p_run)
	{
		assert(false);
		return NULL;
	}

	sys_thread_t *p_thread = (sys_thread_t*)sys_malloc(sizeof(sys_thread_t));
	assert(NULL != p_thread);

    p_thread->p_run = p_run;
	p_thread->p_param = p_param;
	p_thread->user_routine = start_routine;
	*p_run = 1;

	if(pthread_create(&(p_thread->thread), 0, internal_thread_routing, p_thread))
	{
		sys_free(p_thread);
		return NULL;
	}

	return p_thread;
}

void sys_thread_destroy(sys_thread_t *p_thread, int *p_run)
{
	if(NULL == p_thread || NULL == p_run)
	{
		assert(false);
		return;
	}

	assert(p_run == p_thread->p_run);
	*p_run = 0;

#ifdef MACOS_CONFIG
	pthread_exit(p_thread->thread);
#else
	pthread_join(p_thread->thread, NULL);
#endif

	delete p_thread;
}

int sys_get_pid()
{
    int pid = getpid();
    return pid;
}

int sys_get_tid()
{
    //int tid = syscall(__NR_gettid);
    //return tid;
    return 0;
}


void sys_sleep(unsigned int millon_seconds)
{
    if(millon_seconds < 10)
    {
        millon_seconds = 10;
    }

    int iSec = millon_seconds / 1000;
    int	iMicSec = (millon_seconds % 1000) * 1000;

    if (iSec > 0) 
    {
        do 
        {
            iSec = sleep(iSec);
        } while(iSec > 0); 
    }

    if(0 != usleep(iMicSec))
    {
        if (EINTR == errno) 
        {
            printf("the usleep Interrupted by a signal. pid = %d\n", getpid());
        }
        else if (EINVAL == errno) 
        {
            assert(false);
            printf("the usleep param is not smaller than 1000000");
        }
    }
}

sys_mutex_t* sys_mutex_create()
{
	sys_mutex_t *p_mutex = (sys_mutex_t*)sys_malloc(sizeof(sys_mutex_t));
    assert(NULL != p_mutex);

	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&(p_mutex->mutex), &mutexAttr);
	pthread_mutexattr_destroy(&mutexAttr);

	return p_mutex;
}

void sys_mutex_destroy(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

	pthread_mutex_destroy(&(p_mutex->mutex));
	sys_free(p_mutex);
}

#ifdef NDEBUG
void sys_mutex_lock(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);
	pthread_mutex_lock(&(p_mutex->mutex));
}
#endif

void sys_my_mutex_lock(sys_mutex_t *p_mutex, const char *p_file, const char *p_func, int line)
{
    int count = 0;
    while(true)
    {
        if(0 == sys_mutex_trylock(p_mutex))
        {
            break;
        }

        count += 1;
        if( (count >= 300) && (0 == (count % 100)) )
        {
            printf("lock error.count=%d.(%s,%s,%d)\n", count, p_file, p_func, line);
        }

        sys_sleep(10);
    }
}

int sys_mutex_trylock(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

	if(pthread_mutex_trylock(&(p_mutex->mutex)))
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

void sys_mutex_unlock(sys_mutex_t* p_mutex)
{
    assert(NULL != p_mutex);

	pthread_mutex_unlock(&(p_mutex->mutex));
}


#if defined(__APPLE__) && defined(__MACH__)

sys_cond_t* sys_cond_create()
{
    sys_cond_t* p_cond = (sys_cond_t*)sys_malloc(sizeof(sys_cond_t));
    assert(NULL != p_cond);

    memset(p_cond, 0, sizeof(*p_cond));

    if(0 == pthread_cond_init(&p_cond->cond, NULL))
    {
        return p_cond;
    }
    else
    {
        sys_free(p_cond);
        assert(false);
        return NULL;
    }
}

#else /* !(defined(__APPLE__) && defined(__MACH__)) */

sys_cond_t* sys_cond_create()
{
    sys_cond_t* p_cond = (sys_cond_t*)sys_malloc(sizeof(sys_cond_t));
    memset(p_cond, 0, sizeof(*p_cond));

    pthread_condattr_t attr;
    int err;

    err = pthread_condattr_init(&attr);
    if (err)
    {
        sys_free(p_cond);
        assert(false);
        return NULL;
    }

#if !(defined(__ANDROID__) && defined(HAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC))
    err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (err)
    {
        goto error2;
    }
#endif

    err = pthread_cond_init(&p_cond->cond, &attr);
    if (err)
    {
        goto error2;
    }

    err = pthread_condattr_destroy(&attr);
    if (err)
    {
        goto error;
    }

    return p_cond;

error:
    pthread_cond_destroy(&p_cond->cond);
error2:
    pthread_condattr_destroy(&attr);

    sys_free(p_cond);
    assert(false);
    return NULL;
}

#endif /* defined(__APPLE__) && defined(__MACH__) */

void sys_cond_destroy(sys_cond_t *p_cond)
{
     assert(NULL != p_cond);

    if (pthread_cond_destroy(&p_cond->cond))
    {
        assert(false);
    }

    sys_free(p_cond);
}

void sys_cond_signal(sys_cond_t *p_cond)
{
     assert(NULL != p_cond);

    if (pthread_cond_signal(&p_cond->cond))
    {
        assert(false);
    }
}

void sys_cond_broadcast(sys_cond_t *p_cond)
{
     assert(NULL != p_cond);

    if (pthread_cond_broadcast(&p_cond->cond))
    {
         assert(false);
    }
}

void sys_cond_wait(sys_cond_t *p_cond, sys_mutex_t* p_mutex)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    if (pthread_cond_wait(&p_cond->cond, &p_mutex->mutex))
    {
        assert(false);
    }
}

int sys_cond_timedwait(sys_cond_t * p_cond, sys_mutex_t* p_mutex, uint64 timeout)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    int r;
    struct timespec ts;

#if defined(__APPLE__) && defined(__MACH__)
    ts.tv_sec = timeout / NANOSEC;
    ts.tv_nsec = timeout % NANOSEC;
    r = pthread_cond_timedwait_relative_np(&p_cond->cond, &p_mutex->mutex, &ts);
#else

    // todo.
    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64 temp = tv.tv_sec;
    temp *= NANOSEC;
    temp += (uint64)(tv.tv_usec * 1000);

    temp += timeout;

    ts.tv_sec = temp / NANOSEC;
    ts.tv_nsec = temp % NANOSEC;
#if defined(__ANDROID__) && defined(HAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC)
    /*
    * The bionic pthread implementation doesn't support CLOCK_MONOTONIC,
    * but has this alternative function instead.
    */
    r = pthread_cond_timedwait_monotonic_np(&p_cond->cond, &p_mutex->mutex, &ts);
#else
    r = pthread_cond_timedwait(&p_cond->cond, &p_mutex->mutex, &ts);
#endif /* __ANDROID__ */
#endif

    if (r == 0)
    {
        return 0;
    }

    if (r == ETIMEDOUT)
    {
        return -ETIMEDOUT;
    }

    assert(false);
    return -EINVAL;  /* Satisfy the compiler. */
}


sys_atomic_t* sys_atomic_create()
{
    void *ptr = NULL;

    // 原子变量地址必须为对齐的地址
    if(0 != posix_memalign(&ptr, sizeof(void*), sizeof(sys_atomic_t)))
    {
        assert(false);
        return NULL;
    }

    assert(NULL != ptr);

    if(NULL != ptr)
    {
        memset(ptr, 0, sizeof(sys_atomic_t));
    }

    return (sys_atomic_t*)ptr;
}

void sys_atomic_destroy(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    free(p_atomic);
}

void sys_atomic_set_zero(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    __sync_lock_release (&p_atomic->atomic_count);
}

int sys_atomic_set_value(sys_atomic_t *p_atomic, int value)
{
    assert(NULL != p_atomic);
    return __sync_lock_test_and_set(&p_atomic->atomic_count, value);
}


int sys_atomic_get_value(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    return __sync_fetch_and_and(&p_atomic->atomic_count, 0xffffffff);
}


int sys_atomic_inc(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    return __sync_fetch_and_add(&p_atomic->atomic_count, 1);
}

int sys_atomic_dec(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    return __sync_fetch_and_sub(&p_atomic->atomic_count, 1);
}

int sys_atomic_zero(sys_atomic_t * p_atomic)
{
    assert(NULL != p_atomic);

    if(__sync_bool_compare_and_swap(&p_atomic->atomic_count, 0, 0))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void sys_atomic_lock(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    while(!__sync_bool_compare_and_swap(&(p_atomic->atomic_count), 0, 1));
}

int sys_atomic_try_lock(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    if(__sync_bool_compare_and_swap(&(p_atomic->atomic_count), 0, 1))
    {
        // 加锁成功
        return 0;
    }
    else
    {
        // 锁失败
        return 1;
    }

    return 0;
}

void sys_atomic_unlock(sys_atomic_t *p_atomic)
{
    assert(NULL != p_atomic);
    __sync_lock_release(&(p_atomic->atomic_count));
}

#endif
