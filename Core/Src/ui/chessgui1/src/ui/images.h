#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_knight_w;
extern const lv_img_dsc_t img_board;
extern const lv_img_dsc_t img_bishop_w;
extern const lv_img_dsc_t img_king_w;
extern const lv_img_dsc_t img_pawn_w;
extern const lv_img_dsc_t img_queen_w;
extern const lv_img_dsc_t img_rook_w;
extern const lv_img_dsc_t img_bishop_b;
extern const lv_img_dsc_t img_king_b;
extern const lv_img_dsc_t img_knight_b;
extern const lv_img_dsc_t img_pawn_b;
extern const lv_img_dsc_t img_queen_b;
extern const lv_img_dsc_t img_rook_b;
extern const lv_img_dsc_t img_red_dot;
extern const lv_img_dsc_t img_main_page;
extern const lv_img_dsc_t img_stalemate;
extern const lv_img_dsc_t img_white_defeat;
extern const lv_img_dsc_t img_white_victory;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[18];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/