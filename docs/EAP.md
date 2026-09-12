1 Xadrez
    1.1 Apresentações
        1.1.1 Primeira Apresentação
        1.1.2 Segunda Apresentação
        1.1.3 Terceira Apresentação

    1.2 Gerenciamento
        1.2.1 Criar Repositório para Controle de Versão
        1.2.2 Análise de Valor Agregado
        1.2.3 Reuniões
            1.2.3.1 Reunião 04/09
            1.2.3.2 Reunião 11/09
            1.2.3.3 Reunião 18/09
            1.2.3.4 Reunião 25/09
            1.2.3.5 Reunião 02/10
            1.2.3.6 Reunião 09/10
            1.2.3.7 Reunião 16/10
            1.2.3.8 Reunião 23/10
            1.2.3.9 Reunião 30/10
            1.2.3.10 Reunião 06/11
            1.2.3.11 Reunião 13/11
            1.2.3.12 Reunião 20/11
        1.2.4 Documentação
            1.2.4.1 Levantamento de Requisitos
            1.2.4.2 Fazer a EAP
            1.2.4.3 Mapeamento dos Riscos
            1.2.4.4 Descrição do Escopo
        1.2.5 Testes e Validação
            1.2.5.1 Testes da Máquina de Regras
            1.2.5.2 Teste da IA
            1.2.5.3 Testes da Interface
            1.2.5.4 Testes da Integração

    1.3 Clientes Desktop
        1.3.1 Implementação do Tabuleiro
            1.3.1.1 Renderização do Tabuleiro
            1.3.1.2 Renderização das Peças
        1.3.2 Sistema de Interação
            1.3.2.1 Entrada de Lances
            1.3.2.2 Seleção de Peças
            1.3.2.3 Seleção de Movimentos
            1.3.2.4 Destaque de Movimentos Legais
        1.3.3 HUD da Partida
            1.3.3.1 Indicadores de Estado da Partida
            1.3.3.2 Exibição do Histórico de Lances
        1.3.4 Telas de Menu
            1.3.4.1 Seleção de Modo de Jogo
            1.3.4.2 Seleção de Dificuldade
            1.3.4.3 Configurações
        1.3.5 Fluxo da Partida Local
            1.3.5.1 Início de Nova Partida
            1.3.5.2 Configuração dos Parâmetros da IA
            1.3.5.3 Tratamento do Fim da Partida
        1.3.6 Integração com a Engine
            1.3.6.1 Inicialização da Engine
            1.3.6.2 Comunicação com a Engine
            1.3.6.3 Tratamento de Falhas
            1.3.6.4 Encerramento da Engine

    1.4 Máquina de Regras
        1.4.1 Representação da Posição
            1.4.1.1 Estrutura da Posição e seus Metadados
            1.4.1.2 Codificação de Peças
            1.4.1.3 Codificação de Lances
            1.4.1.4 Conversão entre Notações e Representação Interna
        1.4.2 Tabuleiro e Peças
            1.4.2.1 Implementação do Tabuleiro Lógico
            1.4.2.2 Implementação das Peças M.REGRAS
        1.4.3 Geometria e Tabelas de Movimentos
            1.4.3.1 Geometria do Tabuleiro
            1.4.3.2 Tabelas de Movimentos e Ataques
        1.4.4 Geração de Movimentos
            1.4.4.1 Geração de Movimentos Pseudo-Legais
            1.4.4.2 Geração de Movimentos Legais
        1.4.5 Validação de Movimentos
            1.4.5.1 Validador de Movimentos
            1.4.5.2 Validador de Movimentos Especiais
                1.4.5.2.1 Validador de Roque
                1.4.5.2.2 Validador de Promoção
                1.4.5.2.3 Validador de En Passant
        1.4.6 Controle da Partida
            1.4.6.1 Controlador de Turnos
            1.4.6.2 Multiplayer Local
        1.4.7 Detecção de Estados de Jogo
            1.4.7.1 Detecção de Xeque
            1.4.7.2 Detecção de Xeque-Mate
            1.4.7.3 Detecção de Afogamento
            1.4.7.4 Detecção de Empates
        1.4.8 Aplicação e Reversão de Lances
            1.4.8.1 Aplicação de Lances
            1.4.8.2 Reversão de Lances
            1.4.8.3 Histórico de Lances e Undo
        1.4.9 Notação FEN

    1.5 IA
        1.5.1 Pesquisa
        1.5.2 Integração com a Máquina de Regras
        1.5.3 Simulação de Estados
        1.5.4 Avaliação de Posição
            1.5.4.1 Avaliação Material
            1.5.4.2 Avaliação Posicional
        1.5.5 Busca de Melhor Lance
            1.5.5.1 Algoritmo Minimax
            1.5.5.2 Poda Alfa-Beta
            1.5.5.3 Aprofundamento Progressivo
            1.5.5.4 Ordenação de Movimentos
        1.5.6 Seleção de Lance Aleatório

    1.6 Protocolo e Infraestrutura do Motor
        1.6.1 Fundação de Qualidade
            1.6.1.1 Sistema de Build
            1.6.1.2 Correção de Defeitos
            1.6.1.3 Configuração de Avisos do Compilador
            1.6.1.4 Registro de Erros
            1.6.1.5 Framework de Testes Automatizados
        1.6.2 Protocolo de Comunicação
            1.6.2.1 Interface UCI
            1.6.2.2 Histórico de Partida
            1.6.2.3 Comunicação de Entrada e Saída
            1.6.2.4 Parsing de Comandos
            1.6.2.5 Conversão de Notação de Lances

    1.7 Servidor
        1.7.1 Gerenciamento de Partidas
            1.7.1.1 Sessões de Partida
            1.7.1.2 Gerenciamento de Jogadores
            1.7.1.3 Estado das Partidas
        1.7.2 Integração com a Engine
            1.7.2.1 Gerenciamento das Instâncias da Engine
            1.7.2.2 Comunicação com a Engine
            1.7.2.3 Execução Assíncrona da IA
            1.7.2.4 Isolamento de Falhas
        1.7.3 API REST
            1.7.3.1 Criação de Partidas
            1.7.3.2 Consulta de Partidas
            1.7.3.3 Submissão de Lances
            1.7.3.4 Consulta de Lances Legais
            1.7.3.5 Encerramento por Desistência
            1.7.3.6 Verificação de Disponibilidade
        1.7.4 Comunicação em Tempo Real
            1.7.4.1 Notificação de Lances
            1.7.4.2 Notificação de IA Pensando
            1.7.4.3 Notificação de Fim de Partida
            1.7.4.4 Notificação de Lance Inválido
        1.7.5 Validação e Segurança dos Lances Recebidos

    1.8 Cliente Web
        1.8.1 Interface Gráfica
            1.8.1.1 Tabuleiro e Peças
            1.8.1.2 Entrada de Lances
            1.8.1.3 Indicadores de Estado
            1.8.1.4 Destaque de Lances Legais
        1.8.2 Comunicação com o Servidor
            1.8.2.1 Integração com API REST
            1.8.2.2 Integração com WebSocket
            1.8.2.3 Tratamento de Erros e Reconexão
        1.8.3 Fluxo da Partida
            1.8.3.1 Início de Partida
            1.8.3.2 Histórico de Lances
            1.8.3.3 Tratamento do Fim da Partida
        1.8.4 Entrada em Sessões Existentes para Multiplayer]