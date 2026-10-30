import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parent
exe=root/'x64/Debug/zuoyezuihou.exe'
def run(d,seq):
 p=subprocess.run([str(exe)],cwd=d,input='\n'.join(seq)+'\n',encoding='utf-8',capture_output=True,timeout=30)
 assert p.returncode==0,p.stdout+p.stderr
 return p.stdout+p.stderr
with tempfile.TemporaryDirectory(dir=root) as tmp:
 d=pathlib.Path(tmp)
 seq=['1','admin','admin','admin','abcdef','abcdef','8','A','8','B','8','A','2','T','teacher','Math']
 for id in ['001','002','003']: seq+=['1',id,id,'1','0']
 seq+=['7']
 for id,c in [('001','1'),('002','1'),('003','2')]:seq+=['3',id,id,'4',c,'3']
 seq+=['2','T','teacher','1','1','003','1','2','003','100','7','4']
 out=run(d,seq)
 assert '班级已存在' in out and '该学生不属于所选班级' in out,out
 (d/'bad.csv').write_text('stuNum,course,score\n001,Math,80\n003,Math,90\n',encoding='utf-8-sig')
 before=(d/'students.dat').read_bytes()
 out=run(d,['2','T','teacher','8','1',str(d/'bad.csv'),'7','4'])
 assert '本次未导入任何成绩' in out and before==(d/'students.dat').read_bytes(),out
 (d/'good.csv').write_text('学号,科目,成绩\n001,Math,0\n002,Math,80\n',encoding='utf-8-sig')
 out=run(d,['2','T','teacher','8','1',str(d/'good.csv'),'4','1','4','2','8','1',str(d/'good.csv'),'7','3','001','001','1','3','4'])
 assert '成功导入 2 条' in out and '平均分: 40.00' in out and '平均分: 100.00' in out and '班级: A' in out and '本次未导入任何成绩' in out,out
 # Teacher permission and nonfinite scores both reject the entire batch.
 for row in ['002,Math,nan','002,English,90','002,Math,101','999,Math,50','002,Math,70\n002,Math,80']:
  (d/'bad.csv').write_text(row,encoding='utf-8')
  before=(d/'students.dat').read_bytes()
  out=run(d,['2','T','teacher','8','1',str(d/'bad.csv'),'7','4'])
  assert '本次未导入任何成绩' in out and before==(d/'students.dat').read_bytes(),out
 out=run(d,['1','admin','abcdef','3','001','yes','7','4'])
 out=run(d,['1','admin','abcdef','9','7','4'])
 assert any(' A ' in line and '| 1 ' in line for line in out.splitlines()),out
 print('PASS: classes, membership persistence/deletion, class isolation, CSV BOM/headers, atomic rejection, zero scores and separate class averages')
