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
TEACHER=['2','T','teacher']
def assign(id,c):return ['12','4',id,str(c),'0']
def seed(d, *, course='Math', count=3):
    seq=FIRST+['8','A','8','B','2','T','teacher',course,'2','Other','secret','English']
    for i in range(1,count+1):seq+=['1',f'{i:03}','Same' if i<3 else 'Alice王','1','0']
    for i in range(1,count+1):seq+=assign(f'{i:03}',1 if i<3 else 2)
    return run(d,seq+['7','4'])
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
    records=user('admin','abcdef',0)+user('T','teacher',1,'Math')+user('001','001',2)
    (d/'users.dat').write_bytes(struct.pack('<i',3)+records)
    st=text('001',20)+text('中文名',20)+b'\0'*(36*10)+text('T',20)+b'\0'*(20*9)+struct.pack('<iif',1,0,0)
    (d/'students.dat').write_bytes(struct.pack('<i',1)+st)
    school=text('A',60)+text('001',20)+b'\0'*(20*99)+struct.pack('<i',1)
    (d/'classes.dat').write_bytes(struct.pack('<i',1)+school)
