# 个人通信录管理系统

基于 **C++17** 的个人通信录管理工具，提供两种运行方式：

| 方式 | 特点 | 适用场景 |
|------|------|---------|
| **UI 版** (Qt6 桌面程序) | 图形界面，集成 LLM 自然语言交互 | 日常使用、功能展示 |
| **终端版** (纯控制台) | 零依赖，仅需 g++ 编译，终端菜单交互 | 答辩演示、快速验证核心逻辑 |

核心业务代码 `Core.hpp` 两个版本共用，OOP 继承体系、数据存储逻辑完全一致。

---

## 快速启动

### UI 版（双击运行）

项目根目录下的 **`run.bat`** 双击即可启动 Qt6 图形界面。

### 终端版（VS Code 终端）

```bash
cd "C:/Users/lucky dog/Desktop/manager"
g++ -std=c++17 main_terminal.cpp -o terminal.exe
./terminal.exe
```

### 首次启动

程序自动加载已存在的 `AddressBook1~4.txt` 数据文件。文件不存在则空启动，可正常添加联系人，点击"保存数据"后自动创建。

---

## 功能操作

### 传统按钮 / 菜单

| 按钮 | 功能 | 操作步骤 |
|------|------|----------|
| **添加联系人** | 录入新联系人 | 填写弹窗表单 → 选择类别 → 确认 |
| **编辑选中联系人** | 修改已有联系人（姓名和生日只读） | 选中一行 → 点击按钮 → 修改 → 确认 |
| **删除选中联系人** | 删除联系人 | 选中一行 → 点击按钮 → 二次确认 |
| **查询联系人** | 按姓名搜索 | 输入姓名 → 日志区显示详细信息 |
| **按姓名排序** | 按拼音/英文排序 | 点击即排序，表格自动刷新 |
| **按生日排序** | 按日期升序 | 点击即排序，表格自动刷新 |
| **统计本月生日** | 统计指定月份 | 选择月份 → 点击统计 → 日志区显示名单 |
| **扫描 5 天内生日** | 自动生成贺信 | 点击 → 扫描 → 生成 `.txt` 贺信到项目目录 |
| **保存数据** | 持久化到 CSV | 点击 → 写入 `AddressBook1~4.txt` |
| **清空日志** | 清空日志面板 | 点击 → 清除所有历史日志 |

### AI 自然语言交互

在底部 AI 输入框输入中文指令，LLM 自动识别意图并执行。

**支持的操作：**

| 用户说 | 效果 |
|--------|------|
| 查一下张三 | 按姓名搜索 |
| 查一下电话号码为13800000000的人 | 按电话搜索 |
| 搜索邮箱abc@example.com | 按邮箱搜索 |
| 姓李的人有哪些 | 按姓氏搜索 |
| 查一下1999年出生的人 | 按出生年份搜索 |
| 添加一个同学叫李四，生日2000年1月1日，电话13800138000 | 添加联系人 |
| 删除张三 | 按姓名删除 |
| 把张三的电话改成123456 | 修改联系人 |
| 按姓名排序 / 按生日排序 | 排序 |
| 五月份生日的同学 | 统计某月（可加类别过滤） |
| 找出生日相同的同学 | 查找生日相同的人（可加类别过滤） |
| 扫描五天内生日 | 自动生成祝福邮件文件 |
| 列出所有联系人 / 列出所有朋友 | 列出全部 / 按类别列出 |
| 各类别分别有多少人 | 各类别数量统计 |
| 保存数据 | 存盘 |
| 你好 | 闲聊回复 |

---

## 从源码构建

### 前置依赖

- **MSYS2**（已安装于 `C:\msys64`）
- **Qt6** — `pacman -S --noconfirm mingw-w64-x86_64-qt6-base`
- **GCC** — `pacman -S --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-gcc-libs`
  - 需要 GCC ≥ 16.1.0（Qt6 6.11 要求）
- **CMake** — 已安装于 `C:\cmake\bin\cmake.exe`

> **注意**：Qt6 包是 GCC 16.1.0 编译的，GCC 版本过低会出现 `无法定位程序输入点` 错误。运行 `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gcc-libs` 升级 GCC。

### 构建步骤

```bash
cd "C:/Users/lucky dog/Desktop/manager"
export PATH=$PATH:/c/msys64/mingw64/bin
C:\cmake\bin\cmake.exe -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
C:\cmake\bin\cmake.exe --build build
```

### VS Code 构建

**Ctrl+Shift+B** → 选择 **"CMake: configure + build"**。

---

## 文件结构

```
manager/
├── run.bat                   ← 双击启动程序（UI 版）
├── terminal.exe              终端版程序（编译后生成）
├── main_terminal.cpp         终端版入口（纯 C++，无需 Qt）
├── CMakeLists.txt            Qt6 CMake 构建配置
├── Core.hpp                  Date、Person 多态继承体系、AddressBookManager
├── MainWindow.hpp            MainWindow + InputDialog + LLMAssistant 声明
├── MainWindow.cpp            界面实现、信号/槽连接、LLM 路由、main()
│
├── AddressBook1.txt          同学数据 (CSV: name,Y,M,D,phone,email,school)
├── AddressBook2.txt          同事数据
├── AddressBook3.txt          朋友数据
├── AddressBook4.txt          亲戚数据
│
└── build/
    ├── manager.exe           UI 版编译产物
    ├── *.dll                 Qt6 + GCC 运行时
    └── platforms/
        └── qwindows.dll      Windows 平台插件
```

---

## 数据格式

纯文本 CSV，UTF-8 编码，每行一条记录：

| 文件 | 类型 | 格式 |
|------|------|------|
| `AddressBook1.txt` | 同学 | `name,Y,M,D,phone,email,school` |
| `AddressBook2.txt` | 同事 | `name,Y,M,D,phone,email,company` |
| `AddressBook3.txt` | 朋友 | `name,Y,M,D,phone,email,place` |
| `AddressBook4.txt` | 亲戚 | `name,Y,M,D,phone,email,relation` |

可直接用记事本编辑，重启程序后自动加载。

---

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│  Core.hpp (Model)                                           │
│  Date · Person 继承体系 · AddressBookManager                │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  Classmate  Colleague  Friend  Relative                │ │
│  │  └── Person (virtual display/formatForFile)            │ │
│  └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│  MainWindow.cpp (View + Controller)                         │
│  ┌──────────────────────────────┐  ┌──────────────────────┐  │
│  │  QMenuBar (5 菜单)           │  │ LLMAssistant          │  │
│  │  按钮面板                    │  │  └→ DeepSeek API     │  │
│  │  QTableWidget + 日志面板     │  │  └→ onAiCommand 路由 │  │
│  └──────────────────────────────┘  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### LLM 交互流程

```
用户输入 → LLMAssistant::sendQuery() → POST DeepSeek API →
  JSON 意图 → onReplyFinished() 解析 → onAiCommand() 路由 →
    AddressBookManager 方法（cout 输出）→ captureStdout 捕获 → UI 日志显示
```

所有业务方法通过 `std::cout` 输出，UI 层通过 `captureStdout` 重定向到日志面板，后端无侵入。

### 终端版 (`main_terminal.cpp`) 详解

`main_terminal.cpp` 是系统中**剥离了 Qt 和 LLM 大模型**的纯 C++17 独立实现文件，适用于无 GUI 环境、答辩演示或快速核心逻辑验证。编译该文件无需配置复杂的系统环境：

```bash
g++ -std=c++17 main_terminal.cpp -o main_terminal.exe
```

**设计亮点与重构优化：**

1. **零外部依赖：** 使用 `<iostream>`、`<vector>`、`<fstream>` 等纯标准库实现，跨平台兼容性极强。
2. **面向对象与多态：** 基于抽象基类 `Person` 构建了 `Classmate`、`Colleague` 等子类，通过虚函数实现多态展显 (`display`) 和格式化存取 (`formatForFile`)。
3. **简单工厂模式 (Factory Pattern)：** 
   使用静态工厂类 `PersonFactory` 统一管理联系人层级的实例化逻辑。将庞大的 `switch-case` 逻辑从按行读取解析 (`loadFromFile`) 和键盘输入 (`inputPerson`) 的过程中解耦出来，极大遵守了“开闭原则”。
4. **RAII 内存管理：** 核心数据结构使用 `std::vector<std::unique_ptr<Person>>`，告别内存泄漏困扰，指针生命周期全自动管理。
5. **健壮的容错机制 (Robustness)：** 
   在文本反序列化（例如将 CSV 字符 `std::stoi` 转为数字）时加入了完整的 `try-catch` 异常处理；当某行数据遭破坏时（如非法逗号或非数字字符），程序将输出警告并自动跳过，保障主库数据不受影响，杜绝系统崩溃。
6. **交互循环体验优化：** 对缓冲区的残余换行符读取进行了严谨的清空规避，并在暂停继续 `pauseAndContinue()` 的逻辑中进行了独立终端的底层优化，防止因为缓冲池意外而引起的阻塞卡顿现象。

**终端基本操作循环：**
```
启动时 loadFromFile() 
  ↓
显示菜单 showMenu() ↔ 接收指令 inputInt()
  ↓
调用 AddressBookManager 进行查找/修改排序等操作
  ↓
退出时 saveToFile() 统一落盘
```

### AI 映射表

| action | 后端调用 |
|--------|---------|
| `search` | `searchByName()` → `display()` |
| `searchByPhone` | `searchByPhone()` → `display()` |
| `searchByEmail` | `searchByEmail()` → `display()` |
| `searchBySurname` | `searchBySurname()` → 遍历 `display()` |
| `searchByYear` | `searchByBirthYear()` → 遍历 `display()` |
| `add` | `addPerson(make_unique<...>)` |
| `delete` | `deletePerson()` |
| `edit` | `deletePerson()` + `addPerson()` |
| `sortByName` / `sortByBirth` | `sortByName()` / `sortByBirthDate()` |
| `statMonth` | `countByBirthMonth()` 或 联合过滤 |
| `findSameBirthday` | `findSameBirthday()` 分组输出 |
| `scanBirthdays` | `checkBirthdaysAndSendEmails()` |
| `listAll` / `list` | `showAllBrief()` |
| `listByType` | `showByType()` |
| `typeCount` | `countByType()` |
| `save` | `saveToFile()` |
| `reply` | 纯文本输出 |

---

## 常见问题

### 运行提示"无法定位程序输入点"

Qt6 DLL 是 GCC 16.1.0 编译的，但 GCC 运行时版本过低。在 MSYS2 中运行：

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gcc-libs
```

然后重新构建，重新复制 DLL。

### LLM 请求失败

- 检查 `LLM_API_KEY` 和 `LLM_API_URL` 宏定义（`MainWindow.hpp`）
- 确保 `build/` 目录下存在 `Qt6Network.dll` 和 OpenSSL DLL（`libcrypto-3-x64.dll`、`libssl-3-x64.dll`）
- 确保 `build/tls/` 目录下存在 Qt6 TLS 后端插件
