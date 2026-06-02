import type {
  BackendNodeCatalogResponse,
  GraphCompilePayload,
  GraphCompileResponse,
  GraphRunResponse
} from '../types/ui'

const defaultBaseUrl = 'http://127.0.0.1:8080'

function withBase(path: string, baseUrl = defaultBaseUrl) {
  return `${baseUrl}${path}`
}

export async function fetchNodeCatalog(baseUrl?: string): Promise<BackendNodeCatalogResponse> {
  const response = await fetch(withBase('/node_catalog', baseUrl))
  if (!response.ok) {
    throw new Error(`node_catalog_${response.status}`)
  }
  return response.json() as Promise<BackendNodeCatalogResponse>
}

export async function compileGraph(
  payload: GraphCompilePayload,
  baseUrl?: string
): Promise<GraphCompileResponse> {
  const response = await fetch(withBase('/graph/compile', baseUrl), {
    method: 'POST',
    headers: {
      'content-type': 'application/json'
    },
    body: JSON.stringify(payload)
  })

  const result = (await response.json()) as GraphCompileResponse
  if (!response.ok && result.ok) {
    result.ok = false
  }
  return result
}

export async function runGraph(
  payload: GraphCompilePayload,
  baseUrl?: string
): Promise<GraphRunResponse> {
  const response = await fetch(withBase('/graph/run', baseUrl), {
    method: 'POST',
    headers: {
      'content-type': 'application/json'
    },
    body: JSON.stringify(payload)
  })

  const result = (await response.json()) as GraphRunResponse
  if (!response.ok && result.ok) {
    result.ok = false
  }
  return result
}

export { defaultBaseUrl }
