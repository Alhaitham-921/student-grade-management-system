import ctypes as C, pathlib,struct,hashlib,secrets,shutil,datetime,json,subprocess,tempfile
root=pathlib.Path('修复版手工测试').resolve()
class User(C.Structure): _fields_=[('username',C.c_char*20),('password',C.c_char*128),('type',C.c_int),('teachCourse',C.c_char*30)]
class Course(C.Structure): _fields_=[('course',C.c_char*30),('score',C.c_float)]
class Student(C.Structure): _fields_=[('stuNum',C.c_char*20),('name',C.c_char*20),('courses',Course*10),('teachers',(C.c_char*20)*10),('teacherCount',C.c_int),('courseCount',C.c_int),('average',C.c_float)]
class SchoolClass(C.Structure): _fields_=[('name',C.c_char*60),('members',(C.c_char*20)*100),('count',C.c_int)]
def read(name,T):
 p=root/name
 if not p.exists():return []
 b=p.read_bytes();n=struct.unpack('<i',b[:4])[0];assert len(b)==4+n*C.sizeof(T)
 return [T.from_buffer_copy(b[4+i*C.sizeof(T):4+(i+1)*C.sizeof(T)]) for i in range(n)]
def write(name,records):
 p=root/name; tmp=p.with_suffix('.seed.tmp');tmp.write_bytes(struct.pack('<i',len(records))+b''.join(bytes(x) for x in records));tmp.replace(p)
def password(text):
 salt=secrets.token_bytes(16);return ('p1$'+salt.hex()+hashlib.pbkdf2_hmac('sha256',text.encode(),salt,100000).hex()).encode()
u=read('users.dat',User); st=read('students.dat',Student); classes=read('classes.dat',SchoolClass)
assert C.sizeof(User)==184 and C.sizeof(Student)==612
teachers=[('demo_math','数学'),('demo_english','英语'),('demo_chinese','语文')]
assert not any(x.username.decode() in dict(teachers) for x in u),'示例账号已存在，停止防止覆盖'
assert len(u)+13<=100 and len(st)+10<=100 and len(classes)+2<=50
backup=root/('数据备份_添加示例前_'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S'));backup.mkdir()
for name in ['users.dat','students.dat','classes.dat']:
 if (root/name).exists():shutil.copy2(root/name,backup/name)
for name,subject in teachers:
 x=User();x.username=name.encode();x.password=password('Teacher123');x.type=1;x.teachCourse=subject.encode();u.append(x)
fixtures=root/'导入测试文件';fixtures.mkdir(exist_ok=True)
for classno,names in [(1,['张明','李华','王芳','赵强','陈晨']),(2,['刘洋','周婷','吴磊','郑欣','孙浩'])]:
 c=SchoolClass();c.name=f'软件测试{classno}班'.encode();c.count=5
 ids=[]
 for j,name in enumerate(names,1):
  id=f'2610{classno}{j:02d}';assert not any(x.username==id.encode() for x in u)
  ids.append(id);c.members[j-1].value=id.encode()
  x=Student();x.stuNum=id.encode();x.name=name.encode();x.teacherCount=3
  for k,(teacher,subject) in enumerate(teachers):x.teachers[k].value=teacher.encode()
  st.append(x)
  x=User();x.username=id.encode();x.password=password(id);x.type=2;u.append(x)
 classes.append(c)
 for k,(teacher,subject) in enumerate(teachers):
  scores=([80,90,70,100,60] if classno==1 else [90,85,75,65,95]) if k==0 else ([88,76,90,82,94] if k==1 else [78,86,92,84,90])
  (fixtures/f'{classno}班_{subject}.csv').write_text('学号,科目,成绩\n'+''.join(f'{id},{subject},{score}\n' for id,score in zip(ids,scores)),encoding='utf-8-sig')
(fixtures/'失败示例_分数超范围.csv').write_text('学号,科目,成绩\n2610101,数学,80\n2610102,数学,120\n',encoding='utf-8-sig')
# Validate actual executable imports on a temporary copy; installed demo remains ungraded.
with tempfile.TemporaryDirectory(dir=root) as d:
 for name,recs in [('users.dat',u),('students.dat',st),('classes.dat',classes)]:
  (pathlib.Path(d)/name).write_bytes(struct.pack('<i',len(recs))+b''.join(bytes(x) for x in recs))
 seq=['2','demo_math','Teacher123','8',str(len(classes)-1),str(fixtures/'1班_数学.csv'),'4',str(len(classes)-1),'7','4']
 r=subprocess.run([str(root/'x64/Debug/zuoyezuihou.exe')],cwd=d,input='\n'.join(seq)+'\n',encoding='utf-8',capture_output=True,timeout=30)
 assert r.returncode==0 and '成功导入 5 条成绩' in r.stdout and '平均分: 80.00' in r.stdout,r.stdout+r.stderr
for name,recs in [('users.dat',u),('students.dat',st),('classes.dat',classes)]:write(name,recs)
readme='''示例数据与文件导入测试

已新增软件测试1班、软件测试2班，每班5名学生，均分配给以下三位教师。
现有账号、密码和成绩已保留。添加前的数据已备份到：'''+backup.name+'''

教师账号（统一密码 Teacher123，注意大小写）：
demo_math     数学
demo_english  英语
demo_chinese  语文

软件测试1班：2610101 张明、2610102 李华、2610103 王芳、2610104 赵强、2610105 陈晨
软件测试2班：2610201 刘洋、2610202 周婷、2610203 吴磊、2610204 郑欣、2610205 孙浩
学生用户名和初始密码均为学号。班级已选好，学生可在菜单4更换。
新增学生尚未录入成绩，便于测试导入。

第一次导入步骤：
1. 退出已打开的旧程序，双击本目录“开始手动测试.bat”。
2. 选择教师登录，输入 demo_math / Teacher123。
3. 教师菜单输入8（文件导入成绩），选择“软件测试1班”的序号。
4. 输入下列完整路径，不加引号：
'''+str(fixtures/'1班_数学.csv')+'''
5. 应显示“成功导入 5 条成绩”。菜单4选择同班，数学平均分应为80.00。
6. 再导入2班_数学.csv时先选择“软件测试2班”，平均分应为82.00。
7. 英语、语文文件分别用demo_english、demo_chinese登录后导入对应班级。

失败测试：在首次成功导入之前，用数学老师选择1班，导入“失败示例_分数超范围.csv”，第2条数据为120分，应整批拒绝；随后仍可导入正常文件。
已成功导入的文件再次导入会因已有成绩而拒绝，属于预期行为。
以上导入功能已在临时数据副本验证；实际示例数据保留未导入状态。
'''
(root/'示例数据与导入测试说明.txt').write_text(readme,encoding='utf-8-sig')
(root/'先看这里.txt').write_text('本测试副本现已加入两个示例班级及三位示例教师。\n请先阅读“示例数据与导入测试说明.txt”。\n双击“开始手动测试.bat”运行。已有数据已保留，新增学生尚未录入成绩。\n',encoding='utf-8-sig')
print('已新增2个班、3位老师、10名学生，生成6个正常CSV和1个失败CSV；实际程序导入及平均分验证通过。')
print('backup:',backup)
