"""Self-contained black-box fixtures; artifacts retained for diagnosis."""
from pathlib import Path
import os, subprocess, tempfile, struct
ROOT=Path(__file__).resolve().parent
EXE=Path(os.environ.get('SGMS_EXE', ROOT/'x64/Debug/zuoyezuihou.exe')).resolve()
RESULTS=Path(os.environ.get('SGMS_RESULTS', ROOT.parent/'test-results')).resolve()
RESULTS.mkdir(parents=True,exist_ok=True)
def directory(label):
    return Path(tempfile.mkdtemp(prefix=label+'-',dir=RESULTS))
def run(d, seq, *, env=None, expected=0, exe=None):
    p=subprocess.run([str(exe or EXE)],cwd=d,input='\n'.join(map(str,seq))+'\n',encoding='utf-8',capture_output=True,timeout=30,env=dict(os.environ,**(env or {})))
    out=p.stdout+p.stderr
    index=len(list(d.glob('run-*.txt')))
    (d/f'run-{index:03}.txt').write_text(f'exit={p.returncode}\n'+out,encoding='utf-8')
    assert p.returncode==expected,out
    return out
FIRST=['1','admin','admin','admin','abcdef','abcdef']
ADMIN=['1','admin','abcdef']
TEACHER=['2','T0001','teacher']
LOGOUT=['6','4']
def add_class(name):return ['2','3','1',name,'7','4']
def add_teacher(id='T0001',name='张三',course='Math',password='teacher'):
    return ['2','2','1',id,name,course,password,'1','5','4']
def add_student(id,name='Same',c=1,teachers=('1',)):
    return ['2','1','1',id,name,str(c),*teachers,'0','1','6','4']
def assign(id,c):return ['2','3','5',id,str(c),'7','4']
def rename_class(c,name):return ['2','3','4',str(c),name,'7','4']
def toggle_class(c):return ['2','3','6',str(c),'7','4']
def edit_teacher(id,name):return ['2','2','3',id,name,'1','5','4']
def edit_student(id,name):return ['2','1','3',id,'1',name,'1','6','4']
def edit_teaching(id,teachers):return ['2','2','4',id,*map(str,teachers),'0','1','5','4']
def delete_student(id):return ['2','1','4',id,'yes','6','4']
def seed(d, *, course='Math', count=3):
    seq=FIRST+add_class('A')+add_class('B')+add_teacher(course=course)+add_teacher('T0002','王芳','English','secret')
    for i in range(1,count+1):seq+=add_student(f'{i:03}','Same' if i<3 else 'Alice王',1 if i<3 else 2)
    return run(d,seq+LOGOUT)
def marker(d):return (d/'.sgms/current.dat').read_bytes()
def active(d):
    raw=marker(d); n=struct.unpack_from('<I',raw,12)[0]
    return d/'.sgms'/raw[16:16+n].decode('ascii')
def business(d):
    folder=active(d)
    return tuple((folder/name).read_bytes() for name in ['users.dat','students.dat','classes.dat','metadata.dat'])
def legacy(d):
    def text(s,n):return s.encode('gbk').ljust(n,b'\0')
    u=text('admin',20)+text('admin',20)+struct.pack('<i',0)+text('',30)+b'\0'*2
    u+=text('张三',20)+text('1234',20)+struct.pack('<i',2)+text('',30)+b'\0'*2
    (d/'users.dat').write_bytes(struct.pack('<i',2)+u)
    student=text('1234',20)+text('张三',20)+b'\0'*(36*10+20*10)+struct.pack('<iif',0,0,0)
    (d/'students.dat').write_bytes(struct.pack('<i',1)+student)

def legacy_hashed(d):
    import hashlib
    def text(s,n):return s.encode('utf-8').ljust(n,b'\0')
    def user(login,password,role,course=''):
        salt=bytes(range(16))
        digest=hashlib.pbkdf2_hmac('sha256',password.encode(),salt,100000)
        encoded='p1$'+salt.hex()+digest.hex()
        return text(login,20)+text(encoded,128)+struct.pack('<i',role)+text(course,30)+b'\0'*2
    records=user('admin','abcdef',0)+user('111','teacher',1,'Math')+user('001','001',2)
    (d/'users.dat').write_bytes(struct.pack('<i',3)+records)
    st=text('001',20)+text('中文名',20)+b'\0'*(36*10)+text('111',20)+b'\0'*(20*9)+struct.pack('<iif',1,0,0)
    (d/'students.dat').write_bytes(struct.pack('<i',1)+st)
    school=text('A',60)+text('001',20)+b'\0'*(20*99)+struct.pack('<i',1)
    (d/'classes.dat').write_bytes(struct.pack('<i',1)+school)
