#include "images.h"

const ext_img_desc_t images[18] = {
    { "knight_w", &img_knight_w },
    { "board", &img_board },
    { "bishop_w", &img_bishop_w },
    { "king_w", &img_king_w },
    { "pawn_w", &img_pawn_w },
    { "queen_w", &img_queen_w },
    { "rook_w", &img_rook_w },
    { "bishop_b", &img_bishop_b },
    { "king_b", &img_king_b },
    { "knight_b", &img_knight_b },
    { "pawn_b", &img_pawn_b },
    { "queen_b", &img_queen_b },
    { "rook_b", &img_rook_b },
    { "red_dot", &img_red_dot },
    { "main_page", &img_main_page },
    { "stalemate", &img_stalemate },
    { "white_defeat", &img_white_defeat },
    { "white_victory", &img_white_victory },
};