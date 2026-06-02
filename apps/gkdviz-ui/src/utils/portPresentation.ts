import type { CSSProperties } from 'react'
import type { BackendNodeSchema, BackendPortSchema, EditorNodeConfigValueMap } from '../types/ui'

const semanticPalette: Record<string, string> = {
  time: 'var(--ctp-yellow)',
  velocity: 'var(--ctp-blue)',
  velocity_error: 'var(--ctp-sapphire)',
  control_command: 'var(--ctp-mauve)',
  debug_value: 'var(--ctp-green)',
  gain: 'var(--ctp-teal)',
  current: 'var(--ctp-red)',
  limit: 'var(--ctp-peach)'
}

export function getPortAccent(port: BackendPortSchema) {
  return (
    semanticPalette[port.semantic] ??
    (port.value_type === 'bool' ? 'var(--ctp-maroon)' : 'var(--ctp-sky)')
  )
}

export function getPortStyleVars(port: BackendPortSchema): CSSProperties {
  return {
    '--port-accent': getPortAccent(port)
  } as CSSProperties
}

function readStringConfig(config: EditorNodeConfigValueMap, key: string, fallback: string) {
  const value = config[key]
  return typeof value === 'string' ? value : fallback
}

function resolveSignalAdapterPort(
  port: BackendPortSchema,
  config: EditorNodeConfigValueMap,
  prefix: 'input' | 'output'
): BackendPortSchema {
  return {
    ...port,
    value_type: readStringConfig(config, `${prefix}_value_type`, port.value_type),
    semantic: readStringConfig(config, `${prefix}_semantic`, port.semantic),
    unit: readStringConfig(config, `${prefix}_unit`, port.unit)
  }
}

export function resolveNodePorts(
  schema: BackendNodeSchema,
  config: EditorNodeConfigValueMap
): Pick<BackendNodeSchema, 'inputs' | 'outputs'> {
  if (schema.type !== 'SignalAdapter') {
    return {
      inputs: schema.inputs,
      outputs: schema.outputs
    }
  }

  return {
    inputs: schema.inputs.map((port) => resolveSignalAdapterPort(port, config, 'input')),
    outputs: schema.outputs.map((port) => resolveSignalAdapterPort(port, config, 'output'))
  }
}

export function findInputPort(
  schema: BackendNodeSchema,
  config: EditorNodeConfigValueMap,
  handleName: string
) {
  return resolveNodePorts(schema, config).inputs.find((port) => port.name === handleName) ?? null
}

export function findOutputPort(
  schema: BackendNodeSchema,
  config: EditorNodeConfigValueMap,
  handleName: string
) {
  return resolveNodePorts(schema, config).outputs.find((port) => port.name === handleName) ?? null
}

export function arePortsCompatible(fromPort: BackendPortSchema, toPort: BackendPortSchema) {
  return (
    (fromPort.value_type === toPort.value_type ||
      fromPort.value_type === 'unknown' ||
      toPort.value_type === 'unknown') &&
    (fromPort.semantic === toPort.semantic ||
      fromPort.semantic === 'none' ||
      toPort.semantic === 'none') &&
    (fromPort.unit === toPort.unit || fromPort.unit === 'none' || toPort.unit === 'none')
  )
}
