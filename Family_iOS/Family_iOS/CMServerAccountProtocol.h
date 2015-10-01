//
//  CMServerAccountProtocol.h
//  Family_iOS
//
//  Created by zhengyixiong on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AFNetWorking.h"
#import "AFURLRequestSerialization.h"
@interface CMServerAccountProtocol : NSObject

/**
 * 向服务器发送测试请求 用于获取SessionId
 */
+ (void)testWithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送登录请求
 * account  账号
 * password 密码
 * verifyCode 验证码
 */
+ (void)loginByAccount:(NSString *)account Password:(NSString *)password VerifyCode:(NSString *)verifyCode
           WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
               failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送请求注册短信验证码请求
 * cellphone 手机号码
 */
+ (void)request_register_smsByCellPhone:(NSString *)cellphone
                            WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送短信注册请求
 * account 账号
 * password 密码
 * smsCode 短信验证码
 */
+ (void)sms_registerByAccount:(NSString *)account Password:(NSString *)password Sms_code:(NSString *)smsCode
                  WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                      failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送注销请求
 * account 账号
 */
+ (void)logoutByAccount:(NSString *)account
            WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送修改密码请求
 *
 */
+ (void)modify_passwordByAccount:(NSString *)account OldPassword:(NSString *)oldPassword NewPassword:(NSString *)newPassword VerifyCode:(NSString *)verifyCode
                     WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                         failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送获取重置密码验证码请求
 *
 */
+ (void)request_reset_password_by_phoneByAccount:(NSString *)account
                                     WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                         failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送重置密码请求
 */
+ (void)reset_passwordByAccount:(NSString *)account Password:(NSString *)password ResetCode:(NSString *)resetCode
                    WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                        failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送获取修改手机号验证码请求
 */
+ (void)request_modify_cellphoneByAccount:(NSString *)account Password:(NSString *)password NewCellphone:(NSString *)newCellphone
                              WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                  failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送绑定新手机号请求
 */
+ (void)confirm_modify_cellphoneByAccount:(NSString *)account NewCellphone:(NSString *)newCellphone SmsCode:(NSString *)smsCode
                              WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                  failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送获取账户信息请求
 */
+ (void)get_account_infoByAccount:(NSString *)account
                      WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                          failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送更新账户信息请求
 */
+ (void)update_account_infoByAccount:(NSString *)account NickName:(NSString *)nickName Sex:(NSNumber *)sex Address:(NSString *)address Birthday:(NSString *)birthday
                         WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                             failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送绑定设备请求
 */
+ (void)add_deviceByAccount:(NSString *)account DeviceSN:(NSString *)deviceSN
                WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                    failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送解绑设备请求
 */
+ (void)del_deviceByAccount:(NSString *)account DeviceSN:(NSString *)deviceSN
                WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                    failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;

/**
 * 向CMS发送获取设备列表请求
 */
+ (void)get_device_listByAccount:(NSString *)account
                     WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                         failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure;


@end
