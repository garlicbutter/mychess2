#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: green
//

void init_style_green_INDICATOR_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xff1ad036));
};

lv_style_t *get_style_green_INDICATOR_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_green_INDICATOR_DEFAULT(style);
    }
    return style;
};

void add_style_green(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_green_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
};

void remove_style_green(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_green_INDICATOR_DEFAULT(), LV_PART_INDICATOR | LV_STATE_DEFAULT);
};

//
// Style: transp_btn
//

void init_style_transp_btn_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_opa(style, 150);
};

lv_style_t *get_style_transp_btn_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_transp_btn_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_transp_btn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_transp_btn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_transp_btn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_transp_btn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: greenline
//

void init_style_greenline_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_line_color(style, lv_color_hex(0xff347f39));
    lv_style_set_line_width(style, 2);
};

lv_style_t *get_style_greenline_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_greenline_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_greenline(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_greenline_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_greenline(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_greenline_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_green,
        add_style_transp_btn,
        add_style_greenline,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_green,
        remove_style_transp_btn,
        remove_style_greenline,
    };
    remove_style_funcs[styleIndex](obj);
}