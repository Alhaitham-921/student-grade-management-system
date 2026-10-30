# -*- coding: utf-8 -*-
import struct, os, sys

# User 结构: char username[20] + char password[20] + int type + char teachCourse[30]
# 对齐后 sizeof(User) 待探测
USERNAME_LEN = 20
PASSWORD_LEN = 20
COURSE_LEN = 30

def parse_user(buf):
    # 尝试常见对齐: 74(无对齐) / 76(4字节对齐)
    for stride in (74, 76, 80):
        if len(buf) >= stride:
            username = buf[0:20].split(b'\x00')[0].decode('gbk', 'ignore')
            password = buf[20:40].split(b'\x00')[0].decode('gbk', 'ignore')
            type_off = 40 if stride in (74,) else 40
            # int 在 4 字节对齐时可能从 40 开始(74 无填充)或 40
            type_val = struct.unpack('<i', buf[type_off:type_off+4])[0] if len(buf) >= type_off+4 else None
            return username, password, type_val, stride
    return None

files = [
    r'F:\study\软件质量保证与测试\zuoyezuihou\users.dat',
    r'F:\study\软件质量保证与测试\zuoyezuihou\Debug\users.dat',
    r'F:\study\软件质量保证与测试\zuoyezuihou\x64\Debug\users.dat',
    r'F:\study\软件质量保证与测试\zuoyezuihou\zuoyezuihou\Debug\users.dat',
    r'F:\study\软件质量保证与测试\zuoyezuihou\zuoyezuihou\x64\Debug\users.dat',
]

TYPE_NAMES = {0: 'ADMIN', 1: 'TEACHER', 2: 'STUDENT'}

for f in files:
    print('='*70)
    print('文件:', f, '存在:', os.path.exists(f))
    if not os.path.exists(f):
        continue
    size = os.path.getsize(f)
    print('大小:', size, '字节')
    with open(f, 'rb') as fp:
        raw = fp.read()
    if len(raw) < 4:
        print('  文件过短，无有效计数')
        continue
    count = struct.unpack('<i', raw[0:4])[0]
    print('userCount 字段 =', count)
    body = raw[4:]
    # 用 76 试解析
    stride = 76
    for idx in range(count):
        off = idx * stride
        seg = body[off:off+stride]
        if len(seg) < stride:
            print(f'  用户[{idx}] 段不足({len(seg)}B)，文件可能损坏')
            break
        username = seg[0:20].split(b'\x00')[0].decode('gbk', 'ignore')
        password = seg[20:40].split(b'\x00')[0].decode('gbk', 'ignore')
        type_val = struct.unpack('<i', seg[40:44])[0]
        course = seg[44:74].split(b'\x00')[0].decode('gbk', 'ignore')
        print(f'  用户[{idx}] 用户名={username!r} 密码={password!r} 类型={TYPE_NAMES.get(type_val, type_val)} 任课={course!r}')
    # 检查是否有多余字节
    expect = count * stride
    if expect != len(body):
        print(f'  [警告] 按sizeof=76 计算应有 {expect}B 数据，实际 {len(body)}B，差 {len(body)-expect}B -> 结构体大小可能不同或文件异常')
