//
//  MoreTableViewController.m
//  Family_iOS
//
//  Created by zxd on 15/10/7.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "MoreTableViewController.h"
#import "CMServerAccountProtocol.h"
#import "AppDelegate.h"

@implementation MoreTableViewController

- (void)viewDidLoad {
    [self.tableView setDelegate:self];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    if (indexPath.row == 0) {
        [self performSegueWithIdentifier:@"toModifyPassword" sender:self];
    } else if (indexPath.row == 1) {
        
    } else if (indexPath.row == 2) {
        [self performSegueWithIdentifier:@"toHelp" sender:self];
    }
}

- (IBAction)logOutBtnClick:(UIButton *)sender {
    AppDelegate *appDelegate = [[UIApplication sharedApplication] delegate];
    [CMServerAccountProtocol logoutByAccount:appDelegate.accountObject.account WithSuccess:^(AFHTTPRequestOperation *operation, id responseObject) {
        NSLog(@"Logout success");
        [self.navigationController popViewControllerAnimated:YES];
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        NSLog(@"Logout fail,but still close");
//        [self performSegueWithIdentifier:@"toSignIn" sender:self];
    }];
}
@end
