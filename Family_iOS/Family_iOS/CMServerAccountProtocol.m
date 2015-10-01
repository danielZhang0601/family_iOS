//
//  CMServerAccountProtocol.m
//  Family_iOS
//
//  Created by zhengyixiong on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "CMServerAccountProtocol.h"


//全局变量
NSString *BASE_URL = @"http://120.25.250.154";
NSString *API_ACCOUNT;
NSString *UTILS_HELLO;
AFHTTPRequestOperationManager *manager;

@implementation CMServerAccountProtocol

+ (void)initialize {
    API_ACCOUNT = [[NSString alloc]initWithFormat:@"%@/api/account", BASE_URL];
    UTILS_HELLO = [[NSString alloc]initWithFormat:@"%@/hello.html", BASE_URL];
    manager = [AFHTTPRequestOperationManager manager];
    manager.requestSerializer = [AFJSONRequestSerializer serializer];
    manager.responseSerializer = [AFJSONResponseSerializer serializer];
    manager.responseSerializer.acceptableContentTypes = [NSSet setWithObjects:@"text/html",@"text/json",@"application/json",nil];
}



+ (void)testWithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSURLRequest *request = [[NSURLRequest alloc]initWithURL:[NSURL URLWithString:UTILS_HELLO]];
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc]initWithRequest:request];
    [operation setCompletionBlockWithSuccess:success failure:failure];
    [operation start];
}


+ (void)loginByAccount:(NSString *)account Password:(NSString *)password VerifyCode:(NSString *)verifyCode
           WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
               failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"login",@"data":@{@"account":account,@"password":password,@"verify_code":verifyCode}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)request_register_smsByCellPhone:(NSString *)cellphone
                            WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"request_register_sms",@"data":@{@"cellphone":cellphone}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)sms_registerByAccount:(NSString *)account Password:(NSString *)password Sms_code:(NSString *)smsCode
                  WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                      failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"sms_register",@"data":@{@"account":account,@"password":password,@"sms_code":smsCode}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)logoutByAccount:(NSString *)account
            WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"logout",@"data":@{@"account":account}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)modify_passwordByAccount:(NSString *)account OldPassword:(NSString *)oldPassword NewPassword:(NSString *)newPassword VerifyCode:(NSString *)verifyCode
                     WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                         failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"modify_password",@"data":@{@"account":account,@"old_password":oldPassword,@"new_password":newPassword,@"verify_code":verifyCode}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)request_reset_password_by_phoneByAccount:(NSString *)account
                                     WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                         failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"request_reset_password_by_phone",@"data":@{@"account":account}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)reset_passwordByAccount:(NSString *)account Password:(NSString *)password ResetCode:(NSString *)resetCode
                    WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                        failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"reset_password",@"data":@{@"account":account,@"new_password":password,@"reset_code":resetCode}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)request_modify_cellphoneByAccount:(NSString *)account Password:(NSString *)password NewCellphone:(NSString *)newCellphone
                              WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                  failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"request_modify_cellphone",@"data":@{@"account":account,@"password":password,@"new_cellphone":newCellphone}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)confirm_modify_cellphoneByAccount:(NSString *)account NewCellphone:(NSString *)newCellphone SmsCode:(NSString *)smsCode
                              WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                                  failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"confirm_modify_cellphone",@"data":@{@"account":account,@"new_cellphone":newCellphone,@"sms_code":smsCode}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)get_account_infoByAccount:(NSString *)account
                      WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                          failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"get_account_info",@"data":@{@"account":account}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)update_account_infoByAccount:(NSString *)account NickName:(NSString *)nickName Sex:(NSNumber *)sex Address:(NSString *)address Birthday:(NSString *)birthday
                         WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                             failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"update_account_info",@"data":@{@"account":account,@"nick_name":nickName,@"sex":sex,@"address":address,@"birthday":birthday}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)add_deviceByAccount:(NSString *)account DeviceSN:(NSString *)deviceSN
                WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                    failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"add_device",@"data":@{@"account":account,@"device_sn":deviceSN}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)del_deviceByAccount:(NSString *)account DeviceSN:(NSString *)deviceSN
                WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                    failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"del_device",@"data":@{@"account":account,@"device_sn":deviceSN}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

+ (void)get_device_listByAccount:(NSString *)account
                     WithSuccess:(void (^)(AFHTTPRequestOperation *operation, id responseObject))success
                         failure:(void (^)(AFHTTPRequestOperation *operation, NSError *error))failure {
    NSDictionary *parameters = @{@"cmd":@"get_device_list",@"data":@{@"account":account}};
    
    [manager POST:API_ACCOUNT parameters:parameters success:success failure:failure];
}

@end
