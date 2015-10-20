#ifndef __RAND_H__
#define __RAND_H__

#include "util/rj_type.h"


/// @brief 随机生成字符
/// @param [in] dest 需要生成字符的缓存
/// @param [in] len  字符个数  
/// @return char*    返回dest
/// @note 生成函数不会在末尾补充"\0", 调用者需要注意
///       软件初始化前, 推荐使用srand
RJ_API char* rand_char(char *dest, int len);


#endif // __RAND_H__
//end
