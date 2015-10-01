//
//  SignInViewController.m
//  Family_iOS
//
//  Created by zhengyixiong on 15/9/24.
//  Copyright © 2015年 zxd. All rights reserved.
//

#import "SignInViewController.h"
#import "CMServerAccountProtocol.h"
@interface SignInViewController ()
@property (weak, nonatomic) IBOutlet UITextField *accountTextField;
@property (weak, nonatomic) IBOutlet UITextField *passwordTextField;
@property (weak, nonatomic) IBOutlet UIButton *signinBtn;




- (IBAction)signInBtnClick:(UIButton *)sender;
- (IBAction)accountEditChange:(UITextField *)sender;
- (IBAction)passwordEditChange:(UITextField *)sender;



@end

@implementation SignInViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    [CMServerAccountProtocol testWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject) {
        NSLog(@"Response: %@", [[NSString alloc] initWithData:responseObject encoding:NSUTF8StringEncoding]);
        
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        NSLog(@"Error: %@", error);
    }];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)viewWillAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self.navigationController setNavigationBarHidden:YES];
    self.title = @"Sign In";
    [self registerForNotifications];
}

-(void)viewWillDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    [self.navigationController setNavigationBarHidden:NO];
    [self unregisterForNotifications];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/


/**
 *  键盘消息注册
 */
- (void)registerForNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow) name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide) name:UIKeyboardWillHideNotification object:nil];
    
}

//取消键盘注册
- (void)unregisterForNotifications
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillShowNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillHideNotification
                                                  object:nil];
}

//键盘弹起时 整体向上移动
- (void)keyboardWillHide
{
    if (self.view.frame.origin.y>=0)
    {
        return;
    }
    [UIView animateWithDuration:0.25 animations:^{
        self.view.frame = CGRectMake(0, self.view.frame.origin.y+100, self.view.frame.size.width, self.view.frame.size.height);
    } completion:^(BOOL finished) {
        
    }];
}

//键盘隐藏时 整体向下移动
- (void)keyboardWillShow
{
    if (self.view.frame.origin.y<0)
    {
        return;
    }
    [UIView animateWithDuration:0.25 animations:^{
        self.view.frame = CGRectMake(0, self.view.frame.origin.y-100, self.view.frame.size.width, self.view.frame.size.height);
    } completion:^(BOOL finished) {
        
    }];
}

//当点击ROOT view的时候 让输入框失去焦点 键盘会消失
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.accountTextField resignFirstResponder];
    [self.passwordTextField resignFirstResponder];
}

//输入框回车事件 只有密码框会有
- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    [self signInBtnClick:_signinBtn];
    return YES;
}

//使用performSegueWithIdentifier来控制storyboard的条件跳转
- (IBAction)signInBtnClick:(UIButton *)sender {
    NSString *account = _accountTextField.text;
    NSString *password = _passwordTextField.text;
    NSLog(@"Account:%@,Password:%@",account,password);
    [CMServerAccountProtocol loginByAccount:account Password:password VerifyCode:@"123456" WithSuccess:^(AFHTTPRequestOperation *operation, id responseObject) {
        if ([[responseObject objectForKey:@"code"] intValue] == 0) {
            [self performSegueWithIdentifier:@"toMain" sender:self];
        } else {
            NSLog(@"Return:%@",responseObject);
        }
    } failure:^(AFHTTPRequestOperation *operation, NSError *error) {
        NSLog(@"Error: %@", error);
    }];
//    [self performSegueWithIdentifier:@"toMain" sender:self];
}

//编辑框输入改变事件 限制账号输入框最大输入长度为11  手机号
- (IBAction)accountEditChange:(UITextField *)sender {
    if (sender.text.length > 11) {
        sender.text = [sender.text substringToIndex:11];
    }
}

//编辑框输入改变事件 限制密码输入框最大输入长度为20 密码正则校验 8-20位
- (IBAction)passwordEditChange:(UITextField *)sender {
    if (sender.text.length > 20) {
        sender.text = [sender.text substringToIndex:20];
    }
}
@end
