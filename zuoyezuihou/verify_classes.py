from test_support import *
d=directory('classes');seed(d)
out=run(d,ADMIN+['8','A','12','2','1','B','0','7','3','001','001','4','3','4'])
assert '班级已存在' in out and '班级: A（管理员分配）' in out,out
before=business(d)
out=run(d,ADMIN+['12','4','001','0','0','7','4'])
assert business(d)==before,out
out=run(d,ADMIN+['12','5','2','4','001','2','0','7','4'])
assert '班级已停用，不能分班' in out,out
out=run(d,ADMIN+['12','5','2','4','001','2','2','2','NewB','0','7','3','001','001','1','3','4'])
assert '班级: NewB' in out,out
out=run(d,ADMIN+['13','2','T','NewTeacher','13','1','002','新姓名','14','新姓','1','7','2','T','teacher','1','1','002','88','7','4'])
assert '新姓名' in out and 'NewTeacher' in out and '成绩添加完成' in out,out
out=run(d,ADMIN+['13','3','002','2','0','7','2','T','teacher','1','1','002','7','4'])
assert '无权添加成绩' in out,out
out=run(d,ADMIN+['9','7','4'])
assert 'NewB' in out,out
log=(active(d)/'audit.dat').read_text(encoding='utf-8')
assert 'class-transfer' in log and 'from=A to=B' in log and 'teacher-assignment' in log
print('PASS: admin-only membership, cancellation, inactive class, rename, transfer, display names and permission edits')
