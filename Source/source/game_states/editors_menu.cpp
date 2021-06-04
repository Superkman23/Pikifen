/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Editor selector menu state class and state-related functions.
 */

#include <algorithm>

#include "menus.h"

#include "../drawing.h"
#include "../functions.h"
#include "../game.h"
#include "../load.h"
#include "../utils/string_utils.h"


//Path to the GUI information file.
const string editors_menu_state::GUI_FILE_PATH =
    GUI_FOLDER_PATH + "/Editors_menu.txt";


/* ----------------------------------------------------------------------------
 * Creates a "create menu" state.
 */
editors_menu_state::editors_menu_state():
    game_state(),
    bmp_menu_bg(NULL),
    logo_data(logo_map()){
    
}


/* ----------------------------------------------------------------------------
 * Draws the create menu.
 */
void editors_menu_state::do_drawing() {
    al_clear_to_color(al_map_rgb(0, 0, 0));
    draw_bitmap(
        bmp_menu_bg, point(game.win_w * 0.5, game.win_h * 0.5),
        point(game.win_w, game.win_h)
    );
    
    //Draw the logo Pikmin.
    logo_data.do_drawing(logo_data.logo_tint);
    
    gui.draw();
    
    game.fade_mgr.draw();
    
    al_flip_display();
}


/* ----------------------------------------------------------------------------
 * Ticks a frame's worth of logic.
 */
void editors_menu_state::do_logic() {
    //Animate the logo Pikmin.
    logo_data.do_logic();
    
    gui.tick(game.delta_t);
    
    //Fade manager needs to come last, because if
    //the fade finishes and the state changes, and
    //after that we still attempt to do stuff in
    //this function, we're going to have a bad time.
    game.fade_mgr.tick(game.delta_t);
    
}


/* ----------------------------------------------------------------------------
 * Returns the name of this state.
 */
string editors_menu_state::get_name() const {
    return "editors menu";
}


/* ----------------------------------------------------------------------------
 * Handles Allegro events.
 * ev:
 *   Event to handle.
 */
void editors_menu_state::handle_allegro_event(ALLEGRO_EVENT &ev) {
    if(game.fade_mgr.is_fading()) return;
    
    gui.handle_event(ev);
}


/* ----------------------------------------------------------------------------
 * Loads the editors menu into memory.
 */
void editors_menu_state::load() {
    draw_loading_screen("", "", 1.0);
    al_flip_display();
    data_node settings_file(GUI_FILE_PATH);
    
    //Menu items.
    gui.register_coords("animation_editor", 50, 71, 50,  6);
    gui.register_coords("area_editor",      50, 79, 50,  6);
    gui.register_coords("back",             50, 87, 50,  6);
    gui.read_coords(settings_file.get_child_by_name("positions"));

    //Animation editor button.
    button_gui_item* anim_ed_button =
        new button_gui_item("Animation Editor", game.fonts.area_name);
    anim_ed_button->on_activate =
    [] (const point &) {
        game.fade_mgr.start_fade(false, [] () {
            game.change_state(game.states.animation_ed);
        });
    };
    gui.add_item(anim_ed_button, "animation_editor");
    
    //Area editor button.
    button_gui_item* area_ed_button =
        new button_gui_item("Area Editor", game.fonts.area_name);
    area_ed_button->on_activate =
    [] (const point &) {
        game.fade_mgr.start_fade(false, [] () {
            game.change_state(game.states.area_ed);
        });
    };
    gui.add_item(area_ed_button, "area_editor");
    
    //Back button.
    gui.back_item =
        new button_gui_item("Back", game.fonts.area_name);
    gui.back_item->on_activate =
    [] (const point &) {
        game.fade_mgr.start_fade(false, []() {
            game.change_state(game.states.main_menu);
        });
    };
    gui.add_item(gui.back_item, "back");
    
    //Resources.
    bmp_menu_bg = load_bmp(game.asset_file_names.main_menu);
    
    //Logo pikmin.
    logo_data.load(settings_file.get_child_by_name("logo"));
    
    //Finishing touches.
    gui.set_selected_item(anim_ed_button);
    game.fade_mgr.start_fade(true, nullptr);
    
}


/* ----------------------------------------------------------------------------
 * Unloads the editors menu from memory.
 */
void editors_menu_state::unload() {

    //Resources.
    al_destroy_bitmap(bmp_menu_bg);
    
    //Menu items.
    gui.destroy();
    
    //Misc.
    logo_data.unload();
    
}
