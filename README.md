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
├── bindings/                   # 其他语言绑定
│   ├── python/                 # pybind11 绑定（规划中）
│   ├── go/                     # Go 实现（规划中）
│   └── rust/                   # Rust 实现（规划中）
├── web/                        # Web 部署（规划中）
│   ├── backend/                # REST API
│   └── frontend/               # Web 界面
├── llm/                        # 本地 LLM 集成（规划中）
│   └── src/
├── tests/                      # 单元测试
├── scripts/                    # 构建/部署脚本
├── docs/                       # 文档
└── data/                       # 示例数据文件
```

## 构建指南

### 前置依赖

| 组件 | 版本 | 用途 |
|------|------|------|
| CMake | ≥ 3.16 | 构建系统 |
| C++ 编译器 | 支持 C++17 | 编译 |
| Qt6 (可选) | ≥ 6.0 | 桌面版 GUI |
| MinGW (Windows) | — | Windows 构建 |

### 构建

```bash
# 完整构建（终端 + 桌面）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 仅终端版（无需 Qt）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DHELLO_CONTACT_BUILD_DESKTOP=OFF
cmake --build build
```

### 运行

```bash
# 终端版
./build/apps/terminal/hello_contact_terminal

# 桌面版
./build/apps/desktop/hello_contact_desktop
```

## 开发路线图

### V1.0 ✅ — 核心功能（已完成）
- [x] C++ 核心库：OOP 类继承体系
- [x] 终端版：完整菜单交互
- [x] Qt6 桌面版：表格、图表、AI 对话
- [x] 数据持久化：纯文本 CSV 存储

### V2.0 🔄 — 项目重构（进行中）
- [x] Monorepo 规范化目录结构
- [ ] 完善单元测试
- [ ] CI/CD 流水线
- [ ] Python 绑定（pybind11）

### V3.0 📋 — 多语言与扩展（规划中）
- [ ] Python 完整实现
- [ ] Go 实现
- [ ] Rust 实现
- [ ] Web 后端 API

### V4.0 🚀 — 云端与 AI（规划中）
- [ ] Web 前端界面
- [ ] 云端部署
- [ ] 本地 LLM 集成（llama.cpp）
- [ ] AI 对话助手

## 许可证

本项目采用 [MIT License](LICENSE)。
