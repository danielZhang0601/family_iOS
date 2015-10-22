//
//  CameraListViewController.m
//  Family_iOS
//
//  Created by Daniel.Zhang on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "CameraTableViewController.h"
#import "CameraListCell.h"

@interface CameraTableViewController ()

@property (weak, nonatomic) IBOutlet UITableView *cameraTableView;

@end

@implementation CameraTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)viewWillAppear:(BOOL)animated {
    self.tabBarController.title = @"Camera List";
    self.tabBarController.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(toLocalCameraList)];
}

- (void)toLocalCameraList {
    [self performSegueWithIdentifier:@"toLocalCameraList" sender:self];
}

- (void)viewWillDisappear:(BOOL)animated {
    self.tabBarController.navigationItem.rightBarButtonItem = nil;
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

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 10;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *cellIdentifier = @"CameraCell";
    CameraListCell *cell = [tableView dequeueReusableCellWithIdentifier:cellIdentifier];
    return cell;
}

@end
