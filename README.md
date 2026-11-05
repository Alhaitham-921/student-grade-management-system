# 学生成绩管理系统

C 语言实现的 Windows 控制台系统，面向 Visual Studio 2022 / v143。

## 构建和测试

安装 VS2022“使用 C++ 的桌面开发”（含 Windows SDK）和 Python 3.11+，在仓库根目录运行：

```powershell
.\build-test.ps1
# 仅生成程序
.\build-test.ps1 -BuildOnly
# 其他配置
.\build-test.ps1 -Configuration Release -Platform Win32
```

脚本通过 vswhere 定位 VS2022、初始化工具环境并生成 `build/<时间>/zuoyezuihou.exe`。默认使用嵌入对象的编译调试信息、禁用链接 PDB 和增量链接，避免本机 PDB 服务版本冲突；Visual Studio 内常规调试配置保留。测试专用故障注入 exe 单独生成在 `fault-tests/`，正式 exe 不包含该入口。

测试日志、匿名样本、失败现场保留在 `test-results/<时间>/`，不依赖真实数据，不自动清理。五组回归覆盖账号、旧格式迁移、班级、表格、CSV、统计、保存失败和进程中断恢复。GitHub Actions 对 Debug/Release × x64/Win32 运行同一入口。

也可打开 `zuoyezuihou/zuoyezuihou.sln` 生成，工作目录设为 `$(ProjectDir)`。手动运行单个脚本时可用 `SGMS_EXE` 指定 exe；`verify_storage.py` 还需 `SGMS_FAULT_EXE`，建议使用一键入口。

## 功能

- 三角色登录、密码加盐 PBKDF2-HMAC-SHA256、首次管理员强制改密、进程内登录冷却。
- 管理员创建/重命名/停用班级、查看成员、分班/转班；学生只查看班级。
- 编辑学生姓名、教师显示姓名和授课关系；教师登录名保持稳定。
- 学号/部分姓名搜索、班级筛选、分页表格。
- 教师在授权范围内录入和修改任教科目成绩；按班级查询和统计，排名同分并列（1、1、3）。
- CSV 导入预览确认、逐行错误清单和整批提交；按班级和任教科目导出，支持 UTF-8 BOM、引号和逗号。
- 版本化 DAT 数据集提交、完整迁移副本、单实例写入保护、操作日志。

默认管理员 `admin / admin`，首次登录必须改为至少 6 字节密码；学生初始密码为学号。详见 [运行说明](zuoyezuihou/运行说明.md)。

## 数据和目录

- `zuoyezuihou/`：当前维护源码、工程和测试。
- `修复前备份/`：原始备份，不改动。
- `修复版手工测试/`：历史手工测试副本；新增功能请使用当前维护工程，历史演示账号并非随仓库提供。
- `文档制作/`：历史对比材料脚本。

新数据保存于运行工作目录 `.sgms/`；备份必须完整复制该目录。原 `users.dat`、`students.dat`、`classes.dat` 仅作为首次升级输入，升级后不再更新，不能把它们当作最新备份。原文件和 `legacy-backup-*` 完整副本均保留。版本目录不会自动清理。

容量仍为 100 用户（含管理员、教师、学生）、100 学生、50 班级；每名学生最多 10 门成绩和 10 位教师。SQLite、学期和考试批次属于后续工作。

不提交业务数据、日志、编译产物、IDE 缓存或凭据。项目仓库为 `https://github.com/Alhaitham-921/student-grade-management-system`。
