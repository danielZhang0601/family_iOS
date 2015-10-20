//
//  RJSDKManger.h
//  Family_iOS
//
//  Created by zxd on 15/10/17.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RJSDKManger : NSObject

+ (RJSDKManger *)sharedManager;

- (BOOL)startSDKServer;

- (void)stopSDKServer;

@end
