//
//  MainTabBarViewController.m
//  Family_iOS
//
//  Created by Daniel.Zhang on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "MainTabBarViewController.h"

@interface MainTabBarViewController ()

@end

@implementation MainTabBarViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    self.navigationItem.hidesBackButton = YES;
    [self setDelegate:self];
    self.title = @"Camera List";
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (void)toAddCameraView {
    [self performSegueWithIdentifier:@"toAddCamera" sender:self];
}

- (void)tabBarController:(UITabBarController *)tabBarController didSelectViewController:(UIViewController *)viewController {
    if (viewController == self.viewControllers[0]) {
        self.title = @"Camera List";
        UIBarButtonItem *addCameraBtn = [[UIBarButtonItem alloc]initWithTitle:@"+" style:UIBarButtonSystemItemAdd target:self action:@selector(toAddCameraView)];
        self.navigationItem.rightBarButtonItem = addCameraBtn;
    } else if (viewController == self.viewControllers[1]) {
        self.title = @"Event List";
        self.navigationItem.rightBarButtonItem = nil;
    } else if (viewController == self.viewControllers[2]) {
        self.title = @"More Settings";
        self.navigationItem.rightBarButtonItem = nil;
    }
}

@end
