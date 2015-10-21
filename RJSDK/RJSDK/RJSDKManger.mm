//
//  RJSDKManger.m
//  Family_iOS
//
//  Created by zxd on 15/10/17.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "RJSDKManger.h"
#include "CommonSDK.h"

#define MAX_JSON_STRINGL_LENGTH 200 * 20

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

- (void)addDevice:(NSString *) deviceList {
    assert(deviceList.length < MAX_JSON_STRINGL_LENGTH);
    commonSDK->AddDevToSDK([deviceList UTF8String], deviceList.length);
}

- (NSString *)getLocalDevicesList {
    char *devices = new char[MAX_JSON_STRINGL_LENGTH];
    int retLength = commonSDK->GetLocalDev(devices, MAX_JSON_STRINGL_LENGTH);
    if (retLength > 0) {
        devices[retLength] = '0';
        return [NSString stringWithUTF8String:devices];
    } else return nil;
}

- (int)getDeviceStatus:(NSString *) deviceSN {
    return commonSDK->GetDevNetStatus([deviceSN UTF8String]);
}

@end
