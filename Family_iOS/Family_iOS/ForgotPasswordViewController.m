//
//  ForgotPasswordViewController.m
//  Family_iOS
//
//  Created by zhengyixiong on 15/9/29.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "ForgotPasswordViewController.h"
#import "NSString+Utils.h"
#import "MBProgressHUD.h"
#import "CMServerAccountProtocol.h"

@interface ForgotPasswordViewController ()
@property (weak, nonatomic) IBOutlet UITextField *accountTextField;
@property (weak, nonatomic) IBOutlet UITextField *verifitionCodeTextField;
@property (weak, nonatomic) IBOutlet UITextField *passwordTextField;

- (IBAction)getVerifitionCode:(UIButton *)sender;
- (IBAction)resetPassword:(UIButton *)sender;
- (IBAction)accountEditChanged:(UITextField *)sender;
- (IBAction)verifitionCodeEditChanged:(UITextField *)sender;
- (IBAction)passwordEditChanged:(UITextField *)sender;

@end

@implementation ForgotPasswordViewController

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

- (IBAction)getVerifitionCode:(UIButton *)sender {
    NSString *account = self.accountTextField.text;
    
    if ([account isEmptyOrNull]) {
        NSLog(@"account null");
        return;
    }else if (![account checkIsMachesRegex:CELL_PHONE_REG]) {
        NSLog(@"account not match");
        return;
    }
    
    MBProgressHUD* HUD = [MBProgressHUD showHUDAddedTo:self.view animated:YES];
    HUD.labelText = @"Get Code";
    
    [CMServerAccountProtocol request_reset_password_by_phoneByAccount:account WithSuccess:^(AFHTTPRequestOperation *operation, id responseObject) {
        [MBProgressHUD hideAllHUDsForView:self.view animated:NO];
        if ([[responseObject objectForKey:@"code"] intValue] == 0) {
            NSLog(@"send sms success");
        } else {
            NSLog(@"Ret Error:%@", responseObject);
        }
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        [MBProgressHUD hideAllHUDsForView:self.view animated:NO];
        NSLog(@"Error: %@", error);
    }];
}

- (IBAction)resetPassword:(UIButton *)sender {
    NSString *account = self.accountTextField.text;
    NSString *verifitionCode = self.verifitionCodeTextField.text;
    NSString *password = self.passwordTextField.text;
    
    if ([account isEmptyOrNull]) {
        NSLog(@"account null");
        return;
    } else if (![account checkIsMachesRegex:CELL_PHONE_REG]) {
        NSLog(@"account not match");
        return;
    } else if ([verifitionCode isEmptyOrNull]) {
        NSLog(@"verifitionCode null");
        return;
    } else if (![verifitionCode checkIsMachesRegex:VERIFY_CODE_REG]) {
        NSLog(@"verifitionCode not match");
        return;
    } else if ([password isEmptyOrNull]) {
        NSLog(@"password null");
        return;
    } else if (![password checkIsMachesRegex:PASSWORD_REG]) {
        NSLog(@"password not match");
        return;
    }
    
    MBProgressHUD* HUD = [MBProgressHUD showHUDAddedTo:self.view animated:YES];
    HUD.labelText = @"Create User";
    
    [CMServerAccountProtocol reset_passwordByAccount:account Password:password ResetCode:verifitionCode WithSuccess:^(AFHTTPRequestOperation *operation, id responseObject) {
        [MBProgressHUD hideAllHUDsForView:self.view animated:NO];
        if ([[responseObject objectForKey:@"code"] intValue] == 0) {
            NSLog(@"regist success");
        } else {
            NSLog(@"Ret Error:%@", responseObject);
        }
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        [MBProgressHUD hideAllHUDsForView:self.view animated:NO];
        NSLog(@"Error: %@", error);
    }];
}

- (IBAction)accountEditChanged:(UITextField *)sender {
    if (sender.text.length > CELL_PHONE_LENGTH) {
        sender.text = [sender.text substringToIndex:CELL_PHONE_LENGTH];
    }
}

- (IBAction)verifitionCodeEditChanged:(UITextField *)sender {
    if (sender.text.length > VERIFY_CODE_LENGTH) {
        sender.text = [sender.text substringToIndex:VERIFY_CODE_LENGTH];
    }
}

- (IBAction)passwordEditChanged:(UITextField *)sender {
    if (sender.text.length > PASSWORD_MAX_LENGTH) {
        sender.text = [sender.text substringToIndex:PASSWORD_MAX_LENGTH];
    }
}
@end
