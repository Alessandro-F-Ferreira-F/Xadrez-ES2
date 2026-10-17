# Planejamento e Acompanhamento de Sprints (Gantt, Burndown e EVM)

> **Propósito deste documento:** Servir como fonte da verdade para o acompanhamento quinzenal do projeto **Xadrez-ES2**, conectando a [EAP (`docs/EAP.md`)](./EAP.md), o [Cronograma Gantt (`docs/cronograma_gantt.pdf`)](./cronograma_gantt.pdf), as [Estimativas do Planning Poker (`docs/estimativas_poker.md`)](./estimativas_poker.md) e o [Monitoramento e Controle (`docs/monitoramento_controle.md`)](./monitoramento_controle.md) às **Issues do GitHub**.
>
> Este arquivo também estrutura a base de leitura para o **monitoramento automatizado via MCP (GitHub + WhatsApp)**, permitindo avaliar o progresso da Sprint atual, identificar entregas adiantadas de Sprints futuras e calcular o **Burndown** e a **Análise de Valor Agregado (EVM)**.

---

## 1. Parâmetros Globais de Gerenciamento (Linha de Base)

- **Valor da Hora Técnica:** R$ 50,00 / hora (perfil júnior — ver [`estimativas_poker.md`](./estimativas_poker.md))
- **Orçamento Base (Planning Poker — BAC Geral):** 424 horas = **R$ 21.200,00**
  - **Escopo MVP (Pacotes 1.1 a 1.6 — Obrigatórios):** 280 horas = **R$ 14.000,00**
  - **Escopo de Extensão Opcional (Pacotes 1.7 Servidor e 1.8 Cliente Web):** 144 horas = **R$ 7.200,00** *(fora do caminho crítico do MVP, conforme [`EAP.jpeg`](./EAP.jpeg) e [`arquitetura-xadrez.md`](../engine/docs/arquitetura-xadrez.md))*
- **Teto de Margem de Segurança (APF):** 62 PF = 496 horas = **R$ 24.800,00** (ver [`estimativas_apf.md`](./estimativas_apf.md))
- **Cadência das Sprints:**
  - **Fase Inicial (Pré-1ª Apresentação):** 3 iterações semanais (`26/08` a `13/09/2026`).
  - **Fase de Desenvolvimento e Entrega (Pós-1ª Apresentação):** **5 Sprints de ~2 semanas** (`14/09` a `23/11/2026`), sendo as **Sprints 3, 4 e 5** o ciclo atual até a **Terceira Apresentação (`23/11/2026`)**.

### Fórmulas de EVM (Análise de Valor Agregado)
- **PV (*Planned Value* / Valor Planejado):** Horas planejadas para a Sprint $\times$ R$ 50,00
- **EV (*Earned Value* / Valor Agregado):** Horas equivalentes dos pacotes/issues efetivamente **concluídos** $\times$ R$ 50,00 *(inclui tarefas adiantadas de sprints futuras)*
- **AC (*Actual Cost* / Custo Real):** Horas reais gastas pela equipe na Sprint $\times$ R$ 50,00
- **SPI (*Schedule Performance Index* / Índice de Prazo):** $\text{SPI} = \frac{\text{EV}}{\text{PV}}$ ($\ge 1{,}0$: no prazo/adiantado; $< 1{,}0$: atrasado)
- **CPI (*Cost Performance Index* / Índice de Custo):** $\text{CPI} = \frac{\text{EV}}{\text{AC}}$ ($\ge 1{,}0$: dentro do orçamento; $< 1{,}0$: acima do custo)
- **SV (*Schedule Variance* / Variação de Prazo):** $\text{SV} = \text{EV} - \text{PV}$
- **CV (*Cost Variance* / Variação de Custo):** $\text{CV} = \text{EV} - \text{AC}$

---

## 2. Histórico Consolidado: Fase Inicial (Iterações 1 a 3)

Período de inicialização, arquitetura, documentação de gerência e fundação do motor e da interface até a **1ª Apresentação (`14/09/2026`)**, conforme detalhado em [`monitoramento_controle.md`](./monitoramento_controle.md):

| Iteração | Período | Foco Principal (EAP / Gantt) | PV | EV | AC | SPI | CPI | Status |
| :--- | :---: | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Iteração 1** | `26/08` – `01/09` | Repo (`1.2.1`), Estrutura da Posição (`1.4.1`), Geometria (`1.4.3.1`), FEN base (`1.4.9`) | 20h (R$ 1.000) | 20h (R$ 1.000) | 18h (R$ 900) | **1,00** | **1,11** | ✅ Concluída |
| **Iteração 2** | `02/09` – `06/09` | Geração inicial de lances (`1.4.4.1`), Validação FEN (`1.4.9`), Logs (`1.6.1.4`), Escopo/Requisitos (`1.2.4.1`, `1.2.4.4`), Reunião `04/09` | 30h (R$ 1.500) | 25h (R$ 1.250) | 35h (R$ 1.750) | **0,83** | **0,71** | ✅ Concluída |
| **Iteração 3** | `07/09` – `13/09` | EAP (`1.2.4.2`), Riscos (`1.2.4.3`), APF/Poker, Fundação SFML (`1.3.1.1`), 1ª Apresentação (`1.1.1`), Reunião `11/09` | 40h (R$ 2.000) | 35h (R$ 1.750) | 38h (R$ 1.900) | **0,875** | **0,92** | ✅ Concluída |
| **Subtotal Inicial** | **`26/08` – `13/09`** | **Fundação, Gerência e 1ª Apresentação** | **90h (R$ 4.500)** | **80h (R$ 4.000)** | **91h (R$ 4.550)** | **0,89** | **0,88** | ✅ Fechado |

---

## 3. Sprints Quinzenais (Pós-1ª Apresentação até a Entrega Final)

### Sprint 1 — Regras Completas (Pseudo-Legal), `make_move` e Peças na GUI
- **Período:** `14/09/2026` a `27/09/2026` (2 semanas — Reuniões `18/09` e `25/09`)
- **Meta da Sprint:** Completar a geração pseudo-legal de todas as peças e lances especiais no motor, implementar a aplicação de lances (`make_move`), renderizar e selecionar peças na interface SFML e estruturar a base da IA.
- **Orçamento Planejado da Sprint (PV):** **40 horas (R$ 2.000,00)**

| ID EAP | ID Gantt | Tarefa / Entregável | Módulo | Janela no Gantt | Horas (PV) | Issue / PR (GitHub) | Status Atual |
| :---: | :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| `1.2.3.3` / `1.2.3.4` | `#13`, `#14` | Reuniões de acompanhamento (`18/09` e `25/09`) e AVA (`1.2.2`) | Gerência | `18/09` – `25/09` | 3h | [PR #1](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/pull/1) | ✅ Concluído |
| `1.4.3.2` | `#48` | Tabelas de Movimentos e Ataques (`PAWN_ATTACKS`, `KNIGHT_ATTACKS`) | Motor (`engine/`) | `31/08` – `11/09` | 5h | [#18](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/18) | ✅ Concluído |
| `1.4.4.1` | `#44` | Geração de Movimentos Pseudo-Legais completa (peão, cavalo, deslizantes, rei, roque) | Motor (`engine/`) | `14/09` – `02/10` | 8h | [#18](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/18), [#23](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/23) | ✅ Concluído |
| `1.4.5.2.2` / `1.4.5.2.3` | `#53`, `#54` | Validadores de En Passant (`18/09`) e Promoção nas 4 peças (`25/09`) | Motor (`engine/`) | `14/09` – `25/09` | 6h | [#24](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/24) | ✅ Concluído |
| `1.4.8.1` | `#64` | Aplicação de Lances (`make_move` + struct `Undo` + cache do rei) | Motor (`engine/`) | `14/09` – `02/10` | 7h | [#27](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/27) | ✅ Concluído |
| `1.3.1.2` | `#100` | Modularização da Interface e Melhoria Visual / Renderização do Tabuleiro SFML | Interface (`interface/`) | `14/09` – `21/09` | 4h | [#2](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/2), [#3](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/3), [PR #30](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/pull/30) | ✅ Concluído |
| `1.3.2.2` | `#102` | Seleção de Peças e Movimentação no `ChessBoard` | Interface (`interface/`) | `22/09` – `29/09` | 4h | [PR #36](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/pull/36) | ✅ Concluído |
| `1.5.1` / `1.5.6` | `#70`, `#81` | Pesquisa da IA, pseudocódigos (`IA/Pseudocode/`) e seleção de lance aleatório | IA (`IA/`) | `14/09` – `02/10` | 3h | *(commits `IA/`)* | ✅ Concluído |

- **Resumo EVM da Sprint 1:**
  - **PV:** 40h (R\$ 2.000,00) | **EV:** 40h (R\$ 2.000,00) | **AC:** *A preencher pela equipe*

---

### Sprint 2 — Legalidade, `unmake_move`, Perft, Protocolo UCI, Avaliação/Busca da IA e 2ª Apresentação
- **Período:** `28/09/2026` a `15/10/2026` (~2,5 semanas — Reuniões `02/10` e `09/10`; **2ª Apresentação em `05/10`**)
- **Meta da Sprint:** Provar as regras do xadrez com `unmake_move`, filtro de legalidade e bateria Perft nas 6 posições da CPW; implementar o protocolo UCI no motor; implementar avaliação e busca na IA; iniciar a ponte de subprocesso (`EngineBridge`) no C++.
- **Orçamento Planejado da Sprint (PV):** **45 horas (R\$ 2.250,00)**

| ID EAP | ID Gantt | Tarefa / Entregável | Módulo | Janela no Gantt | Horas (PV) | Issue / PR (GitHub) | Status Atual |
| :---: | :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| `1.1.2` | `#5` | **Segunda Apresentação** (Git, Issues, Branches, PRs, CI, Burndown, EVM e Roteiro) | Apresentação | `05/10/2026` | 4h | [#11](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/11), [#12](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/12), [#13](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/13), [#14](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/14), [#29](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/29) | ✅ Apresentado *(fechar issues `#11`–`#14`, `#29` no GitHub)* |
| `1.2.3.5` / `1.2.3.6` | `#15`, `#16` | Reuniões de acompanhamento (`02/10` e `09/10`) e AVA (`1.2.2`) | Gerência | `02/10` – `09/10` | 3h | `-` | ✅ Concluído |
| `1.4.8.2` | `#65` | Reversão de Lances (`unmake_move` completo + round-trip em 11M nós) | Motor (`engine/`) | `05/10` – `20/11` | 6h | [#17](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/17), [#19](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/19) | ✅ Concluído *(adiantou fim do Gantt)* |
| `1.4.4.2` / `1.4.5.1` / `1.4.5.2.1` | `#45`, `#50`, `#52` | Filtro de Legalidade (`is_square_attacked`, `generate_legal_moves`) + regras extras de Roque | Motor (`engine/`) | `14/09` – `02/10` | 7h | [#20](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/20), [#24](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/24) | ✅ Concluído |
| `1.2.5.1` | `#29` | Testes da Máquina de Regras: `perft.c` exato nas 6 posições da CPW + API de testes (`test/test.c`) | Motor (`engine/`) | `14/09` – `20/11` | 5h | [#25](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/25), [#38](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/38) | ✅ Concluído |
| `1.6.2.1` / `1.6.2.3` / `1.6.2.4` / `1.6.2.5` | `#91`, `#93`, `#94`, `#95` | Protocolo UCI (`uci.c`, `uci-comandos.md`, `position`, `legalmoves`, `state`, `go`, `go perft`) + suíte `test/protocol/` | Protocolo (`engine/`) | `14/09` – `20/11` | 8h | [#21](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/21) | ⚡ **Concluído no código** *(Issue `#21` ainda aberta no GitHub)* |
| `1.5.3` / `1.5.4.1` / `1.5.4.2` | `#72`, `#73`, `#74`, `#75` | Simulação de Estados e Função de Avaliação Material e Posicional (`avaliar.c`) | IA (`IA/`) | `05/10` – `12/10` | 6h | *(commits `IA/`)* | ✅ Concluído |
| `1.3.2.1` / `1.3.2.3` | `#103`, `#104` | Parser UCI e Movimentação no `ChessBoard` (`parseSquare`) | Interface (`interface/`) | `14/09` – `07/10` | 3h | [#6](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/6), [PR #32](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/pull/32), [PR #36](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/pull/36) | ✅ Concluído |
| `1.3.6.1` | `#118` | Inicialização da Engine como subprocesso (`EngineBridge::start` e `sendCommand` POSIX) | Interface (`interface/`) | `14/09` – `20/11` | 3h | [#6](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/6), [PR #31](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/pull/31) | ✅ Concluído |

> **Nota de Adiantamento na Sprint 2:** Além das tarefas previstas acima, a equipe de IA já implementou o **Algoritmo Minimax (`1.5.5.1`, Gantt `#77`, previsto para `13/10–02/11`)** e a **Poda Alfa-Beta (`1.5.5.2`, Gantt `#78`, previsto para `03/11–20/11`)** em [`IA/src/ia.c`](../IA/src/ia.c), gerando Valor Agregado (EV) adiantado!

---

### 📍 Sprint 3 (SPRINT ATUAL) — Integração Ponta a Ponta (Motor ↔ IA ↔ Interface), Empates, CI/CD e Qualidade
- **Período:** `16/10/2026` a `30/10/2026` (2 semanas — Reuniões `16/10`, `23/10` e `30/10`)
- **Meta da Sprint:** Fazer o sistema jogar uma partida completa de ponta a ponta: ligar a IA (`IA/`) ao comando `go` do protocolo UCI (`engine/`), finalizar a seleção por clique do mouse e leitura do pipe (`readCommand`) na interface C++ (`interface/`), completar o histórico/empates, configurar o pipeline CI/CD e fechar as pendências de build/testes do motor.
- **Orçamento Planejado da Sprint (PV):** **40 horas (R\$ 2.000,00)**

| ID EAP | ID Gantt | Tarefa / Entregável | Módulo | Janela no Gantt | Horas (PV) | Issue / PR (GitHub) | Status em `16/10` |
| :---: | :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| `1.2.3.7` a `1.2.3.9` | `#17`, `#18`, `#19` | Reuniões semanais (`16/10`, `23/10`, `30/10`), atualização do Burndown/AVA (`1.2.2`) e limpeza das issues da 2ª Apresentação | Gerência | `16/10` – `30/10` | 3h | [#11](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/11)–[#14](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/14), [#29](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/29) | 🔶 Em andamento |
| `1.5.5.1` | `#77` | Algoritmo Minimax (`IA/src/ia.c`) | IA (`IA/`) | `13/10` – `02/11` | 7h | *(commits `IA/`)* | ✅ **Já implementado!** *(falta só integrar ao binário)* |
| `1.5.2` | `#71` | Integração IA ↔ Máquina de Regras: remover `pendenciasRegras.h`/`stubs.c` e chamar `escolherJogada` no comando `go` de `engine/src/uci.c` | IA + Motor | `14/09` – `30/10` | 6h | [#21](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/21) *(fechamento UCI + IA)* | ⬜ Pendente *(caminho crítico)* |
| `1.4.7.1` a `1.4.7.4` / `1.6.2.2` | `#58`–`#62`, `#92` | Detecção de Estados de Jogo e Histórico de Lances: Xeque, Mate, Afogamento e 50 lances prontos em `uci.c`; **faltam Histórico completo (`#26`), Material Insuficiente e Repetição Tripla** | Motor (`engine/`) | `14/09` – `20/11` | 6h | [#26](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/26) | 🔶 Parcial (4 de 6 prontos) |
| `1.6.1.1` a `1.6.1.5` | `#84`–`#89` | Fundação de Qualidade e CI/CD: Pipeline Linux/Windows (`#10`, `#15`, `#16`), Testes automatizados (`#22`), Debug no terminal (`#28`) e Benchmark Perft (`#37`) | Motor / DevOps | `14/09` – `20/11` | 5h | [#10](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/10), [#15](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/15), [#16](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/16), [#22](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/22), [#28](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/28), [#37](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/37) | 🔶 Em andamento |
| `1.3.6.2` a `1.3.6.4` | `#119`–`#121` | Comunicação completa com a Engine (`EngineBridge::readCommand()`, `stop()`) + Seleção e entrada de lances via clique do mouse (`#4`) | Interface (`interface/`) | `14/09` – `20/11` | 6h | [#4](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/4), [#6](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/6) | 🔶 Em andamento (`TODO` em `engine_bridge.cpp`) |
| `1.3.2.4` | `#105` | Destaque visual de movimentos válidos (Highlight) consumindo `legalmoves` do motor | Interface (`interface/`) | `14/09` – `30/10` | 3h | [#5](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/5) | ⬜ Pendente |
| `1.4.6.1` / `1.4.6.2` | `#56`, `#57` | Controlador de Turnos e modo **Multiplayer Local (Hot-Seat)** na interface | Interface + Motor | `05/10` – `26/10` | 4h | [#4](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/4), [#8](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/8) | ⬜ Pendente |

- **Checklist de Burndown da Sprint 3 (40h planejadas):**
  - **Já agregado no início da Sprint (EV inicial):** ~12h (Minimax `#77` já codificado + 4 estados de fim de jogo em `uci.c` + parte do `Makefile`/testes).
  - **Restante para queimar até `30/10`:** ~28h focadas na ligação `uci.c` ↔ `IA/`, `EngineBridge::readCommand()`, clique do mouse (`#4`), highlight de lances válidos (`#5`), histórico (`#26`) e pipelines CI/CD (`#10`, `#15`, `#16`).

---

### Sprint 4 — Otimizações da IA (Alfa-Beta, Ordenação, Tempo), Menus, HUD e Fluxo de Partida
- **Período:** `31/10/2026` a `13/11/2026` (2 semanas — Reuniões `06/11` e `13/11`)
- **Meta da Sprint:** Dar força de jogo e controle de tempo/dificuldade à IA, finalizar todas as telas de menu, HUD (estado da partida, relógio e histórico de lances) e o ciclo completo de início/reinício/fim de partida local no cliente desktop.
- **Orçamento Planejado da Sprint (PV):** **40 horas (R\$ 2.000,00)**

| ID EAP | ID Gantt | Tarefa / Entregável | Módulo | Janela no Gantt | Horas (PV) | Issue / PR (GitHub) | Status em `16/10` |
| :---: | :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| `1.2.3.10` / `1.2.3.11` | `#20`, `#21` | Reuniões semanais (`06/11` e `13/11`) e atualização do Burndown/AVA (`1.2.2`) | Gerência | `06/11` – `13/11` | 2h | `-` | ⬜ Pendente |
| `1.5.5.2` | `#78` | Otimização: **Poda Alfa-Beta** (`IA/src/ia.c`) + teste de paridade com Minimax | IA (`IA/`) | `03/11` – `20/11` | 7h | *(commits `IA/`)* | ⚡ **Já implementado em `IA/src/ia.c`!** |
| `1.5.5.3` | `#80` | Otimização: **Aprofundamento Progressivo (Iterativo)** com relógio para suportar `go movetime` e `go depth` | IA (`IA/`) | `03/11` – `20/11` | 6h | *(abrir issue IA)* | ⬜ Pendente |
| `1.5.5.4` | `#79` | Otimização: **Ordenação de Movimentos** (MVV-LVA, promoções, melhor da iteração anterior) | IA (`IA/`) | `03/11` – `20/11` | 6h | *(abrir issue IA)* | ⬜ Pendente |
| `1.3.4.1` a `1.3.4.3` | `#109`–`#112` | Telas de Menu Principal, Seleção de Modo de Jogo (vs. IA / Local), Seleção de Dificuldade e Configurações | Interface (`interface/`) | `14/09` – `20/11` | 7h | [#9](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/9) | ⬜ Pendente |
| `1.3.5.1` a `1.3.5.3` | `#113`–`#116` | Fluxo da Partida Local: Início de Nova Partida (`27/10–20/11`), Parâmetros da IA e Alertas visuais de estado/fim de jogo | Interface (`interface/`) | `27/10` – `20/11` | 6h | [#7](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/7), [#9](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/9) | ⬜ Pendente |
| `1.3.3.1` / `1.3.3.2` | `#106`–`#108` | HUD lateral com relógio da partida, indicador de turno, alertas de estado e histórico de lances | Interface (`interface/`) | `05/10` – `20/11` | 6h | [#7](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/7), [#8](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/8) | ⬜ Pendente |

---

### Sprint 5 (Sprint Final) — Suíte Definitiva de Testes, Cobertura, Fechamento EVM e 3ª Apresentação
- **Período:** `14/11/2026` a `23/11/2026` (~10 dias — Reunião `20/11` e **3ª Apresentação em `23/11/2026`**)
- **Meta da Sprint:** Executar e documentar a suíte definitiva de testes (unidade, integração, sistema e aceitação alfa/beta) com relatório de cobertura, consolidar o EVM final (incluindo PV e EV individuais por integrante) e realizar a **Terceira Apresentação** com a Demo Final do produto.
- **Orçamento Planejado da Sprint (PV):** **25 horas (R\$ 1.250,00)**

| ID EAP | ID Gantt | Tarefa / Entregável | Módulo | Janela no Gantt | Horas (PV) | Issue / PR (GitHub) | Status em `16/10` |
| :---: | :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| `1.2.3.12` | `#22` | Reunião final de acompanhamento (`20/11`) | Gerência | `20/11/2026` | 1h | `-` | ⬜ Pendente |
| `1.2.5.1` a `1.2.5.4` | `#28`–`#32` | Suíte definitiva de testes automatizados (`#39`) + Relatório de **Testes de Unidade, Integração, Sistema e Aceitação (Alfa e Beta)** + **Cobertura de Testes** | Todos os Módulos | `14/09` – `20/11` | 12h | [#22](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/22), [#38](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/38), [#39](https://github.com/Alessandro-F-Ferreira-F/Xadrez-ES2/issues/39) | 🔶 Parcial (`#38` concluída; `#22` e `#39` abertas) |
| `1.2.2` | `#9` | Fechamento do **Monitoramento e Controle (Burndown + EVM Final)** e tabela de **participação por membro (PV e EV individual)** | Gerência | `03/08` – `23/11` | 6h | `-` | ⬜ Pendente |
| `1.1.3` | `#6` | **Terceira Apresentação** (Slides + Publicação no Classroom + Demo Final do Produto) | Apresentação | `23/11/2026` | 6h | `-` | ⬜ Pendente |

---

## 4. Resumo Consolidado do Cronograma e EVM (Escopo MVP: 280h = R$ 14.000,00)

| Fase / Sprint | Período | PV da Sprint | PV Acumulado | EV da Sprint | EV Acumulado | AC da Sprint | AC Acumulado | SPI Acum. | CPI Acum. |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Iterações 1–3** | `26/08` – `13/09` | 90h (R$ 4.500) | 90h (R$ 4.500) | 80h (R$ 4.000) | 80h (R$ 4.000) | 91h (R$ 4.550) | 91h (R$ 4.550) | **0,89** | **0,88** |
| **Sprint 1** | `14/09` – `27/09` | 40h (R$ 2.000) | 130h (R$ 6.500) | 40h (R$ 2.000) | 120h (R$ 6.000) | *A preencher* | *A preencher* | **0,92** | *- * |
| **Sprint 2** | `28/09` – `15/10` | 45h (R$ 2.250) | 175h (R$ 8.750) | 45h + 14h* (R$ 2.950) | 179h (R$ 8.950) | *A preencher* | *A preencher* | **1,02** | *- * |
| **Sprint 3 (Atual)** | `16/10` – `30/10` | 40h (R$ 2.000) | 215h (R$ 10.750) | *Em curso* | *Em curso* | *Em curso* | *Em curso* | *- * | *- * |
| **Sprint 4** | `31/10` – `13/11` | 40h (R$ 2.000) | 255h (R$ 12.750) | *Planejado* | *Planejado* | *Planejado* | *Planejado* | *- * | *- * |
| **Sprint 5 (Final)** | `14/11` – `23/11` | 25h (R$ 1.250) | **280h (R$ 14.000)** | *Planejado* | *Planejado* | *Planejado* | *Planejado* | *- * | *- * |

*\*Nota: Na Sprint 2, além das 45h planejadas (e da recuperação das 10h pendentes da Fase Inicial), foram adiantadas **14h** de tarefas previstas para as Sprints 3 e 4 (`Minimax` = 7h da Sprint 3 e `Poda Alfa-Beta` = 7h da Sprint 4 em [`IA/src/ia.c`](../IA/src/ia.c)), elevando o EV acumulado para **179h (R$ 8.950,00)** frente a **175h (R$ 8.750,00)** planejadas até `15/10` (**SPI acumulado = 1,02**).*

---

## 5. Backlog de Extensão Opcional (Fora do Caminho Crítico do MVP)

Conforme documentado na [`EAP.jpeg`](./EAP.jpeg) (itens em cinza) e em [`arquitetura-xadrez.md` §5](../engine/docs/arquitetura-xadrez.md), os pacotes `1.7` e `1.8` compõem a reserva de expansão para **Multiplayer Online** (144h = R$ 7.200,00, fechando as 424h = R$ 21.200,00 do Planning Poker) e só entram caso o MVP (Sprints 1 a 5) seja concluído com folga:

- **`1.7` Servidor Backend (89h — R$ 4.450,00 | Gantt `#123`–`#145`):** Gerenciamento de sessões/partidas (`1.7.1`), integração assíncrona com instâncias da Engine (`1.7.2`), API REST (`1.7.3`) e WebSockets em tempo real (`1.7.4`).
- **`1.8` Cliente Web (55h — R$ 2.750,00):** Interface gráfica web (`1.8.1`), integração REST/WebSocket (`1.8.2`) e fluxo de partida online (`1.8.3`–`1.8.4`).

---

## 6. Instruções para o Monitoramento Automatizado (Agente via MCP GitHub + WhatsApp)

Nas execuções periódicas (a cada 1, 3 ou 5 dias), o agente deve seguir este roteiro:

1. **Identificar a Sprint Atual** pela data corrente na Seção 3 deste arquivo (`docs/sprints.md`).
2. **Consultar o Repositório via GitHub MCP** (`Alessandro-F-Ferreira-F/Xadrez-ES2`):
   - Listar as **Issues** abertas e fechadas (filtrando por milestone, label da Sprint ou cruzando com a coluna `Issue (GitHub)` e os títulos da EAP acima).
   - Verificar **Pull Requests** abertos/mergeados e commits recentes nas branches para identificar progresso em andamento mesmo quando uma Issue ainda não foi fechada.
3. **Analisar Burndown e Desvios de Escopo/Prazo:**
   - **Da Sprint Atual:** Quais tarefas/issues já foram concluídas e quais ainda estão abertas? Quantas horas restam no Burndown da Sprint até a data de fechamento?
   - **Adiantamentos (Sprints Futuras):** Alguma issue ou entrega pertencente às Sprints seguintes já foi concluída? (Somar no **EV** da análise!).
   - **Atrasos (Sprints Anteriores):** Alguma issue de Sprint anterior ficou em aberto e escorregou para a Sprint atual?
4. **Calcular Indicadores de Valor Agregado (EVM):**
   - Calcular **PV** até a data, **EV** (horas dos pacotes/issues concluídos $\times$ R$ 50,00) e, quando houver registro de horas gastas nas issues/documento, **AC**, **SPI** ($\text{EV}/\text{PV}$) e **CPI** ($\text{EV}/\text{AC}$).
5. **Enviar Resumo no WhatsApp via MCP:**
   - Enviar uma mensagem direta, clara e motivadora para o grupo contendo:
     - 📅 **Sprint Atual e dias restantes** até o fim da Sprint (e até a Apresentação);
     - ✅ **O que já foi entregue** na Sprint (e o que já adiantaram de Sprints futuras);
     - 🎯 **Foco prioritário (pendências da Sprint atual)** e issues abertas associadas;
     - 📊 **Status do Burndown e EVM** (`PV`, `EV`, `SPI` e alerta caso alguma tarefa do caminho crítico esteja bloqueando a integração).
