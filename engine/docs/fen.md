<img src="game_example.png" width="500" Alt="Description">

O formato FEN desse jogo seria: "rnbqk1nr/pppp1ppp/8/2b5/3pP3/5N2/PPP2PPP/RNBQKB1R"

#define START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
                     |      |                           |      |
                     56     63                          0      7
board[64] = 
                [0, 1, 2, 3, ..., 61, 62, 63]

                0  -->  a1
                7  -->  h1
                56 -->  a8
                63 -->  h8

posição 56: rank = 7, file = 0;
posição 64: rank = 7, file = 7;
posição 48: rank = 6, file 0  ->  6 * 8 + 0 = 48 