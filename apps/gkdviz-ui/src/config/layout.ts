import type { LayoutConfig } from '../types/ui'

export const layoutConfig: LayoutConfig = {
  title: 'gkdviz / prototype / graph-main.json',
  subtitle: '画布优先 · VSCode 风格壳层',
  modeLabel: '控制模式（租约剩余：42s）',
  statusBar: {
    tone: 'default'
  },
  activityItems: [
    { id: 'explorer', icon: 'files', label: '资源管理器' },
    { id: 'graph', icon: 'symbol-method', label: '图编辑' },
    { id: 'settings', icon: 'settings-gear', label: '设置' },
    { id: 'telemetry', icon: 'graph', label: '遥测' },
    { id: 'testing', icon: 'beaker', label: '测试' }
  ],
  sideTabs: [
    { id: 'resources', label: '资源' },
    { id: 'nodes', label: '节点库' }
  ],
  bottomPanel: {
    direction: 'row',
    sections: [
      { id: 'logs', title: '运行日志', flex: 1 },
      { id: 'snapshot', title: '信号快照', flex: 1 }
    ]
  }
}
