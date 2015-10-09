//
//  MBProgressHUD+Add.m
//  Family_iOS
//
//  Created by zhengyixiong on 15/10/9.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "MBProgressHUD+Add.h"

@implementation MBProgressHUD(Add)

+ (void)showText:(NSString *)text inView:(UIView *)view {
    if (nil == view) {
        view = [UIApplication sharedApplication].keyWindow;
    }
    MBProgressHUD *hud = [MBProgressHUD showHUDAddedTo:view animated:YES];
    hud.labelText = text;
    hud.mode = MBProgressHUDModeText;
    hud.removeFromSuperViewOnHide = YES;
    [hud hide:YES afterDelay:1.0];
}

@end
