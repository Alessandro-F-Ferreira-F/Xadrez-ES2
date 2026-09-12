# Monitoramento e Controle (Iteração 1)

Neste documento apresentamos o acompanhamento da Iteração 1 do projeto, cujo foco principal foi o planejamento, a definição de requisitos, a estruturação da EAP e do cronograma.

## 1. Dados do Burndown (Iteração 1)

O *Burndown chart* acompanha a quantidade de horas restantes versus o tempo de projeto na iteração. A iteração 1 teve duração de 1 semana (5 dias úteis), com **40 horas planejadas** (divididas entre os membros da equipe).

| Dia | Trabalho Planejado Restante (h) | Trabalho Real Restante (h) |
| :--- | :---: | :---: |
| Dia 0 (Início) | 40 | 40 |
| Dia 1 | 32 | 34 |
| Dia 2 | 24 | 28 |
| Dia 3 | 16 | 18 |
| Dia 4 | 8 | 10 |
| Dia 5 (Fim) | 0 | 5 |

**Análise do Burndown:**
Houve um leve desvio em relação à reta ideal. Isso ocorreu devido à complexidade extra encontrada na elaboração do Levantamento de Riscos e na definição técnica da arquitetura Cliente/Servidor. Ao final da iteração, restaram 5 horas de trabalho não concluído que transbordarão para a próxima sprint.

## 2. Análise de Valor Agregado (EVM) - Fim da Iteração 1

A Análise de Valor Agregado (Earned Value Management) mede o desempenho do projeto integrando escopo, cronograma e recursos (custo).

**Parâmetros Base:**
* **Valor da Hora:** R$ 50,00
* **Horas Planejadas para a Iteração 1:** 40 horas
* **Horas Entregues/Concluídas na Iteração 1:** 35 horas
* **Horas Realmente Gastas (Esforço Real) na Iteração 1:** 38 horas

**Cálculos do EVM:**
1. **PV (Valor Planejado / Planned Value):** O orçamento autorizado para o trabalho planejado.
   * `PV = 40h × R$ 50,00 = R$ 2.000,00`
2. **EV (Valor Agregado / Earned Value):** A medida do trabalho efetivamente realizado em termos do orçamento autorizado.
   * `EV = 35h × R$ 50,00 = R$ 1.750,00`
3. **AC (Custo Real / Actual Cost):** O custo real incorrido para realizar o trabalho.
   * `AC = 38h × R$ 50,00 = R$ 1.900,00`

**Indicadores de Desempenho:**
* **SPI (Índice de Desempenho de Prazos):** `EV / PV = 1750 / 2000 = 0,875`
  * *Interpretação:* SPI < 1. Estamos progredindo a 87,5% da velocidade planejada (pequeno atraso no cronograma).
* **CPI (Índice de Desempenho de Custos):** `EV / AC = 1750 / 1900 = 0,921`
  * *Interpretação:* CPI < 1. Para cada R$ 1,00 gasto pela equipe, geramos cerca de R$ 0,92 de valor (pequeno estouro no orçamento, exigindo mais esforço do que o planejado para a mesma entrega).

**Conclusão da Iteração:**
O projeto fechou a primeira iteração levemente atrasado e custando um pouco mais do que o previsto. Como plano de contenção para a próxima iteração, os pacotes de desenvolvimento de código serão quebrados em tarefas menores e o uso de programação em pares será incentivado para evitar gargalos na máquina de regras.
