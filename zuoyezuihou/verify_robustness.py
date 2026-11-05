from test_support import *
import csv

d=directory('inputs')
out=run(d,['','x','+1','-1','1.5','1junk','1'*120,'0','5','4'])
assert '输入过长' in out and '请输入' in out and '系统正常结束' in out,out
out=run(d,['1','admin','wrong','admin','wrong','admin','wrong','1','4'])
assert '锁定一分钟' in out and '登录暂时锁定' in out,out
print('PASS: invalid/overlong/empty input, numeric formats, lockout and EOF')
d=directory('csv');seed(d,course='Math,"x"')
bad=d/'bad.csv';bad.write_text('001,Other,80\n999,Other,nan\n',encoding='utf-8-sig');before=business(d)
out=run(d,TEACHER+['3','1',str(bad),'11','4'])
assert '第1行' in out and '第2行' in out and '本次未导入任何成绩' in out and business(d)==before,out
file=d/'good.csv'
with file.open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['学号','姓名','科目','成绩']);w.writerow(['001','Same','Math,"x"','0']);w.writerow(['002','Same','Math,"x"','80'])
out=run(d,TEACHER+['3','1',str(file),'0','11','4'])
assert '已取消导入' in out and business(d)==before,out
out=run(d,TEACHER+['3','1',str(file),'1','5','1','6','1','4','1','2','7','1',str(d/'export.csv'),'11','4'])
assert '成功导入 2 条' in out and '平均分: 40.00' in out and '未录入人数: 0' in out and '成功导出 2 条' in out,out
assert (d/'export.csv').read_bytes().startswith(b'\xef\xbb\xbf')
with (d/'export.csv').open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
assert rows==[['stuNum','course','score'],['001','Math,"x"','0'],['002','Math,"x"','80']],rows
d2=directory('roundtrip');seed(d2,course='Math,"x"')
out=run(d2,TEACHER+['3','1',str(d/'export.csv'),'1','11','4']);assert '成功导入 2 条' in out,out
for row in ['001,Other,90','001,Math,nan','003,Math,80','001,Math,101','001,WrongName,"Math,""x""",50',',,,']:
    bad.write_text(row,encoding='utf-8');before=business(d2)
    out=run(d2,TEACHER+['3','1',str(bad),'11','4']);assert '本次未导入任何成绩' in out and business(d2)==before,out
out=run(d,TEACHER+['2','1','1','80','6','1','11','4'])
assert any('| 1 ' in x and '001' in x for x in out.splitlines()) and any('| 1 ' in x and '002' in x for x in out.splitlines()),out
log=(active(d)/'audit.dat').read_text(encoding='utf-8');assert 'before=0 after=80' in log and 'csv-import' in log
assert 'abcdef' not in log and 'p1$' not in log and '=teacher' not in log
print('PASS: CSV 3/4 columns, names, errors, preview, cancellation, BOM, quotes, roundtrip, zero, ties and audit')
d=directory('statistics');seed(d,count=5)
out=run(d,ADMIN+assign('003',1)+assign('004',1)+['6']+TEACHER+['1','1','001','80','1','1','002','80','1','1','003','0','1','2','005','100','5','1','6','1','4','1','2','11','4'])
assert '有成绩人数: 3 未录入人数: 1' in out and '平均分: 53.33' in out and '及格率: 66.67%' in out,out
assert any('| 3 ' in x and '003' in x for x in out.splitlines()),out
print('PASS: per-class 1,1,3 ranking, missing scores excluded and other class isolated')
d=directory('capacity');seq=list(FIRST)
for i in range(50):seq+=add_class(f'Class{i:02}')
seq+=['2','3','1','7','4','6','4']
out=run(d,seq);assert '班级数量已达上限' in out,out
seq=list(ADMIN)
for i in range(99):seq+=add_teacher(f'T{i+1:04}','TestTeacher','Math','secret')
seq+=['2','2','1','5','4','6','4']
out=run(d,seq);assert '用户数量已达上限' in out,out
print('PASS: class and shared-user capacity boundaries')
d=directory('empty-score');seed(d)
f=d/'spaces.csv';f.write_text('001,Math,   \n',encoding='utf-8');before=business(d)
out=run(d,TEACHER+['3','1',str(f),'11','4'])
assert '成绩必须为0到100的有限数字' in out and business(d)==before,out
print('PASS: whitespace-only CSV score cannot become a real zero')
