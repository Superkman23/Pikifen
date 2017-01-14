/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2017.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * General animation editor-related functions.
 */

#include <algorithm>

#include <allegro5/allegro.h>

#include "animation_editor.h"
#include "../animation.h"
#include "../drawing.h"
#include "../functions.h"
#include "../LAFI/angle_picker.h"
#include "../LAFI/button.h"
#include "../LAFI/checkbox.h"
#include "../LAFI/frame.h"
#include "../LAFI/minor.h"
#include "../LAFI/radio_button.h"
#include "../LAFI/scrollbar.h"
#include "../LAFI/textbox.h"
#include "../misc_structs.h"
#include "../vars.h"


//Maximum zoom level possible in the editor.
const float animation_editor::ZOOM_MAX_LEVEL_EDITOR = 8.0f;
//Minimum zoom level possible in the editor.
const float animation_editor::ZOOM_MIN_LEVEL_EDITOR = 0.05f;


/* ----------------------------------------------------------------------------
 * Initializes animation editor class stuff.
 */
animation_editor::animation_editor() :
    editor(),
    cur_anim(NULL),
    anim_playing(false),
    comparison(true),
    comparison_sprite(nullptr),
    comparison_blink(true),
    comparison_blink_show(true),
    comparison_blink_timer(0),
    cur_body_part_nr(INVALID),
    cur_frame_nr(INVALID),
    cur_frame_time(0),
    cur_hitbox_alpha(0),
    cur_hitbox_nr(INVALID),
    cur_maturity(0),
    cur_sprite(NULL),
    file_dialog(NULL),
    grabbing_hitbox(INVALID),
    grabbing_hitbox_edge(false),
    grabbing_hitbox_x(0),
    grabbing_hitbox_y(0),
    hitboxes_visible(true),
    is_pikmin(false),
    new_hitbox_corner_x(FLT_MAX),
    new_hitbox_corner_y(FLT_MAX) {
    
    top_bmp[0] = NULL;
    top_bmp[1] = NULL;
    top_bmp[2] = NULL;
    comparison_blink_timer =
        timer(
            0.6,
    [this] () {
        this->comparison_blink_show = !this->comparison_blink_show;
        this->comparison_blink_timer.start();
    }
        );
    comparison_blink_timer.start();
}


/* ----------------------------------------------------------------------------
 * Handles the logic part of the main loop of the animation editor.
 */
void animation_editor::do_logic() {
    if(
        anim_playing && mode == EDITOR_MODE_ANIMATION &&
        cur_anim && cur_frame_nr != INVALID
    ) {
        frame* f = &cur_anim->frames[cur_frame_nr];
        if(f->duration != 0) {
            cur_frame_time += delta_t;
            
            while(cur_frame_time > f->duration) {
                cur_frame_time = cur_frame_time - f->duration;
                cur_frame_nr++;
                if(cur_frame_nr >= cur_anim->frames.size()) {
                    cur_frame_nr =
                        (
                            cur_anim->loop_frame >=
                            cur_anim->frames.size()
                        ) ? 0 : cur_anim->loop_frame;
                }
                f = &cur_anim->frames[cur_frame_nr];
            }
        } else {
            anim_playing = false;
        }
        animation_to_gui();
    }
    
    cur_hitbox_alpha += M_PI * 3 * delta_t;
    
    if(comparison_blink) {
        comparison_blink_timer.tick(delta_t);
    } else {
        comparison_blink_show = true;
    }
    
    fade_mgr.tick(delta_t);
    
}


/* ----------------------------------------------------------------------------
 * Handles the drawing part of the main loop of the animation editor.
 */
void animation_editor::do_drawing() {

    gui->draw();
    
    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_translate_transform(
        &transform,
        cam_x + (gui_x / 2.0 / cam_zoom),
        cam_y + (scr_h / 2.0 / cam_zoom)
    );
    al_scale_transform(&transform, cam_zoom, cam_zoom);
    al_use_transform(&transform);
    
    al_set_clipping_rectangle(
        0, 0, gui_x, status_bar_y
    ); {
    
        al_clear_to_color(al_map_rgb(128, 144, 128));
        
        sprite* s = NULL;
        
        if(mode == EDITOR_MODE_ANIMATION) {
            if(cur_frame_nr != INVALID) {
                string name =
                    cur_anim->frames[cur_frame_nr].sprite_name;
                size_t s_pos = anims.find_sprite(name);
                if(s_pos != INVALID) s = anims.sprites[s_pos];
            }
            
        } else if(
            mode == EDITOR_MODE_SPRITE || mode == EDITOR_MODE_TOP ||
            mode == EDITOR_MODE_HITBOXES ||
            mode == EDITOR_MODE_SPRITE_OFFSET
        ) {
            s = cur_sprite;
            
        }
        
        if(s) {
            if(s->bitmap) {
                draw_sprite(
                    s->bitmap,
                    s->offs_x, s->offs_y,
                    s->game_w, s->game_h
                );
            }
            
            if(hitboxes_visible) {
                size_t n_hitboxes = s->hitboxes.size();
                for(size_t h = 0; h < n_hitboxes; ++h) {
                    hitbox* h_ptr = &s->hitboxes[h];
                    
                    ALLEGRO_COLOR hitbox_color, hitbox_outline_color;
                    unsigned char hitbox_outline_alpha =
                        63 + 192 * ((sin(cur_hitbox_alpha) / 2.0) + 0.5);
                        
                    if(h_ptr->type == HITBOX_TYPE_NORMAL) {
                        hitbox_color = al_map_rgba(0, 128, 0, 128);
                        hitbox_outline_color = al_map_rgba(0, 64, 0, 255);
                    } else if(h_ptr->type == HITBOX_TYPE_ATTACK) {
                        hitbox_color = al_map_rgba(128, 0, 0, 128);
                        hitbox_outline_color = al_map_rgba(64, 0, 0, 255);
                    } else {
                        hitbox_color = al_map_rgba(128, 128, 0, 128);
                        hitbox_outline_color = al_map_rgba(64, 64, 0, 255);
                    }
                    
                    al_draw_filled_circle(
                        h_ptr->x,
                        h_ptr->y,
                        h_ptr->radius,
                        hitbox_color
                    );
                    
                    al_draw_circle(
                        h_ptr->x,
                        h_ptr->y,
                        h_ptr->radius,
                        (
                            cur_hitbox_nr == h ?
                            change_alpha(
                                hitbox_outline_color,
                                hitbox_outline_alpha
                            ) :
                            hitbox_outline_color
                        ),
                        (
                            cur_hitbox_nr == h ?
                            3 / cam_zoom :
                            2 / cam_zoom
                        )
                    );
                }
            }
            
            if(s->top_visible && is_pikmin) {
                draw_sprite(
                    top_bmp[cur_maturity],
                    s->top_x, s->top_y,
                    s->top_w, s->top_h,
                    s->top_angle
                );
            }
            
            if(
                comparison && comparison_blink_show &&
                comparison_sprite && comparison_sprite->bitmap
            ) {
                draw_sprite(
                    comparison_sprite->bitmap,
                    comparison_sprite->offs_x, comparison_sprite->offs_y,
                    comparison_sprite->game_w, comparison_sprite->game_h,
                    0
                );
            }
        }
        
        if(hitboxes_visible) {
            float cam_leftmost = -cam_x - (scr_w / 2 / cam_zoom);
            float cam_topmost = -cam_y - (scr_h / 2 / cam_zoom);
            float cam_rightmost = cam_leftmost + (scr_w / cam_zoom);
            float cam_bottommost = cam_topmost + (scr_h / cam_zoom);
            
            al_draw_line(
                0, cam_topmost, 0, cam_bottommost,
                al_map_rgb(240, 240, 240), 1 / cam_zoom
            );
            al_draw_line(
                cam_leftmost, 0, cam_rightmost, 0,
                al_map_rgb(240, 240, 240), 1 / cam_zoom
            );
        }
        
    } al_reset_clipping_rectangle();
    
    ALLEGRO_TRANSFORM id_transform;
    al_identity_transform(&id_transform);
    al_use_transform(&id_transform);
    
    fade_mgr.draw();
    
    al_flip_display();
}


/* ----------------------------------------------------------------------------
 * Returns a file path, but cropped to fit on the gui's buttons.
 * This implies cutting it in two lines, and even replacing the start with
 * ellipsis, if needed.
 */
string animation_editor::get_cut_path(const string &p) {
    if(p.size() <= 22) return p;
    
    string result = p;
    if(p.size() > 44) {
        result = "..." + p.substr(p.size() - 41, 41);
    }
    
    if(p.size() > 22) {
        result =
            result.substr(0, result.size() / 2) + "\n" +
            result.substr(result.size() / 2, (result.size() / 2) + 1);
    }
    
    return result;
}


/* ----------------------------------------------------------------------------
 * Loads the animation's data onto the gui.
 */
void animation_editor::animation_to_gui() {
    lafi::widget* f = gui->widgets["frm_anims"];
    
    ((lafi::button*) f->widgets["but_anim"])->text =
        cur_anim ? cur_anim->name : "";
        
    if(!cur_anim) {
        hide_widget(f->widgets["frm_anim"]);
    } else {
        show_widget(f->widgets["frm_anim"]);
        
        ((lafi::button*) f->widgets["but_anim"])->text = cur_anim->name;
        ((lafi::textbox*) f->widgets["frm_anim"]->widgets["txt_loop"])->text =
            i2s(cur_anim->loop_frame + 1);
            
        frame_to_gui();
    }
}


/* ----------------------------------------------------------------------------
 * Loads the body part's data onto the gui.
 */
void animation_editor::body_part_to_gui() {
    lafi::frame* f = (lafi::frame*) gui->widgets["frm_body_parts"];
    
    ((lafi::label*) f->widgets["lbl_nr"])->text =
        (
            anims.body_parts.empty() ?
            "--/0" :
            (string) (i2s(cur_body_part_nr + 1) + "/" + i2s(anims.body_parts.size()))
        );
        
    if(anims.body_parts.empty()) {
        hide_widget(f->widgets["frm_body_part"]);
        return;
    }
    
    f = (lafi::frame*) f->widgets["frm_body_part"];
    show_widget(f);
    
    ((lafi::textbox*) f->widgets["txt_name"])->text =
        anims.body_parts[cur_body_part_nr]->name;
}


/* ----------------------------------------------------------------------------
 * Loads the frame's data from memory to the gui.
 */
void animation_editor::frame_to_gui() {
    lafi::widget* f = gui->widgets["frm_anims"]->widgets["frm_anim"];
    bool valid = cur_frame_nr != INVALID && cur_anim;
    
    ((lafi::label*) f->widgets["lbl_f_nr"])->text =
        "Current frame: " +
        (valid ? i2s((cur_frame_nr + 1)) : "--") +
        " / " + i2s(cur_anim->frames.size());
        
    if(!valid) {
        hide_widget(f->widgets["frm_frame"]);
    } else {
        show_widget(f->widgets["frm_frame"]);
        
        (
            (lafi::button*) f->widgets["frm_frame"]->widgets["but_sprite"]
        )->text =
            cur_anim->frames[cur_frame_nr].sprite_name;
        (
            (lafi::textbox*) f->widgets["frm_frame"]->widgets["txt_dur"]
        )->text =
            f2s(cur_anim->frames[cur_frame_nr].duration);
    }
}


/* ----------------------------------------------------------------------------
 * Loads the hitbox's data from memory to the gui.
 */
void animation_editor::hitbox_to_gui() {
    lafi::widget* f = gui->widgets["frm_hitboxes"]->widgets["frm_hitbox"];
    
    hitbox* cur_h = NULL;
    if(!cur_sprite->hitboxes.empty()) {
        cur_h = &cur_sprite->hitboxes[cur_hitbox_nr];
    }
    if(cur_h) {
        (
            (lafi::label*) gui->widgets["frm_hitboxes"]->widgets["lbl_name"]
        )->text =
            cur_h->body_part_name;
        ((lafi::textbox*) f->widgets["txt_x"])->text = f2s(cur_h->x);
        ((lafi::textbox*) f->widgets["txt_y"])->text = f2s(cur_h->y);
        ((lafi::textbox*) f->widgets["txt_z"])->text = f2s(cur_h->z);
        ((lafi::textbox*) f->widgets["txt_h"])->text = f2s(cur_h->height);
        ((lafi::textbox*) f->widgets["txt_r"])->text = f2s(cur_h->radius);
    }
    
    open_hitbox_type(cur_h ? cur_h->type : 255);
    
    if(cur_h) {
        show_widget(f);
        if(cur_h->type == HITBOX_TYPE_NORMAL) {
            f = f->widgets["frm_normal"];
            ((lafi::textbox*) f->widgets["txt_mult"])->text =
                f2s(cur_h->multiplier);
            if(cur_h->can_pikmin_latch) {
                ((lafi::checkbox*) f->widgets["chk_latch"])->check();
            } else {
                ((lafi::checkbox*) f->widgets["chk_latch"])->uncheck();
            }
            (
                (lafi::textbox*) f->widgets["txt_hazards"]
            )->text =
                cur_h->hazards_str;
                
        } else if(cur_h->type == HITBOX_TYPE_ATTACK) {
            f = f->widgets["frm_attack"];
            ((lafi::textbox*) f->widgets["txt_mult"])->text =
                f2s(cur_h->multiplier);
            ((lafi::textbox*) f->widgets["txt_hazards"])->text =
                cur_h->hazards_str;
            (
                (lafi::checkbox*) f->widgets["chk_outward"]
            )->set(cur_h->knockback_outward);
            (
                (lafi::angle_picker*) f->widgets["ang_angle"]
            )->set_angle_rads(cur_h->knockback_angle);
            (
                (lafi::textbox*) f->widgets["txt_knockback"]
            )->text = f2s(cur_h->knockback);
            
            if(cur_h->knockback_outward) {
                disable_widget(f->widgets["ang_angle"]);
            } else {
                enable_widget(f->widgets["ang_angle"]);
            }
            
        }
    } else {
        hide_widget(f);
    }
}


/* ----------------------------------------------------------------------------
 * Loads the frame's data from memory to the gui.
 */
void animation_editor::sprite_to_gui() {
    lafi::widget* f = gui->widgets["frm_sprites"];
    
    ((lafi::button*) f->widgets["but_sprite"])->text =
        cur_sprite ? cur_sprite->name : "";
        
    if(!cur_sprite) {
        hide_widget(f->widgets["frm_sprite"]);
    } else {
        show_widget(f->widgets["frm_sprite"]);
        
        f = f->widgets["frm_sprite"];
        
        ((lafi::textbox*) f->widgets["txt_file"])->text =
            cur_sprite->file;
        ((lafi::textbox*) f->widgets["txt_filex"])->text =
            i2s(cur_sprite->file_x);
        ((lafi::textbox*) f->widgets["txt_filey"])->text =
            i2s(cur_sprite->file_y);
        ((lafi::textbox*) f->widgets["txt_filew"])->text =
            i2s(cur_sprite->file_w);
        ((lafi::textbox*) f->widgets["txt_fileh"])->text =
            i2s(cur_sprite->file_h);
        ((lafi::textbox*) f->widgets["txt_gamew"])->text =
            f2s(cur_sprite->game_w);
        ((lafi::textbox*) f->widgets["txt_gameh"])->text =
            f2s(cur_sprite->game_h);
        ((lafi::textbox*) f->widgets["txt_offsx"])->text =
            f2s(cur_sprite->offs_x);
        ((lafi::textbox*) f->widgets["txt_offsy"])->text =
            f2s(cur_sprite->offs_y);
            
        if(is_pikmin) {
            enable_widget(f->widgets["but_top"]);
        } else {
            disable_widget(f->widgets["but_top"]);
        }
    }
}


/* ----------------------------------------------------------------------------
 * Loads the sprite offset's data from memory to the gui.
 */
void animation_editor::sprite_offset_to_gui() {
    lafi::widget* f = gui->widgets["frm_offset"];
    
    ((lafi::textbox*) f->widgets["txt_x"])->text = f2s(cur_sprite->offs_x);
    ((lafi::textbox*) f->widgets["txt_y"])->text = f2s(cur_sprite->offs_y);
    ((lafi::checkbox*) f->widgets["chk_compare"])->set(comparison);
    ((lafi::checkbox*) f->widgets["chk_compare_blink"])->set(
        comparison_blink
    );
    ((lafi::button*) f->widgets["but_compare"])->text =
        (comparison_sprite ? comparison_sprite->name : "");
}


/* ----------------------------------------------------------------------------
 * Loads the Pikmin top's data onto the gui.
 */
void animation_editor::top_to_gui() {
    lafi::widget* f = gui->widgets["frm_top"];
    
    lafi::checkbox* c = (lafi::checkbox*) f->widgets["chk_visible"];
    if(cur_sprite->top_visible) c->check();
    else c->uncheck();
    
    ((lafi::textbox*) f->widgets["txt_x"])->text = f2s(cur_sprite->top_x);
    ((lafi::textbox*) f->widgets["txt_y"])->text = f2s(cur_sprite->top_y);
    ((lafi::textbox*) f->widgets["txt_w"])->text = f2s(cur_sprite->top_w);
    ((lafi::textbox*) f->widgets["txt_h"])->text = f2s(cur_sprite->top_h);
    (
        (lafi::angle_picker*) f->widgets["ang_angle"]
    )->set_angle_rads(cur_sprite->top_angle);
}


/* ----------------------------------------------------------------------------
 * Saves the animation's data to memory using info on the gui.
 */
void animation_editor::gui_to_animation() {
    if(!cur_anim) return;
    
    lafi::widget* f = gui->widgets["frm_anims"]->widgets["frm_anim"];
    
    cur_anim->loop_frame =
        s2i(((lafi::textbox*) f->widgets["txt_loop"])->text) - 1;
    if(cur_anim->loop_frame >= cur_anim->frames.size()) {
        cur_anim->loop_frame = 0;
    }
    
    gui_to_frame();
    animation_to_gui();
    
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the body part's data from the gui.
 */
void animation_editor::gui_to_body_part() {
    body_part_to_gui();
    
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the frame's data to memory using info on the gui.
 */
void animation_editor::gui_to_frame() {
    bool valid = cur_frame_nr != INVALID && cur_anim;
    if(!valid) return;
    
    lafi::widget* fw = gui->widgets["frm_anims"]->widgets["frm_anim"];
    
    frame* f = &cur_anim->frames[cur_frame_nr];
    f->duration =
        s2f(
            (
                (lafi::textbox*)
                fw->widgets["frm_frame"]->widgets["txt_dur"]
            )->text
        );
    if(f->duration < 0) f->duration = 0;
    
    frame_to_gui();
    
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the hitbox's data to memory using info on the gui.
 */
void animation_editor::gui_to_hitbox() {
    bool valid = cur_hitbox_nr != INVALID && cur_sprite;
    if(!valid) return;
    
    lafi::widget* f = gui->widgets["frm_hitboxes"]->widgets["frm_hitbox"];
    
    hitbox* h = &cur_sprite->hitboxes[cur_hitbox_nr];
    
    h->x = s2f(((lafi::textbox*) f->widgets["txt_x"])->text);
    h->y = s2f(((lafi::textbox*) f->widgets["txt_y"])->text);
    h->z = s2f(((lafi::textbox*) f->widgets["txt_z"])->text);
    
    h->height = s2f(((lafi::textbox*) f->widgets["txt_h"])->text);
    h->radius = s2f(((lafi::textbox*) f->widgets["txt_r"])->text);
    if(h->radius <= 0) h->radius = 16;
    
    hitbox* cur_h =
        &cur_sprite->hitboxes[cur_hitbox_nr];
        
    if(((lafi::radio_button*) f->widgets["rad_normal"])->selected) {
        cur_h->type = HITBOX_TYPE_NORMAL;
    } else if(((lafi::radio_button*) f->widgets["rad_attack"])->selected) {
        cur_h->type = HITBOX_TYPE_ATTACK;
    } else {
        cur_h->type = HITBOX_TYPE_DISABLED;
    }
    
    if(cur_h->type == HITBOX_TYPE_NORMAL) {
        cur_h->multiplier =
            s2f(
                (
                    (lafi::textbox*)
                    f->widgets["frm_normal"]->widgets["txt_mult"]
                )->text
            );
        cur_h->can_pikmin_latch =
            (
                (lafi::checkbox*)
                f->widgets["frm_normal"]->widgets["chk_latch"]
            )->checked;
        cur_h->hazards_str =
            (
                (lafi::textbox*)
                f->widgets["frm_normal"]->widgets["txt_hazards"]
            )->text;
            
    } else if(cur_h->type == HITBOX_TYPE_ATTACK) {
        cur_h->multiplier =
            s2f(
                (
                    (lafi::textbox*)
                    f->widgets["frm_attack"]->widgets["txt_mult"]
                )->text
            );
        cur_h->hazards_str =
            (
                (lafi::textbox*)
                f->widgets["frm_attack"]->widgets["txt_hazards"]
            )->text;
        cur_h->knockback_outward =
            (
                (lafi::checkbox*)
                f->widgets["frm_attack"]->widgets["chk_outward"]
            )->checked;
        cur_h->knockback_angle =
            (
                (lafi::angle_picker*)
                f->widgets["frm_attack"]->widgets["ang_angle"]
            )->get_angle_rads();
        cur_h->knockback =
            s2f(
                (
                    (lafi::textbox*)
                    f->widgets["frm_attack"]->widgets["txt_knockback"]
                )->text
            );
            
    }
    
    hitbox_to_gui();
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the sprite's data to memory using info on the gui.
 */
void animation_editor::gui_to_sprite() {
    if(!cur_sprite) return;
    
    lafi::widget* f = gui->widgets["frm_sprites"]->widgets["frm_sprite"];
    
    string new_file;
    int new_fx, new_fy, new_fw, new_fh;
    
    new_file =          ((lafi::textbox*) f->widgets["txt_file"])->text;
    new_fx =            s2i(((lafi::textbox*) f->widgets["txt_filex"])->text);
    new_fy =            s2i(((lafi::textbox*) f->widgets["txt_filey"])->text);
    new_fw =            s2i(((lafi::textbox*) f->widgets["txt_filew"])->text);
    new_fh =            s2i(((lafi::textbox*) f->widgets["txt_fileh"])->text);
    cur_sprite->game_w = s2f(((lafi::textbox*) f->widgets["txt_gamew"])->text);
    cur_sprite->game_h = s2f(((lafi::textbox*) f->widgets["txt_gameh"])->text);
    cur_sprite->offs_x = s2f(((lafi::textbox*) f->widgets["txt_offsx"])->text);
    cur_sprite->offs_y = s2f(((lafi::textbox*) f->widgets["txt_offsy"])->text);
    
    //Automatically fill in the in-game width/height if it hasn't been set yet.
    if(cur_sprite->game_w == 0.0f) cur_sprite->game_w = new_fw;
    if(cur_sprite->game_h == 0.0f) cur_sprite->game_h = new_fh;
    
    if(
        cur_sprite->file != new_file ||
        cur_sprite->file_x != new_fx || cur_sprite->file_y != new_fy ||
        cur_sprite->file_w != new_fw || cur_sprite->file_h != new_fh
    ) {
        //Changed something image-wise. Recreate it.
        if(cur_sprite->parent_bmp) bitmaps.detach(cur_sprite->file);
        if(cur_sprite->bitmap) al_destroy_bitmap(cur_sprite->bitmap);
        cur_sprite->bitmap = NULL;
        cur_sprite->parent_bmp = bitmaps.get(new_file, NULL);
        if(cur_sprite->parent_bmp) {
            cur_sprite->bitmap =
                al_create_sub_bitmap(
                    cur_sprite->parent_bmp, new_fx, new_fy, new_fw, new_fh
                );
        }
        cur_sprite->file = new_file;
        cur_sprite->file_x = new_fx;
        cur_sprite->file_y = new_fy;
        cur_sprite->file_w = new_fw;
        cur_sprite->file_h = new_fh;
    }
    
    last_file_used = new_file;
    
    gui_to_hitbox();
    sprite_to_gui();
    
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the sprite's bitmap offset data to memory using info on the gui.
 */
void animation_editor::gui_to_sprite_offset() {
    lafi::widget* f = gui->widgets["frm_offset"];
    
    cur_sprite->offs_x =
        s2f(((lafi::textbox*) f->widgets["txt_x"])->text);
    cur_sprite->offs_y =
        s2f(((lafi::textbox*) f->widgets["txt_y"])->text);
    comparison =
        ((lafi::checkbox*) f->widgets["chk_compare"])->checked;
    comparison_blink =
        ((lafi::checkbox*) f->widgets["chk_compare_blink"])->checked;
        
    sprite_offset_to_gui();
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the Pikmin top's data to memory using info on the gui.
 */
void animation_editor::gui_to_top() {
    lafi::widget* f = gui->widgets["frm_top"];
    
    cur_sprite->top_visible =
        ((lafi::checkbox*) f->widgets["chk_visible"])->checked;
    cur_sprite->top_x =
        s2f(((lafi::textbox*) f->widgets["txt_x"])->text);
    cur_sprite->top_y =
        s2f(((lafi::textbox*) f->widgets["txt_y"])->text);
    cur_sprite->top_w =
        s2f(((lafi::textbox*) f->widgets["txt_w"])->text);
    cur_sprite->top_h =
        s2f(((lafi::textbox*) f->widgets["txt_h"])->text);
    cur_sprite->top_angle =
        ((lafi::angle_picker*) f->widgets["ang_angle"])->get_angle_rads();
        
    top_to_gui();
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Loads the animation database for the current object.
 */
void animation_editor::load_animation_database() {
    file_path = replace_all(file_path, "\\", "/");
    
    anims.destroy();
    
    data_node file = data_node(file_path);
    if(!file.file_was_opened) {
        file.save_file(file_path, true);
    }
    anims = load_animation_database_from_file(&file);
    
    anim_playing = false;
    cur_anim = NULL;
    cur_sprite = NULL;
    cur_frame_nr = INVALID;
    cur_hitbox_nr = INVALID;
    if(!anims.animations.empty()) {
        cur_anim = anims.animations[0];
        if(cur_anim->frames.size()) cur_frame_nr = 0;
    }
    if(!anims.sprites.empty()) {
        cur_sprite = anims.sprites[0];
        if(cur_sprite->hitboxes.size()) cur_hitbox_nr = 0;
    }
    
    enable_widget(gui->widgets["frm_bottom"]->widgets["but_load"]);
    enable_widget(gui->widgets["frm_bottom"]->widgets["but_save"]);
    hide_widget(gui->widgets["frm_hitboxes"]);
    hide_widget(gui->widgets["frm_top"]);
    
    cam_x = cam_y = 0;
    cam_zoom = 1;
    
    //Find the most popular file name to suggest for new sprites.
    last_file_used.clear();
    
    if(!anims.sprites.empty()) {
        map<string, size_t> file_uses_map;
        vector<pair<size_t, string> > file_uses_vector;
        for(size_t f = 0; f < anims.sprites.size(); ++f) {
            file_uses_map[anims.sprites[f]->file]++;
        }
        for(auto u = file_uses_map.begin(); u != file_uses_map.end(); ++u) {
            file_uses_vector.push_back(make_pair(u->second, u->first));
        }
        sort(
            file_uses_vector.begin(),
            file_uses_vector.end(),
        [] (pair<size_t, string> u1, pair<size_t, string> u2) -> bool {
            return u1.first > u2.first;
        }
        );
        last_file_used = file_uses_vector[0].second;
    }
    
    vector<string> file_path_parts = split(file_path, "/");
    ((lafi::button*) gui->widgets["frm_main"]->widgets["but_file"])->text =
        get_cut_path(file_path);
        
    //Top bitmap.
    for(unsigned char t = 0; t < 3; ++t) {
        if(top_bmp[t] && top_bmp[t] != bmp_error) {
            al_destroy_bitmap(top_bmp[t]);
            top_bmp[t] = NULL;
        }
    }
    
    if(file_path.find(PIKMIN_FOLDER) != string::npos) {
        is_pikmin = true;
        data_node data =
            data_node(
                PIKMIN_FOLDER + "/" +
                file_path_parts[file_path_parts.size() - 2] +
                "/Data.txt"
            );
        top_bmp[0] =
            load_bmp(data.get_child_by_name("top_leaf")->value, &data);
        top_bmp[1] =
            load_bmp(data.get_child_by_name("top_bud")->value, &data);
        top_bmp[2] =
            load_bmp(data.get_child_by_name("top_flower")->value, &data);
    } else {
        is_pikmin = false;
    }
    
    mode = EDITOR_MODE_MAIN;
    change_to_right_frame();
    update_stats();
}


/* ----------------------------------------------------------------------------
 * Opens the correct radio button and frame for the specified hitbox type.
 */
void animation_editor::open_hitbox_type(unsigned char type) {
    lafi::widget* f = gui->widgets["frm_hitboxes"]->widgets["frm_hitbox"];
    
    ((lafi::radio_button*) f->widgets["rad_normal"])->unselect();
    ((lafi::radio_button*) f->widgets["rad_attack"])->unselect();
    ((lafi::radio_button*) f->widgets["rad_disabled"])->unselect();
    
    hide_widget(f->widgets["frm_normal"]);
    hide_widget(f->widgets["frm_attack"]);
    
    if(type == HITBOX_TYPE_NORMAL) {
        ((lafi::radio_button*) f->widgets["rad_normal"])->select();
        show_widget(f->widgets["frm_normal"]);
    } else if(type == HITBOX_TYPE_ATTACK) {
        ((lafi::radio_button*) f->widgets["rad_attack"])->select();
        show_widget(f->widgets["frm_attack"]);
    } else {
        ((lafi::radio_button*) f->widgets["rad_disabled"])->select();
    }
}


/* ----------------------------------------------------------------------------
 * Opens the frame where you pick from a list.
 * For the type of content, use animation_editor::ANIMATION_EDITOR_PICKER_*.
 */
void animation_editor::open_picker(
    const unsigned char type, const bool can_make_new
) {
    vector<string> elements;
    if(type == ANIMATION_EDITOR_PICKER_ANIMATION) {
        for(size_t a = 0; a < anims.animations.size(); ++a) {
            elements.push_back(anims.animations[a]->name);
        }
    } else if(
        type == ANIMATION_EDITOR_PICKER_FRAME ||
        type == ANIMATION_EDITOR_PICKER_FRAME_INSTANCE
    ) {
        for(size_t s = 0; s < anims.sprites.size(); ++s) {
            elements.push_back(anims.sprites[s]->name);
        }
    }
    
    generate_and_open_picker(elements, type, can_make_new);
}


/* ----------------------------------------------------------------------------
 * Picks an item and closes the list picker frame.
 */
void animation_editor::pick(const string &name, const unsigned char type) {
    if(type == ANIMATION_EDITOR_PICKER_ANIMATION) {
        cur_anim = anims.animations[anims.find_animation(name)];
        cur_frame_nr =
            (cur_anim->frames.size()) ? 0 : INVALID;
        cur_hitbox_nr = INVALID;
        animation_to_gui();
        
    } else if(type == ANIMATION_EDITOR_PICKER_FRAME_INSTANCE) {
        cur_anim->frames[cur_frame_nr].sprite_name =
            name;
        cur_anim->frames[cur_frame_nr].sprite_ptr =
            anims.sprites[anims.find_sprite(name)];
        frame_to_gui();
        
    } else if(type == ANIMATION_EDITOR_PICKER_FRAME) {
        if(mode == EDITOR_MODE_SPRITE_OFFSET) {
            comparison_sprite = anims.sprites[anims.find_sprite(name)];
            sprite_offset_to_gui();
        } else {
            cur_sprite = anims.sprites[anims.find_sprite(name)];
            cur_hitbox_nr = INVALID;
            if(cur_sprite->file.empty()) {
                //New frame. Suggest file name.
                cur_sprite->file = last_file_used;
            }
            sprite_to_gui();
        }
        
    }
    
    show_bottom_frame();
    change_to_right_frame();
}


/* ----------------------------------------------------------------------------
 * Populates the history frame with the most recent files.
 */
void animation_editor::populate_history() {
    lafi::frame* f =
        (lafi::frame*) gui->widgets["frm_history"]->widgets["frm_list"];
        
    while(!f->widgets.empty()) {
        f->remove(f->widgets.begin()->first);
    }
    
    if(animation_editor_history.empty()) return;
    
    f->easy_reset();
    f->easy_row();
    
    for(size_t h = 0; h < animation_editor_history.size(); ++h) {
    
        string name = animation_editor_history[h];
        if(name.empty()) continue;
        
        lafi::button* b =
            new lafi::button(0, 0, 0, 0, get_cut_path(name));
            
        auto lambda = [name, this] (lafi::widget*, int, int) {
            file_path = name;
            load_animation_database();
            
            mode = EDITOR_MODE_MAIN;
            show_bottom_frame();
            change_to_right_frame();
            
            update_animation_editor_history(name);
            save_options(); //Save the history on the options.
        };
        b->left_mouse_click_handler = lambda;
        f->easy_add("but_" + i2s(h), b, 100, 32);
        f->easy_row();
    }
}


/* ----------------------------------------------------------------------------
 * Resizes sprites, body parts, etc. by a multiplier.
 */
void animation_editor::resize_everything() {
    lafi::textbox* txt_resize =
        (lafi::textbox*) gui->widgets["frm_tools"]->widgets["txt_resize"];
    float mult = s2f(txt_resize->text);
    
    for(size_t s = 0; s < anims.sprites.size(); ++s) {
        sprite* s_ptr = anims.sprites[s];
        
        s_ptr->game_w *= mult;
        s_ptr->game_h *= mult;
        s_ptr->offs_x *= mult;
        s_ptr->offs_y *= mult;
        s_ptr->top_x  *= mult;
        s_ptr->top_y  *= mult;
        s_ptr->top_w  *= mult;
        s_ptr->top_h  *= mult;
        
        for(size_t h = 0; h < s_ptr->hitboxes.size(); ++h) {
            hitbox* h_ptr = &s_ptr->hitboxes[h];
            
            h_ptr->radius *= mult;
            h_ptr->x      *= mult;
            h_ptr->y      *= mult;
        }
    }
    
    made_changes = true;
}


/* ----------------------------------------------------------------------------
 * Saves the animation database onto the mob's file.
 */
void animation_editor::save_animation_database() {
    data_node file_node = data_node("", "");
    
    data_node* animations_node = new data_node("animations", "");
    file_node.add(animations_node);
    
    for(size_t a = 0; a < anims.animations.size(); ++a) {
        data_node* anim_node = new data_node(anims.animations[a]->name, "");
        animations_node->add(anim_node);
        
        anim_node->add(
            new data_node("loop_frame", i2s(anims.animations[a]->loop_frame))
        );
        data_node* frames_node = new data_node("frames", "");
        anim_node->add(frames_node);
        
        for(
            size_t f = 0; f < anims.animations[a]->frames.size();
            ++f
        ) {
            frame* f_ptr = &anims.animations[a]->frames[f];
            
            data_node* frame_node =
                new data_node(f_ptr->sprite_name, "");
            frames_node->add(frame_node);
            
            frame_node->add(
                new data_node("duration", f2s(f_ptr->duration))
            );
        }
    }
    
    data_node* sprites_node = new data_node("sprites", "");
    file_node.add(sprites_node);
    
    for(size_t s = 0; s < anims.sprites.size(); ++s) {
        sprite* s_ptr = anims.sprites[s];
        data_node* sprite_node = new data_node(anims.sprites[s]->name, "");
        sprites_node->add(sprite_node);
        
        sprite_node->add(new data_node("file", s_ptr->file));
        sprite_node->add(new data_node("file_x", i2s(s_ptr->file_x)));
        sprite_node->add(new data_node("file_y", i2s(s_ptr->file_y)));
        sprite_node->add(new data_node("file_w", i2s(s_ptr->file_w)));
        sprite_node->add(new data_node("file_h", i2s(s_ptr->file_h)));
        sprite_node->add(new data_node("game_w", f2s(s_ptr->game_w)));
        sprite_node->add(new data_node("game_h", f2s(s_ptr->game_h)));
        sprite_node->add(new data_node("offs_x", f2s(s_ptr->offs_x)));
        sprite_node->add(new data_node("offs_y", f2s(s_ptr->offs_y)));
        
        if(is_pikmin) {
            sprite_node->add(
                new data_node("top_visible", b2s(s_ptr->top_visible))
            );
            sprite_node->add(
                new data_node("top_x", f2s(s_ptr->top_x))
            );
            sprite_node->add(
                new data_node("top_y", f2s(s_ptr->top_y))
            );
            sprite_node->add(
                new data_node("top_w", f2s(s_ptr->top_w))
            );
            sprite_node->add(
                new data_node("top_h", f2s(s_ptr->top_h))
            );
            sprite_node->add(
                new data_node("top_angle", f2s(s_ptr->top_angle))
            );
        }
        
        data_node* hitboxes_node =
            new data_node("hitboxes", "");
        sprite_node->add(hitboxes_node);
        
        for(size_t h = 0; h < s_ptr->hitboxes.size(); ++h) {
            hitbox* h_ptr = &s_ptr->hitboxes[h];
            
            data_node* hitbox_node =
                new data_node(h_ptr->body_part_name, "");
            hitboxes_node->add(hitbox_node);
            
            hitbox_node->add(
                new data_node(
                    "coords",
                    f2s(h_ptr->x) + " " + f2s(h_ptr->y) +
                    " " + f2s(h_ptr->z)
                )
            );
            hitbox_node->add(
                new data_node("height", f2s(h_ptr->height))
            );
            hitbox_node->add(
                new data_node("radius", f2s(h_ptr->radius))
            );
            hitbox_node->add(
                new data_node("type", i2s(h_ptr->type))
            );
            hitbox_node->add(
                new data_node("multiplier", f2s(h_ptr->multiplier))
            );
            hitbox_node->add(
                new data_node("can_pikmin_latch", b2s(h_ptr->can_pikmin_latch))
            );
            hitbox_node->add(
                new data_node("hazards", h_ptr->hazards_str)
            );
            hitbox_node->add(
                new data_node("outward", b2s(h_ptr->knockback_outward))
            );
            hitbox_node->add(
                new data_node("angle", f2s(h_ptr->knockback_angle))
            );
            hitbox_node->add(
                new data_node("knockback", f2s(h_ptr->knockback))
            );
        }
    }
    
    data_node* body_parts_node = new data_node("body_parts", "");
    file_node.add(body_parts_node);
    
    for(size_t b = 0; b < anims.body_parts.size(); ++b) {
        data_node* body_part_node = new data_node(anims.body_parts[b]->name, "");
        body_parts_node->add(body_part_node);
        
    }
    
    file_node.save_file(file_path);
    made_changes = false;
}


/* ----------------------------------------------------------------------------
 * Unloads the editor from memory.
 */
void animation_editor::unload() {
    anims.destroy();
    delete(gui->style);
    delete(gui);
    al_destroy_native_file_dialog(file_dialog);
    
    unload_hazards();
    unload_status_types();
}

/* ----------------------------------------------------------------------------
 * Update every frame's hitbox instances in light of new hitbox info.
 */
void animation_editor::update_hitboxes() {
    for(size_t s = 0; s < anims.sprites.size(); ++s) {
    
        sprite* s_ptr = anims.sprites[s];
        
        //Start by deleting non-existent hitboxes.
        for(size_t h = 0; h < s_ptr->hitboxes.size();) {
            string h_name = s_ptr->hitboxes[h].body_part_name;
            bool name_found = false;
            
            for(size_t b = 0; b < anims.body_parts.size(); ++b) {
                if(anims.body_parts[b]->name == h_name) {
                    name_found = true;
                    break;
                }
            }
            
            if(!name_found) {
                s_ptr->hitboxes.erase(
                    s_ptr->hitboxes.begin() + h
                );
            } else {
                h++;
            }
        }
        
        //Add missing hitboxes.
        for(size_t b = 0; b < anims.body_parts.size(); ++b) {
            bool hitbox_found = false;
            string name = anims.body_parts[b]->name;
            
            for(size_t h = 0; h < s_ptr->hitboxes.size(); ++h) {
                if(s_ptr->hitboxes[h].body_part_name == name) {
                    hitbox_found = true;
                    break;
                }
            }
            
            if(!hitbox_found) {
                s_ptr->hitboxes.push_back(hitbox(name));
            }
        }
        
        //Sort them with the new order.
        std::sort(
            s_ptr->hitboxes.begin(),
            s_ptr->hitboxes.end(),
        [this] (hitbox h1, hitbox h2) -> bool {
            size_t pos1 = 0, pos2 = 1;
            for(size_t b = 0; b < anims.body_parts.size(); ++b) {
                if(anims.body_parts[b]->name == h1.body_part_name) pos1 = b;
                if(anims.body_parts[b]->name == h2.body_part_name) pos2 = b;
            }
            return pos1 < pos2;
        }
        );
    }
}


/* ----------------------------------------------------------------------------
 * Update the stats on the main menu, as well as some other minor things.
 */
void animation_editor::update_stats() {
    lafi::widget* f = gui->widgets["frm_main"]->widgets["frm_object"];
    if(file_path.empty()) {
        hide_widget(f);
    } else {
        show_widget(f);
    }
    
    ((lafi::label*) f->widgets["lbl_n_anims"])->text =
        "Animations: " + i2s(anims.animations.size());
    ((lafi::label*) f->widgets["lbl_n_sprites"])->text =
        "Sprites: " + i2s(anims.sprites.size());
    ((lafi::label*) f->widgets["lbl_n_body_parts"])->text =
        "Body parts: " + i2s(anims.body_parts.size());
}


/* ----------------------------------------------------------------------------
 * Creates a new item from the picker frame, given its name.
 */
void animation_editor::create_new_from_picker(const string &name) {
    if(mode == EDITOR_MODE_ANIMATION) {
        if(anims.find_animation(name) != INVALID) return;
        anims.animations.push_back(new animation(name));
        pick(name, ANIMATION_EDITOR_PICKER_ANIMATION);
        
    } else if(mode == EDITOR_MODE_SPRITE) {
        if(anims.find_sprite(name) != INVALID) return;
        anims.sprites.push_back(new sprite(name));
        anims.sprites.back()->create_hitboxes(&anims);
        pick(name, ANIMATION_EDITOR_PICKER_FRAME);
        
    }
}


/* ----------------------------------------------------------------------------
 * Hides all menu frames.
 */
void animation_editor::hide_all_frames() {
    hide_widget(gui->widgets["frm_main"]);
    hide_widget(gui->widgets["frm_picker"]);
    hide_widget(gui->widgets["frm_history"]);
    hide_widget(gui->widgets["frm_anims"]);
    hide_widget(gui->widgets["frm_sprites"]);
    hide_widget(gui->widgets["frm_offset"]);
    hide_widget(gui->widgets["frm_hitboxes"]);
    hide_widget(gui->widgets["frm_top"]);
    hide_widget(gui->widgets["frm_body_parts"]);
    hide_widget(gui->widgets["frm_tools"]);
}


/* ----------------------------------------------------------------------------
 * Switches to the correct frame, depending on the current editor mode.
 */
void animation_editor::change_to_right_frame() {
    hide_all_frames();
    
    if(mode == EDITOR_MODE_MAIN) {
        show_widget(gui->widgets["frm_main"]);
    } else if(mode == EDITOR_MODE_ANIMATION) {
        show_widget(gui->widgets["frm_anims"]);
    } else if(mode == EDITOR_MODE_SPRITE) {
        show_widget(gui->widgets["frm_sprites"]);
    } else if(mode == EDITOR_MODE_BODY_PART) {
        show_widget(gui->widgets["frm_body_parts"]);
    } else if(mode == EDITOR_MODE_HITBOXES) {
        show_widget(gui->widgets["frm_hitboxes"]);
    } else if(mode == EDITOR_MODE_SPRITE_OFFSET) {
        show_widget(gui->widgets["frm_offset"]);
    } else if(mode == EDITOR_MODE_TOP) {
        show_widget(gui->widgets["frm_top"]);
    } else if(mode == EDITOR_MODE_HISTORY) {
        show_widget(gui->widgets["frm_history"]);
    } else if(mode == EDITOR_MODE_TOOLS) {
        show_widget(gui->widgets["frm_tools"]);
    }
}