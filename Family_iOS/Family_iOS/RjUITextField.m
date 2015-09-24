//
//  RjUITextField.m
//  Family_iOS
//
//  Created by zhengyixiong on 15/9/24.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "RjUITextField.h"

@implementation RjUITextField


// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
    
    UIView *paddingView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 15, 0)];
    self.leftView = paddingView;
    self.leftViewMode = UITextFieldViewModeAlways;
}


@end
