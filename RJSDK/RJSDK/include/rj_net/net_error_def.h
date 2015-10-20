#ifndef  __NET_ERROR_DEF_H__
#define  __NET_ERROR_DEF_H__
typedef  enum net_error_enum
{
     //公用0-99
    ERROR_OK      =  0,                        //操作成功
    ERROR_UNKNOWN,                             //未知错误
    ERROR_SERVER_BUSY ,                        //服务器繁忙
    ERROR_NOT_SUPPORT,                         //不支持
    ERROR_PARAMETER,                           //参数错误
    ERROR_REQUEST_TIMEOUT,                     //请求超时
    ERROR_ILLEGAL_CONNECT,                     //非法连接

    //CMS 错误码 格式判断100
    ERROR_CMS_USERNAME_FORMAT_NOT_CORRECT = 100,             //用户名格式不正确
    ERROR_CMS_PASSWORD_FORMAT_NOT_CORRECT,                   //密码格式不正确
    ERROR_CMS_EMAIL_FORMAT_NOT_CORRECT,                      //邮箱格式不正确
    ERROR_CMS_PHONE_FORMAT_NOT_CORRECT,                      //手机号码格式不正确
    ERROR_CMS_AUTH_CODE_FORMAT_NOT_CORRECT,                  //授权码格式不正确
    ERROR_CMS_DEVICE_SN_FORMAT_NOT_CORRECT,                  //设备序列号格式不正确
    ERROR_CMS_USERINFO_NAME_LENGHT_NOT_CORRECT ,              //用户名最大不能超过20个字符
    ERROR_CMS_USERINFO_SEX_FORMAT_NOT_CORRECT,               //您输入的用户的性别格式不正确
    ERROR_CMS_USERINFO_SEX_VALUE_NOT_CORRECT,                 //用户的性别只能为0或者1
    ERROR_CMS_USERINFO_ADDR_FORMAT_NOT_CORRECT,               //您的输入的地址格式不正确
    ERROR_CMS_USERINFO_ADDR_LEN_NOT_CORRECT,                 //地址不能超过128个字节
    ERROR_CMS_USERINFO_BIRTHDAY_FORAMT_NOT_CORRECT ,          //您的输入的生日格式不正确

    //CMS错误码 验证200-
    ERROR_CMS_EMAIL_NO_AUTH  =  200 ,                     //邮箱未认证
    ERROR_CMS_EMAIL_VERICODE_NOPASS ,                     //邮箱验证码未通过
    ERROR_CMS_SMS_VERICODE_NOPASS ,                       //短信验证码未通过
    ERROR_CMS_PIC_VERICODE_NOPASS ,                       //图片验证码未通过
    ERROR_CMS_AUTH_NOPASS ,                               //权限认证不通过
  
    //CMS 错误码 用户逻辑300 -
    ERROR_CMS_USERNAME_REPEAT ,                            //用户已存在
    ERROR_CMS_USER_NOT_EXISTED ,                           //用户名不存在
    ERROR_CMS_USER_UNLOGIN ,                               //用户未登陆
    ERROR_CMS_PASSWORD ,                                  //密码错误
    ERROR_CMS_CEllPHONE_BY_OTHERS ,                       //该手机号码已被其他用户绑定
    ERROR_CMS_EMAIL_BY_OHTERS ,                           //该邮箱已被其他用户绑定
    ERROR_CMS_SAME_PASSWORD ,                             //新密码和旧密码不能相同
    ERROR_CMS_USER_CEllPHONE_REPEAT ,                     //新手机号码和旧手机号码不能相同
    ERROR_CMS_USER_EMAIL_REPEAT ,                         //新邮箱和旧邮箱不能相同
    ERROR_CMS_PHONE_NUMBER_USED ,                        //该手机号码已经被使用
    ERROR_CMS_EMAIL_USED ,                               //该邮箱已经被使用

     //CMS 错误码 设备相关逻辑400 -
    ERROR_CMS_DEV_BY_USER_BIND = 400 ,                        //设备已经被其他用户绑定
    ERROR_CMS_DEV_NOT_EXIST,                                 //设备不存在     
    ERROR_CMS_DEV_NO_BIND,                                   //设备未绑定
    ERROR_CMS_DEV_BIND_REPEAT,                              //设备已经添加过了
    ERROR_CMS_DEV_OFFLINE,                                  //设备不在线

    //DMS 错误码 格式判断1000-1099
    ERROR_DMS_ACCOUNT_FORMAT = 1000,              //用户名格式不正确
    ERROR_DMS_PHONE_FORMAT,                       //手机号码格式不正确
    ERROR_DMS_EMAIL_FORMAT,                       //邮箱格式不正确
    ERROR_DMS_PASSWORD_FORMAT  ,                  //密码格式不正确
    ERROR_DMS_DEVICE_SN_ILLEGAL ,                 //设备序列号不合法
    ERROR_DMS_DEVICE_LICENSE_ILLEGAL  ,           //设备License不合法
    ERROR_DMS_DEVICE_AUTHCODE_ILLEGAL,            //设备授权码不合法

    //DMS 错误码 客户端逻辑相关1100-1199
    ERROR_DMS_DEVICE_OFFLINE = 1100,             //设备不在线
    ERROR_DMS_DEVICE_AUTHCODE ,                  //设备授权码错误
    ERROR_DMS_DEVICE_SN_NO_EXIST ,               //设备序列号不存在
    ERROR_DMS_USER_NO_AUTH ,                     //您无权限访问该设备
   
    //DMS 错误码 设备端逻辑相关1200-1299
    ERROR_DMS_MORE_DEV_SAME_SN  =1200 ,                  //相同序列号的设备已存在
   
    //DMS 错误码 转发服务相关1300-1399
    ERROR_DMS_RESOURCE_NOT_ENOUGH = 1300,            //转发服务资源不足
    ERROR_DMS_NO_RS_SERVER_ONLINE ,                  //当前没有转发服务器在线
    
    //RS错误码,RS公用错误码
    ERROR_RS_MAX_CHANNEL_LIMITED = 2000,            //转发通道数已达到最大上限
   
    //RS错误码 客户端逻辑相关2100-2199
    ERROR_RS_CLIENT_DISCONNECT_TIMEOUT = 2100,       // 客户端与转发服务器连接超时
    ERROR_RS_CLIENT_RS_CHANNEL_FAILED,               // 转发通道未建立成功
    ERROR_RS_NOTIFY_DEVICE_DISCONNECT,              // 设备端与RS的连接已断开

    //RS错误码 设备端逻辑相关2200-2299
    ERROR_RS_DEV_DISCONNECT_TIMEOUT =2200,            // 客户端与转发服务器连接超时
    ERROR_RS_DEV_RS_CHANNEL_FAILED,                   // 转发通道未建立成功
    ERROR_RS_NOTIFY_CLIENT_DISCONNECT,               // 客户端与RS的连接已断开
    
    //RS错误码 与DMS逻辑相关2300-2399
    ERROR_RS_RESOURCEID_MAX_LIMITED = 2300,              //没有转发资源可分配

}net_error_enum;
#endif