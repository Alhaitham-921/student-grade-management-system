from test_support import *
import unicodedata
def positions(s,mark):
    col=0;result=[]
    for ch in s:
        if ch==mark:result.append(col)
        col+=0 if unicodedata.combining(ch) else 2 if unicodedata.east_asian_width(ch) in 'WF' else 1
    return result
d=directory('tables');seed(d,count=12)
out=run(d,ADMIN+add_class('A'*55)+['5','2','6','2','1','2','1','6','3','2','7','4','6'])
borders=None;count=0
for line in out.splitlines():
    if '+---' in line:
        line=line[line.index('+---'):];borders=positions(line,'+')
    elif line.startswith('|'):
        assert positions(line,'|')==borders,line
        assert positions(line,'|')[-1]<120,line
        count+=1
assert count>=35 and '...' in out and 'Alice王' in out,(count,out)
print(f'PASS: {count} mixed-language rows, clipping, filtering and page boundaries, <120 columns')
if '--preview' in __import__('sys').argv:(d/'preview.txt').write_text(out,encoding='utf-8-sig')
