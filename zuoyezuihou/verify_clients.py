from test_support import *
import csv
# Reproduce screenshot: retained teacher has a course but no assigned students.
d=directory('unassigned-clients')
run(d,FIRST+add_teacher('T0001','张三','111')+['5','4','1','6']+LOGOUT)
before=business(d)
out=run(d,TEACHER+list(map(str,range(1,10)))+['11','4'])
assert '尚未分配任何学生' in out and '维护学生授课关系' in out,out
assert '学生学号 (' not in out and '导出CSV路径' not in out and 'CSV路径 (' not in out,out
assert '240101001' not in out and business(d)==before,out
print('PASS: unassigned teacher gets actionable explanation, no empty write prompts or unauthorized data')
# Directory export, Unicode/spaces, path retry and no overwrite.
d=directory('export-clients');seed(d)
run(d,TEACHER+['1','1','001','80','11','4'])
folder=d/'导出 有空格';folder.mkdir()
out=run(d,TEACHER+['7','1',str(folder),'11','4']);files=list(folder.glob('*.csv'))
assert len(files)==1 and '成功导出 1 条' in out,out
with files[0].open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
assert rows[1]==['001','Math','80'],rows
old=files[0].read_bytes();new=d/'retry.csv'
out=run(d,TEACHER+['7','1',str(files[0]),str(d/'missing'/'new.csv'),str(new),'11','4'])
assert '已存在' in out and '父文件夹不存在' in out and new.exists() and files[0].read_bytes()==old,out
empty=d/'should-not-exist.csv'
out=run(d,TEACHER+['7','2','11','4'])
assert '不创建空文件' in out and not empty.exists(),out
print('PASS: export to directory, UTF8 path, retries, no overwrites and no empty exports')
# Students see only their own data and absent grades differ from real zero.
out=run(d,['3','002','002','1','3','4','4'])
assert '平均分: --' in out and '001' not in out and '班级: A' in out,out
out=run(d,['3','001','001','1','4','4']);assert '平均分: 80.0' in out,out
out=run(d,['3','001','001','2','wrong','2',':q','2','001','NewPass123','NewPass123','4','3','001','NewPass123','1','4','4'])
assert '当前密码不正确' in out and '密码修改成功' in out and '平均分: 80.0' in out,out
print('PASS: student identity isolation, missing vs zero, class view, wrong/cancelled password and changed-password restart')

fault=os.environ.get('SGMS_FAULT_EXE');assert fault,'Use build-test.ps1 for native console verification'
d=directory('console-password')
out=run(d,[],env={'SGMS_CONSOLE_PASSWORD_TEST':'1'},exe=fault)
assert 'real Win32 console shows stars' in out,out
print(out.strip())
