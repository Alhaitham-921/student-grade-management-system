import subprocess,tempfile,pathlib,struct,shutil
root=pathlib.Path(__file__).resolve().parent
exe=root/'x64/Debug/zuoyezuihou.exe'
def run(d,lines):
 p=subprocess.run([str(exe)],cwd=d,input='\n'.join(lines)+'\n',encoding='utf-8',capture_output=True,timeout=20)
 return p,p.stdout+p.stderr
with tempfile.TemporaryDirectory(dir=root) as d:
 lines=['1','admin','admin','admin','abcdef','abcdef','8','ClassA','2','T','teacher','Math','1','001','Same','1','0','1','002','Same','1','0','7','3','001','001','4','1','3','2','T','teacher','1','1','001','80','1','1','001','2','1','1','90','7','3','001','001','1','3','1','admin','abcdef','3','001','yes','7','3','002','002','1','3','4']
 p,out=run(d,lines)
 assert p.returncode==0,out
 assert '平均分: 90.0' in out,out
 assert '该课程已有成绩' in out,out
 assert '学生: Same (学号: 002)' in out,out
 raw=(pathlib.Path(d)/'users.dat').read_bytes()
 assert b'abcdef' not in raw and b'teacher' not in raw
 assert struct.unpack('<i',raw[:4])[0]==3
 p,out=run(d,['1','admin','abcdef','7','3','002','002','3','4'])
 assert p.returncode==0 and '[学生菜单]' in out,out
 print('PASS: same-name identity, duplicate course, average, delete isolation, password hash, restart login')
with tempfile.TemporaryDirectory(dir=root) as d:
 p,out=run(d,['1junk','1'*120,'4'])
 assert p.returncode==0 and '输入过长' in out and '请输入' in out,out
 print('PASS: numeric trailing junk and overlong input')
with tempfile.TemporaryDirectory(dir=root) as d:
 f=pathlib.Path(d)/'users.dat';f.write_bytes(struct.pack('<i',1000000));before=f.read_bytes()
 p,out=run(d,[])
 assert p.returncode!=0 and f.read_bytes()==before,out
 print('PASS: corrupt file rejected without overwrite')
with tempfile.TemporaryDirectory(dir=root) as d:
 for name in ('users.dat','students.dat'): shutil.copy(root/name,d)
 p,out=run(d,['3','1234','1234','1','3','4'])
 assert p.returncode==0 and '学号: 1234' in out,out
 print('PASS: supplied legacy data loads and migrates in memory')

