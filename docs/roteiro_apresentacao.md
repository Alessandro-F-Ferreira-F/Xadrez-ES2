# Roteiro da Primeira Apresentação (Slides)

Este documento contém o conteúdo estruturado e pronto para você copiar e colar no **Google Slides** ou **PowerPoint**. A apresentação deve durar entre 15 e 20 minutos.

---

## Slide 1: Capa
**Título:** Projeto Xadrez Cliente/Servidor
**Subtítulo:** Primeira Apresentação de Acompanhamento
**Tópicos/Texto:** 
- Nome dos integrantes do grupo: [NOME 1], [NOME 2], [NOME 3], [NOME 4], [NOME 5], [NOME 6], [NOME 7]
- Data da apresentação
- Professor/Disciplina

---

## Slide 2: Escopo do Produto (Requisitos)
**Título:** Escopo do Produto
**Tópicos/Texto:** 
- **Objetivo:** Aplicação de xadrez em arquitetura cliente-servidor, permitindo partidas contra IA e Multiplayer Local. Expansão futura para Online.
- **Módulos Principais:**
  1. *Cliente Desktop:* Interface gráfica (GUI), tabuleiro renderizado, validação visual de lances.
  2. *Web Backend API:* Máquina de regras rigorosa, validação oficial, execução da IA (Minimax).
  3. *Módulo Multiplayer:* Comunicação via WebSockets e controle de sessões.
- **Destaque:** Sistema modular, multiplataforma (Win/Mac/Linux) e tolerante a falhas (reconexões).

---

## Slide 3: Escopo do Projeto (EAP)
**Título:** Estrutura Analítica do Projeto (EAP)
**Tópicos/Texto:** 
- Separação clara em pacotes de desenvolvimento e gerência:
  - **1.2 Gerenciamento:** Documentação, Riscos e Reuniões.
  - **1.3 Clientes Desktop:** Interface visual, HUD e menus.
  - **1.4 Máquina de Regras:** A "fonte da verdade" do jogo, validações de movimentos e fins de partida (Mate, Afogamento).
  - **1.5 IA:** Algoritmo Minimax com Poda Alfa-Beta.
  - **1.7 e 1.8 Servidor e Cliente Web:** APIs, WebSockets e gestão de partidas simultâneas.

---

## Slide 4: Cronograma e Planejamento
**Título:** Cronograma de Desenvolvimento (Gantt)
**Tópicos/Texto:** 
- *(Insira a imagem ou print do arquivo `cronograma_gantt.pdf` aqui)*
- O desenvolvimento seguirá uma abordagem iterativa e incremental.
- Paralelismo planejado entre a equipe de Interface Gráfica e a equipe do Motor de Regras (Backend).
- Entregas divididas e alinhadas com as datas das próximas apresentações.

---

## Slide 5: Estimativas de Esforço
**Título:** Estimativas de Esforço (APF vs. Planning Poker)
**Tópicos/Texto:** 
- **Análise de Pontos de Função (APF):**
  - Identificados 62 Pontos de Função Ajustados baseados nas transações do sistema (Logins, Partidas, IA).
  - Estimativa de 496 horas totais (assumindo 8h/PF).
- **Planning Poker (Estimativa da Equipe):**
  - Esforço focado nos pacotes da EAP.
  - Estimativa final consolidada: **424 horas**.
  - *Conclusão:* A equipe estima ser levemente mais rápida do que a média projetada pela APF devido ao conhecimento prévio em algoritmos de busca.

---

## Slide 6: Custo e Orçamento
**Título:** Orçamento do Projeto
**Tópicos/Texto:** 
- **Valor base:** Adoção de **R$ 50,00 / hora** (perfil de desenvolvedor júnior).
- **Cálculo Baseado no Planning Poker (424h):**
  - **Custo Total Estimado:** R$ 21.200,00
- **Cálculo Baseado na APF (496h):**
  - **Custo Total Projetado (Teto):** R$ 24.800,00
- *Conclusão:* O orçamento de execução deve ser fixado em **R$ 21.200,00**, com uma margem de segurança técnica até o teto da APF.

---

## Slide 7: Análise de Riscos (Principais)
**Título:** Levantamento e Mitigação de Riscos
**Tópicos/Texto:** 
- **Risco 1:** Erros na implementação das regras do xadrez. *(Probabilidade Alta / Impacto Muito Alto)*
  - *Contenção:* Criação de testes unitários rígidos e TDD na máquina de regras.
- **Risco 2:** Complexidade/Baixo desempenho da IA Minimax. *(Probabilidade Média / Impacto Alto)*
  - *Contenção:* Definir limite de profundidade rígido e adotar poda alfa-beta logo no início.
- **Risco 3:** Subestimação do esforço necessário. *(Probabilidade Alta / Impacto Alto)*
  - *Contenção:* Acompanhamento estrito semanal, divisão das tarefas em partes pequenas (< 8h) e correção no EVM.

---

## Slide 8: Monitoramento e Controle (Sprint 1)
**Título:** Acompanhamento da 1ª Iteração (Burndown)
**Tópicos/Texto:** 
- *(Você pode desenhar um gráfico simples no Google Slides usando estes dados)*
- **Planejado:** 40h de esforço total para os pacotes iniciais de gerenciamento e documentação.
- **Burndown:** Iniciamos com 40h e chegamos ao fim da semana com 5h de trabalho pendentes.
- **Motivo do desvio:** Esforço não previsto no mapeamento aprofundado dos riscos e validações da arquitetura.

---

## Slide 9: Análise de Valor Agregado (EVM)
**Título:** Desempenho do Projeto (EVM) - Iteração 1
**Tópicos/Texto:** 
- **PV (Planejado):** R$ 2.000,00 *(40h)*
- **EV (Agregado):** R$ 1.750,00 *(Trabalho entregue: 35h)*
- **AC (Custo Real):** R$ 1.900,00 *(Trabalho gasto: 38h)*
- **SPI (Desempenho de Prazos):** 0,875 *(Leve atraso no cronograma)*
- **CPI (Desempenho de Custos):** 0,92 *(Gastamos mais esforço que o esperado para a entrega)*
- **Plano de Ação:** Aumentar a coesão da equipe na próxima iteração (programação em pares) para recuperar a velocidade.

---

## Slide 10: Demo Parcial
**Título:** Protótipo / Demonstração
**Tópicos/Texto:** 
- *Aviso Verbal:* "Apesar do foco inicial ter sido no planejamento arquitetural e nos requisitos, temos uma versão inicial do nosso protótipo."
- Demonstração visual do tabuleiro gráfico sendo renderizado usando SFML no cliente C++.
- Demonstração técnica da inicialização do tabuleiro e leitura de FEN (Notação de Forsyth-Edwards) funcionando no motor de regras desenvolvido em C.
- *(Fim da Apresentação, espaço para perguntas e agradecimento).*
