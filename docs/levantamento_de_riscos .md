*********Levantamento De Riscos*********


Pesos de 1 a 5 -> 1 - Muito pouco provável / Impacto muito baixo
	       -> 2 - Pouco provável / Impacto baixo
	       -> 3 - Mais ou menos provável / Impacto mediano
	       -> 4 - Provável / Impacto alto
	       -> 5 - Altamente provável / Impacto muito alto

R1 - Erros na implementação das regras do xadrez - Probabilidade = 4; Impacto = 5

R2 - Complexidade/baixo desempenho do Minimax + Alfa-Beta - Probabilidade = 3; Impacto = 4

R3 - Problemas de integração entre os submódulos - Probabilidade = 3; Impacto = 5 //Mesmo que cada componente funcione individualmente, o sistema completo pode apresentar problemas. Por exemplo: A interface pode enviar um movimento em um formato diferente daquele esperado pela Máquina de Regras, ou a IA pode receber um estado do tabuleiro incorreto.

R4 - Atrasos no desenvolvimento dos submódulos - Probabilidade = 5; Impacto = 4

R5 - Interface gráfica apresentar problemas de usabilidade - Probabilidade = 3; Impacto = 3 //dificuldade para selecionar peças;
ausência de indicação de movimentos válidos; feedback insuficiente; dificuldade para identificar de quem é a vez; tabuleiro pouco intuitivo;
mensagens de erro pouco claras.

R6 - Alterações frequentes nos requisitos - Probabilidade = 2; Impacto = 3

R7 - Subestimação do esforço necessário - Probabilidade = 5; Impacto = 4

R8 - Falhas na comunicação entre membros da equipe - Probabilidade = 3; Impacto = 4

R9 - Bugs difíceis de reproduzir ou identificar - Probabilidade = 4; Impacto = 4


*********Planos de Contingencia / Contenção*********

R1 - 

Plano de Contenção : Especificar as regras antes da implementação; criar testes automatizados para movimentos e casos especiais.

Plano de Contingência : Isolar e corrigir a regra defeituosa; suspender temporariamente funcionalidades dependentes dela; priorizar correções das regras que impedem o funcionamento do jogo.

R2 - 

Plano de Contenção : Definir limite de profundidade; utilizar ordenação de movimentos e função de avaliação simples e eficiente; realizar testes de desempenho desde o início.

Plano de Contingência : Reduzir temporariamente a profundidade da busca ou o tempo máximo de processamento; utilizar uma versão simplificada da função de avaliação; priorizar jogabilidade em vez de força da IA.

R3 - 

Plano de Contenção : Integrar os módulos gradualmente; realizar integração contínua; definir contratos entre Interface, Máquina de Regras e IA.

Plano de Contingência : Identificar o módulo responsável; voltar temporariamente à última versão estável; corrigir a interface e realizar novamente os testes de integração.

R4 - 

Plano de Contenção : Utilizar o Gantt para acompanhar o progresso; definir marcos; identificar dependências entre atividades; reservar pequenas folgas no cronograma.

Plano de Contingência : Repriorizar funcionalidades; reduzir ou adiar funcionalidades não essenciais; redistribuir tarefas entre integrantes; atualizar o Gantt com o novo cronograma.

R5 - 

Plano de Contenção : Criar protótipos simples antes da implementação completa; realizar testes com usuários; manter a interface simples e consistente.

Plano de Contingência : Corrigir os elementos que dificultam a utilização; simplificar telas e comandos; priorizar os problemas que impedem o usuário de jogar.

R6 - 

Plano de Contenção : Definir e documentar os requisitos antes da implementação; estabelecer critérios para aceitar mudanças; separar requisitos essenciais de desejáveis.

Plano de Contingência : Avaliar o impacto da mudança no prazo e no esforço; priorizar a alteração caso seja necessária; adiar funcionalidades menos importantes para acomodar a mudança.

R7 - 

Plano de Contenção : Dividir as atividades em tarefas menores; utilizar estimativas baseadas em tarefas semelhantes; realizar Planning Poker ou outra técnica de estimativa; revisar estimativas periodicamente.

Plano de Contingência : Reestimar as atividades restantes; alterar prioridades; redistribuir recursos; remover ou simplificar funcionalidades de menor prioridade caso o prazo esteja comprometido.

R8 - 

Plano de Contenção : Realizar reuniões periódicas; documentar interfaces e decisões técnicas; definir responsabilidades.

Plano de Contingência : Realizar reunião para alinhar as decisões;

R9 - 

Plano de Contenção : Utilizar testes unitários, testes de integração e logs; testar casos normais e casos extremos; manter código modular.

Plano de Contingência : Reproduzir o erro em um caso de teste; isolar o módulo responsável; corrigir o defeito e adicionar um teste para impedir sua regressão.