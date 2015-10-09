//
//  RJCodeHandler.m
//  Family_iOS
//
//  Created by zhengyixiong on 15/10/9.
//  Copyright © 2015年 zxd. All rights reserved.
//

/*
 [0,     99]	公用的错误码
 [100,   999]	CMS
 [1000,  1999]	DMS
 [2000,  2999]	RS
 [3000,  3999]	Device
*/
#import "RJCodeHandler.h"

typedef NS_ENUM(NSInteger, RJCode) {
    RJCodeSuccess,                  //操作成功
    RJCodeErrorUnknow,
    RJCodeErrorServerBusy,
    RJCodeErrorUnsupport,
    RJCodeErrorInvalidParameter,
    RJCodeErrorIllegalLink,
    RJCodeErrorServerDisconnection,
    
    RJCodeErrorInvalidAccount = 100,
    RJCodeErrorInvalidPassword,
    RJCodeErrorInvalidCellphone,
    RJCodeErrorInvalidEmail,
    RJCodeErrorInvalidVerifitionCode,
    RJCodeErrorInvalidDeviceSN,
    RJCodeErrorAccountOverMax,
    RJCodeErrorInvalidSex,
    RJCodeErrorSexOutOfRange,
    RJCodeErrorInvalidAddress,
    RJCodeErrorAddressOverMax,
    RJCodeErrorInvalidBirthday,
    
    RJCodeErrorEmailNotAuthed = 200,
    RJCodeErrorEmailCodeNotAuthed,
    RJCodeErrorSMSCodeNotAuthed,
    RJCodeErrorPictureCodeNotAuthed,
    RJCodeErrorAccessNotAuthed,
    
    RJCodeErrorAccountExist = 300,
    RJCodeErrorAccountNotExist,
    RJCodeErrorNotLoggedIn,
    RJCodeErrorWrongPassword,
    RJCodeErrorUsedCellphone,
    RJCodeErrorUsedEmail,
    RJCodeErrorSamePassword,
    RJCodeErrorSameCellphone,
    RJCodeErrorSameEmail,
    RJCodeErrorUsedCellphone2,
    RJCodeErrorUsedEmail2,
    
    RJCodeErrorUsedDevice = 400,
    RJCodeErrorDeviceNotExist,
    RJCodeErrorDeviceNotFound,
    RJCodeErrorDeviceExist
    
};

@implementation RJCodeHandler

+ (NSString *)handleCode:(int)returnCode {
    NSString *showMsg = @"";
    
    switch (returnCode) {
        case RJCodeSuccess:
            showMsg = @"成功";
            break;
            
        case RJCodeErrorServerBusy:
            showMsg = @"服务器繁忙";
            break;
        case RJCodeErrorUnsupport:
            showMsg = @"不支持";
            break;
        case RJCodeErrorInvalidParameter:
            showMsg = @"参数错误";
            break;
        case RJCodeErrorIllegalLink:
            showMsg = @"非法连接";
            break;
        case RJCodeErrorServerDisconnection:
            showMsg = @"服务器强制断开连接";
            break;
            
        case RJCodeErrorInvalidAccount:
            showMsg = @"用户名格式不正确";
            break;
        case RJCodeErrorInvalidPassword:
            showMsg = @"密码格式不正确";
            break;
        case RJCodeErrorInvalidCellphone:
            showMsg = @"手机号码格式不正确";
            break;
        case RJCodeErrorInvalidEmail:
            showMsg = @"邮箱格式不正确";
            break;
        case RJCodeErrorInvalidVerifitionCode:
            showMsg = @"授权码格式不正确";
            break;
        case RJCodeErrorInvalidDeviceSN:
            showMsg = @"设备序列号格式不正确";
            break;
        case RJCodeErrorAccountOverMax:
            showMsg = @"用户名最大不能超过20个字符";
            break;
        case RJCodeErrorInvalidSex:
            showMsg = @"您输入的性别格式不正确";
            break;
        case RJCodeErrorSexOutOfRange:
            showMsg = @"用户的性别只能为0或者1";
            break;
        case RJCodeErrorInvalidAddress:
            showMsg = @"地址格式不正确";
            break;
        case RJCodeErrorAddressOverMax:
            showMsg = @"地址不能超过128个字符";
            break;
        case RJCodeErrorInvalidBirthday:
            showMsg = @"生日格式不正确";
            break;

        case RJCodeErrorEmailNotAuthed:
            showMsg = @"邮箱未认证";
            break;
        case RJCodeErrorEmailCodeNotAuthed:
            showMsg = @"邮箱验证码未通过";
            break;
        case RJCodeErrorSMSCodeNotAuthed:
            showMsg = @"短信验证码未通过";
            break;
        case RJCodeErrorPictureCodeNotAuthed:
            showMsg = @"图片验证码未通过";
            break;
        case RJCodeErrorAccessNotAuthed:
            showMsg = @"权限认证未通过";
            break;

        case RJCodeErrorAccountExist:
            showMsg = @"用户名已存在";
            break;
        case RJCodeErrorAccountNotExist:
            showMsg = @"用户名不存在";
            break;
        case RJCodeErrorNotLoggedIn:
            showMsg = @"用户未登录";
            break;
        case RJCodeErrorWrongPassword:
            showMsg = @"密码错误";
            break;
        case RJCodeErrorUsedCellphone:
            showMsg = @"手机号已使用";
            break;
        case RJCodeErrorUsedEmail:
            showMsg = @"邮箱已使用";
            break;
        case RJCodeErrorSamePassword:
            showMsg = @"新密码与旧密码相同";
            break;
        case RJCodeErrorSameCellphone:
            showMsg = @"新手机号与旧手机号相同";
            break;
        case RJCodeErrorSameEmail:
            showMsg = @"新邮箱与旧邮箱相同";
            break;
        case RJCodeErrorUsedCellphone2:
            showMsg = @"手机号已使用";
            break;
        case RJCodeErrorUsedEmail2:
            showMsg = @"邮箱已使用";
            break;
            
        case RJCodeErrorUsedDevice:
            showMsg = @"设备已被其他用户添加";
            break;
        case RJCodeErrorDeviceNotExist:
            showMsg = @"设备不存在";
            break;
        case RJCodeErrorDeviceNotFound:
            showMsg = @"未找到设备";
            break;
        case RJCodeErrorDeviceExist:
            showMsg = @"设备已经存在";
            break;
            
        case RJCodeErrorUnknow:
        default:
            showMsg = @"未知错误";
            break;
    }
    
    return showMsg;
}

@end
