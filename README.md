# gkdviz

Monorepo skeleton for a VSCode-like visual debugging workbench.

## Structure

- `apps/gkdviz-backend`: backend service (`C++26 + Boost.Asio`)
- `apps/gkdviz-ui`: frontend shell (`Vite + Node.js + TypeScript`)
- `proto`: protobuf contracts (planned)
- `core`: domain/core logic (planned)
- `transport`: transport layer (planned)
- `protocol`: protocol codec layer (planned)
- `driver`: device/driver adapters (planned)
- `docs`: architecture and engineering docs (planned)
- `prototype`: standalone frontend prototypes (VSCode-style canvas)
- `doc/backend-canvas-foundation`: Goal-Driven development docs (design/goal/plan/log)

## Build Agent

```bash
xmake f -m debug
xmake
xmake run gkdviz-backend
```

## Run UI

```bash
cd apps/gkdviz-ui
npm install
npm run dev
```
