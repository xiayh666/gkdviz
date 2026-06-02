export interface SelectOption {
  value: string
  label: string
}

const adapterFieldOptions: Record<string, SelectOption[]> = {
  input_value_type: [
    { value: 'unknown', label: 'unknown' },
    { value: 'bool', label: 'bool' },
    { value: 'int32', label: 'int32' },
    { value: 'int64', label: 'int64' },
    { value: 'float32', label: 'float32' },
    { value: 'float64', label: 'float64' },
    { value: 'string', label: 'string' },
    { value: 'bytes', label: 'bytes' }
  ],
  output_value_type: [
    { value: 'unknown', label: 'unknown' },
    { value: 'bool', label: 'bool' },
    { value: 'int32', label: 'int32' },
    { value: 'int64', label: 'int64' },
    { value: 'float32', label: 'float32' },
    { value: 'float64', label: 'float64' },
    { value: 'string', label: 'string' },
    { value: 'bytes', label: 'bytes' }
  ],
  input_semantic: [
    { value: 'none', label: 'none' },
    { value: 'gain', label: 'gain' },
    { value: 'velocity', label: 'velocity' },
    { value: 'velocity_error', label: 'velocity_error' },
    { value: 'velocity_command', label: 'velocity_command' },
    { value: 'current', label: 'current' },
    { value: 'control_command', label: 'control_command' },
    { value: 'time', label: 'time' },
    { value: 'debug_value', label: 'debug_value' }
  ],
  output_semantic: [
    { value: 'none', label: 'none' },
    { value: 'gain', label: 'gain' },
    { value: 'velocity', label: 'velocity' },
    { value: 'velocity_error', label: 'velocity_error' },
    { value: 'velocity_command', label: 'velocity_command' },
    { value: 'current', label: 'current' },
    { value: 'control_command', label: 'control_command' },
    { value: 'time', label: 'time' },
    { value: 'debug_value', label: 'debug_value' }
  ],
  input_unit: [
    { value: 'none', label: 'none' },
    { value: 'second', label: 'second' },
    { value: 'rpm', label: 'rpm' },
    { value: 'ampere', label: 'ampere' },
    { value: 'normalized', label: 'normalized' }
  ],
  output_unit: [
    { value: 'none', label: 'none' },
    { value: 'second', label: 'second' },
    { value: 'rpm', label: 'rpm' },
    { value: 'ampere', label: 'ampere' },
    { value: 'normalized', label: 'normalized' }
  ]
}

export function getAdapterFieldOptions(fieldName: string): SelectOption[] | null {
  return adapterFieldOptions[fieldName] ?? null
}
