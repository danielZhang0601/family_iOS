#ifndef __RAND_H__
#define __RAND_H__

#include "util/rj_type.h"


/// @brief ��������ַ�
/// @param [in] dest ��Ҫ�����ַ��Ļ���
/// @param [in] len  �ַ�����  
/// @return char*    ����dest
/// @note ���ɺ���������ĩβ����"\0", ��������Ҫע��
///       �����ʼ��ǰ, �Ƽ�ʹ��srand
RJ_API char* rand_char(char *dest, int len);


#endif // __RAND_H__
//end
