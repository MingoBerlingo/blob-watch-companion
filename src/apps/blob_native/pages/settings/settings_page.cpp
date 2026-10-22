#include "apps/blob_native/pages/settings/settings_page.h"
#include "apps/blob_native/pages/page_manager.h"
#include "apps/blob_native/blob/physics_settings.h"
#include "apps/blob_native/shared_state.h"
#include "lvgl.h"
#include <stdio.h>
#include <initializer_list>
namespace blob_native {
namespace {
lv_obj_t *g_screen, *g_title, *g_status;
lv_obj_t *g_names[2], *g_values[2], *g_sliders[2];
lv_obj_t *g_prev, *g_next;
int g_page=0;
bool g_dirty=false;
const char *kTitles[]={"Follow","Shape","Motion","Edge","Size"};
constexpr int kPageCount=(PHYSICS_SETTING_COUNT+1)/2;
void status(const char *message) { lv_label_set_text_fmt(g_status,"%s  %d/%d",message,g_page+1,kPageCount); }
void value_label(int row) {
    const int index=g_page*2+row;
    const int value=physics_settings_value(index);
    if(index==4) lv_label_set_text_fmt(g_values[row],"%d.%dx",value/10,value%10);
    else if(index==5 || index==6 || index==8) lv_label_set_text_fmt(g_values[row],"%d%%",value);
    else lv_label_set_text_fmt(g_values[row],"%d",value);
}
void refresh() {
    lv_label_set_text(g_title,kTitles[g_page]);
    status(g_dirty ? "Unsaved":"Physics");
    for(int row=0;row<2;++row) {
        const int index=g_page*2+row;
        for(auto *object:{g_names[row],g_values[row],g_sliders[row]}) {
            if(index>=PHYSICS_SETTING_COUNT) lv_obj_add_flag(object,LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(object,LV_OBJ_FLAG_HIDDEN);
        }
        if(index>=PHYSICS_SETTING_COUNT) continue;
        const auto &c=physics_settings_control(index);
        lv_label_set_text(g_names[row],c.label);
        lv_slider_set_range(g_sliders[row],c.minimum,c.maximum);
        lv_slider_set_value(g_sliders[row],physics_settings_value(index),LV_ANIM_OFF);
        value_label(row);
    }
    if(g_page==0) lv_obj_add_state(g_prev,LV_STATE_DISABLED); else lv_obj_clear_state(g_prev,LV_STATE_DISABLED);
    if(g_page==kPageCount-1) lv_obj_add_state(g_next,LV_STATE_DISABLED); else lv_obj_clear_state(g_next,LV_STATE_DISABLED);
}
void changed(lv_event_t *event) {
    const int row=static_cast<int>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    const int index=g_page*2+row;
    const int before=physics_settings_value(index);
    physics_settings_set(index,lv_slider_get_value(g_sliders[row]));
    lv_slider_set_value(g_sliders[row],physics_settings_value(index),LV_ANIM_OFF);
    value_label(row);
    if(before!=physics_settings_value(index)) { g_dirty=true;status("Unsaved"); }
}
void action(lv_event_t *event) {
    const int id=static_cast<int>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    if(id==0) { page_manager_show_blob();return; }
    if(id==1 && g_page>0) --g_page;
    if(id==2 && g_page<kPageCount-1) ++g_page;
    if(id==3) { physics_settings_reset();g_dirty=true; }
    if(id==4) {
        if(physics_settings_save()) { g_dirty=false;status("Saved"); }
        else status("Save failed");
        return;
    }
    refresh();
}
lv_obj_t *label(int x,int y,const lv_font_t *font) {
    auto *o=lv_label_create(g_screen);
    lv_obj_set_pos(o,x,y);
    lv_obj_set_style_text_color(o,lv_color_white(),0);
    lv_obj_set_style_text_font(o,font,0);
    return o;
}
lv_obj_t *button(const char *text,int x,int y,int width,int id,bool accent=false) {
    auto *o=lv_btn_create(g_screen);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,width,26);
    lv_obj_set_style_radius(o,8,0);
    lv_obj_set_style_shadow_width(o,0,0);
    lv_obj_set_style_bg_color(o,accent ? lv_theme_get_color_primary(g_screen):lv_color_hex(0x39393D),0);
    lv_obj_set_style_pad_all(o,0,0);
    auto *l=lv_label_create(o);lv_label_set_text(l,text);
    lv_obj_set_style_text_font(l,&lv_font_montserrat_12,0);lv_obj_center(l);
    lv_obj_add_event_cb(o,action,LV_EVENT_CLICKED,reinterpret_cast<void*>(static_cast<uintptr_t>(id)));
    return o;
}
}
void settings_page_init() {
    g_screen=lv_obj_create(nullptr);
    lv_obj_clear_flag(g_screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(g_screen,lv_color_black(),0);
    lv_obj_set_style_bg_opa(g_screen,LV_OPA_COVER,0);
    lv_obj_set_style_pad_all(g_screen,0,0);lv_obj_set_style_border_width(g_screen,0,0);
    g_title=label(0,22,&lv_font_montserrat_20);
    lv_obj_set_width(g_title,240);lv_obj_set_style_text_align(g_title,LV_TEXT_ALIGN_CENTER,0);
    g_status=label(0,46,&lv_font_montserrat_12);
    lv_obj_set_width(g_status,240);lv_obj_set_style_text_align(g_status,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_opa(g_status,LV_OPA_60,0);
    for(int row=0;row<2;++row) {
        const int y=72+row*49;
        g_names[row]=label(44,y,&lv_font_montserrat_12);
        g_values[row]=label(150,y,&lv_font_montserrat_12);
        lv_obj_set_width(g_values[row],46);lv_obj_set_style_text_align(g_values[row],LV_TEXT_ALIGN_RIGHT,0);
        auto *slider=g_sliders[row]=lv_slider_create(g_screen);
        lv_obj_set_pos(slider,48,y+24);lv_obj_set_size(slider,144,10);
        lv_obj_set_ext_click_area(slider,10);
        lv_obj_set_style_bg_color(slider,lv_theme_get_color_primary(g_screen),LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(slider,lv_color_white(),LV_PART_KNOB);
        lv_obj_set_style_pad_all(slider,5,LV_PART_KNOB);
        lv_obj_add_event_cb(slider,changed,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(static_cast<uintptr_t>(row)));
    }
    g_prev=button("Prev",40,164,46,1);
    button("Back",92,164,56,0);
    g_next=button("Next",154,164,46,2);
    button("Reset",57,199,60,3);
    button("Save",123,199,60,4,true);
    refresh();
}
void settings_page_show() { g_page=0;refresh();lv_scr_load(g_screen); }
}
