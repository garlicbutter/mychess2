#include "main.h"
#include "engine.h"
#include "render.h"

S_BOARD engine_board = { 0 };

void make_dumb_computer_move(void) {
	S_MOVELIST list[1];
	GenerateAllMoves(&engine_board, list);

	int legal_moves[256]; // Buffer to store the actual safe moves
	int legal_count = 0;

	/* 1. Filter the pseudo-legal list into a strictly legal list */
	for (int moveNum = 0; moveNum < list->count; ++moveNum) {
		int move = list->moves[moveNum].move;

		/* If the move is safe, save it and immediately undo it */
		if (MakeMove(&engine_board, move)) {
			legal_moves[legal_count] = move;
			legal_count++;

			TakeMove(&engine_board); // Put the piece back!
		}
	}

	/* 2. Pick a random move from the safe list */
	if (legal_count > 0) {
		int random_index = rand() % legal_count;
		int chosen_move = legal_moves[random_index];

		/* Make the final chosen move for real */
		MakeMove(&engine_board, chosen_move);
		engine_board.ply = 0; // Reset ply counter to prevent memory leaks

		/* Optional: Print what the computer did to your terminal */
		char from_str[3], to_str[3];
		printf("Com: %s to %s\n",
				sq64_to_str(SQ64(FROMSQ(chosen_move)), from_str),
				sq64_to_str(SQ64(TOSQ(chosen_move)), to_str));
	}
}

int attempt_human_move(int from_sq, int to_sq) {
	S_MOVELIST list[1];
	GenerateAllMoves(&engine_board, list);

	int move = 0;
	int Legal = 0;

	// Loop through all generated moves
	for (int moveNum = 0; moveNum < list->count; ++moveNum) {

		move = list->moves[moveNum].move;

		// Check if the generated move matches where you dropped the piece
		if (FROMSQ(move) == from_sq && TOSQ(move) == to_sq) {
			//        	TODO: handle promotion

			// Try to make the move (VICE checks if it leaves King in check here)
			if (!MakeMove(&engine_board, move)) {
				continue; // Move was pseudo-legal but left king in check
			}

			/* ADD THIS: Reset search ply since we aren't using the Alpha-Beta AI yet */
			engine_board.ply = 0;

			// Move was completely legal and has now been made on engine_board!
			Legal = 1;
			break;
		}
	}

	return Legal;
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

/* Returns 1 if the game is over, 0 if the game continues, TODO: this is unnecessary once we add the ai search*/
int check_game_over(S_BOARD *pos) {
	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int legal_moves = 0;

	/* Loop through all generated moves to see if ANY are legal */
	for (int moveNum = 0; moveNum < list->count; ++moveNum) {

		/* MakeMove returns FALSE if the move leaves the King in check */
		if (MakeMove(pos, list->moves[moveNum].move)) {
			/* We found a valid move! The game is not over. */
			legal_moves++;

			/* CRITICAL: We must undo the test move so we don't corrupt the board! */
			TakeMove(pos);
			break;
		}
	}

	/* If we found 0 legal moves, the game is over. But who won? */
	if (legal_moves == 0) {

		/* Ask VICE if the current side's King is under attack by the OPPOSITE side */
		int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);

		if (InCheck) {
			if (pos->side == WHITE) {
				printf("\nBlack wins!\n");
			} else {
				printf("\nWhite wins!\n");
			}
		} else {
			printf("\nSTALEMATE! Game is a Draw!\n");
		}
		return 1; // Game Over
	}

	return 0; // Game continues
}

void init_chess_engine(void) {
	AllInit(); // VICE's internal lookup table setup

	// Allocate Hash Table (Ensure this size is small enough for your RAM!)
	//    engine_board.HashTable->pTable = NULL;
	//    InitHashTable(engine_board.HashTable, 1); // 1 MB or less

	// Load the starting pieces onto the board
	ParseFen(START_FEN, &engine_board);
}
