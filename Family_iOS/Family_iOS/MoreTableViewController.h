//
//  MoreTableViewController.h
//  Family_iOS
//
//  Created by zxd on 15/10/7.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MoreTableViewController : UITableViewController
@property (strong, nonatomic) IBOutlet UITableView *moreTableView;
- (IBAction)logOutBtnClick:(UIButton *)sender;

@end
