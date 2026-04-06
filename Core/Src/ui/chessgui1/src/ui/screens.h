#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_START_SCREEN = 1,
    SCREEN_ID_GAME_SCREEN = 2,
    _SCREEN_ID_LAST = 2
};

typedef struct _objects_t {
    lv_obj_t *start_screen;
    lv_obj_t *game_screen;
    lv_obj_t *start_button;
    lv_obj_t *dropdown_mode;
    lv_obj_t *debug_terminal;
    lv_obj_t *test_button;
    lv_obj_t *bar_rtos;
    lv_obj_t *label_rtos;
    lv_obj_t *crown;
    lv_obj_t *chessboard;
    lv_obj_t *loading_board;
} objects_t;

extern objects_t objects;

void create_screen_start_screen();
void tick_screen_start_screen();

void create_screen_game_screen();
void tick_screen_game_screen();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/