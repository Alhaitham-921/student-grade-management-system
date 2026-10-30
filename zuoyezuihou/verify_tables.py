from pathlib import Path
import subprocess,tempfile,shutil,unicodedata
root=Path(__file__).resolve().parent
exe=root/'x64/Debug/zuoyezuihou.exe'
def run(d,seq):
 r=subprocess.run([str(exe)],cwd=d,input='\n'.join(seq)+'\n',encoding='utf-8',capture_output=True,timeout=30)
 assert r.returncode==0,r.stdout+r.stderr
 return r.stdout
def positions(s,mark):
 col=0;result=[]
 for ch in s:
  if ch==mark:result.append(col)
  col+=0 if unicodedata.combining(ch) else 2 if unicodedata.east_asian_width(ch) in 'WF' else 1
 return result
with tempfile.TemporaryDirectory(dir=root) as d:
 seq=['1','admin','admin','admin','abcdef','abcdef','8','软件测试一班','8','A'*55,'2','Teacher中文','teacher','数学','1','001','张明','1','0','1','002','Alice王','1','0','4','9','10','11','7','3','001','001','4','1','3','3','002','002','4','1','3','2','Teacher中文','teacher','1','1','001','0','1','1','002','100','2','1','1','88','3','1','001','3','2','数学','5','1','5','2','5','3','9','10','7','3','001','001','1','3','4']
 out=run(d,seq)
 borders=None;count=0
 for line in out.splitlines():
  # A prompt can precede the first border in redirected console output.
  if '+---' in line:
   line=line[line.index('+---'):]
   borders=positions(line,'+')
  elif line.startswith('|'):
   assert positions(line,'|')==borders,line
   assert positions(line,'|')[-1] < 120,line
   count+=1
 assert count>35,count
 assert '...' in out and 'Alice王' in out and '张明' in out
 print(f'PASS: {count} table rows aligned, Chinese/ASCII mixed text, clipped long class names, all lists and ranking modes; tables fit 120 columns')
# Preview using a copy of the supplied demo data, without changing user records.
with tempfile.TemporaryDirectory(dir=root) as d:
 source=root.parent/'修复版手工测试'
 for name in ('users.dat','students.dat','classes.dat'):shutil.copy2(source/name,Path(d)/name)
 seq=['2','demo_math','Teacher123','8','1',str(source/'导入测试文件/1班_数学.csv'),'9','10','5','1','7','4']
 out=run(d,seq)
 rows=[]
 for line in out.splitlines():
  if '+---' in line:rows.append(line[line.index('+---'):])
  elif line.startswith('|'):rows.append(line)
 (source/'表格效果预览.txt').write_text('\n'.join(rows)+'\n',encoding='utf-8-sig')
