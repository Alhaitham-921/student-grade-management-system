"""Non-destructive integration verification. Run ONLY against a fresh --Demo server."""
import urllib.request,urllib.error,http.cookiejar,json
BASE='http://127.0.0.1:3000/api/'
class Client:
 def __init__(self):
  self.http=urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()));self.csrf='';self.rev=1
  assert self.call('session').get('testMode'), 'Refusing to test a non-Demo server'
 def call(self,url,body=None,fail=False):
  req=urllib.request.Request(BASE+url,data=None if body is None else json.dumps(body).encode(),headers={'Content-Type':'application/json','x-csrf-token':self.csrf})
  try:
   with self.http.open(req) as r: j=json.load(r)
  except urllib.error.HTTPError as e:j=json.load(e)
  if j.get('csrf'):self.csrf=j['csrf']
  if 'revision'in j:self.rev=j['revision']
  if fail:assert j.get('error'),j
  else:assert not j.get('error'),j
  return j
 def act(self,op,p,fail=False,rev=None):return self.call('action/'+op,{'params':p,'revision':self.rev if rev is None else rev},fail)
a=Client();a.call('login',{'username':'admin','password':'admin'});a.call('snapshot',fail=True)
a.act('password',{'oldPassword':'admin','password':'WebTest123'});a.act('seed',{});snap=a.call('snapshot')['data'];assert len(snap['students'])==30
teacher=next(u for u in snap['users']if u['role']==1 and u['course']=='数学')
t=Client();t.call('login',{'username':teacher['username'],'password':'Teacher123'});ts=t.call('snapshot')['data'];assert len(ts['students'])==30
assert ts['students'][0]['grades'][0]['score']==0
assert 'password'not in json.dumps(ts)
t.act('class',{'id':-1,'name':'Denied'},True)
s=Client();s.call('login',{'username':'240101001','password':'240101001'});ss=s.call('snapshot')['data'];assert len(ss['students'])==1
s.act('grade',{'id':'240101002','score':99},True)
s.call('audit',fail=True)
old=t.rev;t.act('grade',{'id':'240101001','score':0});t.act('grade',{'id':'240101001','score':99},True,old)
a.call('snapshot');a.act('grade',{'id':'240101001','score':99},True)
a.call('audit');p=a.call('danger/preview',{'op':'delete','id':'240101001'});assert p['data']['token'] # Do not commit destructive operations.
a.call('snapshot');a.act('student',{'id':'0000123','name':'测试学生','classId':0,'teachers':[teacher['username']]});assert any(x['id']=='0000123' for x in a.call('snapshot')['data']['students'])
print('PASS: roles, default-password gate, shared DAT, zero score, permissions, stale revision, leading zero, audit and danger preview; no deletions executed.')
# Additional non-destructive CSV and bounds checks, on the same fresh test server.
c=Client();c.call('login',{'username':'T0004','password':'Teacher123'});state=c.call('snapshot')['data'];target=next(x for x in state['students']if x['id']=='240101010');assert not any(g['course']=='C语言'for g in target['grades'])
bad=c.call('csv/preview',{'classId':0,'text':'学号,姓名,科目,成绩\n240101010,错误姓名,C语言,75'});assert bad['data']['errors'];c.call('csv/commit',{'token':bad['data']['token']},True)
p=c.call('csv/preview',{'classId':0,'text':'\ufeff学号,姓名,科目,成绩\r\n240101010,何雨,C语言,75.5\r\n'});assert not p['data']['errors'];c.call('csv/commit',{'token':p['data']['token']});c.call('csv/commit',{'token':p['data']['token']},True)
c.call('snapshot');assert next(g['score']for x in c.call('snapshot')['data']['students']if x['id']=='240101010'for g in x['grades']if g['course']=='C语言')==75.5
c.act('grade',{'id':'240101010','score':101},True)
a.call('snapshot');a.act('class',{'id':0,'name':'24级软件1班','inactive':True});c.call('snapshot');c.act('grade',{'id':'0000123','score':80},True);c.act('grade',{'id':'240101010','score':76})
a.call('snapshot');a.act('class',{'id':0,'name':'24级软件1班','inactive':False})
print('PASS: CSV BOM/CRLF/4 columns, error batch rejection, single-use confirmation, range bounds and inactive-class rules.')
# Capacity uses synthetic students in this isolated Demo dataset only.
a.call('snapshot');before=len(a.call('snapshot')['data']['users']);needed=100-before
for i in range(needed):a.act('student',{'id':'999'+str(i).zfill(6),'name':'测试学生','classId':0,'teachers':['T0002']})
a.act('student',{'id':'999999999','name':'测试学生','classId':0,'teachers':['T0002']},True)
assert len(a.call('snapshot')['data']['users'])==100
print('PASS: 100-user capacity boundary rejects overflow without partial student creation.')
