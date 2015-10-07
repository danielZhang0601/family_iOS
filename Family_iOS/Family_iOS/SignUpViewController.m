//
//  SignUpViewController.m
//  Family_iOS
//
//  Created by Daniel.Zhang on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "SignUpViewController.h"
#import "NSString+Utils.h"

@interface SignUpViewController ()
@property (weak, nonatomic) IBOutlet UITextField *accountTextField;
@property (weak, nonatomic) IBOutlet UITextField *verifitionCodeTextField;
@property (weak, nonatomic) IBOutlet UITextField *passwordTextField;


- (IBAction)getVerifitionCode:(id)sender;
- (IBAction)createNewAccount:(id)sender;
- (IBAction)accountEditChanged:(UITextField *)sender;
- (IBAction)verifitionEditChanged:(UITextField *)sender;
- (IBAction)passwordEditChanged:(UITextField *)sender;


@end

@implementation SignUpViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
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

- (IBAction)getVerifitionCode:(id)sender {
    
}

- (IBAction)createNewAccount:(id)sender {
}

- (IBAction)accountEditChanged:(UITextField *)sender {
    if (sender.text.length > CELL_PHONE_LENGTH) {
        sender.text = [sender.text substringToIndex:CELL_PHONE_LENGTH];
    }
}

- (IBAction)verifitionEditChanged:(UITextField *)sender {
    if (sender.text.length > VERIFY_CODE_LENGTH) {
        sender.text = [sender.text substringToIndex:VERIFY_CODE_LENGTH];
    }
}

- (IBAction)passwordEditChanged:(UITextField *)sender {
    if (sender.text.length > PASSWORD_MAX_LENGTH) {
        sender.text = [sender.text substringToIndex:PASSWORD_MAX_LENGTH];
    }else if (sender.text.length < PASSWORD_MIN_LENGTH) {
        
    }
}


@end
