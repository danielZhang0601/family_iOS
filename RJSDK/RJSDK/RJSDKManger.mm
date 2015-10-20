//
//  RJSDKManger.m
//  Family_iOS
//
//  Created by zxd on 15/10/17.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "RJSDKManger.h"
#include "CommonSDK.h"

@implementation RJSDKManger{
    CommonSDK *commonSDK;
}

+ (RJSDKManger *)sharedManager
{
    static RJSDKManger* sharedManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedManager = [[RJSDKManger alloc] init];
    });
    return sharedManager;
}

- (id)init{
    if (self = [super init]) {
        commonSDK = new CommonSDK();
    }
    return self;
}

- (BOOL)startSDKServer {
    return commonSDK->Start();
}

- (void)stopSDKServer {
    return commonSDK->Stop();
}

@end
