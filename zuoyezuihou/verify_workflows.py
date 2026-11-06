"""Additional end-to-end menu, migration and boundary tests. Keep every fixture."""
from test_support import *
import csv
import json
import traceback

checks = []


def case(name, fn):
    try:
        fn()
    except Exception:
        checks.append({'name': name, 'status': 'FAIL', 'error': traceback.format_exc()})
        print('FAIL:', name)
    else:
        checks.append({'name': name, 'status': 'PASS'})
        print('PASS:', name)


def unchanged(d, seq, message=None):
    before = business(d)
    out = run(d, seq)
    assert business(d) == before, out
    if message:
        assert message in out, out
    return out


def demo():
    d = directory('workflow-demo')
    run(d, FIRST + ['5', '4', '1', '6'] + LOGOUT)
    return d


def admin_reads():
    d = demo()
    seq = ADMIN + ['1', '2', '1', '2', '1', '2', '2', '1', '6',
                   '2', '2', '5', '3', '2', '3', '1', '7', '4',
                   '4', '1', '240101001', '2', 'T0002', '3', '1', '4',
                   '5', '2', '3', '6'] + LOGOUT
    out = unchanged(d, seq)
    for text in ['学生: 30 教师: 3 班级: 3 成绩记录: 87', '刘晨', '董博',
                 '教师: T0002 李华 科目: 数学', '授权学生人数: 30',
                 '班级人数: 10', 'demo-initialize']:
        assert text in out, (text, out)


def admin_missing_and_cancel():
    d = demo()
    seq = ADMIN + ['2', '1', '3', '999', '3', ':q', '4', '999',
                   '4', '240101001', 'no', '4', '240101001', ':q',
                   '5', 'Absent', '1', '6', '2', '3', 'T9999',
                   '3', ':q', '4', '999', '5', '3', '5', '999',
                   '4', '4', '7', '4', '4', '1', '999', '2', 'T9999',
                   '3', '4', '4', '5', '5', '1', 'no', '1', ':q', '2', '6'] + LOGOUT
    out = unchanged(d, seq)
    assert '学生不存在' in out and '教师不存在' in out and '共 0 条记录' in out, out
    assert '学生删除成功' not in out and '所有用户数据已清空' not in out, out


def admin_grade_reads():
    d = demo()
    export = d / 'admin.csv'
    out = unchanged(d, ADMIN + ['3', '1', '1', '1', '2', '1', '1',
                                '3', '1', '1', str(export), '4'] + LOGOUT)
    assert '有成绩人数: 10 未录入人数: 0' in out and '成功导出 10 条' in out, out
    with export.open(encoding='utf-8-sig', newline='') as f:
        rows = list(csv.reader(f))
    assert len(rows) == 11 and rows[1] == ['240101001', '数学', '0'], rows


def student_edit_relationship():
    d = directory('workflow-student-edit'); seed(d)
    out = run(d, ADMIN + ['2', '1', '3', '001', '2', '2', '0', '1', '6', '4'] + LOGOUT)
    assert '学生信息更新成功' in out, out
    out = run(d, ['3', '001', '001', '1', '4', '4'])
    assert '授课教师: 王芳' in out and '张三' not in out, out
    unchanged(d, TEACHER + ['1', '1', '001', '11', '4'], '无权添加成绩')


def passwords():
    d = directory('workflow-passwords'); seed(d)
    for login, menu, old, logout, new in [
        (ADMIN, ['5', '1'], 'abcdef', ['6', '6', '4'], 'AdminNext123'),
        (TEACHER, ['10'], 'teacher', ['11', '4'], 'TeachNext123'),
        (['3', '001', '001'], ['2'], '001', ['4', '4'], 'StudentNext123'),
    ]:
        unchanged(d, login + menu + ['wrong'] + logout, '当前密码不正确')
        unchanged(d, login + menu + [old, ':q'] + logout, '已取消操作')
        unchanged(d, login + menu + [old, 'short', 'short', ':q'] + logout, '至少6')
        unchanged(d, login + menu + [old, 'abcdef', 'mismatch', ':q'] + logout, '两次输入不一致')
        out = run(d, login + menu + [old, new, new] + logout)
        assert '密码修改成功' in out, out
        role, username = login[:2]
        out = run(d, [role, username, old, ':q', role, username, new] + logout)
        assert '用户名、密码或用户类型错误' in out and '当前用户: ' + username in out, out


def role_mismatch():
    d = directory('workflow-role-mismatch'); seed(d)
    for role, username, password in [('1', '001', '001'), ('2', 'admin', 'abcdef'),
                                     ('3', 'T0001', 'teacher'), ('3', 'admin', 'abcdef')]:
        out = unchanged(d, [role, username, password, ':q', '4'])
        assert '用户名、密码或用户类型错误' in out, out
        assert '当前用户:' not in out, out


def teacher_queries():
    d = directory('workflow-teacher-queries'); seed(d)
    run(d, TEACHER + ['1', '1', '001', '0', '1', '1', '002', '100', '11', '4'])
    out = unchanged(d, TEACHER + ['4', '1', '1', '001', '4', '1', '2',
                                  '5', '1', '6', '1', '8', '1', '8', '2', '2',
                                  '9', 'Alice', '2', '2', '11', '4'])
    for text in ['授权教师查看完整成绩', '平均分: 50.00', '及格率: 50.00%',
                 '参与排名人数: 2', 'Alice王', '共 1 条记录']:
        assert text in out, (text, out)
    unchanged(d, TEACHER + ['4', '1', '1', '003', '11', '4'], '无授课权限')
    unchanged(d, TEACHER + ['1', '1', '003', '11', '4'], '不属于所选班级')
    unchanged(d, TEACHER + ['1', '1', '999', '11', '4'], '该学生不存在')


def score_validation():
    d = directory('workflow-score-validation'); seed(d)
    invalid = ['', 'nan', 'inf', '-1', '101', '80junk', '1' * 120]
    out = run(d, TEACHER + ['1', '1', '001'] + invalid + ['0', '11', '4'])
    assert '成绩添加完成' in out and '请输入 0.0 到 100.0 之间的分数' in out, out
    unchanged(d, TEACHER + ['1', '1', '002', ':q', '11', '4'])
    unchanged(d, TEACHER + ['2', '4', '11', '4'])
    unchanged(d, TEACHER + ['2', '2', '11', '4'], '暂无成绩记录')
    unchanged(d, TEACHER + ['2', '1', '2', '11', '4'])
    out = run(d, TEACHER + ['2', '1', '1'] + invalid + ['100', '11', '4'])
    assert '成绩修改成功' in out, out
    out = run(d, ['3', '001', '001', '1', '4', '4'])
    assert '平均分: 100.0' in out, out


def other_subject_denied():
    d = directory('workflow-other-subject'); seed(d)
    run(d, ADMIN + edit_teaching('001', [1, 2]) + LOGOUT)
    run(d, ['2', 'T0002', 'secret', '1', '1', '001', '75', '11', '4'])
    unchanged(d, TEACHER + ['2', '1', '1', '11', '4'], '不是你所教授的科目')
    out = unchanged(d, TEACHER + ['5', '1', '6', '1', '11', '4'])
    assert '有成绩人数: 0 未录入人数: 2' in out and '参与排名人数: 0' in out, out


def inactive_class():
    d = directory('workflow-inactive'); seed(d)
    run(d, TEACHER + ['1', '1', '001', '50', '11', '4'])
    run(d, ADMIN + toggle_class(1) + LOGOUT)
    unchanged(d, TEACHER + ['1', '1', '11', '4'], '班级已停用')
    unchanged(d, TEACHER + ['3', '1', '11', '4'], '班级已停用')
    unchanged(d, ADMIN + ['2', '1', '1', '004', 'Bob', '1', '6', '4'] + LOGOUT, '班级已停用')
    out = run(d, TEACHER + ['2', '1', '1', '70', '5', '1', '11', '4'])
    assert '成绩修改成功' in out and '平均分: 70.00' in out, out
    out = run(d, ADMIN + assign('001', 2) + ['6', '3', '001', '001', '1', '4', '4'])
    assert '班级: B' in out and '平均分: 70.0' in out, out


def csv_rejections():
    d = directory('workflow-csv-rejections'); seed(d)
    samples = {
        'quote-tail': b'001,"Math"oops,80\n',
        'unclosed-quote': b'001,"Math,80\n',
        'quote-in-bare': b'001,Ma"th,80\n',
        'two-columns': b'001,Math\n',
        'five-columns': b'001,Same,Math,80,extra\n',
        'long-field': b'001,' + b'M' * 512 + b',80\n',
        'nul': b'001,Math,8\x000\n',
        'non-utf8': b'001,\xff,80\n',
        'mixed-columns': b'stuNum,name,course,score\n001,Math,80\n',
        'duplicate': b'001,Math,80\n001,Math,90\n',
        'late-error': b'001,Math,80\n002,Math,nan\n',
        'wrong-class': b'003,Math,80\n',
        'wrong-name': b'001,Other,Math,80\n',
        'blank-score': b'001,Math,\t \n',
        'overflow': b'001,Math,1e100\n',
        'no-student': b'999,Math,80\n',
    }
    for name, data in samples.items():
        path = d / (name + '.csv'); path.write_bytes(data)
        unchanged(d, TEACHER + ['3', '1', str(path), '11', '4'], '本次未导入任何成绩')
    path = d / 'large.csv'; path.write_bytes(b'x' * (8 * 1024 * 1024 + 1))
    unchanged(d, TEACHER + ['3', '1', str(path), '11', '4'], 'CSV超过8MiB')
    for text in ['', '\ufeffstuNum,name,course,score\n', '\n\r\n']:
        path = d / ('empty-%d.csv' % len(text)); path.write_text(text, encoding='utf-8')
        unchanged(d, TEACHER + ['3', '1', str(path), '11', '4'], '没有可导入的成绩')
    unchanged(d, TEACHER + ['3', '1', str(d / 'missing.csv'), '11', '4'], '无法打开CSV')


def csv_valid_formats():
    for name, data, course in [
        ('headerless', b'\n001,Math,59.5\r002,Math,60', 'Math'),
        ('english-four', b'stuNum,name,course,score\r\n001,Same,Math,80\r\n002,Same,Math,100\r\n', 'Math'),
        ('multiline', b'001,"Math\nLab",80\n002,"Math\nLab",90\n', 'Math\nLab'),
    ]:
        d = directory('workflow-csv-' + name)
        if name == 'multiline':
            # A quoted CSV field may span lines; a valid stored course must still match exactly.
            seed(d, course='Math')
            path = d / 'input.csv'; path.write_bytes(data)
            unchanged(d, TEACHER + ['3', '1', str(path), '11', '4'], '科目不匹配')
            continue
        seed(d, course=course)
        path = d / 'input.csv'; path.write_bytes(data)
        out = run(d, TEACHER + ['3', '1', str(path), '1', '5', '1', '11', '4'])
        assert '成功导入 2 条' in out, out
        unchanged(d, TEACHER + ['3', '1', str(path), '11', '4'], '该课程已有成绩')


def maximum_identifiers():
    d = directory('workflow-long-id'); seed(d)
    identity = '0' * 18 + '1'
    out = run(d, ADMIN + add_student(identity, '张三') + LOGOUT)
    assert '学生添加成功' in out, out
    out = run(d, ['3', identity, identity, '1', '4', '4'])
    assert '学号: ' + identity in out, out
    out = run(d, ADMIN + add_teacher('T9999', 'Anne-Marie', 'Last') + LOGOUT)
    assert '教师添加成功' in out, out


def teacher_and_course_capacity():
    d = directory('workflow-teacher-course-capacity')
    seq = FIRST + add_class('A')
    for i in range(1, 12):
        seq += add_teacher(f'T{i:04d}', 'Teacher', f'Course{i}')
    seq += add_student('001', 'Alice', teachers=tuple(map(str, range(1, 11)))) + LOGOUT
    out = run(d, seq)
    assert '学生添加成功' in out and '教师数: 10' in out, out
    out = run(d, ADMIN + edit_teaching('001', list(range(1, 11)) + [10, 11]) + LOGOUT)
    assert '该教师已选择' in out and '最多选择10位教师' in out and '授课关系更新成功' in out, out
    seq = []
    for i in range(1, 11):
        seq += ['2', f'T{i:04d}', 'teacher', '1', '1', '001', str(i * 10), '11']
    out = run(d, seq + ['4'])
    assert out.count('成绩添加完成') == 10, out
    out = run(d, ['3', '001', '001', '1', '4', '4'])
    assert '平均分: 55.0' in out, out
    run(d, ADMIN + edit_teaching('001', [11]) + LOGOUT)
    login = ['2', 'T0011', 'teacher']
    unchanged(d, login + ['1', '1', '001', '11', '4'], '课程数量已达上限')
    path = d / 'eleventh.csv'; path.write_text('001,Course11,80\n', encoding='utf-8')
    unchanged(d, login + ['3', '1', str(path), '11', '4'], '课程数量已达上限')


def menu_eof_and_invalid():
    d = directory('workflow-eof'); seed(d)
    run(d, TEACHER + ['1', '1', '001', '80', '11', '4'])
    invalid = ['', 'abc', '-1', '+1', '1.2', '99', '1' * 120]
    unchanged(d, TEACHER + invalid + ['11', '3', '001', '001'] + invalid + ['4', '4'], '输入过长')
    prefixes = [
        ['1'], ['1', '1'], ['1', '1', '002'], ['2'], ['2', '1'], ['2', '1', '1'],
        ['3'], ['3', '1'], ['4'], ['4', '1'], ['4', '1', '1'], ['5'], ['6'],
        ['7'], ['7', '1'], ['8'], ['8', '2'], ['9'], ['9', 'Same'], ['9', 'Same', '2'],
        ['10'], ['10', 'teacher'], ['10', 'teacher', 'NewPass123'],
    ]
    for prefix in prefixes:
        unchanged(d, TEACHER + prefix, '系统正常结束')
    for prefix in [[], ['001'], ['001', 'NewPass123']]:
        unchanged(d, ['3', '001', '001', '2'] + prefix, '系统正常结束')
    for prefix in [['2', '1', '3'], ['2', '1', '3', '001'], ['2', '1', '3', '001', '1'],
                   ['2', '1', '3', '001', '1', 'NewName'], ['2', '2', '3'],
                   ['2', '2', '3', 'T0001'], ['2', '2', '3', 'T0001', 'NewTeacher'],
                   ['2', '3', '4'], ['2', '3', '4', '1'], ['2', '3', '5'],
                   ['2', '3', '5', '001'], ['5', '5', '1']]:
        unchanged(d, ADMIN + prefix, '系统正常结束')


def gbk_class_migration():
    d = directory('workflow-gbk-class'); legacy(d)
    name = '24级软件1班'
    block = name.encode('gbk').ljust(60, b'\0') + b'1234'.ljust(20, b'\0') + b'\0' * (20 * 99) + struct.pack('<i', 1)
    (d / 'classes.dat').write_bytes(struct.pack('<i', 1) + block)
    original = {p.name: p.read_bytes() for p in d.glob('*.dat')}
    for _ in range(2):
        out = run(d, ['3', '1234', '1234', '1', '3', '4', '4'])
        assert '班级: ' + name in out, out
    assert original == {p.name: p.read_bytes() for p in d.glob('*.dat')}
    assert name.encode() in (active(d) / 'classes.dat').read_bytes()
    backup = next(d.glob('legacy-backup-*'))
    assert original == {p.name: p.read_bytes() for p in backup.glob('*.dat')}


def utf8_class_migration():
    d = directory('workflow-utf8-class'); legacy_hashed(d)
    p = d / 'classes.dat'; raw = bytearray(p.read_bytes())
    raw[4:64] = '中文班'.encode().ljust(60, b'\0'); p.write_bytes(raw)
    out = run(d, ['3', '001', '001', '3', '4', '4'])
    assert '班级: 中文班' in out, out


def delete_and_reseed():
    # These operations run exclusively on newly created synthetic fixtures.
    d = demo()
    out = run(d, ADMIN + delete_student('240101001') + ['4', '2', 'T0002', '4', '1'] + LOGOUT)
    assert '学生删除成功' in out and '授权学生人数: 29' in out and '学生: 29' in out, out
    out = run(d, ['3', '240101002', '240101002', '1', '4', '4'])
    assert '王悦' in out, out
    out = run(d, ADMIN + ['5', '5', '1', 'confirm', '2', '4', '1', '6', '1'] + LOGOUT)
    assert '所有用户数据已清空' in out and '示例数据初始化成功' in out, out
    assert '学生: 30 教师: 3 班级: 3 成绩记录: 87' in out, out


def mutation_rollbacks():
    fault = os.environ.get('SGMS_FAULT_EXE')
    assert fault, 'Use build-test.ps1 to build the fault executable'
    d = directory('workflow-rollbacks'); seed(d)
    run(d, TEACHER + ['1', '1', '001', '50', '11', '4'])
    sequences = [
        ADMIN + edit_student('001', 'NewName') + LOGOUT,
        ADMIN + edit_teacher('T0001', 'NewTeacher') + LOGOUT,
        ADMIN + edit_teaching('001', [2]) + LOGOUT,
        ADMIN + assign('001', 2) + LOGOUT,
        ADMIN + rename_class(1, 'Changed') + LOGOUT,
        ADMIN + toggle_class(1) + LOGOUT,
        ADMIN + delete_student('001') + LOGOUT,
        ADMIN + ['5', '5', '1', 'confirm', '2', '6'] + LOGOUT,
        ADMIN + ['5', '1', 'abcdef', 'NewAdmin123', 'NewAdmin123', '6'] + LOGOUT,
        TEACHER + ['2', '1', '1', '90', '11', '4'],
    ]
    for seq in sequences:
        before = business(d)
        out = run(d, seq, exe=fault, env={'SGMS_FAULT': 'before-marker', 'SGMS_FAULT_AFTER': '2'})
        assert '已恢复上次状态' in out and business(d) == before, out
    out = run(d, ['3', '001', '001', '1', '4', '4'])
    assert '班级: A' in out and '平均分: 50.0' in out, out


for name, fn in [
    ('administrator dashboard, lists, class members, all overview and audit entries', admin_reads),
    ('administrator missing records, empty search, return and dangerous-action cancellation', admin_missing_and_cancel),
    ('administrator grade query, statistics and school export', admin_grade_reads),
    ('student-editor teaching relationship updates and permissions', student_edit_relationship),
    ('all three roles: wrong, cancelled, short, mismatched and successful password changes', passwords),
    ('all login entrances reject accounts of a different role', role_mismatch),
    ('teacher query modes, class filters, search, zero/100 statistics and isolation', teacher_queries),
    ('manual scores: malformed, boundary, cancelled and absent-grade modification', score_validation),
    ('teacher cannot modify another subject or rank its scores', other_subject_denied),
    ('inactive class rejects new records, permits corrections and transfer out', inactive_class),
    ('CSV malformed files, partial-valid batches, size limits and empty input', csv_rejections),
    ('CSV CR/LF, no final newline, English header, multiline fields and existing grades', csv_valid_formats),
    ('19-digit leading-zero student account and T9999 teacher', maximum_identifiers),
    ('ten teachers complete with zero; duplicates, eleventh teacher/course and ten-grade average', teacher_and_course_capacity),
    ('teacher/student invalid menus and EOF at 38 additional action prompts', menu_eof_and_invalid),
    ('GBK Chinese class migration, full backup and restart', gbk_class_migration),
    ('hashed UTF8 legacy class migration preserves text', utf8_class_migration),
    ('isolated deletion, remaining-account identity and clear/reseed lifecycle', delete_and_reseed),
    ('ten mutation paths roll back completely on storage failure', mutation_rollbacks),
]:
    case(name, fn)

report = directory('workflow-summary') / 'checks.json'
report.write_text(json.dumps(checks, ensure_ascii=False, indent=2), encoding='utf-8')
failed = [x for x in checks if x['status'] != 'PASS']
print(f'{len(checks) - len(failed)}/{len(checks)} workflow groups passed; details: {report}')
for entry in failed:
    print(entry['error'])
raise SystemExit(bool(failed))
