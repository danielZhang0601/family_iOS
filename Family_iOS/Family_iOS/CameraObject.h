//
//  CameraObject.h
//  Family_iOS
//
//  Created by zhengyixiong on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface CameraObject : NSObject

/*
 *  相机图片
 */
@property (nonatomic, copy) NSString *image;

/*
 *  相机名称
 */
@property (nonatomic, copy) NSString *name;

/*
 *  所有者
 */
@property (nonatomic, copy) NSString *owner;

- (instancetype)initWithDict: (NSDictionary *) dict;

+ (instancetype)cameraObjectWithDict: (NSDictionary *) dict;

@end
