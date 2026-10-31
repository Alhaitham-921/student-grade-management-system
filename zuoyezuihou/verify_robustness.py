import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parent
exe=root/'x64/Debug/zuoyezuihou.exe'
def run(seq):
 with tempfile.TemporaryDirectory(dir=root) as d:
  p=subprocess.run([str(exe)],cwd=d,input='\n'.join(seq)+'\n',encoding='utf-8',capture_output=True,timeout=20)
  return p
cases=[['1junk','1'*120,'4'],['1','admin','admin','admin','abcdef','abcdef','4']]
for seq in cases:
 p=run(seq)
 assert p.returncode==0, p.stdout+p.stderr
print('PASS: robustness smoke cases')
