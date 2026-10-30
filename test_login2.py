# -*- coding: utf-8 -*-
import subprocess, os

def run_login(exe_path, cwd, label):
    print('='*70)
    print(label, ':', exe_path)
    print('工作目录:', cwd)
    # 输入: 主菜单选1(管理员登录) -> admin/admin -> 管理员菜单选7(返回) -> 主菜单选4(退出)
    # 每个 system("pause") 可能消费字符，多给一些换行兜底
    inp = '1\nadmin\nadmin\n7\n4\n' + '\n' * 30
    try:
        p = subprocess.run(
            [exe_path], cwd=cwd, input=inp.encode('gbk'),
            capture_output=True, timeout=25,
        )
        out = p.stdout.decode('gbk', 'replace')
        err = p.stderr.decode('gbk', 'replace')
        print('--- stdout ---')
        print(out[:2500])
        if err.strip():
            print('--- stderr ---')
            print(err[:800])
        print('退出码:', p.returncode)
    except subprocess.TimeoutExpired:
        print('>>> 超时：程序未退出（可能卡在 EOF 死循环或 pause）')
    except Exception as e:
        print('运行异常:', e)

run_login(r'F:\study\软件质量保证与测试\zuoyezuihou\Debug\zuoyezuihou.exe',
          r'F:\study\软件质量保证与测试\zuoyezuihou\Debug', 'Debug(x86)')
run_login(r'F:\study\软件质量保证与测试\zuoyezuihou\x64\Debug\zuoyezuihou.exe',
          r'F:\study\软件质量保证与测试\zuoyezuihou\x64\Debug', 'x64 Debug')
