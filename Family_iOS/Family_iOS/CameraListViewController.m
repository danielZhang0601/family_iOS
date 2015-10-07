//
//  CameraListViewController.m
//  Family_iOS
//
//  Created by Daniel.Zhang on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "CameraListViewController.h"
#import "CameraListCell.h"

@interface CameraListViewController ()
@property (weak, nonatomic) IBOutlet UITableView *cameraTableView;

@end

@implementation CameraListViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    [self.cameraTableView setDelegate:self];
    [self.cameraTableView setDataSource:self];
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
