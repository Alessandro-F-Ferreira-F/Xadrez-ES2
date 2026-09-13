# Estimativas de Esforço e Custo - APF (Análise de Pontos de Função)

## 1. Contagem de Pontos de Função (Estimativa Simplificada)

Considerando a complexidade média do projeto, identificamos os seguintes elementos funcionais principais:

| Função (Requisito) | Tipo | Complexidade | Pontos de Função (PF) |
| :--- | :---: | :---: | :---: |
| Autenticação e Login de Usuários (RF14, RF15) | ALI (Arquivo Lógico Interno) | Baixa | 7 |
| Histórico e Registro de Partidas (RF20, RF21) | ALI (Arquivo Lógico Interno) | Média | 10 |
| Motor de Xadrez / Máquina de Regras (RF09, RF11, RF12) | ALI (Arquivo Lógico Interno) | Alta | 15 |
| Envio de Movimento / Lance (RF05) | EE (Entrada Externa) | Média | 4 |
| Configuração de Partida (Modos, Dificuldade) (RF01, RF02) | EE (Entrada Externa) | Baixa | 3 |
| Convite Multiplayer Online (RF16) | EE (Entrada Externa) | Média | 4 |
| Consulta de Movimentos Legais (RF06, RF18) | CE (Consulta Externa) | Média | 4 |
| Busca Automática de Oponentes / Matchmaking (RF17) | CE (Consulta Externa) | Alta | 6 |
| Visualização do Tabuleiro / Interface (RF04, RF07, RF08) | SE (Saída Externa) | Média | 5 |
| Exportação de PGN (RF21) | SE (Saída Externa) | Baixa | 4 |
| **Total de Pontos de Função (Não Ajustados)** | | | **62 PF** |



## 2. Estimativa de Esforço

Utilizando uma produtividade de referência de mercado de **8 horas por Ponto de Função (PF)** para equipes juniores utilizando frameworks modernos:

* **Esforço Total Estimado:** 62 PF × 8 horas/PF = **496 horas**

## 3. Estimativa de Custos e Orçamento

Com base no valor homem-hora definido para a equipe de desenvolvimento (perfil júnior):

* **Valor da Hora:** R$ 50,00
* **Esforço Total:** 496 horas

**Custo Total do Projeto (Orçamento APF):**
> 496 horas × R$ 50,00 = **R$ 24.800,00**
