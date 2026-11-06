from test_support import *
d=directory('login');seed(d)
out=run(d,[' \t3  ','001','001','4','4'])
assert '学生菜单' in out and '请输入' not in out,out
out=run(d,['1','q','2','Q','3',':q','4'])
assert '用户类型错误' not in out and '锁定一分钟' not in out,out
out=run(d,['3','001',':q','3','001','001','1','4','4'])
assert '学生菜单' in out and '用户类型错误' not in out,out
out=run(d,['3','001','Student123','q','3','001','001','4','4'])
assert '用户类型错误' in out and '初始密码与学号相同' in out and '学生菜单' in out,out
out=run(d,ADMIN+add_teacher('T0003','陈明','Physics','TopSecret234')+LOGOUT)
assert 'TopSecret234' not in out,out
out=run(d,['2','T0003','TopSecret234','11','4']);assert '教师菜单' in out and 'TopSecret234' not in out,out
for seq in [['3'],['3','001'],['3','001',':q']]:
    out=run(d,seq);assert '系统正常结束' in out,out
print('PASS: spaced menu choice, q/Q/:q cancellation, no failed attempt on cancel, student password hint, passwords absent from output, EOF')
