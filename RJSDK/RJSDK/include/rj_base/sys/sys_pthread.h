///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/19
//
/// @file	 sys_pthread.h
/// @brief	 线程相关定义
/// @author  
/// @version 0.1
/// @history 修改历史
///	  \n xxx 2015/09/19	0.1	创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __SYS_PTHREAD_H__
#define __SYS_PTHREAD_H__

#include "util/rj_type.h"


///	@struct sys_thread_t
///	@brief	线程
typedef struct sys_thread_t sys_thread_t;


/// @brief 线程创建后的入口函数
/// @param [in] pParam  参数
/// @param [in] pRun    是否运行
typedef int (*sys_start_routine)(void *p_param, int *p_run);


/// @brief 创建线程
/// @param [in] start_routine 线程创建后的入口函数
/// @param [in] pParam        参数
/// @param [in] pRun          是否运行的标记(这个值绑定一直到线程结束)
/// @return sys_thread_t*     返回线程
RJ_API sys_thread_t* sys_thread_create(sys_start_routine start_routine, void *p_param, int *p_run);


/// @brief 销毁线程
/// @param [in] pThread 线程
/// @param [in] pRun    是否运行的标记
RJ_API void sys_thread_destroy(sys_thread_t *p_thread, int *p_run);



/// @brief 获取进程pid
RJ_API int sys_get_pid();


/// @brief 获取线程tid
RJ_API int sys_get_tid();


/// @brief 线程休眠
/// @param [in] millon_seconds 毫秒, 最小10毫秒
RJ_API void sys_sleep(unsigned int millon_seconds);


///	@struct sys_mutex_t
///	@brief	普通锁
typedef struct sys_mutex_t sys_mutex_t;

/// @brief 创建锁
RJ_API sys_mutex_t* sys_mutex_create();

/// @brief 销毁锁
RJ_API void sys_mutex_destroy(sys_mutex_t *p_mutex);

/// @brief 尝试加锁, 0成功; 非0失败
RJ_API int  sys_mutex_trylock(sys_mutex_t *p_mutex);


/// @brief 解锁
RJ_API void sys_mutex_unlock(sys_mutex_t *p_mutex);

#ifndef NDEBUG
/// @brief 可以调试的锁
RJ_API void sys_my_mutex_lock(sys_mutex_t *p_mutex, const char *p_file, const char *p_func, int line);

#undef sys_mutex_lock
#define sys_mutex_lock(PTR_MUTEX)       sys_my_mutex_lock((PTR_MUTEX), __FILE__, __FUNCTION__, __LINE__)

#undef sys_mutex_trylock
#define sys_mutex_trylock(PTR_MUTEX)    sys_mutex_trylock(PTR_MUTEX)

#undef sys_mutex_unlock
#define sys_mutex_unlock(PTR_MUTEX)     sys_mutex_unlock(PTR_MUTEX)

#else
/// @brief 加锁
RJ_API void sys_mutex_lock(sys_mutex_t *p_mutex);
#endif


///	@struct sys_cond_t
///	@brief	线程唤醒信号
typedef struct sys_cond_t sys_cond_t;

/// @brief 创建信号
RJ_API sys_cond_t* sys_cond_create();

/// @brief 销毁信号
RJ_API void sys_cond_destroy(sys_cond_t *p_cond);

/// @brief 唤醒一个等待p_cond信号的线程
RJ_API void sys_cond_signal(sys_cond_t *p_cond);

/// @brief 唤醒所有等待p_cond信号的线程
RJ_API void sys_cond_broadcast(sys_cond_t *p_cond);

/// @brief 一直等待信号p_cond
/// @param [in] p_cond  唤醒信号
/// @param [in] p_mutex 与信号协作的锁
/// @note 此函数调用过程中大概执行动作为:
///   1. 内核执行加锁(p_mutex)
///   2. 内核将线程移到休眠列表, 即不会得到执行时间片
///   3. 其他线程调用sys_cond_signal, 注入唤醒信号(信号只有true,false两种状态)
///   4. 内核将等待p_cond信号的线程激活
///   5. 内核执行解锁(p_mutex)
///   6. 函数返回, 继续执行
RJ_API void sys_cond_wait(sys_cond_t *p_cond, sys_mutex_t *p_mutex);

/// @brief 超时等待信号p_cond
/// @param [in] p_cond  唤醒信号
/// @param [in] p_mutex 与信号协作的锁
/// @param [in] timeout 超时时间, 单位纳秒, 使用相对时间
RJ_API int  sys_cond_timedwait(sys_cond_t *p_cond, sys_mutex_t *p_mutex, uint64 timeout);



/// @struct sys_atomic_t
/// @brief  原子变量
typedef struct sys_atomic_t sys_atomic_t;

/// @brief 创建原子变量
RJ_API sys_atomic_t* sys_atomic_create();

/// @brief 销毁原子变量
RJ_API void sys_atomic_destroy(sys_atomic_t *p_atomic);

/// @brief 清零原子变量
RJ_API void sys_atomic_set_zero(sys_atomic_t *p_atomic);

/// @brief 给原子变量设置值
RJ_API int sys_atomic_set_value(sys_atomic_t *p_atomic, int value);

/// @brief 取原子变量的值
RJ_API int sys_atomic_get_value(sys_atomic_t *p_atomic);

/// @brief 取原子变量减一
RJ_API int sys_atomic_inc(sys_atomic_t *p_atomic);

/// @brief 取原子变量加一
RJ_API int sys_atomic_dec(sys_atomic_t *p_atomic);

/// @brief 检查原子变量是否为0, 返回0时, 原子变量为0, 否则非0
///   比sys_atomic_get_value高效
RJ_API int sys_atomic_zero(sys_atomic_t *p_atomic);


/// @brief 原子锁加锁
/// @param [in] p_atomic 原子变量
/// @note !原子变量做为锁使用时, 必须只能使用加解锁操作, 否则会出现异常
///   !禁止同一个线程嵌套原子锁
///   !禁止在原子锁中做繁重任务或休眠
///   !禁止在三个线程以上做同步, 否则实际效果不如普通锁
RJ_API void sys_atomic_lock(sys_atomic_t *p_atomic);

/// @brief 原子锁尝试加锁
/// @param [in] p_atomic 原子变量
/// @return 0,加锁成功; 非0,加锁失败
RJ_API int sys_atomic_try_lock(sys_atomic_t *p_atomic);

/// @brief 原子锁解锁
/// @param [in] p_atomic 原子变量
RJ_API void sys_atomic_unlock(sys_atomic_t *p_atomic);


#endif // __SYS_PTHREAD_H__
//end
