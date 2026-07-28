# LUANDRO — Native Runtime Platform (NRP)
## 📋 Planning & Progress Tracker

> **Última atualização:** 2026-07-28  
> **Package Root:** `io.github.luandro`  
> **Plataforma:** Android ARM64 (`arm64-v8a`)  
> **Min SDK:** 21

---

## 🎯 Objetivo Principal

Desenvolver uma **Native Runtime Platform (NRP)** modular e de alta performance para Android ARM64.

- Toda a lógica de negócio, parsing, execução e gerenciamento de memória **DEVE** residir em código nativo (C++20).
- Kotlin é apenas uma **thin public API**.
- Luau é outro frontend que **compartilha a mesma implementação nativa**.
- **Nenhuma lógica duplicada** é permitida.

---

## 🏗️ Arquitetura

```
         Android App
               │
               ▼
      Kotlin Public API
  ┌───────────────────────┐
  │  io.github.luandro    │
  │  ├── lexsoup          │
  │  ├── regex            │
  │  ├── js               │
  │  └── luau             │
  └───────────┬───────────┘
              │ Thin JNI Layer
              ▼
  ╔═══════════════════════╗
  ║  Native Runtime Core  ║
  ║  ─────────────────── ║
  ║  Object Manager       ║
  ║  Handle Manager       ║
  ║  Memory Manager       ║
  ║  String Manager       ║
  ║  Exception Manager    ║
  ║  Type Converter       ║
  ║  Shared Allocator     ║
  ╠═══════════════════════╣
  ║  Native Engines       ║
  ║  ─────────────────── ║
  ║  LexSoup (Lexbor)     ║
  ║  Regex (jsregexp)     ║
  ║  QuickJS (QJS-NG)     ║
  ║  Luau VM              ║
  ╠═══════════════════════╣
  ║  Third Party          ║
  ║  ─────────────────── ║
  ║  Lexbor               ║
  ║  QuickJS-NG           ║
  ║  Luau                 ║
  ║  jsregexp             ║
  ╚═══════════════════════╝
```

---

## 📦 Estrutura de Diretórios

```
/
├── docs/                    ← Documentação de fases
├── examples/                ← Exemplos de uso
├── tests/                   ← Testes
├── spec/                    ← API Specification Language (ASL)
│   ├── lexsoup/
│   ├── regex/
│   ├── js/
│   └── luau/
├── thirdparty/              ← Git Submodules
│   ├── luau/
│   ├── lexbor/
│   ├── quickjs/
│   └── jsregexp/
├── native/
│   ├── runtime/
│   │   ├── object_manager/
│   │   ├── handle_manager/
│   │   ├── memory/
│   │   ├── strings/
│   │   ├── exceptions/
│   │   ├── converter/
│   │   ├── allocator/
│   │   └── utilities/
│   ├── lexsoup/
│   ├── regex/
│   ├── quickjs/
│   ├── luau/
│   └── binding/
│       ├── kotlin/
│       └── luau/
├── kotlin/
│   └── io/github/luandro/
│       ├── lexsoup/
│       ├── regex/
│       ├── js/
│       └── luau/
└── generated/               ← Código gerado pelo Binding Generator
    ├── kotlin/
    ├── jni/
    ├── luau/
    ├── native/
    └── docs/
```

---

## 🗺️ Fases do Projeto

### 📐 Fases de Preparação

| Fase | Nome | Status | Descrição |
|------|------|--------|-----------|
| **0** | Repository Foundation | ✅ DONE | Criar estrutura inicial do repo, Gradle multi-module, CMake |
| **0.5** | Project Standards & Guidelines | ✅ DONE | Definir padrões de desenvolvimento (C++20, RAII, JNI thin, etc.) |
| **0.6** | Build & Toolchain Specification | ✅ DONE | Ambiente de build, NDK, AGP, Gradle, CI/CD |
| **0.7** | Code Generation Strategy | ✅ DONE | Projetar sistema de geração de código (antes de qualquer implementação) |
| **0.8** | API Specification Language (ASL) | ✅ DONE | Projetar a linguagem formal de especificação de APIs (YAML) |

### 🏛️ Fases de Design

| Fase | Nome | Status | Descrição |
|------|------|--------|-----------|
| **1** | Architecture Design | ⬜ TODO | Documentação completa da arquitetura (sem código-fonte) |
| **2** | Public API Specification | ⬜ TODO | Especificar todas as APIs públicas Kotlin, Native e Luau |

### ⚙️ Fases de Implementação

| Fase | Nome | Status | Descrição |
|------|------|--------|-----------|
| **3** | Runtime Core | ⬜ TODO | Implementar ObjectManager, HandleManager, MemoryManager, etc. |
| **4** | Binding Infrastructure | ⬜ TODO | JNI Framework + Luau Binding Framework reutilizáveis |
| **5** | LexSoup Engine | ⬜ TODO | HTML parser compatível com JSoup usando Lexbor |
| **6** | Regex Engine | ⬜ TODO | Regex com jsregexp (Pattern, Matcher, MatchResult, etc.) |
| **7** | QuickJS Engine | ⬜ TODO | Motor JavaScript via QuickJS-NG (Runtime, Context, Module, Promise) |
| **8** | Luau Engine | ⬜ TODO | Luau VM com auto-registro de LexSoup, Regex e QuickJS |
| **9** | Integration | ⬜ TODO | Integrar todos os engines + testes completos |
| **10** | Binding Generator | ⬜ TODO | Gerador automático de bindings a partir de specs ASL |
| **11** | Migration to Generated Bindings | ⬜ TODO | Migrar todos os bindings manuais para o sistema gerado |

---

## 📝 Legenda de Status

| Símbolo | Significado |
|---------|-------------|
| ✅ | Concluído |
| 🔄 | Em progresso |
| ⬜ | Pendente |
| ⛔ | Bloqueado |
| 🔍 | Em revisão |

---

## 📋 Detalhamento das Fases

### FASE 0 — Repository Foundation
**Objetivo:** Criar a estrutura inicial do repositório.

**Regras:**
- NÃO implementar nenhum engine
- NÃO escrever JNI
- NÃO escrever lógica de negócio

**Tarefas:**
- [x] Criar layout modular do repositório
- [x] Configurar projeto Gradle multi-module
- [x] Configurar Android Library
- [x] Configurar CMake
- [x] Criar scripts de build raiz
- [x] Criar diretório `docs/`
- [x] Criar diretório `tests/`
- [x] Criar diretório `examples/`
- [x] Criar diretório `thirdparty/`

**Critério de Sucesso:** Projeto compila sem implementar nenhuma funcionalidade nativa.

---

### FASE 0.5 — Project Standards & Guidelines
**Objetivo:** Definir padrões de desenvolvimento antes de qualquer implementação.

**Padrões definidos:**
- [ ] Estilo C++: C++20, RAII, smart pointers, sem raw pointer ownership
- [ ] Kotlin: thin wrapper, sem lógica de negócio
- [ ] JNI: apenas conversão (string, primitivos, arrays, handles, exceptions)
- [ ] Estrutura de módulos com responsabilidade única
- [ ] Gerenciamento de objetos via handles
- [ ] Gerenciamento centralizado de memória
- [ ] Tratamento de erros (sem exceptions cruzando JNI)
- [ ] Modelo de thread safety
- [ ] Política de testes obrigatórios
- [ ] Padrões de documentação
- [ ] Logging configurável (debug/info/warning/error)

**Critério de Sucesso:** Documento torna-se o padrão oficial de engenharia da NRP.

---

### FASE 0.6 — Build & Toolchain Specification
**Objetivo:** Definir completamente o ambiente de build antes de qualquer código.

**Especificações:**
- [ ] Target: Android ARM64 (`arm64-v8a`), Min SDK 21
- [ ] Gradle (versão estável mais recente + AGP compatível)
- [ ] Kotlin (versão única e consistente)
- [ ] CMake com C++20
- [ ] NDK (uma única versão)
- [ ] Build Debug: assertions, logging, sanitizers
- [ ] Build Release: máxima otimização, strip, LTO
- [ ] Módulos: Library, Native, Thirdparty, Docs, Examples, Tests, Benchmark
- [ ] Política de Git Submodules para dependências
- [ ] Política de patches a terceiros
- [ ] GitHub Actions CI/CD (build, testes, formatação, release)
- [ ] Semantic Versioning

**Critério de Sucesso:** Ambiente completamente definido, builds reproduzíveis.

---

### FASE 0.7 — Code Generation Strategy
**Objetivo:** Projetar o sistema de geração de código antes de qualquer módulo Runtime.

**Capacidades do gerador:**
- [ ] Native C API
- [ ] JNI Registration & Wrapper Functions
- [ ] Kotlin Wrapper Classes e Native Methods
- [ ] Luau Binding Functions e Registration Tables
- [ ] API Documentation e Markdown Reference
- [ ] Unit Test Skeletons

**Convenções:**
- [ ] Arquivos gerados NUNCA editados manualmente
- [ ] Nomes previsíveis: `Document.gen.kt`, `Document.gen.cpp`, etc.
- [ ] Header indicando que é código gerado
- [ ] Integração com build system (gerar antes de compilar)

**Critério de Sucesso:** Adicionar nova API requer apenas: atualizar spec → rodar gerador → compilar.

---

### FASE 0.8 — API Specification Language (ASL)
**Objetivo:** Projetar linguagem formal de spec de APIs (YAML).

**Estrutura ASL:**
- [ ] Formato YAML, um arquivo por classe
- [ ] Campos: package, class, description, constructors, methods, properties, enums, constants, exceptions, lifecycle, ownership, thread safety, examples
- [ ] Sistema de tipos: primitivos, coleções, objetos nativos
- [ ] Suporte a enumerações fortemente tipadas
- [ ] Modelo de exceções (Native → Kotlin/Lua)
- [ ] Mapeamento JNI automático
- [ ] Mapeamento Luau automático
- [ ] Versionamento (experimental/stable/deprecated/removed)
- [ ] Validação pré-geração

**Diretório de specs:**
```
spec/
  lexsoup/  → Document.yaml, Element.yaml, Elements.yaml
  regex/    → Pattern.yaml, Matcher.yaml
  js/       → Runtime.yaml, Context.yaml
  luau/     → VM.yaml
```

**Critério de Sucesso:** Toda API pública descrita em ASL antes da implementação.

---

### FASE 1 — Architecture Design
**Objetivo:** Documentar arquitetura completa antes de qualquer implementação.

**Documentos a produzir:**
- [ ] `Architecture.md` — arquitetura de módulos
- [ ] `Runtime.md` — arquitetura do Runtime
- [ ] `Memory.md` — gerenciamento de memória
- [ ] `ObjectLifecycle.md` — ciclo de vida de objetos
- [ ] `JNI.md` — arquitetura JNI
- [ ] `LuauBinding.md` — arquitetura Luau binding

**Inclui:** diagramas de módulos, ciclo de vida, modelo de handles, fluxo de exceções, modelo de threads.

**Critério de Sucesso:** Arquitetura completa documentada e aprovada antes da implementação.

---

### FASE 2 — Public API Specification
**Objetivo:** Especificar todas as APIs públicas antes de qualquer código.

**APIs a especificar:**
- [ ] `io.github.luandro.lexsoup` (Document, Element, Elements, Node)
- [ ] `io.github.luandro.regex` (Pattern, Matcher, MatchResult)
- [ ] `io.github.luandro.js` (Runtime, Context, Module, Promise)
- [ ] `io.github.luandro.luau` (LuauVM, Script, Compiler)
- [ ] Native API (C++ Runtime interface)
- [ ] Luau API (global modules)

**Entregas:** Especificação de API, diagramas de classe, exemplos de uso.

**Critério de Sucesso:** Toda API pública finalizada antes da implementação.

---

### FASE 3 — Runtime Core
**Objetivo:** Implementar a Native Runtime Platform (sem nenhum engine).

**Componentes:**
- [ ] `ObjectManager` — gerenciamento de objetos nativos
- [ ] `HandleManager` — handles seguros para objetos
- [ ] `MemoryManager` — alocação centralizada
- [ ] `SharedAllocator` — alocador compartilhado
- [ ] `StringManager` — gerenciamento de strings
- [ ] `ExceptionManager` — gerenciamento de exceções
- [ ] `TypeConverter` — conversão de tipos
- [ ] `Utilities` — biblioteca de utilitários

**Requisitos:** C++20, RAII, sem raw pointer exposure, sem lógica JNI.

**Entregas:** Runtime library + unit tests + documentação.

**Critério de Sucesso:** Runtime compila independentemente e passa em todos os testes.

---

### FASE 4 — Binding Infrastructure
**Objetivo:** Implementar infra de binding reutilizável (sem LexSoup/Regex/QuickJS/Luau).

**JNI Framework:**
- [ ] Conversão de strings
- [ ] Conversão de arrays
- [ ] Conversão de handles
- [ ] Conversão de exceções

**Luau Binding Framework:**
- [ ] userdata
- [ ] metatables
- [ ] object registration
- [ ] lifetime management
- [ ] automatic destruction

**Critério de Sucesso:** Todos os engines futuros usam a mesma infra.

---

### FASE 5 — LexSoup Engine
**Objetivo:** Implementar LexSoup usando Lexbor (`io.github.luandro.lexsoup`).

**API compatível com JSoup:**
- [ ] `parse(html)` → Document
- [ ] `doc.title()`
- [ ] `doc.select(cssQuery)` → Elements
- [ ] DOM traversal (parent, children, siblings)
- [ ] DOM modification
- [ ] Serialization (outerHtml, innerHtml, text)
- [ ] Node management

**Entregas:** Native Engine + Kotlin API + Luau Global Module + Tests + Performance Tests + Docs.

**Critério de Sucesso:** Kotlin e Luau compartilham a MESMA implementação nativa.

---

### FASE 6 — Regex Engine
**Objetivo:** Implementar Regex usando jsregexp (`io.github.luandro.regex`).

**API:**
- [ ] `Pattern` — compilação de regex
- [ ] `Matcher` — matching
- [ ] `MatchResult` — resultado de match
- [ ] `replace()` — substituição
- [ ] `split()` — divisão
- [ ] `find()` — busca
- [ ] `matches()` — verificação

**Entregas:** Regex engine + JNI binding + Luau binding + Tests + Docs.

**Critério de Sucesso:** Implementação Regex existe apenas UMA vez no native.

---

### FASE 7 — QuickJS Engine
**Objetivo:** Implementar QuickJS via QuickJS-NG (`io.github.luandro.js`).

**API:**
- [ ] `Runtime` — runtime JS
- [ ] `Context` — contexto de execução
- [ ] `Module` — módulos ES
- [ ] `Promise` — suporte a promises
- [ ] `JSON` — parse/stringify JSON
- [ ] Script execution
- [ ] Module loading

**Entregas:** QuickJS engine + JNI binding + Luau binding + Tests + Docs.

**Critério de Sucesso:** Kotlin e Luau executam JavaScript via MESMA implementação.

---

### FASE 8 — Luau Engine
**Objetivo:** Implementar Luau VM (`io.github.luandro.luau`).

**Componentes:**
- [ ] `VM Manager`
- [ ] Script execution
- [ ] Global registration
- [ ] Native function registration
- [ ] Auto-registro de: LexSoup, Regex, QuickJS
- [ ] Auto type conversion: String, Boolean, Number, Array, Map, List, Document, Element, Node

**Entregas:** Luau runtime + JNI binding + Tests + Docs.

**Critério de Sucesso:** Todo Luau VM expõe automaticamente todos os módulos nativos.

---

### FASE 9 — Integration
**Objetivo:** Integrar todos os engines em uma única Native Runtime Platform.

**Integração:**
- [ ] Runtime + LexSoup + Regex + QuickJS + Luau

**Testes:**
- [ ] Unit Tests
- [ ] Integration Tests
- [ ] JNI Tests
- [ ] Memory Leak Tests
- [ ] Performance Tests
- [ ] Stress Tests

**Entregas:** Runtime integrado + relatórios de testes + documentação.

**Critério de Sucesso:** Todos os módulos compartilham um único runtime e passam em todos os testes.

---

### FASE 10 — Binding Generator
**Objetivo:** Criar gerador de bindings para eliminar código repetitivo.

**Geração automática a partir de ASL:**
- [ ] Native C API
- [ ] JNI Registration
- [ ] JNI Wrapper
- [ ] Kotlin Wrapper
- [ ] Luau Binding
- [ ] API Documentation

**Entregas:** Binding Generator + Templates + Generated bindings + Docs.

**Critério de Sucesso:** Nova API não requer implementação manual de JNI ou Luau binding.

---

### FASE 11 — Migration to Generated Bindings
**Objetivo:** Migrar todos os bindings manuais para o sistema gerado.

**Tarefas:**
- [ ] Identificar todos os bindings manuais (JNI, Kotlin, Luau, C API wrappers)
- [ ] Converter em specs ASL
- [ ] Regenerar com o Binding Generator
- [ ] Remover implementações manuais obsoletas
- [ ] Integrar gerador no build system (gerar antes de compilar, incremental)
- [ ] Atualizar: Developer Guide, Contribution Guide, Architecture Guide, Generator Guide, ASL Guide, Migration Guide

**Validação:**
- [ ] Unit Tests sem regressão
- [ ] Integration Tests
- [ ] JNI Tests
- [ ] Luau Tests
- [ ] Performance Benchmarks

**Critério de Sucesso:** ASL é a única fonte de verdade. Novos recursos: atualizar ASL → gerar → compilar → testar.

---

## 🔗 Dependências (Git Submodules)

| Biblioteca | URL | Status |
|------------|-----|--------|
| Luau | https://github.com/luau-lang/luau | ⬜ TODO |
| Lexbor | https://github.com/lexbor/lexbor | ⬜ TODO |
| QuickJS-NG | https://github.com/quickjs-ng/quickjs | ⬜ TODO |
| jsregexp | https://github.com/kmarius/jsregexp | ⬜ TODO |

---

## ✅ Critérios Globais de Qualidade

- [ ] **Single Source of Truth:** Toda lógica de negócio vive apenas em C++ nativo
- [ ] **Zero Duplicação:** Kotlin e Luau chamam a mesma implementação nativa
- [ ] **Handle Safety:** Raw pointers nunca cruzam fronteiras de linguagem
- [ ] **Memory Safety:** RAII em todo o código C++, sem leaks
- [ ] **JNI Thin:** JNI faz apenas conversão de tipos
- [ ] **Luau Thin:** Luau binding faz apenas conversão e forward de chamadas
- [ ] **Testes Obrigatórios:** Todo módulo possui unit tests, integration tests e performance tests
- [ ] **Docs Completa:** Todo componente documentado com overview, responsibilities, lifecycle, examples
- [ ] **CI/CD:** GitHub Actions com build, testes, formatação e release
- [ ] **Semantic Versioning:** Major.Minor.Patch com regras claras

---

## 📊 Progresso Geral

```
Fases de Preparação:    [ 5/5 ]  (100%)
Fases de Design:        [ 0/2 ]  (  0%)
Fases de Implementação: [ 0/9 ]  (  0%)
─────────────────────────────────────
TOTAL:                  [ 5/16]  ( 31%)
```

---

*Gerado automaticamente a partir dos documentos em `docs/`. Atualizar conforme as fases são concluídas.*
