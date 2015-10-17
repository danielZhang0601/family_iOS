#include "pub/rj_caption.h"
#include "util/rj_type.h"
#include "util/hash.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const int RC_RAW_NUM        = 32;
const int RC_RAW_CAP_NUM    = 256;
const int RC_MAX_CAP_LEN    = 256;

typedef struct rj_node_t
{
    uint32  h_val;
    char    *p_caption;
}rj_node_t;

typedef struct rj_cap_t
{
    rj_node_t   node[RC_RAW_NUM][RC_RAW_CAP_NUM];
}rj_cap_t;

//////////////////////////////////////////////////////////////////////////

rj_cap_h rj_cap_create()
{
    rj_cap_t * p_cap = new rj_cap_t;
    memset(p_cap, 0, sizeof(rj_cap_t));

    return (rj_cap_h)p_cap;
}

void rj_cap_destroy(rj_cap_h cap_h)
{
    assert (NULL != cap_h);

    rj_cap_t * p_cap = (rj_cap_t *)cap_h;

    for (int r=0; r<RC_RAW_NUM; ++r)
        for (int i=0; i<RC_RAW_CAP_NUM; ++i)
            if (NULL != p_cap->node[r][i].p_caption)
                delete [] p_cap->node[r][i].p_caption;

    delete p_cap;
}

void rj_cap_push(rj_cap_h cap_h, int index, const char *p_caption)
{
    assert (NULL != cap_h);
    assert (NULL != p_caption);

    rj_cap_t * p_cap = (rj_cap_t *)cap_h;
    assert (index < (RC_RAW_NUM * RC_RAW_CAP_NUM));

    int r   = (index >> 8);
    int i   = (index & 0xFF);

    int cap_len = strlen(p_caption);
    assert ((0 < cap_len) && (cap_len <= RC_MAX_CAP_LEN));

    assert (NULL == p_cap->node[r][i].p_caption);

    if (NULL == p_cap->node[r][i].p_caption)
        p_cap->node[r][i].p_caption  = new char [cap_len+4];

    p_cap->node[r][i].h_val     = hash_dx(p_caption, cap_len);
    strcpy(p_cap->node[r][i].p_caption, p_caption);
}

const char * rj_caption(rj_cap_h cap_h, int index)
{
    assert (NULL != cap_h);

    rj_cap_t * p_cap = (rj_cap_t *)cap_h;
    assert (index < (RC_RAW_NUM * RC_RAW_CAP_NUM));

    int r   = (index >> 8);
    int i   = (index & 0xFF);

    assert (NULL != p_cap->node[r][i].p_caption);

    return p_cap->node[r][i].p_caption;
}

int rj_cap_index(rj_cap_h cap_h, const char *p_caption)
{
    assert (NULL != cap_h);
    assert (NULL != p_caption);

    rj_cap_t * p_cap = (rj_cap_t *)cap_h;

    uint32 h_val = hash_dx(p_caption, strlen(p_caption));

    for (int r=0; r<RC_RAW_NUM; ++r)
        for (int i=0; i<RC_RAW_CAP_NUM; ++i)
            if (h_val == p_cap->node[r][i].h_val)
                return ((r << 8) + (i & 0xFF));

    return -1;
}
//end
