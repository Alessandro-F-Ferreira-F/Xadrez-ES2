---
marp: true
theme: default
class: 
  - lead
  - invert
paginate: true
backgroundColor: #1a1a2e
---

<!-- Eduardo -->
# Projeto Xadrez
## Primeira Apresentação de Acompanhamento

**Integrantes:** Felipe Cabral Liporage, Eduardo Rottschaefer Oliveira, Caio Veiga Pires, Alessandro Felipe Ferreira, José Enrique Viana, João Vitor Pereira Rodrigues, Allan Vignoli
**Data:** 14 de Setembro de 2026
**Disciplina:** Engenharia de Software 2

---

<!-- José -->
<!-- Abrir Requisito -->
# Escopo do Produto

- **Objetivo:** Aplicação de xadrez em arquitetura cliente-servidor (contra IA e Multiplayer Local).
- **Módulos Principais:**
  1. *Cliente Desktop:* Interface gráfica (GUI), tabuleiro visual, validação de lances.
  2. *Engine* Máquina de Regras e IA com comunicação via E/S
  3. *Web Backend API:* Multiplayer Online com comunicação via WebSockets * 
- **Destaque:** Sistema modular, multiplataforma (Win/Mac/Linux) e tolerante a falhas.

---

# Estrutura Analítica do Projeto (EAP)
<!-- Alan -->
- **1.2 Gerenciamento:** Documentação, Riscos e Reuniões.
<!-- Eduardo, Alan -->
- **1.3 Clientes Desktop:** Interface visual, HUD e menus.
<!-- Caio -->
- **1.4 Máquina de Regras:** Validações de movimentos.
<!-- Alessandro -->
- **1.5 IA:** Algoritmo Minimax.
<!-- João -->
- **1.7 e 1.8 Servidor e Cliente Web:** APIs, WebSockets e gestão de partidas. * 

---

<!-- José -->
# Cronograma de Desenvolvimento (Gantt)

- O desenvolvimento seguirá uma abordagem iterativa e incremental.
- Paralelismo planejado entre a equipe de Interface Gráfica e a equipe do Motor de Regras.
- Entregas divididas e alinhadas com as datas das próximas apresentações.


---

# Estimativas de Esforço

  <!-- Cabral -->
- **Análise de Pontos de Função (APF):**
  - Identificados 62 Pontos de Função Ajustados.
  - Estimativa de **496 horas** totais (assumindo 8h/PF).
  <!-- Cabral -->
- **Planning Poker (Estimativa da Equipe):**
  - Esforço focado nos pacotes da EAP.
  - Estimativa final consolidada: **424 horas**.


---
<!-- Cabral -->
# Orçamento do Projeto

- **Valor base:** Adoção de **R$ 50,00 / hora** (perfil júnior).
- **Cálculo Baseado no Planning Poker (424h):**
  - **Custo Total Estimado:** R$ 21.200,00
- **Cálculo Baseado na APF (496h):**
  - **Custo Total Projetado (Teto):** R$ 24.800,00

*Conclusão:* O orçamento de execução deve ser fixado em **R$ 21.200,00**, com margem de segurança técnica até o teto da APF.

---
<!-- Alan -->
# Levantamento e Mitigação de Riscos

- **Risco 1:** Erros na implementação das regras do xadrez. *(Prob: Alta / Imp: M. Alto)*
  - *Mitigação:* Testes unitários rígidos e TDD na máquina de regras.
- **Risco 2:** Complexidade/Baixo desempenho da IA Minimax. *(Prob: Média / Imp: Alto)*
  - *Mitigação:* Limite de profundidade rígido e poda alfa-beta.
- **Risco 3:** Subestimação do esforço necessário. *(Prob: Alta / Imp: Alto)*
  - *Mitigação:* Acompanhamento estrito semanal, tarefas < 8h e correção no EVM.

---
<!-- João -->
# Monitoramento e Controle

- O acompanhamento do projeto passou a ser feito ao longo de **3 iterações semanais**.
- *Iteração 1 (Fundação C):* Início forte, time superou expectativas.
- *Iteração 2 (Motor e Validação):* Maior complexidade técnica, causando leve atraso.
- *Iteração 3 (SFML e Gerência):* Divisão de equipes ajudou a paralelizar entregas, mas curva de aprendizado do C++ exigiu foco.

---

<!-- Caio -->
# Desempenho do Projeto (EVM) - Consolidado

| Iteração | Planejado (PV) | Agregado (EV) | Custo Real (AC) | SPI (Prazo) | CPI (Custo) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **1ª Sem** | 20h (R$ 1k) | 20h (R$ 1k) | 18h (R$ 900) | **1.00** | **1.11** |
| **2ª Sem** | 30h (R$ 1.5k) | 25h (R$ 1.25k)| 35h (R$ 1.75k)| **0.83** | **0.71** |
| **3ª Sem** | 40h (R$ 2k) | 35h (R$ 1.75k)| 38h (R$ 1.9k) | **0.87** | **0.92** |
| **Total**  | 90h (R$ 4.5k) | 80h (R$ 4k)   | 91h (R$ 4.55k)| **0.88** | **0.87** |

*Plano de Ação:* Pareamento de tarefas mais complexas no C++ (HUD e mouse drag) na próxima iteração.

---

# Protótipo / Demonstração

<!-- - **Demonstração visual** do tabuleiro gráfico sendo renderizado (C++ / SFML).
- **Demonstração técnica** da inicialização do tabuleiro e leitura de posições FEN no motor de regras (C). -->

## Obrigado! Perguntas?
