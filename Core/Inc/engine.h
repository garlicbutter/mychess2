#ifndef __ENGINE_H
#define __ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vice_defs.h"

void make_dumb_computer_move(void);
int attempt_human_move(int from_sq, int to_sq);
char* sq64_to_str(int sq64, char *buf);
int check_game_over(S_BOARD *pos);
void init_chess_engine(void);

extern S_BOARD engine_board;

#ifdef __cplusplus
}
#endif

#endif /* __ENGINE_H */
