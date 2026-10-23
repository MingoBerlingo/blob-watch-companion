#include "apps/blob_native/pages/steps/steps_page.h"
#include "apps/blob_native/steps/pedometer.h"
#include "apps/blob_native/steps/step_counter.h"
#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/lv_port/lv_port.h"
#include "lvgl.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
namespace blob_native {
namespace {
lv_obj_t *g_screen,*g_ring,*g_count,*g_dialog,*g_prompt;
uint32_t g_shown=0;
bool g_has_shown=false;
void shoe(lv_event_t *event){
 auto *ctx=lv_event_get_draw_ctx(event);lv_area_t area;lv_obj_get_coords(lv_event_get_target(event),&area);
 lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.width=2;line.color=lv_color_white();
 line.round_start=line.round_end=true;
 // Original small sneaker silhouette, laces and motion marks; no bitmap needed.
 const int8_t outline[][2]={{17,25},{29,13},{31,23},{39,29},{45,28},{51,44},
                          {58,47},{60,53},{59,57},{39,57},{17,35},{14,31},{17,25}};
 auto segment=[&](int x,int y,int xx,int yy){
  lv_point_t a={static_cast<lv_coord_t>(area.x1+x-5),static_cast<lv_coord_t>(area.y1+y-4)};
  lv_point_t b={static_cast<lv_coord_t>(area.x1+xx-5),static_cast<lv_coord_t>(area.y1+yy-4)};
  lv_draw_line(ctx,&line,&a,&b);
 };
 for(unsigned i=1;i<sizeof(outline)/sizeof(outline[0]);++i)
  segment(outline[i-1][0],outline[i-1][1],outline[i][0],outline[i][1]);
 segment(19,30,41,52);segment(41,52,60,52);
 segment(44,34,48,32);segment(46,40,50,38);segment(48,46,52,44);
 segment(7,24,13,24);segment(5,39,11,39);segment(13,51,22,51);
}
void format_count(uint32_t total,char *out,size_t size){
 char digits[11];snprintf(digits,sizeof(digits),"%lu",static_cast<unsigned long>(total));
 const size_t length=strlen(digits);size_t j=0;
 for(size_t i=0;i<length && j+1<size;++i){
  if(i && (length-i)%3==0 && j+1<size) out[j++]=',';
  if(j+1<size) out[j++]=digits[i];
 }
 out[j]=0;
}
void close_dialog(lv_event_t *){lv_obj_add_flag(g_dialog,LV_OBJ_FLAG_HIDDEN);}
void reset(lv_event_t *){
 if(pedometer_reset(millis())){lv_obj_add_flag(g_dialog,LV_OBJ_FLAG_HIDDEN);g_has_shown=false;steps_page_tick();}
 else lv_label_set_text(g_prompt,"Save failed. Retry?");
}
lv_obj_t *button(const char *text,int x,lv_event_cb_t callback){
 auto *b=lv_btn_create(g_dialog);lv_obj_set_pos(b,x,145);lv_obj_set_size(b,68,34);
 lv_obj_set_style_pad_all(b,0,0);lv_obj_set_style_radius(b,10,0);lv_obj_set_style_shadow_width(b,0,0);
 auto *label=lv_label_create(b);lv_label_set_text(label,text);
 lv_obj_set_style_text_font(label,&lv_font_montserrat_14,0);lv_obj_center(label);
 lv_obj_add_event_cb(b,callback,LV_EVENT_CLICKED,nullptr);return b;
}
}
void steps_page_init(){
 g_screen=lv_obj_create(nullptr);lv_obj_clear_flag(g_screen,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_style_bg_color(g_screen,lv_color_black(),0);lv_obj_set_style_bg_opa(g_screen,LV_OPA_COVER,0);
 lv_obj_set_style_border_width(g_screen,0,0);lv_obj_set_style_pad_all(g_screen,0,0);
 g_ring=lv_arc_create(g_screen);lv_obj_set_pos(g_ring,10,10);lv_obj_set_size(g_ring,220,220);
 lv_obj_clear_flag(g_ring,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(g_ring,0,0);
 lv_arc_set_bg_angles(g_ring,0,360);lv_arc_set_rotation(g_ring,270);lv_arc_set_range(g_ring,0,STEP_GOAL);
 lv_arc_set_mode(g_ring,LV_ARC_MODE_REVERSE);lv_arc_set_value(g_ring,0);
 lv_obj_set_style_arc_width(g_ring,9,LV_PART_MAIN);lv_obj_set_style_arc_width(g_ring,9,LV_PART_INDICATOR);
 lv_obj_set_style_arc_color(g_ring,lv_color_black(),LV_PART_MAIN);
 lv_obj_set_style_arc_color(g_ring,lv_rgb565(TIMER_PLAY_COLOR),LV_PART_INDICATOR);
 lv_obj_set_style_arc_rounded(g_ring,true,LV_PART_INDICATOR);
 lv_obj_remove_style(g_ring,nullptr,LV_PART_KNOB);
 auto *badge=lv_obj_create(g_screen);lv_obj_remove_style_all(badge);
 lv_obj_clear_flag(badge,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_pos(badge,88,44);lv_obj_set_size(badge,64,64);
 lv_obj_set_style_bg_opa(badge,LV_OPA_TRANSP,0);
 lv_obj_add_event_cb(badge,shoe,LV_EVENT_DRAW_MAIN,nullptr);
 g_count=lv_label_create(g_screen);lv_obj_set_pos(g_count,15,121);lv_obj_set_width(g_count,210);
 lv_obj_set_style_text_color(g_count,lv_color_white(),0);lv_obj_set_style_text_align(g_count,LV_TEXT_ALIGN_CENTER,0);
 auto *label=lv_label_create(g_screen);lv_label_set_text(label,"Steps");
 lv_obj_set_style_text_font(label,&lv_font_montserrat_20,0);lv_obj_set_style_text_color(label,lv_rgb565(TIMER_ENDED_TIME_COLOR),0);
 lv_obj_align(label,LV_ALIGN_TOP_MID,0,167);
 g_dialog=lv_obj_create(g_screen);lv_obj_set_pos(g_dialog,0,0);lv_obj_set_size(g_dialog,240,240);
 lv_obj_clear_flag(g_dialog,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(g_dialog,0,0);
 lv_obj_set_style_border_width(g_dialog,0,0);lv_obj_set_style_bg_color(g_dialog,lv_color_black(),0);
 lv_obj_set_style_bg_opa(g_dialog,LV_OPA_COVER,0);
 g_prompt=lv_label_create(g_dialog);lv_label_set_text(g_prompt,"Reset steps?");
 lv_obj_set_style_text_font(g_prompt,&lv_font_montserrat_20,0);lv_obj_align(g_prompt,LV_ALIGN_TOP_MID,0,84);
 auto *cancel=button("Cancel",45,close_dialog);auto *confirm=button("Reset",127,reset);
 lv_obj_set_style_bg_color(cancel,lv_rgb565(TIMER_BACK_COLOR),0);
 lv_obj_set_style_text_color(cancel,lv_color_black(),0);
 lv_obj_set_style_bg_color(confirm,lv_rgb565(TIMER_CANCEL_COLOR),0);
 lv_obj_set_style_text_color(confirm,lv_color_black(),0);
 lv_obj_add_flag(g_dialog,LV_OBJ_FLAG_HIDDEN);
}
void steps_page_tick(){
 const uint32_t total=pedometer_total();if(g_has_shown && total==g_shown) return;g_shown=total;g_has_shown=true;
 char text[16];format_count(total,text,sizeof(text));lv_label_set_text(g_count,text);
 const size_t length=strlen(text);
 lv_obj_set_style_text_font(g_count,length>7 ? &lv_font_montserrat_24:(length>5 ? &lv_font_montserrat_32:&lv_font_montserrat_40),0);
 lv_arc_set_value(g_ring,total>=STEP_GOAL ? STEP_GOAL:total);
}
void steps_page_show(){lv_obj_add_flag(g_dialog,LV_OBJ_FLAG_HIDDEN);g_has_shown=false;steps_page_tick();lv_scr_load(g_screen);}
void steps_page_request_reset(){
 lv_label_set_text(g_prompt,"Reset steps?");lv_obj_clear_flag(g_dialog,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(g_dialog);
}
bool steps_page_reset_open(){return !lv_obj_has_flag(g_dialog,LV_OBJ_FLAG_HIDDEN);}
}
