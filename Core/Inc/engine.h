#ifndef __ENGINE_H
#define __ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vice_defs.h"

extern S_BOARD engine_board;

void init_chess_board(S_BOARD* board);
int engine_make_move(S_BOARD* board);
int make_random_move(S_BOARD* board);
int make_smart_move(S_BOARD* board);
int check_game_over(S_BOARD *board);
int check_human_move_valid(S_BOARD* board, int from_sq, int to_sq);

char* sq64_to_str(int sq64, char *buf);


#ifdef __cplusplus
}
#endif

#endif /* __ENGINE_H */
