# HelloContact 架构设计

## 架构概览

```
┌──────────────────────────────────────────────────┐
│                  HelloContact                     │
├──────────────┬─────────────────┬─────────────────┤
│  Terminal    │    Desktop      │   Future Apps   │
│  (C++ CLI)   │   (Qt6 GUI)    │   (Python/Go)   │
├──────────────┴─────────────────┴─────────────────┤
│              Core Library (C++17)                 │
│        AddressBookManager, Person hierarchy       │
├──────────────────────────────────────────────────┤
│              Data Layer (CSV files)               │
│         AddressBook1~4.txt, BirthdayEmail*        │
└──────────────────────────────────────────────────┘
```

## 核心组件

### Core Library (`core/`)
- header-only 库，零外部依赖
- `Person` 抽象基类 → `Classmate`, `Colleague`, `Friend`, `Relative`
- `AddressBookManager` — 联系人增删改查、排序、统计、文件 I/O
- `Date` — 日期结构，含生日检查

### Terminal App (`apps/terminal/`)
- 纯 C++ 标准库，无需 Qt
- 菜单驱动交互
- 适用于无图形界面的环境

### Desktop App (`apps/desktop/`)
- Qt6 Widgets 图形界面
- 表格展示、图表统计
- LLM 自然语言交互（DeepSeek API）
- 暗色/亮色主题切换

## 构建策略

顶层 CMake 通过 option 控制子项目构建：
- `HELLO_CONTACT_BUILD_TERMINAL` — 终端版（默认 ON）
- `HELLO_CONTACT_BUILD_DESKTOP` — 桌面版（默认 ON，需 Qt6）
- `HELLO_CONTACT_BUILD_TESTS` — 单元测试（默认 ON）
- `HELLO_CONTACT_BUILD_LLM` — LLM 模块（默认 OFF）
