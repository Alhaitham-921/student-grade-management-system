from test_support import *

d=directory('menu-validation');run(d,FIRST+LOGOUT)
# All menu loops reject malformed choices and return to their parent.
invalid=['','x','-1','+1','1.5','99999999999999999999999','0','9'*120]
seq=ADMIN+invalid+['2']+invalid+['1']+invalid+['6','2']+invalid+['5','3']+invalid+['7','4','3']+invalid+['4','4']+invalid+['4','5']+invalid+['5']+invalid+['2','6','6','4']
out=run(d,seq);assert out.count('请输入')>30 and '系统正常结束' in out,out
assert '15. ' not in out and '基础信息管理' in out,out
# Invalid teacher identifiers/names and short password must never create partial users.
out=run(d,ADMIN+['2','2','1','','111','t0001','T0000','T00001','T0001','','123','张三1','张三','Math','short','teacher','1','5','4']+add_class('A')+LOGOUT)
assert '教师添加成功' in out and '工号必须' in out and '姓名必须' in out,out
before=business(d)
out=run(d,ADMIN+['2','1','1','abc','12a','-1','','00001','123','张三1','李华',':q','6','4']+LOGOUT)
assert business(d)==before,out
out=run(d,ADMIN+add_student('00001','  李华  ')+LOGOUT)
assert '学号: 00001 姓名: 李华' in out,out
out=run(d,['3','00001','00001','1','4','4']);assert '学号: 00001' in out,out
out=run(d,ADMIN+['2','1','1','00001',':q','6','4','6','4']);assert '该学号已存在' in out,out
print('PASS: menu hierarchy, strict numeric choices, primary keys, names, trimming, leading zeros, duplicates and cancellation')
# EOF at every stage must unwind safely without saving half a record.
for part in [[],['001'],['001','李华'],['001','李华','1'],['001','李华','1','1'],['001','李华','1','1','0']]:
    before=business(d)
    out=run(d,ADMIN+['2','1','1']+part)
    assert '系统正常结束' in out and business(d)==before,out
for part in [[],['T0002'],['T0002','王芳'],['T0002','王芳','English'],['T0002','王芳','English','teacher']]:
    before=business(d);out=run(d,ADMIN+['2','2','1']+part)
    assert '系统正常结束' in out and business(d)==before,out
print('PASS: EOF returns through all layers without partial teacher/student data')
# Seed into an existing legacy fixture: preserve original password/data and reuse class.
d=directory('demo');legacy_hashed(d)
p=d/'classes.dat';raw=bytearray(p.read_bytes());raw[4:64]='24级软件1班'.encode().ljust(60,b'\0');p.write_bytes(raw)
out=run(d,ADMIN+['5','4','1','6','1','6','4'])
assert '示例数据初始化成功' in out and '学生: 31 教师: 4 班级: 3' in out,out
before=business(d);out=run(d,ADMIN+['5','4','6','6','4']);assert '不重复创建' in out and business(d)==before,out
out=run(d,['2','T0002','Teacher123','5','1','11','3','240101001','240101001','1','4','4'])
assert '有成绩人数: 10' in out and '刘晨' in out,out
out=run(d,ADMIN+['3','2','1','2','4','6','4']);assert '有成绩人数: 10' in out,out
print('PASS: teacher migration, demo counts, class reuse, initial passwords, deterministic scores and seed idempotence')
# Any conflicting ID must abort the complete batch.
d=directory('demo-conflict');run(d,FIRST+add_teacher('T0002','Existing')+LOGOUT);before=business(d)
out=run(d,ADMIN+['5','4','6','6','4']);assert '初始化冲突' in out and business(d)==before,out
print('PASS: demo conflict protection preserves existing accounts')

# Existing edited demo grades/names survive another initialization request.
baseline_dir=directory('demo-edited');run(baseline_dir,FIRST+['5','4','1','6']+LOGOUT)
out=run(baseline_dir,ADMIN+edit_teacher('T0002','李老师')+['6','2','T0002','Teacher123','2','1','1','55','11','4'])
assert '教师姓名更新成功' in out and '成绩修改成功' in out,out
before=business(baseline_dir);out=run(baseline_dir,ADMIN+['5','4','6']+LOGOUT)
assert business(baseline_dir)==before,out
# Administrator may export/read school grades, but the menu exposes no import or mutation action.
file=baseline_dir/'admin-export.csv'
out=run(baseline_dir,ADMIN+['3','1','1','1','2','1','1','3','1','1',str(file),'4']+LOGOUT)
assert '成功导出 10 条成绩' in out and 'CSV批量导入' not in out,out
print('PASS: demo preserves edited passwords/names/grades and administrator read-only grade export')
# Unknown legacy teacher identity is migrated but cannot operate until a real name is provided.
d=directory('pending-name');legacy_hashed(d)
for name in ['users.dat','students.dat']:
    p=d/name;raw=p.read_bytes().replace(b'111',b'Old');p.write_bytes(raw)
out=run(d,['2','T0001','teacher','4'])
assert '教师姓名待完善' in out and '教师菜单' not in out,out
out=run(d,ADMIN+edit_teacher('T0001',"Anne-Marie O'Connor")+['6']+TEACHER+['11','4'])
assert '教师菜单' in out,out
print('PASS: pending legacy name blocks teacher access until administrator completes it')
# Typed text cancellation works at every field, and invalid long UTF-8/keys retry safely.
d=directory('cancel-fields');seed(d)
for partial in [[],['T0003'],['T0003','陈明'],['T0003','陈明','Math']]:
    before=business(d);out=run(d,ADMIN+['2','2','1']+partial+[':q','5','4']+LOGOUT);assert business(d)==before,out
for partial in [[],['00077'],['00077','陈明']]:
    before=business(d);out=run(d,ADMIN+['2','1','1']+partial+[':q','6','4']+LOGOUT);assert business(d)==before,out
out=run(d,ADMIN+['2','1','1','9'*20,'00077','张'*10,'陈明','1','1','0','1','6','4']+LOGOUT)
assert '输入过长' in out and '学生添加成功' in out,out
print('PASS: field cancellation, byte limits and complete-only commit')
