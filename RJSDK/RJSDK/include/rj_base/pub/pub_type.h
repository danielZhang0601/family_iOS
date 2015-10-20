#ifndef __PUB_TYPE_H__
#define __PUB_TYPE_H__

#include "util/rj_type.h"

const uint32    RJ_MAX_FRAME_LEN    = 1536 * 1024;
const uint32    RJ_MAX_FRAME_BUF    = RJ_MAX_FRAME_LEN + 4096;

typedef struct _RJ_MEM_BLOCK_
{
    char           *p_buf;
    uint32          buf_len;
    uint32          data_len;
}_RJ_MEM_BLOCK_;

#endif // __PUB_TYPE_H__
//end
