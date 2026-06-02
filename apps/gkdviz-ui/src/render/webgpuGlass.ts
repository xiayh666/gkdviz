function alignTo(value: number, alignment: number): number {
  return Math.ceil(value / alignment) * alignment
}

export async function initWebGpuGlassTexture(): Promise<void> {
  if (typeof window === 'undefined' || !('gpu' in navigator)) return

  try {
    const adapter = await navigator.gpu.requestAdapter()
    if (!adapter) return
    const device = await adapter.requestDevice()

    const width = 256
    const height = 256
    const format: GPUTextureFormat = 'rgba8unorm'

    const texture = device.createTexture({
      size: { width, height },
      format,
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_SRC
    })

    const shader = device.createShaderModule({
      code: `
struct VSOut {
  @builtin(position) position: vec4<f32>,
  @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vid: u32) -> VSOut {
  var pos = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -3.0),
    vec2<f32>(-1.0, 1.0),
    vec2<f32>(3.0, 1.0)
  );
  var out: VSOut;
  out.position = vec4<f32>(pos[vid], 0.0, 1.0);
  out.uv = out.position.xy * 0.5 + vec2<f32>(0.5, 0.5);
  return out;
}

fn hash(p: vec2<f32>) -> f32 {
  let h = dot(p, vec2<f32>(127.1, 311.7));
  return fract(sin(h) * 43758.5453123);
}

fn noise(p: vec2<f32>) -> f32 {
  let i = floor(p);
  let f = fract(p);
  let a = hash(i);
  let b = hash(i + vec2<f32>(1.0, 0.0));
  let c = hash(i + vec2<f32>(0.0, 1.0));
  let d = hash(i + vec2<f32>(1.0, 1.0));
  let u = f * f * (3.0 - 2.0 * f);
  return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

fn fbm(p0: vec2<f32>) -> f32 {
  var p = p0;
  var a = 0.5;
  var f = 0.0;
  for (var i = 0; i < 4; i = i + 1) {
    f = f + a * noise(p);
    p = p * 2.03 + vec2<f32>(13.7, 9.2);
    a = a * 0.5;
  }
  return f;
}

@fragment
fn fs_main(in: VSOut) -> @location(0) vec4<f32> {
  let low = fbm(in.uv * 7.0 + vec2<f32>(0.7, 1.9));
  let mid = fbm(in.uv * 16.0 + vec2<f32>(3.2, 1.1));
  let hi = noise(in.uv * 64.0 + vec2<f32>(7.3, 5.7));
  let haze = (low * 0.58 + mid * 0.30 + hi * 0.12) * 0.46;

  // Directional micro-groove streaks for frosted glass anisotropy.
  let streak = noise(vec2<f32>(in.uv.x * 5.0 + low * 0.8, in.uv.y * 120.0));
  let streakMask = smoothstep(0.58, 0.9, streak) * 0.09;

  // Soft center glow + edge rim to mimic light scattering in glass.
  let center = length(in.uv - vec2<f32>(0.5, 0.46));
  let centerGlow = (1.0 - smoothstep(0.0, 0.78, center)) * 0.08;
  let rim = smoothstep(0.62, 0.96, center) * 0.06;

  let top = vec3<f32>(0.98, 0.98, 0.99);
  let bottom = vec3<f32>(0.86, 0.89, 0.94);
  let base = mix(top, bottom, clamp(in.uv.y, 0.0, 1.0));
  let scatter = haze + streakMask + centerGlow - rim;
  let color = clamp(base + vec3<f32>(scatter), vec3<f32>(0.0), vec3<f32>(1.0));

  return vec4<f32>(color, 0.86);
}
      `
    })

    const pipeline = device.createRenderPipeline({
      layout: 'auto',
      vertex: { module: shader, entryPoint: 'vs_main' },
      fragment: { module: shader, entryPoint: 'fs_main', targets: [{ format }] },
      primitive: { topology: 'triangle-list' }
    })

    const bytesPerPixel = 4
    const unpaddedBytesPerRow = width * bytesPerPixel
    const bytesPerRow = alignTo(unpaddedBytesPerRow, 256)
    const bufferSize = bytesPerRow * height

    const readback = device.createBuffer({
      size: bufferSize,
      usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
    })

    const encoder = device.createCommandEncoder()
    const pass = encoder.beginRenderPass({
      colorAttachments: [
        {
          view: texture.createView(),
          clearValue: { r: 0, g: 0, b: 0, a: 0 },
          loadOp: 'clear',
          storeOp: 'store'
        }
      ]
    })

    pass.setPipeline(pipeline)
    pass.draw(3, 1, 0, 0)
    pass.end()

    encoder.copyTextureToBuffer(
      { texture },
      { buffer: readback, bytesPerRow, rowsPerImage: height },
      { width, height, depthOrArrayLayers: 1 }
    )

    device.queue.submit([encoder.finish()])
    await readback.mapAsync(GPUMapMode.READ)

    const mapped = new Uint8Array(readback.getMappedRange())
    const tight = new Uint8ClampedArray(width * height * bytesPerPixel)
    for (let y = 0; y < height; y += 1) {
      const srcOffset = y * bytesPerRow
      const dstOffset = y * unpaddedBytesPerRow
      tight.set(mapped.subarray(srcOffset, srcOffset + unpaddedBytesPerRow), dstOffset)
    }

    readback.unmap()

    const canvas = document.createElement('canvas')
    canvas.width = width
    canvas.height = height
    const ctx2d = canvas.getContext('2d')
    if (!ctx2d) return

    const image = new ImageData(tight, width, height)
    ctx2d.putImageData(image, 0, 0)

    const url = canvas.toDataURL('image/png')
    document.documentElement.style.setProperty('--gpu-glass-texture', `url(${url})`)
    document.documentElement.classList.add('gpu-glass-enabled')
    document.documentElement.classList.remove('gpu-glass-failed')
    console.info('[gkdviz] WebGPU glass enabled')
  } catch {
    document.documentElement.classList.add('gpu-glass-failed')
    document.documentElement.classList.remove('gpu-glass-enabled')
    console.warn('[gkdviz] WebGPU glass failed, fallback to CSS-only')
  }
}
