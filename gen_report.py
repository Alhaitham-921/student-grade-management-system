# -*- coding: utf-8 -*-
from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn

doc = Document()

# 设置默认字体为宋体
style = doc.styles['Normal']
style.font.name = 'Times New Roman'
style.font.size = Pt(11)
style.element.rPr.rFonts.set(qn('w:eastAsia'), '宋体')

# 页边距
for section in doc.sections:
    section.top_margin = Cm(2.5)
    section.bottom_margin = Cm(2.5)
    section.left_margin = Cm(2.8)
    section.right_margin = Cm(2.8)

def set_cn_font(run, name='宋体', size=11, bold=False, color=None):
    run.font.name = 'Times New Roman'
    run._element.rPr.rFonts.set(qn('w:eastAsia'), name)
    run.font.size = Pt(size)
    run.font.bold = bold
    if color:
        run.font.color.rgb = color

def add_title(text, level=1):
    p = doc.add_paragraph()
    if level == 0:
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(text)
        set_cn_font(r, '黑体', 22, True)
    elif level == 1:
        r = p.add_run(text)
        set_cn_font(r, '黑体', 16, True)
        p.paragraph_format.space_before = Pt(12)
        p.paragraph_format.space_after = Pt(6)
    elif level == 2:
        r = p.add_run(text)
        set_cn_font(r, '黑体', 13, True)
        p.paragraph_format.space_before = Pt(8)
        p.paragraph_format.space_after = Pt(4)
    return p

def add_body(text, bold=False):
    p = doc.add_paragraph()
    p.paragraph_format.first_line_indent = Pt(22)
    p.paragraph_format.line_spacing = 1.5
    r = p.add_run(text)
    set_cn_font(r, '宋体', 11, bold)
    return p

def add_bullet(text):
    p = doc.add_paragraph(style='List Bullet')
    p.paragraph_format.line_spacing = 1.5
    r = p.runs[0] if p.runs else p.add_run('')
    p.runs[0].text = ''
    r = p.add_run(text)
    set_cn_font(r, '宋体', 11)
    return p

# ========== 封面标题 ==========
add_title('学生成绩管理系统', 0)
add_title('初步缺陷报告', 0)

p = doc.add_paragraph()
p.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = p.add_run('软件质量保证与测试 · 第一阶段实验')
set_cn_font(r, '楷体', 14)

doc.add_paragraph()

# 基本信息表
info = doc.add_table(rows=4, cols=2)
info.style = 'Light Grid Accent 1'
info.alignment = WD_TABLE_ALIGNMENT.CENTER
info_data = [
    ('项目名称', '学生成绩管理系统（C语言控制台版）'),
    ('测试对象', 'zuoyezuihou.exe（含 admin/teacher/student 三端）'),
    ('测试方法', '静态代码审查 + 动态黑盒测试'),
    ('报告日期', '2026年10月'),
]
for i, (k, v) in enumerate(info_data):
    c0 = info.cell(i, 0).paragraphs[0]
    r0 = c0.add_run(k)
    set_cn_font(r0, '宋体', 11, True)
    c1 = info.cell(i, 1).paragraphs[0]
    r1 = c1.add_run(v)
    set_cn_font(r1, '宋体', 11)

doc.add_paragraph()

# ========== 一、实验概述 ==========
add_title('一、实验概述', 1)
add_body('本次测试对象为一个 C 语言编写的控制台版学生成绩管理系统。系统包含管理员、教师、学生三类角色，'
         '支持用户登录、学生信息管理、教师任课与成绩录入、学生成绩查询等基本业务流程，'
         '数据通过 users.dat 与 students.dat 二进制文件持久化存储。')
add_body('本阶段实验采用静态代码审查（通读全部 .c/.h 源码）结合动态黑盒测试（实际运行程序并模拟用户操作）的方式，'
         '围绕登录、增删改查、成绩管理等核心业务路径，重点考察输入校验、数据一致性、权限控制和异常处理等方面，'
         '共发现缺陷 18 项。')

# ========== 二、缺陷分类标准 ==========
add_title('二、缺陷分类标准', 1)
add_body('本次报告依据 IEEE 1044 软件异常分类标准，结合课程要求，从严重程度和影响范围两个维度对缺陷进行分类。')

add_title('2.1 按严重程度分级', 2)
sev_table = doc.add_table(rows=5, cols=3)
sev_table.style = 'Light Grid Accent 1'
sev_table.alignment = WD_TABLE_ALIGNMENT.CENTER
sev_header = ['等级', '名称', '判定标准']
sev_rows = [
    ('I 级', '致命（Critical）', '导致程序崩溃、数据丢失或错误、安全漏洞，核心功能完全不可用'),
    ('II 级', '严重（Major）', '主要功能失效或结果错误，但程序未崩溃，存在临时绕过手段'),
    ('III 级', '一般（Normal）', '界面显示错误、局部功能受限，不影响主流程'),
    ('IV 级', '轻微（Minor）', '代码规范、易用性、兼容性问题，对功能无实质影响'),
]
for j, h in enumerate(sev_header):
    c = sev_table.cell(0, j).paragraphs[0]
    r = c.add_run(h)
    set_cn_font(r, '宋体', 11, True)
for i, row in enumerate(sev_rows):
    for j, v in enumerate(row):
        c = sev_table.cell(i+1, j).paragraphs[0]
        r = c.add_run(v)
        set_cn_font(r, '宋体', 10.5)

add_title('2.2 按影响范围分类', 2)
add_body('全局问题：影响所有用户角色或整个系统安全；模块问题：仅影响某一角色或某一功能模块；'
         '局部问题：仅在特定输入或边界条件下触发。')

# ========== 三、缺陷汇总 ==========
add_title('三、缺陷汇总', 1)
add_body('本次共发现缺陷 18 项，其中致命缺陷 4 项、严重缺陷 8 项、一般缺陷 3 项、轻微缺陷 3 项。分布如下：')

sum_table = doc.add_table(rows=6, cols=4)
sum_table.style = 'Light Grid Accent 1'
sum_table.alignment = WD_TABLE_ALIGNMENT.CENTER
sum_header = ['严重等级', '数量', '占比', '典型模块']
sum_rows = [
    ('I 级 致命', '4', '22%', '删除学生、密码安全、文件读取、账号唯一性'),
    ('II 级 严重', '8', '45%', '成绩录入、登录、输入校验、数据一致性'),
    ('III 级 一般', '3', '17%', '界面编码、平均分显示、空输入'),
    ('IV 级 轻微', '3', '16%', '代码规范、跨平台兼容性'),
    ('合计', '18', '100%', '—'),
]
for j, h in enumerate(sum_header):
    c = sum_table.cell(0, j).paragraphs[0]
    r = c.add_run(h)
    set_cn_font(r, '宋体', 11, True)
for i, row in enumerate(sum_rows):
    for j, v in enumerate(row):
        c = sum_table.cell(i+1, j).paragraphs[0]
        r = c.add_run(v)
        bold = (i == 4)
        set_cn_font(r, '宋体', 10.5, bold)

# ========== 四、缺陷详细记录 ==========
add_title('四、缺陷详细记录', 1)

defects = [
    # (编号, 标题, 等级, 模块, 重现步骤, 预期结果, 实际结果, 原因分析, 建议)
    ('BUG-001', '删除学生时误删下一位学生的登录账号',
     'I 级 致命', '管理员-删除学生',
     '1) 管理员依次添加学生 A、学生 B；2) 在菜单选择"删除学生"，输入学生 A 的学号并确认 yes。',
     '学生 A 的档案及其登录账号被删除，学生 B 账号不受影响。',
     '学生 A 的档案被删除，但学生 B 的登录账号被错误删除，导致学生 B 无法登录系统。',
     'admin.c deleteStudent() 中，先用数组前移覆盖了 students[index]，再用 students[index].name 去 users 数组匹配账号；此时 students[index] 已是学生 B 的数据，故删错账号。',
     '应在移动数组之前先保存被删学生的 name，再用该 name 匹配 users。'),

    ('BUG-002', '所有用户密码明文存储并在界面直接显示',
     'I 级 致命', '管理员-查看用户 / 数据文件',
     '1) 管理员登录后选择"查看所有用户"；2) 打开 users.dat 文件。',
     '密码应加密存储，界面不应显示明文密码。',
     'viewAllUsers() 直接 printf 出每个用户的 password 字段；users.dat 中密码以明文二进制保存，任何人可直接读取。',
     '系统未对密码做任何哈希或加密处理，UI 层也未做掩码。',
     '密码应加盐哈希存储；查看用户列表时密码列应隐藏或显示为 ***。'),

    ('BUG-003', '损坏的数据文件导致数组越界读取',
     'I 级 致命', '系统-数据加载',
     '1) 手动编辑或截断 users.dat / students.dat，使文件中记录数与实际数据不匹配；2) 启动程序。',
     '程序应检测文件异常并提示，不崩溃。',
     'loadAll() 直接 fread(&userCount,...) 后按该计数读 users 数组，未校验 fread 返回值与 MAX_USERS 上限，文件损坏时发生堆/栈越界，程序崩溃或读取垃圾数据。',
     'loadAll() 缺少对 userCount/studentCount 上限判断和 fread 返回值检查。',
     '读取后立即判断计数是否超过数组上限；检查 fread 返回值；失败时回退到空数据并警告。'),

    ('BUG-004', '学生登录账号使用姓名，重名学生互相冲突',
     'I 级 致命', '管理员-添加学生 / 学生登录',
     '1) 添加两位姓名均为"张三"的学生（学号不同）；2) 以学生身份用"张三"登录。',
     '不同学生应有独立账号（如用学号登录），互不影响。',
     '系统用学生姓名作为登录用户名，学号作为初始密码；findStudentByUsername() 按姓名匹配，永远只找到第一位张三，第二位张三登录后看到的是第一位张三的成绩。',
     'admin.c addStudent() 中 username=name，未检查姓名是否已存在；student.c viewMyScores() 按 name 查找。',
     '学生账号应使用学号（唯一）作为用户名；登录后按 stuNum 而非 name 定位。'),

    ('BUG-005', '可将密码修改为空',
     'II 级 严重', '修改密码',
     '1) 任意角色登录后选择"修改密码"；2) 输入原密码正确；3) 新密码处直接回车，确认密码处也直接回车。',
     '新密码不能为空，应提示错误。',
     '两次空串相等，密码被成功改为空串，之后登录只需输用户名即可进入。',
     'changePassword() 未对 newPass 做非空校验。',
     '增加新密码长度与非空校验，至少 6 位。'),

    ('BUG-006', '重复为同一门课录入成绩，产生重复课程记录',
     'II 级 严重', '教师-添加成绩',
     '1) 教师连续两次对同一学生执行"添加成绩"；2) 查看该学生成绩列表。',
     '同一门课应只保留一条成绩记录，重复添加应提示或更新。',
     'addScores() 每次都把 courseName 写为当前教师的任课课程并 append 到 courses 数组，不调用 findCourseIndex() 查重，导致同一课程出现多条记录，平均分被错误重复累加。',
     'teacher.c addScores() 缺少课程查重逻辑。',
     '添加前先 findCourseIndex，若已存在则提示是否覆盖。'),

    ('BUG-007', '修改/录入成绩后平均分不自动更新',
     'II 级 严重', '成绩管理',
     '1) 教师给学生录入新成绩；2) 学生端立即查看平均分。',
     '平均分应随成绩变动自动重算。',
     'addScores()、modifyScores() 都不更新 average 字段；学生看到的 average 仍是上次教师手动点"计算平均分"时的旧值。',
     'average 字段与 courses 数组之间没有维护一致性。',
     '成绩变动后立即在函数末尾重算 average。'),

    ('BUG-008', '登录失败 3 次后无锁定，可无限重试',
     'II 级 严重', '登录',
     '1) 连续输错密码 3 次；2) 程序返回主菜单后再次尝试登录。',
     '应在短时间内锁定账号或增加延时，防止暴力破解。',
     'login() 失败后仅 return 0，主循环立刻重新显示登录菜单，攻击者可无限次尝试，无延时、无锁定、无日志。',
     'login() 缺少失败计数持久化和冷却机制。',
     '增加连续失败锁定 N 分钟；LOGIN_ATTEMPTS 宏定义了但未使用。'),

    ('BUG-009', '超长输入导致缓冲区残留，后续读取串台',
     'II 级 严重', '通用输入',
     '1) 在用户名提示处输入超过 19 个字符；2) 观察密码提示。',
     '超长输入应被截断或提示重新输入，不影响下一项。',
     'safeReadString() 用 fgets(buf, size, stdin) 只读 size-1 个字符，剩余字符留在 stdin 缓冲区；下一次 fgets 会自动读到这串残留，导致密码等后续字段被"自动填好"，用户无法正常输入。',
     '未清空 stdin 残留数据。',
     'fgets 后若末尾不是换行符，循环读掉剩余字符直到换行。'),

    ('BUG-010', '学生姓名、学号允许为空',
     'II 级 严重', '管理员-添加学生',
     '1) 添加学生时在学号或姓名处直接回车。',
     '学号、姓名为必填项，不应允许空。',
     'safeReadString() 不检查空串，空学号、空姓名可成功入库，后续查找、显示出现异常。',
     'addStudent() 缺少空输入校验。',
     '读取后立即判断字符串长度，为空则要求重新输入。'),

    ('BUG-011', '用户表满时学生已入库但无法登录',
     'II 级 严重', '管理员-添加学生',
     '1) 将用户表加到 MAX_USERS；2) 再添加一名学生。',
     '要么全部成功，要么全部回滚，不应出现"有档案无账号"的中间态。',
     'students 数组先追加成功，随后 userCount 已满，只打印一句警告，学生档案已保存但登录账号未创建，该学生永远无法登录。',
     'addStudent() 中学生档案与用户账号非原子操作。',
     '应先检查 userCount 余量，再决定是否允许添加；或失败时回滚学生记录。'),

    ('BUG-012', '默认管理员口令 admin/admin',
     'II 级 严重', '系统初始化',
     '1) 首次启动程序。',
     '默认口令应强制首次登录修改，或使用随机初始密码。',
     'initSystem() 硬编码 admin/admin，且不强制修改，存在被直接登录风险。',
     'system.c initSystem() 写死默认账号。',
     '首次登录强制修改密码；或部署时生成随机初始密码。'),

    ('BUG-013', 'modifyScores 在课程数为 0 时先打印表头再判断',
     'III 级 一般', '教师-修改成绩',
     '1) 选择一名尚无任何成绩的学生。',
     '应先判断该生无成绩，直接提示。',
     'modifyScores() 先空循环打印表头，然后才判断 courseCount==0，界面上多出无意义的空表格。',
     '判断顺序不合理。',
     '将 courseCount==0 判断移到打印列表之前。'),

    ('BUG-014', '中文字符在 UTF-8 控制台显示为乱码',
     'III 级 一般', '界面显示',
     '1) 在 Windows Terminal（UTF-8 默认）或 chcp 65001 下运行程序。',
     '中文菜单应正常显示。',
     '源码以 GBK 保存，程序在 UTF-8 代码页下输出乱码（见 main.c 菜单）。',
     '未设置控制台代码页或使用宽字符 API。',
     '程序启动时调用 system("chcp 936")，或将源码与字符串统一为 UTF-8 + 宽字符输出。'),

    ('BUG-015', '学生端无任课教师列表为空时打印空行',
     'III 级 一般', '学生-查看成绩',
     '1) 学生无任何任课教师记录。',
     '应提示"暂无任课教师"。',
     'viewMyScores() 中 for 循环不执行，"任课教师："后直接换行，留下不完整提示。',
     '未处理 teacherCount==0 分支。',
     '增加空分支提示。'),

    ('BUG-016', 'extern 声明数组大小与定义不一致',
     'IV 级 轻微', '代码规范',
     '阅读 system.h 第 42 行。',
     '声明与定义应一致。',
     'system.h 中写 extern Student students[MAX_USERS]，而 system.c 中定义为 students[MAX_STUDENTS]，当前两宏恰好都为 100，但属潜在隐患。',
     '头文件复制粘贴未改。',
     '统一改为 MAX_STUDENTS。'),

    ('BUG-017', 'system("cls") 仅支持 Windows',
     'IV 级 轻微', '跨平台',
     '在 Linux/macOS 下编译运行。',
     '应兼容多平台。',
     '清屏命令写死为 cls，非 Windows 下命令报错但不影响主流程。',
     '未做平台判断。',
     '用 #ifdef _WIN32 区分 cls/clear。'),

    ('BUG-018', 'LOGIN_ATTEMPTS 宏定义后未使用',
     'IV 级 轻微', '代码规范',
     '阅读 system.h 与 login()。',
     '应使用宏而非硬编码。',
     'system.h 定义 LOGIN_ATTEMPTS 3，但 login() 中 attempts 硬编码为 3，宏成了死代码。',
     '重构时漏改。',
     '将 login() 中 attempts 初始化改为 LOGIN_ATTEMPTS。'),
]

for d in defects:
    bid, title, sev, module, steps, expect, actual, cause, suggest = d
    add_title(f'{bid}  {title}', 2)
    t = doc.add_table(rows=8, cols=2)
    t.style = 'Light Grid Accent 1'
    t.alignment = WD_TABLE_ALIGNMENT.CENTER
    rows_data = [
        ('严重程度', sev),
        ('所属模块', module),
        ('重现步骤', steps),
        ('预期结果', expect),
        ('实际结果', actual),
        ('原因分析', cause),
        ('修改建议', suggest),
        ('报告状态', '新建 / 待开发确认'),
    ]
    for i, (k, v) in enumerate(rows_data):
        c0 = t.cell(i, 0).paragraphs[0]
        r0 = c0.add_run(k)
        set_cn_font(r0, '宋体', 10.5, True)
        c1 = t.cell(i, 1).paragraphs[0]
        r1 = c1.add_run(v)
        set_cn_font(r1, '宋体', 10.5)
    # 设置列宽
    for row in t.rows:
        row.cells[0].width = Cm(2.8)
        row.cells[1].width = Cm(13.5)
    doc.add_paragraph()

# ========== 五、总结 ==========
add_title('五、测试总结与后续建议', 1)
add_body('本次静态审查 + 动态测试共发现 18 项缺陷，集中暴露了该系统在以下三方面的问题：')
add_bullet('数据一致性问题突出：删除学生误删他人账号、成绩重复录入、平均分不同步，说明开发过程中缺乏对关联数据的整体考虑。')
add_bullet('安全防护薄弱：密码明文、默认口令、无登录锁定、无输入校验，任一问题都可被轻易利用。')
add_bullet('异常处理缺失：文件损坏、缓冲区溢出、空输入等边界情况几乎未做防御，鲁棒性不足。')
add_body('建议下一阶段优先修复 4 项 I 级致命缺陷，再处理 II 级安全与数据一致性问题，并在第三、四阶段的黑盒/白盒测试中，'
         '围绕登录模块、学生删除流程、成绩录入流程设计更系统的测试用例。')

doc.save(r'F:\study\软件质量保证与测试\初步缺陷报告.docx')
print('OK')
