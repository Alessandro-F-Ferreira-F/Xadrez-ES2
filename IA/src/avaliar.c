#include "avaliar.h"
#include "piece.h"

const int32_t VALOR_PECA[7] = {0, 100, 320, 330, 500, 900, 0};

// Fase do jogo, para interpolar as duas tabelas do rei. Pesos do PeSTO (não de Michniewski): cavalo e
// bispo 1, torre 2, dama 4, peão e rei 0. Na posição inicial a soma é FASE_MAX; com promoções pode
// passar, e é limitada a FASE_MAX
#define FASE_MAX 24
static const int32_t pesoFase[7] = {0, 0, 1, 1, 2, 4, 0};

// PSTs de Michniewski (Simplified Evaluation Function), do ponto de vista das brancas, escritas como na
// fonte: a primeira linha é a fileira 8 (a8..h8), a última a fileira 1 (a1..h1). No motor a1 = 0 e
// h8 = 63, então a peça branca na casa usa o índice casa ^ 56, e a preta (espelhada) usa a própria casa.
// Indexadas por PIECE_TYPE, de EMPTY a QUEEN; o rei tem as duas tabelas à parte
static const int32_t pst[6][64] = {
    // EMPTY
    {0},
    // PAWN
    {
          0,   0,   0,   0,   0,   0,   0,   0,
         50,  50,  50,  50,  50,  50,  50,  50,
         10,  10,  20,  30,  30,  20,  10,  10,
          5,   5,  10,  25,  25,  10,   5,   5,
          0,   0,   0,  20,  20,   0,   0,   0,
          5,  -5, -10,   0,   0, -10,  -5,   5,
          5,  10,  10, -20, -20,  10,  10,   5,
          0,   0,   0,   0,   0,   0,   0,   0,
    },
    // KNIGHT
    {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20,   0,   0,   0,   0, -20, -40,
        -30,   0,  10,  15,  15,  10,   0, -30,
        -30,   5,  15,  20,  20,  15,   5, -30,
        -30,   0,  15,  20,  20,  15,   0, -30,
        -30,   5,  10,  15,  15,  10,   5, -30,
        -40, -20,   0,   5,   5,   0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50,
    },
    // BISHOP
    {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,  10,  10,   5,   0, -10,
        -10,   5,   5,  10,  10,   5,   5, -10,
        -10,   0,  10,  10,  10,  10,   0, -10,
        -10,  10,  10,  10,  10,  10,  10, -10,
        -10,   5,   0,   0,   0,   0,   5, -10,
        -20, -10, -10, -10, -10, -10, -10, -20,
    },
    // ROOK
    {
          0,   0,   0,   0,   0,   0,   0,   0,
          5,  10,  10,  10,  10,  10,  10,   5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
          0,   0,   0,   5,   5,   0,   0,   0,
    },
    // QUEEN
    {
        -20, -10, -10,  -5,  -5, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,   5,   5,   5,   0, -10,
         -5,   0,   5,   5,   5,   5,   0,  -5,
          0,   0,   5,   5,   5,   5,   0,  -5,
        -10,   5,   5,   5,   5,   5,   0, -10,
        -10,   0,   5,   0,   0,   0,   0, -10,
        -20, -10, -10,  -5,  -5, -10, -10, -20,
    },
};

static const int32_t pstReiMeio[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20,
};

static const int32_t pstReiFinal[64] = {
    -50, -40, -30, -20, -20, -30, -40, -50,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -50, -30, -30, -30, -30, -30, -30, -50,
};

// Material + PST de cada peça; o rei entra pela interpolação das duas tabelas pela fase:
// (meio * fase + final * (FASE_MAX - fase)) / FASE_MAX, sobre o termo líquido (brancas - pretas).
// A divisão inteira trunca em direção a zero: -x / 24 == -(x / 24), o que mantém a simetria entre as cores
int32_t avaliar(const Board *b){
    int32_t totBrancas = 0, totPretas = 0;
    int32_t fase = 0;
    int reiBranco = 0, reiPreto = 0;

    for(int casa = 0; casa <= 63; casa ++){
        Piece aux = b->array[casa];
        if(aux == EMPTY) continue;

        PieceType tipo = PIECE_TYPE(aux);
        fase += pesoFase[tipo];

        if(PIECE_COLOR(aux) == WHITE){
            if(tipo == KING) reiBranco = casa;
            else totBrancas += VALOR_PECA[tipo] + pst[tipo][casa ^ 56];
        }
        else{
            if(tipo == KING) reiPreto = casa;
            else totPretas += VALOR_PECA[tipo] + pst[tipo][casa];
        }
    }

    if(fase > FASE_MAX) fase = FASE_MAX;

    int32_t reiMeio = pstReiMeio[reiBranco ^ 56] - pstReiMeio[reiPreto];
    int32_t reiFinal = pstReiFinal[reiBranco ^ 56] - pstReiFinal[reiPreto];
    int32_t rei = (reiMeio * fase + reiFinal * (FASE_MAX - fase)) / FASE_MAX;

    return totBrancas - totPretas + rei;
}
