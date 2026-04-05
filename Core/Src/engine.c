#include "stdlib.h"
#include "main.h"
#include "engine.h"
#include "render.h"

static S_SEARCHINFO info[1];

void init_chess_board(S_BOARD* board) {
	AllInit(); // VICE's internal lookup table setup

	board->HashTable->pTable = NULL;
	InitHashTableKb(board->HashTable, 4);

	// Load the starting pieces onto the board
	ParseFen(START_FEN, board);
}

int engine_make_move(S_BOARD* board) {
	static int engine_make_move_printed = 1;

	int move;

	move = make_random_move(board);
	if (engine_make_move_printed) {
		printf("move=random\n");
		engine_make_move_printed = 0;
	}

//	move = make_smart_move(board);
//	if (engine_make_move_printed) {
//		printf("move=search\n");
//		engine_make_move_printed = 0;
//	}

	ASSERT(MakeMove(board, move));
	return move;
}

// very crude AI
int make_random_move(S_BOARD* board) {
    S_MOVELIST list[1];
    int legal_moves[256]; // Max possible chess moves in any position is ~218
    int legal_count = 0;

    GenerateAllMoves(board, list);
    for (int i = 0; i < list->count; ++i) {
    	board->ply = 0; // Reset ply to prevent memory overflow

        int move = list->moves[i].move;
        if (MakeMove(board, move)) {
            // The move is strictly legal! Undo it so we don't change the board state yet.
            TakeMove(board);
            legal_moves[legal_count++] = move;
        }
    }
    if (legal_count == 0) {
        return 0;
    }

    // --- PRIORITIZE CAPTURES ---
    int legal_captures[256];
    int capture_count = 0;
    srand(HAL_GetTick()); // randomize seed

    for (int i = 0; i < legal_count; ++i) {
        if (CAPTURED(legal_moves[i]) != 0) {
            legal_captures[capture_count++] = legal_moves[i];
        }
    }

    // If there are captures available, pick a random one!
    if (capture_count > 0) {
        int random_index = rand() % capture_count;
        return legal_captures[random_index];
    }
    int random_index = rand() % legal_count;
    return legal_moves[random_index];
}

int make_smart_move(S_BOARD* board) {

    /* 1. Configure the Search Limits */
    info->depth = 2;        // Start with 3 half-moves deep. Increase to 4 if it's too fast.
    info->timeset = TRUE;  // Tell VICE not to stop based on a clock timer
    info->stoptime = GetTimeMs() + 1000;
    info->quit = FALSE;
    info->nodes = 0;        // Reset node counter

    board->ply = 0;   // Reset your ply depth

    /* 2. Run the AI (This will block the CPU until it finishes!) */
    int best_move = SearchPosition(board, info);

    /* 3. Execute the best move if one was found */
    if(best_move != 0) {
        board->ply = 0; // Reset ply to prevent memory overflow on next turn
        printf("AI(Searched %ld nodes)\n",info->nodes);
    }

	return best_move;
}

int check_human_move_valid(S_BOARD* board, int from_sq, int to_sq) {
    S_MOVELIST list[1];
    GenerateAllMoves(board, list);
    
    int parsed_move = NOMOVE;
    for (int i = 0; i < list->count; ++i) {
        int move = list->moves[i].move;
        
        if (FROMSQ(move) == from_sq && TOSQ(move) == to_sq) {
            int promoted_piece = PROMOTED(move);
            if (promoted_piece != EMPTY) {
                // For a basic GUI, auto-promote to Queen. 
                // TODO: handle underpromotions (Knight, Bishop, Rook).
                if (promoted_piece == wQ || promoted_piece == bQ) {
                    parsed_move = move;
                    break;
                }
                continue; 
            }
            // Standard move (no promotion)
            parsed_move = move;
            break;
        }
    }

    if (parsed_move != NOMOVE) {
        if (MakeMove(board, parsed_move)) {
            return TRUE; 
        }
    }
    
    return FALSE; 
}


char* sq64_to_str(int sq64, char *buf) {
	if (sq64 < 0 || sq64 > 63) {
		buf[0] = '-';
		buf[1] = '-';
		buf[2] = '\0';
		return buf;
	}

	int file = sq64 % 8;
	int rank = sq64 / 8;

	buf[0] = 'a' + file;
	buf[1] = '1' + rank;
	buf[2] = '\0';

	return buf;
}

int check_game_over(S_BOARD *board) {
    S_MOVELIST list[1];
    GenerateAllMoves(board, list);
    
    int legal_moves_found = 0;
    for (int i = 0; i < list->count; ++i) {
        int move = list->moves[i].move;
        if (MakeMove(board, move)) {
            legal_moves_found++;
            TakeMove(board); // We found a valid move, so undo it immediately
            break;         // We only need to find 1 legal move to know the game isn't over
        }
    }
    
    // No legal moves exist for the current side
    if (legal_moves_found == 0) {
        int InCheck = SqAttacked(board->KingSq[board->side], board->side ^ 1, board);
        if (InCheck) {
            if (board->side == WHITE) {
                printf("\nCheckmate! Black wins!\n");
            } else {
                printf("\nCheckmate! White wins!\n");
            }
        } else {
            printf("\nSTALEMATE! Game is a Draw!\n");
        }
        return 1; // Game Over
    } 
    
    if (board->fiftyMove >= 100) {
        printf("\nDraw by Fifty-Move Rule!\n");
        return 1;
    }
    
    // TODO:  handle Threefold Repetition
    return 0; // Game Continues
}
