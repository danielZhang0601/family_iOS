//
//  NSString+Utils.h
//  Family_iOS
//
//  Created by zxd on 15/10/7.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import <Foundation/Foundation.h>

#define CELL_PHONE_REG @"^1[3|4|5|8](\\d){9}$"
#define VERIFY_CODE_REG @"^(\\d){6}$"
#define PASSWORD_REG @"^[a-zA-Z0-9]{8,20}$"

#define CELL_PHONE_LENGTH 11
#define VERIFY_CODE_LENGTH 6
#define PASSWORD_MIN_LENGTH 8
#define PASSWORD_MAX_LENGTH 20

@interface NSString (Utils)

- (BOOL)isEmptyOrNull;

- (BOOL)checkIsMachesRegex:(NSString *)regStr;

@end
