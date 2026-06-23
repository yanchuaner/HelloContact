# HelloContact 发展路线图

> **时间单位**: 周 | **投入**: 每周 10~15 小时（业余）  
> **里程碑**: V1.0 → V2.0 → V3.0 | 当前阶段: **V2.0**

---

## 版本总览

| 版本 | 主题 | 周期 | 状态 |
|------|------|------|------|
| V1.0 | 核心功能 | 已完成 | ✅ 交付 |
| V2.0 | 项目重构与基础设施 | 第 1~2 周 | 🔄 进行中 |
| V3.0 | AI 交互抽象与多语言绑定 | 第 3~8 周 | 📋 规划 |
| V4.0 | Web 全栈部署 | 第 9~14 周 | 📋 规划 |
| V5.0 | 本地 LLM 深度集成 | 第 15~22 周 | 📋 规划 |

---

## V2.0 — 项目重构与基础设施（第 1~2 周）

### 目标
完成项目基础设施搭建，确保 CI/CD、测试框架、数据路径统一。

### 子任务

**第 1 周**

- [x] Monorepo 目录结构调整（core/apps/tests/scripts/data）
- [x] 多层 CMake 构建系统（header-only 核心库 + 终端/桌面/测试子项目）
- [x] 数据路径统一：saveToFile/loadFromFile 默认写 data/ 目录
- [ ] **GitHub Actions CI** — `.github/workflows/ci.yml`
  - ubuntu-latest (gcc-11 / clang-14)
  - windows-latest (msvc / mingw)
  - 构建终端版 + 运行单元测试
  - master / release 分支自动触发
- [ ] **单元测试增强**
  - 新增日期边界测试（闰年、月末）
  - 新增文件 I/O 异常测试（目录不可写、文件损坏）
  - 新增 CSV 解析鲁棒性测试（空行、多余空格、UTF-8 BOM）

**第 2 周**

- [ ] **环境变量 API Key** — 替换 MainWindow.hpp 中的硬编码 sk-xxx
  - main.cpp 启动时读取 `LLM_API_KEY` 环境变量
  - 若未设置，提示用户配置
  - LLM API URL 改为可配置
- [ ] **运行时数据迁移**
  - 根目录 AddressBook*.txt → data/ （已完成清理）
  - 更新 .gitignore 确保运行时数据不被提交
  - 提供迁移脚本 `scripts/migrate_data.sh`
- [ ] **错误处理增强**
  - 文件 I/O 异常使用异常安全 RAII 包装
  - 日志统一输出规范（时间戳 + 级别）

### 验收标准
- [ ] CI Pipeline 在 Ubuntu/Windows 上绿通过
- [ ] GitHub Secrets 不包含任何明文 API Key
- [ ] 26+ 测试覆盖核心路径和边界条件
- [ ] 删除根目录数据文件后，程序自动从 data/ 读写

---

## V3.0 — AI 交互抽象与多语言绑定（第 3~8 周）

### 目标
将 AI 能力从 Qt 桌面版解耦为通用接口，支持多模型后端切换；用 pybind11 将核心库导出为 Python 模块，实现 Python CLI 版本。

### 技术选型

| 组件 | 方案 | 理由 |
|------|------|------|
| AI Provider 抽象 | 策略模式 (`IAIProvider`) | 多后端可插拔（DeepSeek / OpenAI / 通义千问） |
| Python 绑定 | **pybind11** | C++ 绑定主流方案，header-only，CMake 原生集成 |
| Python HTTP | httpx + pydantic | 异步、类型安全 |
| Python CLI | typer / click | 零样板代码的 CLI 框架 |
| 序列化 | nlohmann/json (C++) | 现代 JSON 库，与 AI API 天然适配 |
| 对话存储 | SQLite (通过 sqlite3) | 轻量嵌入式，易于集成到 Python 绑定 |

### 子任务

**第 3 周 — AI Provider 抽象层**

- 创建 `core/include/hello_contact/ai_provider.hpp`
  - `class IAIProvider` — 纯虚基类
    - `virtual std::string chat(std::string_view prompt) = 0`
    - `virtual std::string name() const = 0`
  - `class DeepSeekProvider : public IAIProvider` — 现有 API 逻辑迁移至此
  - `class OpenAIProvider : public IAIProvider`
  - `class AIFactory` — 工厂方法，从环境变量创建 Provider
- 重构 `MainWindow::LLMAssistant` 以依赖注入方式使用 `IAIProvider`
- 终端版添加 `/ai <query>` 命令，复用同一 AI Provider 接口

**第 4 周 — 对话持久化**

- 创建 `core/include/hello_contact/conversation.hpp`
  - `struct Message { std::string role; std::string content; int64_t timestamp; }`
  - `class ConversationLog` — 管理联系人的对话历史
- 对话存为 `data/conversations/{contact_name}.jsonl`
- 联系人详情界面显示对话历史

**第 5~6 周 — pybind11 绑定**

- 在根 CMakeLists.txt 中添加 `HELLO_CONTACT_BUILD_PYTHON` option
- 创建 `bindings/python/CMakeLists.txt`
  - find_package(pybind11 REQUIRED)
  - pybind11_add_module(hello_contact_py python_bindings.cpp)
  - target_link_libraries(hello_contact_py PRIVATE hello_contact_core)
- 创建 `bindings/python/python_bindings.cpp`
  - `py::class_<Person>` — 导出抽象基类
  - `py::class_<Classmate, Person>` — 导出子类
  - `py::class_<AddressBookManager>` — 导出管理器
  - 属性 getter/setter 映射 `getName()` → `.name`
  - `.def("search_by_name", &AddressBookManager::searchByName)`
- 创建 `bindings/python/hello_contact/__init__.py`
  - Python 包装层，提供 Pythonic API

**第 7 周 — Python CLI 版本**

- 创建 `bindings/python/cli/main.py`
  - typer 菜单，实现 CRUD 命令
  - 示例: `hc add --name "张三" --type 同学 --birth 2000-01-01`
- 创建 `bindings/python/requirements.txt`
- 创建 `bindings/python/pyproject.toml` — poetry/pip 可安装

**第 8 周 — Python 绑定测试与文档**

- `bindings/python/tests/test_core.py` — pytest 测试（镜像 C++ 测试用例）
- `docs/api/python.md` — Python API 使用文档
- 示例 notebook `docs/examples/hello_contact_demo.ipynb`

### 验收标准
- [ ] 终端版 `/ai` 命令可用，支持 DeepSeek / OpenAI 切换
- [ ] pybind11 绑定编译通过，Python 可 `import hello_contact`
- [ ] Python CLI `hc` 命令可执行 CRUD 操作
- [ ] 对话历史可读写并关联特定联系人
- [ ] 测试覆盖 C++ 绑定层和 Python 包装层

### 技术风险与应对
| 风险 | 等级 | 应对 |
|------|------|------|
| pybind11 跨编译器 ABI 不兼容 | 🟡 中 | 使用静态链接 + 要求编译器一致 |
| AI API 调用超时（国产网络环境） | 🟡 中 | 客户端超时 30s + 重试机制 |
| CMake + pybind11 与现有 MinGW 冲突 | 🟢 低 | 优先在 Linux CI 中验证 Python 绑定 |

---

## V4.0 — Web 全栈部署（第 9~14 周）

### 目标
构建 RESTful API + 极简前端，实现 Docker 一键部署。

### 技术选型

| 组件 | 方案 | 理由 |
|------|------|------|
| Web 框架 | **FastAPI** (Python) | 自动 OpenAPI 文档、异步支持、pydantic 验证 |
| ORM | SQLAlchemy + aiosqlite | 异步 SQLite，迁移方便 |
| 前端 | **React** (Vite + TypeScript) | 生态成熟、开发效率高 |
| 部署 | Docker + docker-compose | 一键启动，环境隔离 |
| 容器化 | multi-stage build | 分离构建/运行阶段，减小镜像 |

### 子任务

**第 9~10 周 — FastAPI 后端**

- 创建 `bindings/python/hello_contact/api/` 包
  - `main.py` — FastAPI app 入口
  - `router/contacts.py` — CRUD 路由
  - `router/search.py` — 搜索路由
  - `router/ai.py` — AI 对话路由
  - `schemas.py` — pydantic 请求/响应模型
  - `dependencies.py` — 依赖注入（Factory → AddressBookManager → SQLite）
- API 端点（自动生成于 `/docs`）：
  - `GET /contacts` — 分页列表
  - `POST /contacts` — 创建
  - `PUT /contacts/{id}` — 更新
  - `DELETE /contacts/{id}` — 删除
  - `GET /contacts/search?q=` — 搜索
  - `GET /contacts/birthdays` — 未来 5 天生日的联系人
  - `POST /ai/chat` — AI 对话

**第 11 周 — 极简前端**

- `web/frontend/` — Vite + React + TypeScript
  - `src/App.tsx` — 主页面
  - `src/components/ContactTable.tsx` — 联系人表格
  - `src/components/ContactForm.tsx` — 录入/编辑表单
  - `src/components/AIChat.tsx` — AI 对话面板
  - `src/api/client.ts` — API 客户端（fetch/axios）
- 打包配置：Vite 输出到 `web/frontend/dist/`

**第 12 周 — API 集成与端到端测试**

- FastAPI 挂载前端静态文件
- Playwright E2E 测试（开联系人、搜索、AI 对话）
- API 性能基准测试（locust）

**第 13~14 周 — Docker 部署**

- `Dockerfile` — multi-stage:
  - Stage 1: 构建前端 (node:20-alpine)
  - Stage 2: 构建 Python 依赖 (python:3.12-slim)
  - Stage 3: 合并运行
- `docker-compose.yml`
  - `api` 服务 (暴露 8000)
  - `data` 卷 (持久化 SQLite + CSV)
- `docs/deployment.md` — 部署文档

### 验收标准
- [ ] `docker compose up` 一键启动前后端
- [ ] 浏览器可完成联系人 CRUD、搜索、AI 对话
- [ ] Playwright E2E 测试全部通过
- [ ] API 文档可访问（`/docs`）

### 技术风险与应对
| 风险 | 等级 | 应对 |
|------|------|------|
| AI API 在容器内无法访问宿主机网络 | 🟡 中 | Docker host network mode 或 API 代理配置 |
| 前端联系人大量渲染性能 | 🟢 低 | 虚拟滚动（react-window） |
| SQLite 并发写入 | 🟢 低 | WAL 模式 + 连接池 |

---

## V5.0 — 本地 LLM 深度集成（第 15~22 周）

### 目标
引入 llama.cpp 实现完全离线 AI 推理，构建 RAG 检索增强管道，实现联系人语义搜索和智能生日祝福。

### 技术选型

| 组件 | 方案 | 理由 |
|------|------|------|
| LLM 推理 | **llama.cpp** (GGML) | C++ 原生、CPU/GPU 混合推理、MIT 许可 |
| 子模块集成 | git submodule → `third_party/llama.cpp` | 版本锁定、便于交叉编译 |
| Embedding | llama.cpp embedding API | 统一推理引擎，减少依赖 |
| 向量存储 | FAISS (CPU) | 轻量、高效、C++ 原生接口 |
| RAG Pipeline | 自实现 (core/include/hello_contact/rag_engine.hpp) | 避免引入重型框架 |

### 子任务

**第 15~16 周 — llama.cpp 子模块引入**

- `git submodule add https://github.com/ggerganov/llama.cpp.git third_party/llama.cpp`
- 创建 `llm/CMakeLists.txt`（实际构建版本）
  - `add_subdirectory(third_party/llama.cpp)`
  - 链接 `common` 和 `llama` 库
  - BLAS 加速选项（OpenBLAS / CUDA）
- 创建 `llm/src/llm_agent.cpp`
  - `class LocalLLMAgent` — 封装 llama.cpp 推理
  - `std::string chat(std::string_view prompt, LLMParams params)`
  - 模型下载脚本 `scripts/download_model.sh`（从 Hugging Face 拉取 1B~3B GGUF）

**第 17 周 — 模型量化与性能测试**

- 集成量化参数选择（Q4_K_M → Q8_0）
- 内存占用基准：
  - 1B 模型 ~ 1GB RAM (Q4)
  - 3B 模型 ~ 2.5GB RAM (Q4)
- 推理速度基准：token/s（CPU 多线程 + BLAS）
- 编写 `docs/llm/performance.md` — 性能报告

**第 18~19 周 — Embedding + 向量存储**

- 创建 `core/include/hello_contact/vector_store.hpp`
  - `struct EmbeddedContact` — `{ person_id, embedding, metadata }`
  - `class VectorStore` — 封装 FAISS 索引
    - `void add_contact(const Person &p)` — 生成 Embedding 并添加
    - `std::vector<Person*> semantic_search(std::string_view query, int k=5)`
    - 序列化索引到 `data/vector_index.faiss`
- Embedding 生成复用 llama.cpp 的 embedding API
- 新增定时任务：联系人数据变更时自动重建索引

**第 20~21 周 — RAG 检索增强管道**

- 创建 `core/include/hello_contact/rag_engine.hpp`
  - `class RAGEngine`
  - 接收用户问题 → Embedding 检索 top-K 联系人 → 构建 Prompt → LLM 推理 → 返回
  - 示例场景：
    - "帮我找生日在 6 月、性格活泼的朋友" → 语义检索
    - "给张三写一段温馨的生日祝福" → RAG + LLM 生成
- 集成到终端版 `/ai` 和桌面版 AI 面板

**第 22 周 — 离线验证与文档**

- 完全离线运行验证（断网 + 纯 CPU）
- `docs/llm/setup.md` — 模型下载、量化选择、启动参数
- `docs/llm/rag.md` — RAG 架构说明

### 验收标准
- [ ] 完全离线运行，不依赖任何外部 API
- [ ] 语义搜索联系人（"在北京的程序员朋友"）返回正确结果
- [ ] AI 生成生日祝福（接入 RAG 增强）
- [ ] 1B 模型在 8GB RAM 机器上流畅运行
- [ ] 向量索引支持增量更新

### 技术风险与应对
| 风险 | 等级 | 应对 |
|------|------|------|
| 模型文件过大（GGUF 3B ~ 2GB） | 🟡 中 | 提供自动下载脚本 + 断点续传 |
| 纯 CPU 推理速度慢（< 5 token/s） | 🔴 高 | 默认使用 Q4_K_M 量化 + 4 线程 + 小模型 (1B) |
| FAISS 与 MinGW 兼容性 | 🟡 中 | Linux 优先开发 + WSL 作为 Windows 备选 |
| Embedding 模型准确度不足 | 🟡 中 | 提供模型切换配置，支持 BGE-M3 / text2vec 等 |

---

## 附录

### 依赖版本锁定

```text
# V2.0 基础构建工具
cmake >= 3.16
gcc >= 10 / clang >= 12 / msvc >= 2022

# V3.0 pybind11
pybind11 >= 2.12
Python >= 3.10

# V4.0 Web
Python >= 3.11
Node.js >= 20
Docker >= 24

# V5.0 LLM
llama.cpp (HEAD of main, submodule pinned)
FAISS >= 1.8
```

### GitHub Actions 矩阵

| Job | OS | Compiler | 构建 |
|-----|----|----------|------|
| linux-terminal | ubuntu-22.04 | gcc-11 | terminal + tests |
| linux-clang | ubuntu-22.04 | clang-16 | terminal + tests |
| windows-terminal | windows-2022 | msvc 2022 | terminal + tests |
| windows-mingw | windows-2022 | mingw (msys2) | terminal + tests |
| python-bindings (V3.0+) | ubuntu-22.04 | gcc-11 | terminal + pybind11 + pytest |
| docker-build (V4.0+) | ubuntu-22.04 | — | docker build + compose test |
