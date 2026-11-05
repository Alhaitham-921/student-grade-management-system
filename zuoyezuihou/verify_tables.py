from test_support import *
import unicodedata

def positions(s, mark):
    col=0;result=[]
    for ch in s:
        if ch==mark:result.append(col)
        col+=0 if unicodedata.combining(ch) else 2 if unicodedata.east_asian_width(ch) in 'WF' else 1
    return result

d=directory('tables');seed(d,count=12)
out=run(d,ADMIN+['8','A'*55,'4','9','10','1','11','1','14','Same','1','7','2','T','teacher','9','2','1','10','1','5','1','7','4'])
borders=None;count=0
for line in out.splitlines():
    if '+---' in line:
        line=line[line.index('+---'):];borders=positions(line,'+')
    elif line.startswith('|'):
        assert positions(line,'|')==borders,line
        assert positions(line,'|')[-1]<120,line
        count+=1
assert count>35 and '...' in out and 'Alice王' in out,(count,out)
print(f'PASS: {count} mixed-language rows, clipping, filtering and page boundaries, <120 columns')
# Demo preview is opt-in and uses generated fixtures only.
if '--preview' in __import__('sys').argv:
    (d/'preview.txt').write_text(out,encoding='utf-8-sig')
    print('Preview:',d/'preview.txt')
