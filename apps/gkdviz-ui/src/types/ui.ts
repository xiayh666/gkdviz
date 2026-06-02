export type ThemeMode = 'latte'

export interface ThemeConfig {
  mode: ThemeMode
  glass: boolean
}

export interface ActivityItem {
  id: string
  icon: string
  label: string
}

export interface SideTab {
  id: string
  label: string
}

export interface BottomPanelSection {
  id: string
  title: string
  flex?: number
}

export interface BottomPanelConfig {
  direction: 'row' | 'column'
  sections: BottomPanelSection[]
}

export interface LayoutConfig {
  title: string
  subtitle: string
  modeLabel: string
  statusBar?: {
    tone?: 'default' | 'control' | 'running' | 'warn' | 'danger'
  }
  activityItems: ActivityItem[]
  sideTabs: SideTab[]
  bottomPanel: BottomPanelConfig
}

export interface CanvasToolbarItem {
  id: string
  icon: string
  label: string
}

export interface CanvasTopAction {
  id: string
  label: string
  icon?: string
  variant?: 'primary' | 'danger' | 'neutral'
}

export interface CanvasBottomAction {
  id: string
  label: string
  icon?: string
}

export interface CanvasNodeData {
  title: string
  category: '输入' | '处理' | '命令' | '输出'
  primaryInputs?: Array<{ key: string }>
  primaryOutputs?: Array<{ key: string }>
  fields: Array<
    | { type: 'manual'; key: string; value: string }
    | { type: 'socket'; key: string; value?: string; input?: boolean; output?: boolean }
  >
}

export interface PrototypeScene {
  tabs: string[]
  activeTab: string
  canvasLeftRail: CanvasToolbarItem[]
  canvasTopActions: CanvasTopAction[]
  canvasBottomActions: CanvasBottomAction[]
  zoomPercent: number
  runState: 'idle' | 'running'
}

export interface BackendConfigField {
  name: string
  value_type: string
  semantic: string
  unit: string
  default: boolean | number | string | null
  plottable: boolean
  loggable: boolean
  range: [number, number] | null
}

export interface BackendPortSchema {
  name: string
  value_type: string
  semantic: string
  unit: string
  plottable: boolean
  loggable: boolean
}

export interface BackendNodeSchema {
  type: string
  display_name: string
  category: string
  config: BackendConfigField[]
  inputs: BackendPortSchema[]
  outputs: BackendPortSchema[]
  runtime_type_id: string
}

export interface BackendNodeCatalogResponse {
  nodes: BackendNodeSchema[]
}

export interface GraphCompileResponse {
  ok: boolean
  node_count?: number
  edge_count?: number
  errors: Array<{ message: string }>
}

export interface GraphRunResponse {
  ok: boolean
  logs: string[]
  errors: Array<{ message: string }>
}

export interface GraphCompilePayload {
  nodes: Array<{
    id: string
    type: string
    config: Record<string, boolean | number | string | null>
  }>
  edges: Array<{
    from: string
    to: string
  }>
}

export interface EditorNodeConfigValueMap {
  [key: string]: boolean | number | string | null
}

export interface EditorCanvasNodeData {
  schema: BackendNodeSchema
  configValues: EditorNodeConfigValueMap
  onConfigChange?: (nodeId: string, fieldName: string, rawValue: string) => void
}
