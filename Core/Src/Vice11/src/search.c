// search.c
#include "vice_defs.h"
#include "stm32f4xx_hal.h"

S_MOVELIST engine_move_lists[MAXDEPTH];

int GetTimeMs() {
	return HAL_GetTick();
}

static int ShouldStopSearch(S_SEARCHINFO *info) {
	if ((info->nodes & 2047) == 0) {
		if(info->timeset == TRUE && GetTimeMs() > info->stoptime) {
			info->stopped = TRUE;
		}
	}
	return FALSE;
}

static int IsRepetition(const S_BOARD *pos) {
	for(int index = pos->hisPly - pos->fiftyMove; index < pos->hisPly-1; ++index) {
		if (index < 0 || index >= MAXGAMEMOVES) break;
		if(pos->posKey == pos->history[index].posKey) {
			return TRUE;
		}
	}
	return FALSE;
}

// Swap the move with the highest score to the current index
static void PickNextMove(int moveNum, S_MOVELIST *list) {
    int bestScore = 0;
    int bestNum = moveNum;

    // Find the move with the highest score from moveNum to the end of the list
    for (int index = moveNum; index < list->count; ++index) {
        if (list->moves[index].score > bestScore) {
            bestScore = list->moves[index].score;
            bestNum = index;
        }
    }

    // Swap the best move we found into the current moveNum position
    S_MOVE temp = list->moves[moveNum];
    list->moves[moveNum] = list->moves[bestNum];
    list->moves[bestNum] = temp;
}

static void ScoreMoves(const S_BOARD *pos, S_MOVELIST *list, int depth) {
    for (int i = 0; i < list->count; ++i) {
        int move = list->moves[i].move;
        int score = 0;

        // Is it a capture?
        if (CAPTURED(move) != EMPTY) {
            int attacker = pos->pieces[FROMSQ(move)]; // The piece making the move
            int victim = CAPTURED(move);              // The piece being eaten
            
            // Score captures from 1,000,000 to 1,000,605
            score = 1000000 + MvvLvaScores[attacker][victim]; 
        } 
        // If not a capture, is it a Killer Move?
        else {
            if (pos->searchKillers[0][depth] == move) {
                score = 900000; // 1st Killer (high score, but below captures)
            } else if (pos->searchKillers[1][depth] == move) {
                score = 800000; // 2nd Killer
            }
        }

        list->moves[i].score = score;
    }
}

static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info) {
	//	alpha: lower bound, what's the worst move I can play
	//	beta: upper bound, opponent's best score.

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);
	ASSERT(depth>=0);

    // time up or interruption requested
	if (ShouldStopSearch(info)) {
		return 0;
	}

    // BASE CASE (Depth limit reached):
	if (depth == 0) {
		return EvalPosition(pos); // TODO: swap this out for a "Quiescence Search"
	}

	info->nodes++;

    // DRAW DETECTION
	if(IsRepetition(pos) || pos->fiftyMove >= 100) {
		return 0;
	}


	int legal = 0;
	int score = -INFINITE;

    // MOVE GENERATION:
	S_MOVELIST* moves_list = &engine_move_lists[depth]; // statically allocated buffer.
	GenerateAllMoves(pos,moves_list);
    ScoreMoves(pos, moves_list, depth);
    
	for (int i = 0; i < moves_list->count; ++i) {
        PickNextMove(i, moves_list);

		int move = moves_list->moves[i].move;

		// check legality
        if (!MakeMove(pos,move))  {
            continue; // not a legal move, it handles TakeMove for illegal moves internally.
        }

        legal++;
        score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info);
        TakeMove(pos); // reset the MakeMove

		// PRUNING LOGIC
        if (score >= beta) {
            if (CAPTURED(move) == EMPTY) {
                // Shift the old 1st killer to the 2nd slot
                pos->searchKillers[1][depth] = pos->searchKillers[0][depth];
                // Put the new killer in the 1st slot
                pos->searchKillers[0][depth] = move;
            }
        	return beta;
        }

        if (score > alpha) {
        	alpha = score;
        }
	}

	if (legal == 0) {
		if (SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos)) {
			// Checkmate: Return negative mate score, adjusted by how deep we are (ply)
			// so the engine prefers faster checkmates.
			return -INFINITE  + pos->ply;
		} else {
			return 0; // Stalemate
		}
	}
    return alpha;
}

int SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {
    int bestMove = 0; // NOMOVE
    int bestScore = -INFINITE;
    int PvMove = 0;

    info->best_score = 0;
    info->stopped = FALSE;
    info->nodes = 0;
    info->depth_searched = 0;
    info->quit = FALSE;

    // Clear killers for the new search
    for (int i = 0; i < MAXDEPTH; ++i) {
    	pos->searchKillers[0][i] = 0;
    	pos->searchKillers[1][i] = 0;
    }

    // ITERATIVE DEEPENING LOOP
    // Start at depth 1 and go up to the target depth.
    // If the GUI interrupts or time runs out, we still have a valid bestMove from the previous depth!
    for (int currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {

        // Root variables for this specific depth iteration
        S_MOVELIST *moves_list = &engine_move_lists[currentDepth];
        GenerateAllMoves(pos, moves_list);

        for (int i = 0; i < moves_list->count; ++i) {
			if (moves_list->moves[i].move == PvMove) {
				// Give the PV move a massive score so it is always picked first
				moves_list->moves[i].score = 2000000;
			} else {
				moves_list->moves[i].score = 0;
			}
		}

        int alpha = -INFINITE;
        int beta = INFINITE;
        int currentDepthBestMove = 0;
        int currentDepthBestScore = -INFINITE;

        // The Root Move Loop (Depth 0 of the tree)
        for (int i = 0; i < moves_list->count; ++i) {
        	PickNextMove(i, moves_list); // Brings the highest scoring move to index 'i'

            int move = moves_list->moves[i].move;

            if (!MakeMove(pos, move)) {
                continue;
            }

            int score = -AlphaBeta(-beta, -alpha, currentDepth - 1, pos, info);
            TakeMove(pos);

            // If time ran out during the recursive call, discard this branch completely
            if (info->stopped) {
                break;
            }

            // Track the best move at the root level
            if (score > currentDepthBestScore) {
                currentDepthBestScore = score;
                currentDepthBestMove = move;

                if (score > alpha) {
                    alpha = score;
                }
            }
        }

        // If the search was interrupted, we don't want to use the incomplete data from this depth
        if (info->stopped) {
            break;
        }

        // If we completed this depth safely, update the overall bests
        PvMove = bestMove = currentDepthBestMove;
        info->best_score = bestScore = currentDepthBestScore;
        info->depth_searched = currentDepth;
    }

    return bestMove;
}
