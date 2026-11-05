from test_support import *
d=directory('identity')
seed(d)
out=run(d,TEACHER+['1','1','001','80','1','1','001','2','1','1','90','7','3','001','001','1','3','4'])
assert '该课程已有成绩' in out and '平均分: 90.0' in out,out
raw=(active(d)/'users.dat').read_bytes()
assert b'abcdef' not in raw and b'teacher' not in raw
out=run(d,ADMIN+['3','001','yes','7','3','002','002','1','3','4'])
assert '学生: Same (学号: 002)' in out,out
out=run(d,['3','001','001','001','001','001','001','4'])
assert '[学生菜单]' not in out,out
print('PASS: identity, duplicate score, average, deletion isolation, password hashing and restart')
d=directory('legacy');legacy(d)
original={p.name:p.read_bytes() for p in d.glob('*.dat')}
out=run(d,['3','1234','1234','1','3','4'])
assert '张三' in out and '学号: 1234' in out,out
assert original=={p.name:p.read_bytes() for p in d.glob('*.dat')}
assert len(list(d.glob('legacy-backup-*')))==1
out=run(d,['3','1234','1234','1','3','4'])
assert '[学生菜单]' in out,out
print('PASS: generated GBK legacy migration, complete backup, original preservation, restart')
d=directory('clear');seed(d)
out=run(d,ADMIN+['6','confirm','9','7','3','002','002','002','002','002','002','4'])
assert '所有用户数据已清空' in out and '[学生菜单]' not in out,out
print('PASS: clear isolated fixture preserves administrator')

d=directory('hashed-legacy');legacy_hashed(d)
original={p.name:p.read_bytes() for p in d.glob('*.dat')}
out=run(d,TEACHER+['1','1','001','88','7','3','001','001','1','3','4'])
assert '中文名' in out and '平均分: 88.0' in out and original=={p.name:p.read_bytes() for p in d.glob('*.dat')},out
print('PASS: generated hashed UTF-8 DAT upgrade with teacher permission and class membership')
