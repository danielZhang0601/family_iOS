#include "pub/rj_key.h"
#include <string.h>
#include <stdio.h>
#include "util/logger.h"
#include "util/cJSON.h";

static int read_file(const char* fn, char *buf, int buf_len)
{
    FILE* f_key_res = fopen(fn, "r");
    if (NULL == f_key_res)
    {
        LOG_DEBUG("Can not open file: %s", fn);
        fclose(f_key_res);
        return -1;
    }

    int read_sz = fread(buf, 1, buf_len, f_key_res);
    if (read_sz > 0 && read_sz < buf_len)
    {
        buf[read_sz] = 0;
    }
    else
    {
        if (read_sz <= 0)
        {
            LOG_DEBUG("Read file err : %s", fn);
        }
        else
        {
            LOG_DEBUG("File size invalid : %s  size=%d", fn, read_sz);
        }
        read_sz = 0;
    }
    fclose(f_key_res);
    return read_sz;
}

int rj_key_load_res(rj_cap_h key_cap, const char* fn)
{
    const int BUF_SIZE = 128 * 1024;
    char* buf = new char[BUF_SIZE];

    if (NULL == buf)
    {
        return -1;
    }

    int sz = read_file(fn, buf, BUF_SIZE);
    if (sz <= 0)
    {
        delete[] buf;
        buf = NULL;
        return -1;
    }

    cJSON *js = cJSON_Parse(buf);
    if (NULL == js)
    {
        LOG_DEBUG("Parse json failed! file=%s", fn);
        delete[] buf;
        buf = NULL;
        return -1;
    }

    cJSON *item = js->child;
    while (item != NULL)
    {
        if (item->string == NULL)
        {
            LOG_DEBUG("rj_key_res has invalid format! file=%s", fn);
            delete[] buf;
            buf = NULL;
            return -1;
        }
        
        rj_cap_push(key_cap, item->valueint, item->string);

        item = item->next;
    }

    delete[] buf;
    buf = NULL;
    return 0;
}
