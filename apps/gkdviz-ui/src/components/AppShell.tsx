import { useEffect, useMemo, useState } from 'react'
import type { Connection, Edge, EdgeChange, Node, NodeChange } from '@xyflow/react'
import { Button, Input, Select } from 'antd'
import { compileGraph, defaultBaseUrl, fetchNodeCatalog, runGraph } from '../api/backend'
import type {
  BackendConfigField,
  BackendNodeSchema,
  EditorCanvasNodeData,
  GraphCompilePayload,
  GraphCompileResponse,
  GraphRunResponse,
  LayoutConfig,
  PrototypeScene
} from '../types/ui'
import {
  addEditorEdge,
  applyEditorEdgeChanges,
  applyEditorNodeChanges,
  NodeCanvas
} from './NodeCanvas'
import { getAdapterFieldOptions } from '../utils/adapterOptions'
import {
  getPortStyleVars,
  resolveNodePorts
} from '../utils/portPresentation'

interface Props {
  layout: LayoutConfig
  scene: PrototypeScene
}

const resourceItems = [
  ['backend_catalog', 'HTTP'],
  ['graph_compile', 'POST'],
  ['front_left', '执行器'],
  ['imu_main', '传感器']
]

function formatCompileSummary(result: GraphCompileResponse | null) {
  if (!result) {
    return '尚未编译图'
  }
  if (result.ok) {
    return `编译成功 · ${result.node_count ?? 0} 节点 / ${result.edge_count ?? 0} 连线`
  }
  return `编译失败 · ${result.errors[0]?.message ?? 'unknown_error'}`
}

function toDefaultConfigValues(schema: BackendNodeSchema) {
  return Object.fromEntries(schema.config.map((field) => [field.name, field.default ?? null]))
}

function coerceFieldValue(field: BackendConfigField, raw: string) {
  if (field.value_type === 'bool') {
    return raw === 'true'
  }
  if (field.value_type.startsWith('int') || field.value_type.startsWith('float')) {
    const value = Number(raw)
    return Number.isFinite(value) ? value : field.default ?? 0
  }
  return raw
}

function buildCompilePayload(nodes: Node<EditorCanvasNodeData>[], edges: Edge[]): GraphCompilePayload {
  return {
    nodes: nodes.map((node) => ({
      id: node.id,
      type: node.data.schema.type,
      config: node.data.configValues
    })),
    edges: edges
      .filter((edge) => edge.sourceHandle && edge.targetHandle)
      .map((edge) => ({
        from: `${edge.source}.${edge.sourceHandle}`,
        to: `${edge.target}.${edge.targetHandle}`
      }))
  }
}

export function AppShell({ layout, scene }: Props) {
  const [activeTool, setActiveTool] = useState(layout.activityItems[0]?.id ?? 'explorer')
  const [activeSideTab, setActiveSideTab] = useState(layout.sideTabs[0]?.id ?? 'resources')
  const [backendUrl, setBackendUrl] = useState(defaultBaseUrl)
  const [catalog, setCatalog] = useState<BackendNodeSchema[]>([])
  const [catalogError, setCatalogError] = useState<string>('')
  const [isLoadingCatalog, setIsLoadingCatalog] = useState(false)
  const [isCompiling, setIsCompiling] = useState(false)
  const [compileResult, setCompileResult] = useState<GraphCompileResponse | null>(null)
  const [runResult, setRunResult] = useState<GraphRunResponse | null>(null)
  const [activityLog, setActivityLog] = useState<string[]>([
    '[ui] 等待连接 backend /node_catalog',
    '[ui] 从左侧节点库点击即可部署节点到画布'
  ])
  const [nodes, setNodes] = useState<Node<EditorCanvasNodeData>[]>([])
  const [edges, setEdges] = useState<Edge[]>([])
  const [selectedNodeId, setSelectedNodeId] = useState<string | null>(null)
  const [spawnCounter, setSpawnCounter] = useState(0)

  const activity = useMemo(() => layout.activityItems, [layout.activityItems])
  const panelTemplate =
    layout.bottomPanel.direction === 'row'
      ? layout.bottomPanel.sections.map((s) => `${s.flex ?? 1}fr`).join(' ')
      : layout.bottomPanel.sections.map((s) => `${s.flex ?? 1}fr`).join(' ')
  const statusTone = layout.statusBar?.tone ?? 'default'
  const selectedCanvasNode = nodes.find((node) => node.id === selectedNodeId) ?? null
  const selectedResolvedPorts = useMemo(
    () =>
      selectedCanvasNode
        ? resolveNodePorts(selectedCanvasNode.data.schema, selectedCanvasNode.data.configValues)
        : { inputs: [], outputs: [] },
    [selectedCanvasNode]
  )

  useEffect(() => {
    function handleKeyDown(event: KeyboardEvent) {
      const target = event.target as HTMLElement | null
      if (
        target instanceof HTMLInputElement ||
        target instanceof HTMLTextAreaElement ||
        target instanceof HTMLSelectElement
      ) {
        return
      }
      if ((event.key === 'Delete' || event.key === 'Backspace') && selectedNodeId) {
        event.preventDefault()
        deleteNodeById(selectedNodeId)
      }
    }

    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [selectedNodeId])

  async function refreshCatalog() {
    setIsLoadingCatalog(true)
    setCatalogError('')
    try {
      const payload = await fetchNodeCatalog(backendUrl)
      setCatalog(payload.nodes)
      setActivityLog((current) => [
        `[api] GET /node_catalog -> ${payload.nodes.length} nodes`,
        ...current
      ].slice(0, 8))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'catalog_fetch_failed'
      setCatalogError(message)
      setActivityLog((current) => [`[api] GET /node_catalog failed: ${message}`, ...current].slice(0, 8))
    } finally {
      setIsLoadingCatalog(false)
    }
  }

  useEffect(() => {
    void refreshCatalog()
  }, [])

  function deployNode(schema: BackendNodeSchema) {
    const nextCount = spawnCounter + 1
    setSpawnCounter(nextCount)
    const nextId = `${schema.type.toLowerCase()}_${nextCount}`
    const nextNode: Node<EditorCanvasNodeData> = {
      id: nextId,
      type: 'backend',
      position: { x: 120 + (nextCount % 4) * 260, y: 120 + Math.floor(nextCount / 4) * 180 },
      data: {
        schema,
        configValues: toDefaultConfigValues(schema)
      }
    }
    setNodes((current) => [...current, nextNode])
    setSelectedNodeId(nextId)
    setActiveSideTab('nodes')
    setActivityLog((current) => [`[canvas] deploy ${schema.type} -> ${nextId}`, ...current].slice(0, 8))
  }

  function updateNodeField(nodeId: string, fieldName: string, rawValue: string) {
    setNodes((current) =>
      current.map((node) => {
        if (node.id !== nodeId) {
          return node
        }
        const field = node.data.schema.config.find((item) => item.name === fieldName)
        if (!field) {
          return node
        }
        return {
          ...node,
          data: {
            ...node.data,
            configValues: {
              ...node.data.configValues,
              [fieldName]: coerceFieldValue(field, rawValue)
            }
          }
        }
      })
    )
  }

  async function runCompile() {
    const payload = buildCompilePayload(nodes, edges)
    setIsCompiling(true)
    try {
      const result = await compileGraph(payload, backendUrl)
      setCompileResult(result)
      setActivityLog((current) => [
        `[api] POST /graph/compile -> ${result.ok ? 'ok' : 'failed'}`,
        ...current
      ].slice(0, 8))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'graph_compile_failed'
      const failedResult: GraphCompileResponse = { ok: false, errors: [{ message }] }
      setCompileResult(failedResult)
      setActivityLog((current) => [`[api] POST /graph/compile failed: ${message}`, ...current].slice(0, 8))
    } finally {
      setIsCompiling(false)
    }
  }

  async function runGraphOnce() {
    const payload = buildCompilePayload(nodes, edges)
    setIsCompiling(true)
    try {
      const result = await runGraph(payload, backendUrl)
      setRunResult(result)
      setActivityLog((current) => [
        `[api] POST /graph/run -> ${result.ok ? 'ok' : 'failed'}`,
        ...result.logs.map((line) => `[run] ${line}`),
        ...current
      ].slice(0, 8))
    } catch (error) {
      const message = error instanceof Error ? error.message : 'graph_run_failed'
      const failedResult: GraphRunResponse = { ok: false, logs: [], errors: [{ message }] }
      setRunResult(failedResult)
      setActivityLog((current) => [`[api] POST /graph/run failed: ${message}`, ...current].slice(0, 8))
    } finally {
      setIsCompiling(false)
    }
  }

  function deleteNodeById(nodeId: string) {
    if (!nodeId) {
      return
    }
    setNodes((current) => current.filter((node) => node.id !== nodeId))
    setEdges((current) =>
      current.filter((edge) => edge.source !== nodeId && edge.target !== nodeId)
    )
    setActivityLog((current) => [`[canvas] delete ${nodeId}`, ...current].slice(0, 8))
    setSelectedNodeId((current) => (current === nodeId ? null : current))
  }

  function deleteSelectedNode() {
    if (!selectedNodeId) {
      return
    }
    deleteNodeById(selectedNodeId)
  }

  function handleTopAction(actionId: string) {
    if (actionId === 'run') {
      void runGraphOnce()
      return
    }
    if (actionId === 'stop') {
      deleteSelectedNode()
    }
  }

  function onNodesChange(changes: NodeChange<Node<EditorCanvasNodeData>>[]) {
    setNodes((current) => applyEditorNodeChanges(changes, current))
  }

  function onEdgesChange(changes: EdgeChange<Edge>[]) {
    setEdges((current) => applyEditorEdgeChanges(changes, current))
  }

  function onConnect(connection: Connection) {
    setEdges((current) => addEditorEdge(connection, current))
  }

  const renderPanelContent = (id: string) => {
    if (id === 'logs') {
      return (
        <>
          {activityLog.map((line, index) => (
            <div className="log-line" key={`${index}-${line}`}>
              {line}
            </div>
          ))}
        </>
      )
    }
    if (id === 'snapshot') {
      const payload = buildCompilePayload(nodes, edges)
      return (
        <>
          <div className="log-line">{formatCompileSummary(compileResult)}</div>
          <div className="log-line">canvas nodes: {nodes.length}</div>
          <div className="log-line">canvas edges: {edges.length}</div>
          <div className="log-line">graph payload nodes: {payload.nodes.length}</div>
          {catalogError ? <div className="log-line err">catalog error: {catalogError}</div> : null}
        </>
      )
    }
    return <div className="log-line">未定义分区：{id}</div>
  }

  return (
    <div className="app">
      <header className="title">
        <span className="crumb">{layout.title}</span>
        <span className="subtitle">{layout.subtitle}</span>
        <span className="mode">{layout.modeLabel}</span>
      </header>

      <nav className="activity">
        {activity.map((item) => (
          <Button
            key={item.id}
            type="text"
            className={`icon ${activeTool === item.id ? 'active' : ''}`}
            onClick={() => setActiveTool(item.id)}
            title={item.label}
          >
            <span className={`codicon codicon-${item.icon}`} />
          </Button>
        ))}
      </nav>

      <aside className="side glass-capsule-surface gpu-glass-panel">
        <div className="side-tabs">
          {layout.sideTabs.map((tab) => (
            <Button
              key={tab.id}
              type="text"
              className={`side-tab ${activeSideTab === tab.id ? 'active' : ''}`}
              onClick={() => setActiveSideTab(tab.id)}
            >
              {tab.label}
            </Button>
          ))}
        </div>
        <div className="side-body">
          {activeSideTab === 'resources' ? (
            <>
              <div className="section-title">资源</div>
              {resourceItems.map(([name, type]) => (
                <div className="tree-item" key={name}>
                  {name}
                  <small>{type}</small>
                </div>
              ))}
            </>
          ) : (
            <>
              <div className="section-title">后端节点库</div>
              <div className="backend-toolbar">
                <Input
                  size="small"
                  value={backendUrl}
                  onChange={(event) => setBackendUrl(event.target.value)}
                  placeholder="backend url"
                />
                <Button size="small" onClick={() => void refreshCatalog()} loading={isLoadingCatalog}>
                  刷新
                </Button>
              </div>
              {catalog.map((node) => (
                <button
                  key={node.type}
                  className="tree-item tree-button"
                  onClick={() => deployNode(node)}
                  type="button"
                >
                  {node.display_name}
                  <small>{node.category}</small>
                </button>
              ))}
              {catalog.length === 0 ? <div className="empty-state">未拉到后端节点</div> : null}
              {catalogError ? <div className="log-line err">{catalogError}</div> : null}
            </>
          )}
        </div>
      </aside>

      <main className="editor">
        <header className="editor-header glass-capsule-surface gpu-glass-panel">
          <div className="tabs">
            {scene.tabs.map((tab) => (
              <div key={tab} className={`tab ${tab === scene.activeTab ? 'active' : ''}`}>
                {tab}
              </div>
            ))}
          </div>
        </header>
        <section className="editor-body">
          <NodeCanvas
            scene={scene}
            nodes={nodes}
            edges={edges}
            selectedNodeId={selectedNodeId}
            onNodesChange={onNodesChange}
            onEdgesChange={onEdgesChange}
            onConnect={onConnect}
            onSelectionChange={setSelectedNodeId}
            onNodeFieldChange={updateNodeField}
            onDeleteNode={deleteNodeById}
            onTopAction={handleTopAction}
          />
        </section>
      </main>

      <aside className="inspector glass-capsule gpu-glass-panel">
        <div className="inspector-head">
          属性检查器 · {selectedCanvasNode?.data.schema.display_name ?? '未选择画布节点'}
        </div>
        <div className="inspector-body">
          <label className="field-label">节点 ID</label>
          <Input value={selectedCanvasNode?.id ?? ''} readOnly />

          <label className="field-label">节点类型</label>
          <Input value={selectedCanvasNode?.data.schema.type ?? ''} readOnly />

          <label className="field-label">分类</label>
          <Input value={selectedCanvasNode?.data.schema.category ?? ''} readOnly />

          <div className="field-section">
            <div className="info-list">
              <div className="info-item">
                <span className="info-key">运行时类型</span>
                <span className="info-value">{selectedCanvasNode?.data.schema.runtime_type_id || 'builtin'}</span>
              </div>
              <div className="info-item">
                <span className="info-key">输入端口数</span>
                <span className="info-value">{selectedResolvedPorts.inputs.length}</span>
              </div>
              <div className="info-item">
                <span className="info-key">输出端口数</span>
                <span className="info-value">{selectedResolvedPorts.outputs.length}</span>
              </div>
            </div>
            <div className="port-list">
              <div className="port-list-title">输入端口</div>
              {selectedResolvedPorts.inputs.length ? (
                selectedResolvedPorts.inputs.map((port) => (
                  <div className="port-item" key={`input-${port.name}`} style={getPortStyleVars(port)}>
                    <div className="port-item-head">
                      <span className="port-name-main">{port.name}</span>
                      <span className="port-direction in">IN</span>
                    </div>
                    <div className="info-list compact">
                      <div className="info-item compact">
                        <span className="info-key">Type</span>
                        <span className="info-value">{port.value_type}</span>
                      </div>
                      <div className="info-item compact">
                        <span className="info-key">Semantic</span>
                        <span className="info-value">{port.semantic}</span>
                      </div>
                      <div className="info-item compact">
                        <span className="info-key">Unit</span>
                        <span className="info-value">{port.unit}</span>
                      </div>
                    </div>
                  </div>
                ))
              ) : (
                <div className="info-empty">无输入端口</div>
              )}
            </div>
            <div className="port-list">
              <div className="port-list-title">输出端口</div>
              {selectedResolvedPorts.outputs.length ? (
                selectedResolvedPorts.outputs.map((port) => (
                  <div className="port-item" key={`output-${port.name}`} style={getPortStyleVars(port)}>
                    <div className="port-item-head">
                      <span className="port-name-main">{port.name}</span>
                      <span className="port-direction out">OUT</span>
                    </div>
                    <div className="info-list compact">
                      <div className="info-item compact">
                        <span className="info-key">Type</span>
                        <span className="info-value">{port.value_type}</span>
                      </div>
                      <div className="info-item compact">
                        <span className="info-key">Semantic</span>
                        <span className="info-value">{port.semantic}</span>
                      </div>
                      <div className="info-item compact">
                        <span className="info-key">Unit</span>
                        <span className="info-value">{port.unit}</span>
                      </div>
                    </div>
                  </div>
                ))
              ) : (
                <div className="info-empty">无输出端口</div>
              )}
            </div>
          </div>

          {selectedCanvasNode?.data.schema.config.length ? (
            <div className="field-section">
              <div className="field-section-title">可编辑配置</div>
              {selectedCanvasNode.data.schema.config.map((field) => (
                <div key={field.name}>
                  <label className="field-label">
                    {field.name} · {field.value_type}
                  </label>
                  {field.value_type === 'bool' ? (
                    <Select
                      value={String(selectedCanvasNode.data.configValues[field.name] ?? false)}
                      options={[
                        { value: 'true', label: 'true' },
                        { value: 'false', label: 'false' }
                      ]}
                      onChange={(value) => updateNodeField(selectedCanvasNode.id, field.name, value)}
                    />
                  ) : getAdapterFieldOptions(field.name) ? (
                    <Select
                      value={String(selectedCanvasNode.data.configValues[field.name] ?? field.default ?? '')}
                      options={getAdapterFieldOptions(field.name) ?? []}
                      onChange={(value) => updateNodeField(selectedCanvasNode.id, field.name, value)}
                    />
                  ) : (
                    <Input
                      value={String(selectedCanvasNode.data.configValues[field.name] ?? '')}
                      onChange={(event) =>
                        updateNodeField(selectedCanvasNode.id, field.name, event.target.value)
                      }
                    />
                  )}
                </div>
              ))}
            </div>
          ) : null}

          <div className="spacer" />
          <Button type="primary" block loading={isCompiling} onClick={() => void runCompile()}>
            编译当前画布
          </Button>
          <Button block loading={isCompiling} onClick={() => void runGraphOnce()}>
            运行当前画布
          </Button>
          <Button block danger disabled={!selectedCanvasNode} onClick={deleteSelectedNode}>
            删除选中节点
          </Button>
        </div>
      </aside>

      <section
        className={`panel glass-capsule-surface gpu-glass-panel panel-${layout.bottomPanel.direction}`}
        style={
          layout.bottomPanel.direction === 'row'
            ? { gridTemplateColumns: panelTemplate }
            : { gridTemplateRows: panelTemplate }
        }
      >
        {layout.bottomPanel.sections.map((section, index) => (
          <div className="panel-col" key={section.id}>
            <div className="section-title">{section.title}</div>
            {renderPanelContent(section.id)}
            {index < layout.bottomPanel.sections.length - 1 ? <div className="panel-divider" /> : null}
          </div>
        ))}
      </section>

      <footer className={`status glass-capsule-surface gpu-glass-panel status-${statusTone}`}>
        <span>{catalogError ? 'backend 未连接' : 'backend 已连接'}</span>
        <span>catalog: {catalog.length} 节点</span>
        <span>canvas: {nodes.length} 节点 / {edges.length} 连线</span>
        <span>{formatCompileSummary(compileResult)}</span>
        <span>{runResult ? `run: ${runResult.ok ? 'ok' : 'failed'}` : 'run: idle'}</span>
      </footer>
    </div>
  )
}
