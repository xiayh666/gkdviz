import type { Edge, Node } from '@xyflow/react'
import type { CanvasNodeData, PrototypeScene } from '../types/ui'

export const sceneConfig: PrototypeScene = {
  tabs: ['graph-main.json', 'motor-step-test.json'],
  activeTab: 'graph-main.json',
  canvasLeftRail: [
    { id: 'assets', icon: 'archive', label: '资源' },
    { id: 'nodes', icon: 'symbol-method', label: '节点' },
    { id: 'models', icon: 'symbol-variable', label: '模型' },
    { id: 'workflows', icon: 'repo', label: '工作流' },
    { id: 'templates', icon: 'library', label: '模板' }
  ],
  canvasTopActions: [
    { id: 'run', label: '运行', icon: 'play', variant: 'primary' },
    { id: 'queue', label: '队列 1', icon: 'list-ordered', variant: 'neutral' },
    { id: 'stop', label: '停止', icon: 'debug-stop', variant: 'danger' }
  ],
  canvasBottomActions: [
    { id: 'fit', label: '适应视图', icon: 'screen-full' },
    { id: 'center', label: '居中', icon: 'target' },
    { id: 'map', label: '小地图', icon: 'map' }
  ],
  zoomPercent: 54,
  runState: 'idle'
}

export const comfyNodes: Node<CanvasNodeData>[] = [
  {
    id: 'n1',
    type: 'comfy',
    position: { x: 80, y: 80 },
    data: {
      title: 'Load Checkpoint',
      category: '输入',
      primaryOutputs: [{ key: 'MODEL' }, { key: 'CLIP' }, { key: 'VAE' }],
      fields: [{ type: 'manual', key: 'ckpt_name', value: 'v1-5-pruned.safetensors' }]
    }
  },
  {
    id: 'n2',
    type: 'comfy',
    position: { x: 380, y: 20 },
    data: {
      title: 'CLIP Text Encode',
      category: '处理',
      primaryInputs: [{ key: 'clip' }],
      primaryOutputs: [{ key: 'CONDITIONING' }],
      fields: [{ type: 'manual', key: 'text', value: 'beautiful scenery, purple galaxy bottle' }]
    }
  },
  {
    id: 'n3',
    type: 'comfy',
    position: { x: 380, y: 230 },
    data: {
      title: 'CLIP Text Encode (Neg)',
      category: '处理',
      primaryInputs: [{ key: 'clip' }],
      primaryOutputs: [{ key: 'CONDITIONING' }],
      fields: [{ type: 'manual', key: 'text', value: 'text, watermark' }]
    }
  },
  {
    id: 'n4',
    type: 'comfy',
    position: { x: 760, y: 95 },
    data: {
      title: 'Velocity Command',
      category: '命令',
      primaryInputs: [{ key: 'model' }, { key: 'positive' }, { key: 'negative' }, { key: 'latent_image' }],
      primaryOutputs: [{ key: 'LATENT' }],
      fields: [
        { type: 'manual', key: 'mode', value: 'velocity' },
        { type: 'manual', key: 'target', value: '200 rpm' },
        { type: 'manual', key: 'accel_limit', value: '300 rpm/s' }
      ]
    }
  },
  {
    id: 'n5',
    type: 'comfy',
    position: { x: 1080, y: 115 },
    data: {
      title: 'Command Out',
      category: '输出',
      primaryInputs: [{ key: 'LATENT' }],
      fields: [{ type: 'manual', key: 'route', value: 'SafetyManager' }]
    }
  }
]

export const comfyEdges: Edge[] = [
  { id: 'e1', source: 'n1', sourceHandle: 'primary-out-CLIP', target: 'n2', targetHandle: 'primary-in-clip', animated: true, style: { strokeWidth: 2 } },
  { id: 'e2', source: 'n1', sourceHandle: 'primary-out-CLIP', target: 'n3', targetHandle: 'primary-in-clip', style: { strokeWidth: 2 } },
  { id: 'e3', source: 'n2', sourceHandle: 'primary-out-CONDITIONING', target: 'n4', targetHandle: 'primary-in-positive', animated: true, style: { strokeWidth: 2 } },
  { id: 'e4', source: 'n3', sourceHandle: 'primary-out-CONDITIONING', target: 'n4', targetHandle: 'primary-in-negative', style: { strokeWidth: 2 } },
  { id: 'e5', source: 'n4', sourceHandle: 'primary-out-LATENT', target: 'n5', targetHandle: 'primary-in-LATENT', animated: true, style: { strokeWidth: 2 } }
]
