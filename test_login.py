# -*- coding: utf-8 -*-
import subprocess, struct, os

# ---- 先解析 students.dat 检查是否损坏 ----
def check_students(path):
    print('='*70)
    print('students.dat:', path, '存在:', os.path.exists(path))
    if not os.path.exists(path):
        return
    size = os.path.getsize(path)
    with open(path, 'rb') as fp:
        raw = fp.read()
    count = struct.unpack('<i', raw[0:4])[0]
    print(f'大小 {size}B, studentCount 字段 = {count}, 数据段 {len(raw)-4}B')
    # 尝试可能的 sizeof(Student)
    for stride in (572, 592, 576, 612):
        if (len(raw)-4) % stride == 0 and (len(raw)-4) // stride == count:
            print(f'  -> 与 sizeof(Student)={stride} 完全吻合，记录数={count}')
            return
    print(f'  -> 与常见结构体大小均不吻合（余数: {[(s, (len(raw)-4)%s) for s in (572,592,576,612)]}），文件可疑')

check_students(r'F:\study\软件质量保证与测试\zuoyezuihou\students.dat')
check_students(r'F:\study\软件质量保证与测试\zuoyezuihou\Debug\students.dat')

# ---- 实际运行 exe 测试 admin/admin 登录 ----
def run_login(exe_path, cwd):
    print('='*70)
    print('运行:', exe_path)
    print('工作目录:', cwd)
    try:
        p = subprocess.run(
            [exe_path],
            cwd=cwd,
            input='1\nadmin\nadmin\n4\n',
            capture_output=True,
            timeout=15,
        )
        out = p.stdout.decode('gbk', 'replace')
        err = p.stderr.decode('gbk', 'replace')
        print('--- stdout ---')
        print(out[:2000])
        if err.strip():
            print('--- stderr ---')
            print(err[:1000])
        print('退出码:', p.returncode)
    except Exception as e:
        print('运行异常:', e)

run_login(r'F:\study\软件质量保证与测试\zuoyezuihou\Debug\zuoyezuihou.exe',
          r'F:\study\软件质量保证与测试\zuoyezuihou\Debug')
run_login(r'F:\study\软件质量保证与测试\zuoyezuihou\x64\Debug\zuoyezuihou.exe',
          r'F:\study\软件质量保证与测试\zuoyezuihou\x64\Debug')
