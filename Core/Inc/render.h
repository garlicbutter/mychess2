#ifndef __RENDER_H
#define __RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"
#include "stream_buffer.h"

#include "lvgl/lvgl.h"
#include "ui.h"
#include "vice_defs.h"

LV_IMG_DECLARE(img_pawn_w)
LV_IMG_DECLARE(img_knight_w)
LV_IMG_DECLARE(img_bishop_w)
LV_IMG_DECLARE(img_rook_w)
LV_IMG_DECLARE(img_queen_w)
LV_IMG_DECLARE(img_king_w)
LV_IMG_DECLARE(img_pawn_b)
LV_IMG_DECLARE(img_knight_b)
LV_IMG_DECLARE(img_bishop_b)
LV_IMG_DECLARE(img_rook_b)
LV_IMG_DECLARE(img_queen_b)
LV_IMG_DECLARE(img_king_b)
LV_IMG_DECLARE(img_red_dot)



void render_init(void);
void render_board_state(void);
uint32_t render_timer_handler(void);

void update_memory_bars(void);

void show_loading_spinner(void);
void hide_loading_spinner(void);

void show_crown(void);

void drag_event_cb(lv_event_t *e);
void my_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
const void* get_sprite(int vice_piece);
void my_input_read(lv_indev_t *indev, lv_indev_data_t *data);
void update_debug_terminal(StreamBufferHandle_t stream);

void clear_move_markers(void);
void show_move_markers(int from_sq120);

extern osThreadId registerChessTaskHandle;
extern osMutexId lvgl_mutex;

#ifdef __cplusplus
}
#endif

#endif /* __RENDER_H */
