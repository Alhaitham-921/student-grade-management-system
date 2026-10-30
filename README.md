# 学生成绩管理系统

使用 C 语言实现的 Windows 控制台学生成绩管理系统，提供管理员、教师、学生三种角色，以及学生档案、班级、成绩管理、CSV 成绩导入和统计等功能。

## 项目目录

| 目录 | 内容 |
| --- | --- |
| `zuoyezuihou/` | 当前维护的源码、Visual Studio 工程与回归测试 |
| `修复前备份/` | 修复前的源代码，供缺陷复现与对比 |
| `修复版手工测试/` | 手工测试源码副本、说明和 CSV 导入测试文件 |
| `文档制作/` | 修改前后对比文档的生成脚本 |

根目录还包含辅助 Python 脚本。账号和学生数据（`*.dat`）、课程文档、编译输出、IDE 缓存和重复压缩包保留在本地，不纳入版本控制。

## 编译运行

1. 在 Windows 上安装 Visual Studio 2022，选择“使用 C++ 的桌面开发”，安装 v143 工具集与 Windows SDK。
2. 打开 `zuoyezuihou/zuoyezuihou.sln`。
3. 选择 `Debug / x64` 并生成项目。
4. 将调试工作目录设为 `$(ProjectDir)`，然后按 Ctrl+F5 运行。

首次在空数据目录启动时，默认管理员为 `admin / admin`，登录后必须修改默认密码。详细功能、数据格式和注意事项见 [运行说明](zuoyezuihou/运行说明.md)。

## 回归测试

先生成 `Debug / x64`，然后在 `zuoyezuihou` 目录运行（需 Python 3）：

```powershell
python verify_fixes.py
python verify_classes.py
python verify_tables.py
```

测试在临时目录中运行。`verify_fixes.py` 最后一项旧数据迁移测试依赖本地 `users.dat` 和 `students.dat`，这两个文件不随仓库上传；全新克隆需自行提供相应测试数据才能运行这一项。手工测试请先编译 `修复版手工测试/zuoyezuihou.sln`，再阅读该目录的说明；其中提到的已有示例账号与学生数据仅存在于本地副本，CSV 文件本身不会建立这些账号。

## 版本维护

当前开发目录为 `zuoyezuihou/`，修复前备份用于保留原始对比依据。后续修改通过 Git 提交记录，并推送到本仓库的 `main` 分支。

仓库路径使用 `student-grade-management-system`，中文项目名称为“学生成绩管理系统”。
