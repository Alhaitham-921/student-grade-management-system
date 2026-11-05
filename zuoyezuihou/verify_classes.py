from test_support import *
d=directory('classes');seed(d)
out=run(d,ADMIN+add_class('A')+rename_class(1,'B')+['6','3','001','001','3','4','4'])
assert '班级已存在' in out and '班级: A（管理员分配）' in out,out
before=business(d)
out=run(d,ADMIN+assign('001',3)+LOGOUT);assert business(d)==before,out
out=run(d,ADMIN+toggle_class(2)+assign('001',2)+LOGOUT)
assert '班级已停用，不能分班' in out,out
out=run(d,ADMIN+toggle_class(2)+assign('001',2)+rename_class(2,'NewB')+['6','3','001','001','1','4','4'])
assert '班级: NewB' in out,out
out=run(d,ADMIN+edit_teacher('T0001','NewTeacher')+edit_student('002','新姓名')+['2','1','5','新姓','1','6','4','6']+TEACHER+['1','1','002','88','11','4'])
assert '新姓名' in out and '成绩添加完成' in out,out
out=run(d,ADMIN+edit_teaching('002',[2])+['6']+TEACHER+['1','1','002','11','4'])
assert '无权添加成绩' in out,out
log=(active(d)/'audit.dat').read_text(encoding='utf-8')
assert 'from=A to=B' in log and 'teacher-assignment' in log
print('PASS: admin-only membership, cancellation, inactive class, rename, transfer, names and permission edits')
