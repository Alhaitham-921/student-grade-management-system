from test_support import *
import shutil, time

d=directory('corrupt-legacy')
f=d/'users.dat';f.write_bytes(struct.pack('<i',1000000));before=f.read_bytes()
out=run(d,[],expected=1);assert f.read_bytes()==before and '损坏' in out,out
print('PASS: corrupt legacy file rejected without overwrite')

d=directory('snapshot');seed(d)
# Incomplete generations and abandoned pending markers never become visible.
(d/'.sgms'/'orphan').mkdir();(d/'.sgms'/'orphan'/'users.dat').write_bytes(b'incomplete')
(d/'.sgms'/'pending.tmp').write_bytes(b'partial marker')
out=run(d,['3','001','001','1','3','4']);assert '班级: A' in out,out
# Corrupt the authoritative snapshot; do not silently fall back to old legacy files.
p=active(d)/'students.dat';raw=p.read_bytes();p.write_bytes(raw+b'junk');m=marker(d)
out=run(d,[],expected=1);assert '数据集损坏' in out and marker(d)==m,out
print('PASS: uncommitted sets ignored, checksum corruption rejected, no mixed-version fallback')

d=directory('lock');seed(d)
p=subprocess.Popen([str(EXE)],cwd=d,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,encoding='utf-8')
try:
    # Initialisation is complete when the first menu prompt is flushed.
    seen=''
    while '选择:' not in seen:
        ch=p.stdout.read(1);assert ch,'first instance exited';seen+=ch
    out=run(d,['4'],expected=1);assert '已有实例' in out,out
    p.stdin.write('4\n');p.stdin.flush();p.communicate(timeout=10);assert p.returncode==0
finally:
    if p.poll() is None:p.kill();p.communicate()
run(d,['4'])
print('PASS: single writer rejects second instance and releases lock on exit')

fault_exe=os.environ.get('SGMS_FAULT_EXE')
assert fault_exe,'Environment: use build-test.ps1 to build the SGMS_TESTING executable'
for fault,code in [('before-marker',0),('crash-before-marker',86)]:
    d=directory(fault);seed(d);before=business(d)
    out=run(d,ADMIN+['8','ShouldNotExist','9','7','4'],env={'SGMS_FAULT':fault,'SGMS_FAULT_AFTER':'2'},expected=code,exe=fault_exe)
    if code==0:assert '已恢复上次状态' in out and '班级添加成功' not in out,out
    assert business(d)==before
    out=run(d,ADMIN+['9','7','4']);assert 'ShouldNotExist' not in out,out
print('PASS: failed commit rolls memory back; process crash before marker preserves whole previous dataset')
# Shipping executable ignores test-only fault variables.
d=directory('production');run(d,['4'],env={'SGMS_FAULT':'crash-before-marker'})
print('PASS: production executable has no environment-triggered fault injection')

d=directory('after-marker');seed(d)
run(d,ADMIN+['8','CommittedClass'],env={'SGMS_FAULT':'crash-after-marker','SGMS_FAULT_AFTER':'2'},expected=87,exe=fault_exe)
out=run(d,ADMIN+['9','7','4']);assert 'CommittedClass' in out,out
print('PASS: crash after atomic marker preserves the entire new dataset')
d=directory('score-rollback');seed(d);before=business(d)
out=run(d,TEACHER+['1','1','001','80','7','4'],env={'SGMS_FAULT':'before-marker','SGMS_FAULT_AFTER':'2'},exe=fault_exe)
assert '成绩添加完成' not in out and business(d)==before,out
print('PASS: score failure restores student arrays as well as metadata')
