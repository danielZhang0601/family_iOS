//
//  NSString+Utils.m
//  Family_iOS
//
//  Created by zxd on 15/10/7.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "NSString+Utils.h"

@implementation NSString (Utils)

- (BOOL)isEmptyOrNull {
    if (!self) {
        return YES;
    }else if ([self isEqualToString:@""] || self.length == 0) {
        return YES;
    }else {
        return NO;
    }
}

- (BOOL)checkIsMachesRegex:(NSString *)regStr {
    if ([self isEmptyOrNull]) {
        return NO;  //tmp
    }
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:regStr options:NSRegularExpressionCaseInsensitive error:nil];
    if ([regex firstMatchInString:self options:0 range:NSMakeRange(0, self.length)]) {
        return YES;
    }else {
        return NO;
    }
}

@end
