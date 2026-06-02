import React from 'react'
import { createRoot } from 'react-dom/client'
import '@vscode/codicons/dist/codicon.css'
import '@xyflow/react/dist/style.css'
import { ConfigProvider, theme as antdTheme } from 'antd'
import 'antd/dist/reset.css'
import { AppShell } from './components/AppShell'
import { layoutConfig } from './config/layout'
import { sceneConfig } from './config/scene'
import { latteTokens, uiStyleVars, vscodeThemeVars } from './config/theme'
import { initWebGpuGlassTexture } from './render/webgpuGlass'
import './style.css'

const rootEl = document.querySelector<HTMLDivElement>('#app')
if (!rootEl) throw new Error('Missing #app container')

const root = createRoot(rootEl)

for (const [name, value] of Object.entries(latteTokens)) {
  document.documentElement.style.setProperty(`--ctp-${name}`, value)
}

for (const [name, value] of Object.entries(vscodeThemeVars)) {
  document.documentElement.style.setProperty(name, value)
}

for (const [name, value] of Object.entries(uiStyleVars)) {
  document.documentElement.style.setProperty(name, value)
}

root.render(
  <React.StrictMode>
    <ConfigProvider
      theme={{
        algorithm: antdTheme.defaultAlgorithm,
        token: {
          colorPrimary: latteTokens.mauve,
          colorBgContainer: '#ffffff',
          colorBgElevated: '#ffffff',
          colorText: latteTokens.text,
          colorBorder: latteTokens.surface1,
          colorTextPlaceholder: latteTokens.subtext0,
          borderRadius: 8,
          borderRadiusSM: 6,
          borderRadiusLG: 10,
          controlHeight: 32,
          controlHeightSM: 28,
          paddingXS: 8,
          paddingSM: 10,
          padding: 12,
          fontFamily: vscodeThemeVars['--vscode-font-family']
        },
        components: {
          Select: {
            selectorBg: '#ffffff',
            activeBorderColor: latteTokens.lavender,
            optionSelectedBg: latteTokens.crust,
            optionActiveBg: latteTokens.mantle,
            optionSelectedColor: latteTokens.text
          },
          Input: {
            activeBorderColor: latteTokens.lavender,
            hoverBorderColor: latteTokens.surface1
          },
          Button: {
            colorPrimary: latteTokens.mauve,
            colorPrimaryHover: latteTokens.lavender,
            defaultBg: '#ffffff',
            defaultBorderColor: latteTokens.surface1,
            defaultColor: latteTokens.text
          }
        }
      }}
    >
      <AppShell layout={layoutConfig} scene={sceneConfig} />
    </ConfigProvider>
  </React.StrictMode>
)

void Promise.resolve(document.fonts?.ready)
  .catch(() => undefined)
  .then(async () => {
    await new Promise<void>((resolve) => requestAnimationFrame(() => resolve()))
    await initWebGpuGlassTexture()
  })
