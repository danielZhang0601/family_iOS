#ifndef __SYS_IO_TO_CONSOLE_H__
#define __SYS_IO_TO_CONSOLE_H__

#include "util/rj_type.h"

// copy from http://www.halcyon.com/~ast/dload/guicon.htm
// modify


/// @brief 将_windows窗口程序的控制台重定向
/// @note  只需要在_windows程序启动流程时,调用一次即可.
///        可以在新建立的控制台窗口看到打印信息
RJ_API void sys_io_to_console();



#endif // __SYS_IO_TO_CONSOLE_H__
//end
