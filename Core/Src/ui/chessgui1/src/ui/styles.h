#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: green
lv_style_t *get_style_green_INDICATOR_DEFAULT();
void add_style_green(lv_obj_t *obj);
void remove_style_green(lv_obj_t *obj);

// Style: transp_btn
lv_style_t *get_style_transp_btn_MAIN_DEFAULT();
void add_style_transp_btn(lv_obj_t *obj);
void remove_style_transp_btn(lv_obj_t *obj);

// Style: greenline
lv_style_t *get_style_greenline_MAIN_DEFAULT();
void add_style_greenline(lv_obj_t *obj);
void remove_style_greenline(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/