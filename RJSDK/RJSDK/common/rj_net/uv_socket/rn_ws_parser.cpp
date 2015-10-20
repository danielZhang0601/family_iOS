#include "rn_ws_parser.h"
#include "sha1.h"
#include <stdio.h>
#include <string.h>
#include "base64.h"

int ws_pack(char *h, char f, char op, char has_k, char* key, uint64 len)
{
    int h_len = 2;

    h[0] = (f | op);
    
    if (len < 0x7E)// < 126
    {
        h[1] = len;
    }
    else if (len <= 0xFFFF)// < 65535
    {
        int move = 8;
        for (int i=2; i<4; ++i)
        {
            h[i] = (len >> move) & 0xFF;
            move -= 8;
        }

        h[1] = 0x7E;
        h_len += 2;
    }
    else
    {
        int move = 56;
        for (int i=2; i<10; ++i)
        {
            h[i] = (len >> move) & 0xFF;
            move -= 8;
        }

        h_len += 8;
    }

    h[1] |= (0 != has_k) ? 0x80 : 0x00;

    if (0 != has_k)    //复制MASK KEY
    {
        memcpy(h+h_len, key, 4);
        h_len += 4;
    }

    return h_len;
}

int ws_unpack(char *h, char d_len, char *f, char *op, char *has_k, char *key, uint64 *len)
{
    unsigned char *p_h = (unsigned char *)h;

    if (d_len < 2)
        return 0;

    if (0x00 != (0x70 & h[0]))//RVS1,RVS2,RVS3未启用
        return -1;

    if (0x04 == (0x04 & h[0]))//(4-7)(0100,01001,0110,0111)没有定义
        return -1;

    if ((0x00 == (0x80 & h[0])) && (0x08 == (0x0C & h[0])))//PING,PONG,CLOSE，但是fin为零
        return -1;

    *len = 0;

    int h_len = 2;

    *f  = (0x80 & h[0]);
    *op = (0x0F & h[0]);

    if (0x7F == (0x7F & h[1]))        //127
    {
        h_len += 8;

        if (d_len < h_len)
            return 0;

        int move = 56;
        for (int i=2; i<10; ++i)
        {
            (*len) += ((uint64)p_h[i] << move);
            move -= 8;
        }
    }
    else if (0x7E == (0x7F & h[1]))  //126
    {
        h_len += 2;

        if (d_len < h_len)
            return 0;

        int move = 8;
        for (int i=2; i<4; ++i)
        {
            (*len) += ((uint64)p_h[i] << move);
            move -= 8;
        }
    }
    else        // <= 125
    {
        *len = (0x7F & h[1]);
    }

    if (0x80 == (0x80 & h[1]))
    {
        *has_k  = 1;
        memcpy(key, h+h_len, 4);
        h_len += 4;

        if (d_len < h_len)
            return 0;
    }
    else
        *has_k = 0;

    return h_len;
}

//////////////////////////////////////////////////////////////////////////
void ws_rand_key(char *p_buf, int buf_len)
{
    assert (NULL != p_buf);
    assert (32 <= buf_len);

    char str [16] = {0}; 
    const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="; 
    const int base_len = sizeof(base) - 1;

    for(int i=0;i<16;i++)
    {
        str[i] = base[ rand()%base_len ];
    }
    base64_encode(p_buf, str, 16);
}


char * ws_get_value(char *p_buf, int buf_len, char *p_str, const char *p_caption)
{
    assert (NULL != p_buf);
    assert (NULL != p_str);
    assert (NULL != p_caption);

    //assert ((0 == strcmp("Sec-WebSocket-Key: ", p_caption)) || (0 == strcmp("Sec-WebSocket-Accept: ", p_caption)));

    char *p_key = strstr(p_str, p_caption);
    if(NULL != p_key)
    {
        //跳到KEY的首地址
        p_key += strlen(p_caption);

        //把整个KEY复制到t_key
        int i = 0;
        buf_len = buf_len - 1;
        while((*p_key != '\r') && (*p_key != '\n'))
        {
            p_buf[i++] = *p_key;
            p_key ++;
        }
        p_buf[i] = '\0';

        return p_buf;
    }
    else
        p_buf[0] = '\0';

    return NULL;
}

void ws_encode_key(char *p_dst, char *p_src)
{
    assert (NULL != p_dst);
    assert (NULL != p_src);

    char t_ws_key [1024]    = {0};
    char t_buf[24]          = {0};

    strcpy(t_ws_key, p_src);
    strcat(t_ws_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    get_sha1_string(t_buf, t_ws_key);
    base64_encode(p_dst, t_buf, 20);
}

int ws_hs_client(char *p_buf, int buf_len, char *p_key, char *p_protocol, char *p_ip, uint16 port)
{
    assert (NULL != p_buf);
    assert (NULL != p_key);
    assert (NULL != p_protocol);
    assert (NULL != p_ip);

    char  server[128]        ={0};
    char  ws_protocol[128]   ={0};
    sprintf(server,"%s%s:%d\r\n","Host: ", p_ip, port);
    sprintf(ws_protocol,"%s%s\r\n","Sec-WebSocket-Protocol: ", p_protocol);

    sprintf(p_buf, "%s\r\n", "GET / HTTP/1.1");
    strcat(p_buf, "Upgrade: websocket\r\n");
    strcat(p_buf, "Connection: Upgrade\r\n");
    strcat(p_buf, "Sec-WebSocket-Version: 13\r\n");
    strcat(p_buf, ws_protocol);
    strcat(p_buf, server);
    strcat(p_buf, "Sec-WebSocket-Key: ");
    strcat(p_buf, p_key);
    strcat(p_buf, "\r\n\r\n");

    return strlen(p_buf);
}

int ws_hs_server(char *p_buf, int buf_len, char *p_key, char *p_protocol)
{
    assert (NULL != p_buf);
    assert (NULL != p_key);
    assert (NULL != p_protocol);

    char  ws_protocol[128]   ={0};
    sprintf(ws_protocol,"%s%s\r\n","Sec-WebSocket-Protocol: ", p_protocol);

    sprintf(p_buf, "%s\r\n", "HTTP/1.1 101 Switching Protocols");
    strcat(p_buf, "Upgrade: websocket\r\n");
    strcat(p_buf, "Connection: Upgrade\r\n");
    strcat(p_buf, "Sec-WebSocket-Version: 13\r\n");
    strcat(p_buf, ws_protocol);
    strcat(p_buf, "Sec-WebSocket-Accept: ");
    strcat(p_buf, p_key);
    strcat(p_buf, "\r\n\r\n");

    return strlen(p_buf);
}
//////////////////////////////////////////////////////////////////////////
uint32 encryption(char *p_src, uint32 src_len, char *p_dst, uint32 dst_len)
{
    assert (NULL != p_src);
    assert (NULL != p_dst);
    assert ((16 <= src_len) && (0x00 == (0x0F & src_len)));
    assert (src_len < dst_len);

    memcpy(p_dst, p_src, src_len);
    return src_len;
}

uint32 decrypt(char *p_src, uint32 src_len, char *p_dst, uint32 dst_len)
{
    assert (NULL != p_src);
    assert (NULL != p_dst);
    assert ((16 <= src_len) && (0x00 == (0x0F & src_len)));
    assert (src_len < dst_len);

    memcpy(p_dst, p_src, src_len);
    return src_len;
}

/*
请求:

GET / HTTP/1.1
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Version: 13
Sec-WebSocket-Key: qPDDeYhyaIhjCtefxxR4/Q==
Host: 192.168.8.181:3455
Sec-WebSocket-Protocol: dev_man_protocol

回复:

HTTP/1.1 400 Bad Request
Connection: close
X-WebSocket-Reject-Reason: Client must provide a Host header.

HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: xY9zZMQTFsuY8fI6UBpaOmQ4x0Y=
Sec-WebSocket-Protocol: dev_man_protocol

*/

//end
