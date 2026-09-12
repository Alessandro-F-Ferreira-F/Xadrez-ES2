---
marp: true
theme: default
class: 
  - lead
  - invert
paginate: true
backgroundColor: #1a1a2e
---

# Projeto Xadrez Cliente/Servidor
## Primeira Apresentação de Acompanhamento

**Integrantes:** [NOME 1], [NOME 2], [NOME 3], [NOME 4], [NOME 5], [NOME 6], [NOME 7]
**Data:** 12 de Setembro de 2026
**Disciplina:** Engenharia de Software 2

---

# Escopo do Produto

- **Objetivo:** Aplicação de xadrez em arquitetura cliente-servidor (contra IA e Multiplayer Local).
- **Módulos Principais:**
  1. *Cliente Desktop:* Interface gráfica (GUI), tabuleiro visual, validação de lances.
  2. *Web Backend API:* Máquina de regras rigorosa, execução da IA (Minimax).
  3. *Módulo Multiplayer:* Comunicação via WebSockets.
- **Destaque:** Sistema modular, multiplataforma (Win/Mac/Linux) e tolerante a falhas.

---

# Estrutura Analítica do Projeto (EAP)

- **1.2 Gerenciamento:** Documentação, Riscos e Reuniões.
- **1.3 Clientes Desktop:** Interface visual, HUD e menus.
- **1.4 Máquina de Regras:** "Fonte da verdade" do jogo, validações de movimentos.
- **1.5 IA:** Algoritmo Minimax com Poda Alfa-Beta.
- **1.7 e 1.8 Servidor e Cliente Web:** APIs, WebSockets e gestão de partidas.

---

# Cronograma de Desenvolvimento (Gantt)

- O desenvolvimento seguirá uma abordagem iterativa e incremental.
- Paralelismo planejado entre a equipe de Interface Gráfica e a equipe do Motor de Regras (Backend).
- Entregas divididas e alinhadas com as datas das próximas apresentações.

*(Consulte o arquivo cronograma_gantt.pdf para a visão completa)*

---

# Estimativas de Esforço

- **Análise de Pontos de Função (APF):**
  - Identificados 62 Pontos de Função Ajustados.
  - Estimativa de **496 horas** totais (assumindo 8h/PF).

- **Planning Poker (Estimativa da Equipe):**
  - Esforço focado nos pacotes da EAP.
  - Estimativa final consolidada: **424 horas**.

*Conclusão:* A equipe estima ser levemente mais rápida devido ao conhecimento prévio em algoritmos de busca.

---

# Orçamento do Projeto

- **Valor base:** Adoção de **R$ 50,00 / hora** (perfil júnior).
- **Cálculo Baseado no Planning Poker (424h):**
  - **Custo Total Estimado:** R$ 21.200,00
- **Cálculo Baseado na APF (496h):**
  - **Custo Total Projetado (Teto):** R$ 24.800,00

*Conclusão:* O orçamento de execução deve ser fixado em **R$ 21.200,00**, com margem de segurança técnica até o teto da APF.

---

# Levantamento e Mitigação de Riscos

- **Risco 1:** Erros na implementação das regras do xadrez. *(Prob: Alta / Imp: M. Alto)*
  - *Mitigação:* Testes unitários rígidos e TDD na máquina de regras.
- **Risco 2:** Complexidade/Baixo desempenho da IA Minimax. *(Prob: Média / Imp: Alto)*
  - *Mitigação:* Limite de profundidade rígido e poda alfa-beta.
- **Risco 3:** Subestimação do esforço necessário. *(Prob: Alta / Imp: Alto)*
  - *Mitigação:* Acompanhamento estrito semanal, tarefas < 8h e correção no EVM.

---

# Monitoramento e Controle (Sprint 1)
## Acompanhamento da 1ª Iteração (Burndown)

- **Planejado:** 40h de esforço total para os pacotes iniciais.
- **Burndown:** Iniciamos com 40h e chegamos ao fim da semana com 5h pendentes.
- **Motivo do desvio:** Esforço não previsto no mapeamento aprofundado dos riscos e validações da arquitetura.

*(Tarefas individuais mapeadas em nossa ferramenta de gestão)*

---

# Desempenho do Projeto (EVM) - Iteração 1

- **PV (Planejado):** R$ 2.000,00 *(40h)*
- **EV (Agregado):** R$ 1.750,00 *(Trabalho entregue: 35h)*
- **AC (Custo Real):** R$ 1.900,00 *(Trabalho gasto: 38h)*
- **SPI (Desempenho de Prazos):** 0,875 *(Leve atraso)*
- **CPI (Desempenho de Custos):** 0,92 *(Gastamos mais esforço que o esperado)*

*Plano de Ação:* Aumentar a coesão da equipe na próxima iteração (pair programming) para recuperar a velocidade.

---

# Protótipo / Demonstração

- Apesar do foco inicial ter sido no planejamento arquitetural e nos requisitos, temos uma versão inicial do nosso protótipo.
- **Demonstração visual** do tabuleiro gráfico sendo renderizado (C++ / SFML).
- **Demonstração técnica** da inicialização do tabuleiro e leitura de posições FEN no motor de regras (C).

## Obrigado! Perguntas?
