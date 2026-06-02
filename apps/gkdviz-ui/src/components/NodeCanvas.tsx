import { useMemo, useState } from 'react'
import {
  addEdge,
  applyEdgeChanges,
  applyNodeChanges,
  Background,
  Controls,
  Handle,
  MiniMap,
  Position,
  ReactFlow,
  useConnection,
  useNodeConnections,
  type Connection,
  type Edge,
  type EdgeChange,
  type Node,
  type NodeChange,
  type NodeMouseHandler,
  type NodeProps
} from '@xyflow/react'
import { Button } from 'antd'
import type { EditorCanvasNodeData, PrototypeScene } from '../types/ui'
import { getAdapterFieldOptions } from '../utils/adapterOptions'
import {
  arePortsCompatible,
  findInputPort,
  findOutputPort,
  getPortStyleVars,
  resolveNodePorts
} from '../utils/portPresentation'

function BackendNode({ id, data, selected }: NodeProps<EditorCanvasNodeData>) {
  const connections = useNodeConnections({ id })
  const connection = useConnection()
  const resolvedPorts = useMemo(
    () => resolveNodePorts(data.schema, data.configValues),
    [data.schema, data.configValues]
  )
  const handleFieldChange = (fieldName: string, rawValue: string) => {
    data.onConfigChange?.(id, fieldName, rawValue)
  }

  function isValidConnectionTarget(inputHandleName: string) {
    if (!connection.inProgress || !connection.fromNode || !connection.fromHandle) {
      return false
    }
    if (connection.fromNode.id === id) {
      return false
    }
    const fromPort = findOutputPort(
      connection.fromNode.data.schema,
      connection.fromNode.data.configValues,
      connection.fromHandle.id
    )
    const toPort = findInputPort(data.schema, data.configValues, inputHandleName)
    return !!(fromPort && toPort && arePortsCompatible(fromPort, toPort))
  }

  function isValidConnectionSource(outputHandleName: string) {
    if (!connection.inProgress || !connection.fromNode || !connection.fromHandle) {
      return false
    }
    if (connection.fromNode.id === id) {
      return false
    }
    const fromPort = findOutputPort(data.schema, data.configValues, outputHandleName)
    const toPort = findInputPort(
      connection.fromNode.data.schema,
      connection.fromNode.data.configValues,
      connection.fromHandle.id
    )
    return !!(fromPort && toPort && arePortsCompatible(fromPort, toPort))
  }

  return (
    <div className={`comfy-node ${selected ? 'selected' : ''}`}>
      <div className="comfy-node-head">
        <div className="comfy-node-title">
          <span>{data.schema.display_name}</span>
          <span className="comfy-node-tag">{data.schema.category}</span>
        </div>
      </div>

      {resolvedPorts.inputs.length > 0 || resolvedPorts.outputs.length > 0 ? (
        <div className="comfy-primary-io">
          <div className="primary-io-col in">
            {resolvedPorts.inputs.map((port) => (
              <div className="primary-io-row" key={`in-${port.name}`}>
                <Handle
                  type="target"
                  id={port.name}
                  position={Position.Left}
                  style={getPortStyleVars(port)}
                  className={`node-handle in row-handle ${
                    connections.some((connection) => connection.targetHandle === port.name)
                      ? 'is-connected'
                      : ''
                  } ${
                    isValidConnectionTarget(port.name) ? 'is-connectable-target' : ''
                  }`}
                />
                <span className="io-name" style={getPortStyleVars(port)}>
                  <span className="io-name-main">{port.name}</span>
                </span>
              </div>
            ))}
          </div>
          <div className="primary-io-col out">
            {resolvedPorts.outputs.map((port) => (
              <div className="primary-io-row" key={`out-${port.name}`}>
                <span className="io-name" style={getPortStyleVars(port)}>
                  <span className="io-name-main">{port.name}</span>
                </span>
                <Handle
                  type="source"
                  id={port.name}
                  position={Position.Right}
                  style={getPortStyleVars(port)}
                  className={`node-handle out row-handle ${
                    connections.some((connection) => connection.sourceHandle === port.name)
                      ? 'is-connected'
                      : ''
                  } ${
                    isValidConnectionSource(port.name) ? 'is-connectable-target' : ''
                  }`}
                />
              </div>
            ))}
          </div>
        </div>
      ) : null}

      <div className="comfy-node-body">
        {data.schema.config.length > 0 ? (
          data.schema.config.map((field) => (
            <div className="comfy-node-field" key={field.name}>
              <label className="node-inline-label">{field.name}</label>
              {field.value_type === 'bool' ? (
                <select
                  className="node-inline-input"
                  value={String(data.configValues[field.name] ?? false)}
                  onMouseDown={(event) => event.stopPropagation()}
                  onClick={(event) => event.stopPropagation()}
                  onChange={(event) => handleFieldChange(field.name, event.target.value)}
                >
                  <option value="true">true</option>
                  <option value="false">false</option>
                </select>
              ) : getAdapterFieldOptions(field.name) ? (
                <select
                  className="node-inline-input"
                  value={String(data.configValues[field.name] ?? field.default ?? '')}
                  onMouseDown={(event) => event.stopPropagation()}
                  onClick={(event) => event.stopPropagation()}
                  onChange={(event) => handleFieldChange(field.name, event.target.value)}
                >
                  {getAdapterFieldOptions(field.name)?.map((option) => (
                    <option key={option.value} value={option.value}>
                      {option.label}
                    </option>
                  ))}
                </select>
              ) : (
                <input
                  className="node-inline-input"
                  value={String(data.configValues[field.name] ?? '')}
                  onMouseDown={(event) => event.stopPropagation()}
                  onClick={(event) => event.stopPropagation()}
                  onChange={(event) => handleFieldChange(field.name, event.target.value)}
                />
              )}
            </div>
          ))
        ) : (
          <div className="comfy-node-field">
            <span className="k">config</span>
            <span className="v">none</span>
          </div>
        )}
      </div>
    </div>
  )
}

interface Props {
  scene: PrototypeScene
  nodes: Node<EditorCanvasNodeData>[]
  edges: Edge[]
  selectedNodeId: string | null
  onNodesChange: (changes: NodeChange<Node<EditorCanvasNodeData>>[]) => void
  onEdgesChange: (changes: EdgeChange<Edge>[]) => void
  onConnect: (connection: Connection) => void
  onSelectionChange: (nodeId: string | null) => void
  onNodeFieldChange: (nodeId: string, fieldName: string, rawValue: string) => void
  onDeleteNode: (nodeId: string) => void
  onTopAction?: (actionId: string) => void
}

export function NodeCanvas({
  scene,
  nodes,
  edges,
  selectedNodeId,
  onNodesChange,
  onEdgesChange,
  onConnect,
  onSelectionChange,
  onNodeFieldChange,
  onDeleteNode,
  onTopAction
}: Props) {
  const [isInteracting, setIsInteracting] = useState(false)
  const [contextMenu, setContextMenu] = useState<{
    nodeId: string
    x: number
    y: number
  } | null>(null)
  const nodeTypes = useMemo(() => ({ backend: BackendNode }), [])
  const interactiveNodes = useMemo(
    () =>
      nodes.map((node) => ({
        ...node,
        data: {
          ...node.data,
          onConfigChange: (_nodeId: string, fieldName: string, rawValue: string) =>
            onNodeFieldChange(node.id, fieldName, rawValue),
        }
      })),
    [nodes, onNodeFieldChange]
  )

  function isValidConnection(connection: Connection) {
    if (
      !connection.source ||
      !connection.target ||
      !connection.sourceHandle ||
      !connection.targetHandle ||
      connection.source === connection.target
    ) {
      return false
    }

    const sourceNode = nodes.find((node) => node.id === connection.source)
    const targetNode = nodes.find((node) => node.id === connection.target)
    if (!sourceNode || !targetNode) {
      return false
    }

    const fromPort = findOutputPort(
      sourceNode.data.schema,
      sourceNode.data.configValues,
      connection.sourceHandle
    )
    const toPort = findInputPort(
      targetNode.data.schema,
      targetNode.data.configValues,
      connection.targetHandle
    )
    return !!(fromPort && toPort && arePortsCompatible(fromPort, toPort))
  }

  const onNodeContextMenu: NodeMouseHandler<Node<EditorCanvasNodeData>> = (event, node) => {
    event.preventDefault()
    onSelectionChange(node.id)
    setContextMenu({
      nodeId: node.id,
      x: event.clientX,
      y: event.clientY
    })
  }

  function closeContextMenu() {
    setContextMenu(null)
  }

  return (
    <section
      className={`canvas node-canvas ${isInteracting ? 'is-interacting' : ''}`}
      onClick={closeContextMenu}
    >
      <div className="canvas-workflow-bar">
        <Button className="pill">Live Graph</Button>
        <Button className="pill ghost">{nodes.length}</Button>
      </div>

      <div className="canvas-left-rail glass-capsule gpu-glass-panel">
        {scene.canvasLeftRail.map((item) => (
          <Button key={item.id} className="rail-item" type="text">
            <span className={`codicon codicon-${item.icon}`} />
            <span>{item.label}</span>
          </Button>
        ))}
      </div>

      <div className="canvas-top-actions">
        {scene.canvasTopActions.map((action) => (
          <Button
            key={action.id}
            className={`top-action ${action.variant ?? 'neutral'}`}
            onClick={() => onTopAction?.(action.id)}
          >
            {action.icon ? <span className={`codicon codicon-${action.icon}`} /> : null}
            <span>{action.label}</span>
          </Button>
        ))}
      </div>

      <div className="canvas-flow-layer">
        <ReactFlow
          nodes={interactiveNodes}
          edges={edges}
          nodeTypes={nodeTypes}
          isValidConnection={isValidConnection}
          defaultEdgeOptions={{
            animated: false,
            style: {
              strokeWidth: 2.4
            }
          }}
          connectionLineStyle={{
            strokeWidth: 2.4
          }}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          nodeOrigin={[0, 0]}
          defaultViewport={{ x: 0, y: 0, zoom: 1 }}
          minZoom={0.3}
          maxZoom={1.8}
          onMoveStart={() => setIsInteracting(true)}
          onMoveEnd={() => setIsInteracting(false)}
          onNodeDragStart={() => setIsInteracting(true)}
          onNodeDragStop={() => setIsInteracting(false)}
          onNodeClick={(_, node) => onSelectionChange(node.id)}
          onNodeContextMenu={onNodeContextMenu}
          onPaneClick={() => {
            onSelectionChange(null)
            closeContextMenu()
          }}
          fitView
        >
          <Background color="var(--ctp-surface1)" gap={24} size={1} />
          <MiniMap pannable zoomable nodeStrokeWidth={2} position="bottom-right" />
          <Controls showInteractive={false} position="bottom-left" />
        </ReactFlow>
      </div>

      <div className="canvas-bottom-actions">
        <span className="zoom-label">{scene.zoomPercent}%</span>
        <span className="zoom-label">{selectedNodeId ? `selected:${selectedNodeId}` : 'no selection'}</span>
        {scene.canvasBottomActions.map((action) => (
          <Button key={action.id} className="bottom-action" type="text" title={action.label}>
            {action.icon ? <span className={`codicon codicon-${action.icon}`} /> : null}
          </Button>
        ))}
      </div>

      {contextMenu ? (
        <div
          className="node-context-menu"
          style={{ left: contextMenu.x, top: contextMenu.y }}
          onClick={(event) => event.stopPropagation()}
        >
          <button
            className="node-context-item danger"
            type="button"
            onClick={() => {
              onDeleteNode(contextMenu.nodeId)
              closeContextMenu()
            }}
          >
            <span className="codicon codicon-trash" />
            <span>删除节点</span>
          </button>
        </div>
      ) : null}
    </section>
  )
}

export function applyEditorNodeChanges(
  changes: NodeChange<Node<EditorCanvasNodeData>>[],
  nodes: Node<EditorCanvasNodeData>[]
) {
  return applyNodeChanges(changes, nodes)
}

export function applyEditorEdgeChanges(changes: EdgeChange<Edge>[], edges: Edge[]) {
  return applyEdgeChanges(changes, edges)
}

export function addEditorEdge(connection: Connection, edges: Edge[]) {
  return addEdge({ ...connection, animated: false }, edges)
}
