#ifndef __HASH_H__
#define __HASH_H__

#include "util/rj_type.h"


RJ_API uint32 hash_dx(const char *p_name, int len);
RJ_API uint32 hash_rs(const char *p_str, int len);
RJ_API uint32 hash_js(const char *p_str, int len);
RJ_API uint32 hash_pjw(const char *p_str, int len);
RJ_API uint32 hash_elf(const char *p_str, int len);
RJ_API uint32 hash_bkdr(const char *p_str, int len);
RJ_API uint32 hash_sdbm(const char *p_str, int len);
RJ_API uint32 hash_djb(const char *p_str, int len);
RJ_API uint32 hash_dek(const char *p_str, int len);
RJ_API uint32 hash_bp(const char *p_str, int len);
RJ_API uint32 hash_fnv(const char *p_str, int len);
RJ_API uint32 hash_ap(const char *p_str, int len);

#endif // __HASH_H__
//end
