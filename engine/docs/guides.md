## O que cada mecanismo deve implementar/verificar?

`board_check_invariants()` deve verificar o seguinte:
* exatamente 1 rei de cada cor no tabuleiro
* `board->king_sq[n]` é de fato a posição dos reis no tabuleiro 
* nenhum peão no rank 1 ou 8
* `ep_square`, se definido, está na 6ª fileira quando é a vez das brancas e na 3ª quando é das pretas 
* se `castling & CASTLE_WK` então deve haver um rei branco em e1 e uma torre branca em h1 (e casos análogos)
* nenhum código de peça inválido