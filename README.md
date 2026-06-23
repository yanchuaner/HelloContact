# HelloContact

📇 **个人通信录管理系统** — 一个跨平台、多版本、模块化的通信录管理项目。

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

## 项目简介

HelloContact 是我个人的技术学习汇总项目，旨在通过一个完整的通信录管理系统探索现代软件工程的全栈技术。

- **C++ 核心库** — 头文件化（header-only），OOP 继承体系，纯文本 CSV 数据存储
- **终端版** — 零依赖控制台程序，菜单驱动交互
- **桌面版** — Qt6 图形界面，含表格、图表、LLM 自然语言交互、主题切换

## 目录结构

```
HelloContact/
├── CMakeLists.txt              # 顶层构建配置
├── core/                       # C++ 核心库（所有版本共用）
│   ├── CMakeLists.txt
│   └── include/hello_contact/
│       └── core.hpp
├── apps/                       # 应用程序
│   ├── terminal/               # 终端版（零依赖）
│   └── desktop/                # Qt6 桌面版
├── scripts/                    # 构建/部署脚本
│   ├── build.bat               # Windows 一键构建
│   ├── build.sh                # Linux/macOS 一键构建
│   ├── run.bat                 # Windows 启动桌面版
│   └── run.ps1                 # PowerShell 启动桌面版
├── tests/                      # 单元测试（C++）
├── data/                       # 运行时数据文件
├── bindings/                   # 其他语言绑定（规划中）
│   ├── python/                 # pybind11 绑定
│   ├── go/                     # Go 实现
│   └── rust/                   # Rust 实现
├── web/                        # Web 部署（规划中）
│   ├── backend/                # REST API
│   └── frontend/               # Web 界面
├── llm/                        # 本地 LLM 集成（规划中）
├── docs/                       # 文档
└── cmake/                      # CMake 模块（预留）
```

## 构建指南

### 前置依赖

| 组件 | 版本 | 用途 |
|------|------|------|
| CMake | ≥ 3.16 | 构建系统 |
| C++ 编译器 | 支持 C++17 | 编译 |
| Qt6 (可选) | ≥ 6.0 | 桌面版 GUI（仅构建桌面版时需要） |
| MinGW (Windows) | — | Windows 构建 |

### 方式一：使用构建脚本（推荐）

```bash
# Windows — 完整构建（终端 + 桌面）
scripts\build.bat

# Windows — 仅构建终端版
set BUILD_DESKTOP=OFF && scripts\build.bat

# Linux/macOS — 完整构建
./scripts/build.sh

# Linux/macOS — 仅终端版
./scripts/build.sh terminal
```

### 方式二：直接使用 CMake

```bash
# 完整构建（终端 + 桌面）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 仅终端版（无需 Qt6，零依赖）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DHELLO_CONTACT_BUILD_DESKTOP=OFF
cmake --build build
```

### 方式三：运行桌面版（自动构建 + 启动）

```bash
# Windows
scripts\run.bat

# PowerShell
./scripts/run.ps1
```

### 运行产物

```bash
# 终端版
./build/apps/terminal/hello_contact_terminal

# 桌面版
./build/apps/desktop/hello_contact_desktop

# 单元测试
./build/tests/hello_contact_tests
```

## 项目架构

详细架构说明请参见 [docs/architecture.md](docs/architecture.md)。

## 开发路线图

### V1.0 ✅ — 核心功能（已完成）
- [x] C++ 核心库：OOP 类继承体系（Person → Classmate/Colleague/Friend/Relative）
- [x] 终端版：完整菜单交互，零外部依赖
- [x] Qt6 桌面版：表格展示、图表统计、暗色/亮色主题
- [x] 数据持久化：纯文本 CSV 按类型分文件存储
- [x] LLM AI 对话（DeepSeek API），自然语言控制通讯录

### V2.0 🔄 — 项目重构与基础设施（当前阶段）
- [x] Monorepo 规范化目录结构（core/apps/tests/scripts/data）
- [x] 多层 CMake 构建系统（支持 option 选择性构建）
- [x] 单元测试框架（26 个测试用例，全部通过）
- [ ] CI/CD 流水线（GitHub Actions）
- [ ] 运行时数据文件路径统一（data/ 目录）
- [ ] API 密钥改用环境变量注入（移除硬编码 sk-xxx）

### V3.0 🧩 — AI 交互抽象与多语言绑定（规划中，预计 6-8 周）
- [ ] **抽象 AI Provider 接口** — 支持 DeepSeek / OpenAI / 通义千问 多后端切换
- [ ] 终端版添加 `/ai` 命令，支持 AI 对话
- [ ] AI 对话记录存入联系人备注
- [ ] **pybind11 绑定** — 将 core 库导出为 Python 模块
- [ ] Python CLI 版本（基于 pybind11 绑定）
- [ ] GitHub Actions 自动构建 + 测试

### V4.0 🌐 — Web 全栈部署（规划中，预计 4-6 周）
- [ ] **FastAPI 后端** — 提供 RESTful API 封装核心业务
- [ ] **React/Vue 极简前端** — 联系人 CRUD 界面
- [ ] Docker 一键启动（docker-compose.yml）
- [ ] 数据文件云同步（SQLite 替代纯文本）

### V5.0 🤖 — 本地 LLM 深度集成（规划中，预计 8-12 周）
- [ ] **llama.cpp 子模块集成** — CPU/GPU 混合推理
- [ ] **RAG 检索增强** — 联系人向量化（Embedding + 语义搜索）
- [ ] 完全离线 AI 对话（无 API 依赖）
- [ ] 智能生日提醒 + 个性化祝福生成

## 数据文件规范

运行时数据文件统一存放于 `data/` 目录：

| 文件 | 用途 | 格式 |
| --- | --- | --- |
| `data/AddressBook1.txt` | 同学 | CSV: `name,Y,M,D,phone,email,school` |
| `data/AddressBook2.txt` | 同事 | CSV: `name,Y,M,D,phone,email,company` |
| `data/AddressBook3.txt` | 朋友 | CSV: `name,Y,M,D,phone,email,place` |
| `data/AddressBook4.txt` | 亲戚 | CSV: `name,Y,M,D,phone,email,relation` |
| `data/BirthdayEmail_*.txt` | 自动生成的生日贺信 | 纯文本 |

## 许可证

本项目采用 [MIT License](LICENSE)。
