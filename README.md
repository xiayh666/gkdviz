# gkdviz

`gkdviz` 是一个面向设备侧调试、图形化编排与运行时观测的可视化工作台。


## 项目定位

`gkdviz` 的设计方向是：
- 使用节点图表达调试与编排流程
- 使用统一类型系统约束节点之间的数据连接
- 使用后端运行时承接节点目录、图编译与执行逻辑
- 使用浏览器前端作为可视化交互与观测界面

在当前实现中，后端已经具备基于 C++26 reflection 的节点 schema 生成基础能力，并提供最小可用的图编译与单次执行路径。

## 核心能力

当前版本已经提供以下能力：

- 动态节点目录获取：`GET /node_catalog`
- 图结构校验与编译：`POST /graph/compile`
- 图的最小单次执行：`POST /graph/run`
- 前端节点部署、连线、参数编辑与执行触发
- 实例级动态端口类型节点：`SignalAdapter`
- 若干内置测试节点：
  - `NumberInput`
  - `BoolInput`
  - `TextInput`
  - `Comment`
  - `SignalAdapter`
  - `DoublePrinter`

其中，`DoublePrinter` 用于验证端到端链路：接收一个 `float64` 输入，在后端执行时输出其两倍结果。

## 系统架构

```text
Frontend
  - Node Canvas
  - Node Catalog
  - Inspector
  - Run / Compile UI
  - Activity Log
        |
        | HTTP
        v
Backend
  - Reflection Provider
  - NodeSchema Generator
  - NodeCatalog Service
  - GraphCompiler
  - Minimal Graph Runtime
        |
        v
Execution Result / Logs
```

## 目录结构

```text
apps/
  gkdviz-backend/        后端入口与 HTTP 服务
  gkdviz-ui/             前端界面（Vite + React + TypeScript）

core/
  algorithm/             算法包装与示例算法
  command/               命令通道框架
  graph/                 图配置、编译器、运行计划、slot 存储
  reflection/            反射、schema、端口类型系统
  safety/                安全框架
  telemetry/             遥测框架

doc/
  backend-canvas-foundation/
  domain-core-model/
  reflection-runtime-architecture/
```

## 快速开始

### 环境要求

后端：
- `xmake`
- 支持 `-freflection` 的实验性 C++26 工具链

前端：
- `Node.js`
- `npm`

### 构建后端

```bash
xmake -m debug
xmake
```

### 运行后端

```bash
xmake run gkdviz-backend
```

默认监听地址：

```text
http://127.0.0.1:8080
```

### 构建与运行前端

```bash
cd apps/gkdviz-ui
npm install
npm run dev
```

## HTTP 接口

### `GET /node_catalog`

返回当前后端注册的节点目录与对应的节点 schema。

### `POST /graph/compile`

接收前端提交的图配置，执行节点存在性、端口存在性、类型兼容性与图拓扑校验。

### `POST /graph/run`

接收前端提交的图配置，在完成编译校验后执行一次最小运行时流程，并返回运行结果与日志。

## 测试链路

当前推荐通过以下最小流程验证系统链路：

1. 在前端节点库中放置 `NumberInput`
2. 放置 `DoublePrinter`
3. 将 `NumberInput.out` 连到 `DoublePrinter.in`
4. 为 `NumberInput.value` 指定数值
5. 执行“运行当前画布”

预期结果：
- 前端日志显示 `DoublePrinter` 的执行结果
- 后端终端同步输出同一条运行日志

该链路用于验证以下能力同时成立：
- 节点目录可被前端发现
- 图可被前端正确序列化并提交
- 后端可完成图校验
- 后端可执行最小图运行逻辑
- 执行结果可返回前端

## SignalAdapter

`SignalAdapter` 是当前版本中首个实例级动态端口类型节点。

它的输入/输出端口类型并非完全静态，而是由节点实例配置决定：
- `input_value_type`
- `input_semantic`
- `input_unit`
- `output_value_type`
- `output_semantic`
- `output_unit`

前端与后端都会基于这些配置解析节点实例的有效端口类型

## 当前状态

当前版本适合用于：
- 验证节点目录生成与图编译流程
- 验证前后端节点图提交流程
- 验证最小图执行链路
- 验证动态端口类型与适配器设计


## 设计文档

- [reflection-runtime-architecture/design.md](/home/xiayh/Projects/gkdviz/doc/reflection-runtime-architecture/design.md)
- [reflection-runtime-architecture/goal.md](/home/xiayh/Projects/gkdviz/doc/reflection-runtime-architecture/goal.md)
- [reflection-runtime-architecture/plan.md](/home/xiayh/Projects/gkdviz/doc/reflection-runtime-architecture/plan.md)
- [reflection-runtime-architecture/log.md](/home/xiayh/Projects/gkdviz/doc/reflection-runtime-architecture/log.md)
- [todo.md](/home/xiayh/Projects/gkdviz/todo.md)
