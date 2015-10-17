///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/19
//
/// @file	 sys_pthread.h
/// @brief	 �߳���ض���
/// @author  
/// @version 0.1
/// @history �޸���ʷ
///	  \n xxx 2015/09/19	0.1	�����ļ�
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __SYS_PTHREAD_H__
#define __SYS_PTHREAD_H__

#include "util/rj_type.h"


///	@struct sys_thread_t
///	@brief	�߳�
typedef struct sys_thread_t sys_thread_t;


/// @brief �̴߳��������ں���
/// @param [in] pParam  ����
/// @param [in] pRun    �Ƿ�����
typedef int (*sys_start_routine)(void *p_param, int *p_run);


/// @brief �����߳�
/// @param [in] start_routine �̴߳��������ں���
/// @param [in] pParam        ����
/// @param [in] pRun          �Ƿ����еı��(���ֵ��һֱ���߳̽���)
/// @return sys_thread_t*     �����߳�
RJ_API sys_thread_t* sys_thread_create(sys_start_routine start_routine, void *p_param, int *p_run);


/// @brief �����߳�
/// @param [in] pThread �߳�
/// @param [in] pRun    �Ƿ����еı��
RJ_API void sys_thread_destroy(sys_thread_t *p_thread, int *p_run);



/// @brief ��ȡ����pid
RJ_API int sys_get_pid();


/// @brief ��ȡ�߳�tid
RJ_API int sys_get_tid();


/// @brief �߳�����
/// @param [in] millon_seconds ����, ��С10����
RJ_API void sys_sleep(unsigned int millon_seconds);


///	@struct sys_mutex_t
///	@brief	��ͨ��
typedef struct sys_mutex_t sys_mutex_t;

/// @brief ������
RJ_API sys_mutex_t* sys_mutex_create();

/// @brief ������
RJ_API void sys_mutex_destroy(sys_mutex_t *p_mutex);

/// @brief ���Լ���, 0�ɹ�; ��0ʧ��
RJ_API int  sys_mutex_trylock(sys_mutex_t *p_mutex);


/// @brief ����
RJ_API void sys_mutex_unlock(sys_mutex_t *p_mutex);

#ifndef NDEBUG
/// @brief ���Ե��Ե���
RJ_API void sys_my_mutex_lock(sys_mutex_t *p_mutex, const char *p_file, const char *p_func, int line);

#undef sys_mutex_lock
#define sys_mutex_lock(PTR_MUTEX)       sys_my_mutex_lock((PTR_MUTEX), __FILE__, __FUNCTION__, __LINE__)

#undef sys_mutex_trylock
#define sys_mutex_trylock(PTR_MUTEX)    sys_mutex_trylock(PTR_MUTEX)

#undef sys_mutex_unlock
#define sys_mutex_unlock(PTR_MUTEX)     sys_mutex_unlock(PTR_MUTEX)

#else
/// @brief ����
RJ_API void sys_mutex_lock(sys_mutex_t *p_mutex);
#endif


///	@struct sys_cond_t
///	@brief	�̻߳����ź�
typedef struct sys_cond_t sys_cond_t;

/// @brief �����ź�
RJ_API sys_cond_t* sys_cond_create();

/// @brief �����ź�
RJ_API void sys_cond_destroy(sys_cond_t *p_cond);

/// @brief ����һ���ȴ�p_cond�źŵ��߳�
RJ_API void sys_cond_signal(sys_cond_t *p_cond);

/// @brief �������еȴ�p_cond�źŵ��߳�
RJ_API void sys_cond_broadcast(sys_cond_t *p_cond);

/// @brief һֱ�ȴ��ź�p_cond
/// @param [in] p_cond  �����ź�
/// @param [in] p_mutex ���ź�Э������
/// @note �˺������ù����д��ִ�ж���Ϊ:
///   1. �ں�ִ�м���(p_mutex)
///   2. �ں˽��߳��Ƶ������б�, ������õ�ִ��ʱ��Ƭ
///   3. �����̵߳���sys_cond_signal, ע�뻽���ź�(�ź�ֻ��true,false����״̬)
///   4. �ں˽��ȴ�p_cond�źŵ��̼߳���
///   5. �ں�ִ�н���(p_mutex)
///   6. ��������, ����ִ��
RJ_API void sys_cond_wait(sys_cond_t *p_cond, sys_mutex_t *p_mutex);

/// @brief ��ʱ�ȴ��ź�p_cond
/// @param [in] p_cond  �����ź�
/// @param [in] p_mutex ���ź�Э������
/// @param [in] timeout ��ʱʱ��, ��λ����, ʹ�����ʱ��
RJ_API int  sys_cond_timedwait(sys_cond_t *p_cond, sys_mutex_t *p_mutex, uint64 timeout);



/// @struct sys_atomic_t
/// @brief  ԭ�ӱ���
typedef struct sys_atomic_t sys_atomic_t;

/// @brief ����ԭ�ӱ���
RJ_API sys_atomic_t* sys_atomic_create();

/// @brief ����ԭ�ӱ���
RJ_API void sys_atomic_destroy(sys_atomic_t *p_atomic);

/// @brief ����ԭ�ӱ���
RJ_API void sys_atomic_set_zero(sys_atomic_t *p_atomic);

/// @brief ��ԭ�ӱ�������ֵ
RJ_API int sys_atomic_set_value(sys_atomic_t *p_atomic, int value);

/// @brief ȡԭ�ӱ�����ֵ
RJ_API int sys_atomic_get_value(sys_atomic_t *p_atomic);

/// @brief ȡԭ�ӱ�����һ
RJ_API int sys_atomic_inc(sys_atomic_t *p_atomic);

/// @brief ȡԭ�ӱ�����һ
RJ_API int sys_atomic_dec(sys_atomic_t *p_atomic);

/// @brief ���ԭ�ӱ����Ƿ�Ϊ0, ����0ʱ, ԭ�ӱ���Ϊ0, �����0
///   ��sys_atomic_get_value��Ч
RJ_API int sys_atomic_zero(sys_atomic_t *p_atomic);


/// @brief ԭ��������
/// @param [in] p_atomic ԭ�ӱ���
/// @note !ԭ�ӱ�����Ϊ��ʹ��ʱ, ����ֻ��ʹ�üӽ�������, ���������쳣
///   !��ֹͬһ���߳�Ƕ��ԭ����
///   !��ֹ��ԭ���������������������
///   !��ֹ�������߳�������ͬ��, ����ʵ��Ч��������ͨ��
RJ_API void sys_atomic_lock(sys_atomic_t *p_atomic);

/// @brief ԭ�������Լ���
/// @param [in] p_atomic ԭ�ӱ���
/// @return 0,�����ɹ�; ��0,����ʧ��
RJ_API int sys_atomic_try_lock(sys_atomic_t *p_atomic);

/// @brief ԭ��������
/// @param [in] p_atomic ԭ�ӱ���
RJ_API void sys_atomic_unlock(sys_atomic_t *p_atomic);


#endif // __SYS_PTHREAD_H__
//end
