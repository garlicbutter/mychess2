#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *bishop_w_1;
    lv_obj_t *bishop_w_2;
    lv_obj_t *rook_w_1;
    lv_obj_t *rook_w_2;
    lv_obj_t *knight_w_1;
    lv_obj_t *knight_w_2;
    lv_obj_t *king_w;
    lv_obj_t *queen_w;
    lv_obj_t *pawn_w_1;
    lv_obj_t *pawn_w_2;
    lv_obj_t *pawn_w_3;
    lv_obj_t *pawn_w_4;
    lv_obj_t *pawn_w_5;
    lv_obj_t *pawn_w_6;
    lv_obj_t *pawn_w_7;
    lv_obj_t *pawn_w_8;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/