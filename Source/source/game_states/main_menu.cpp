/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Main menu state class and main menu state-related functions.
 */

#include <algorithm>

#include "menus.h"

#include "../drawing.h"
#include "../functions.h"
#include "../game.h"
#include "../load.h"
#include "../utils/string_utils.h"


//Path to the GUI information file.
const string main_menu_state::GUI_FILE_PATH =
    GUI_FOLDER_PATH + "/Main_menu.txt";


/* ----------------------------------------------------------------------------
 * Creates a "main menu" state.
 */
main_menu_state::main_menu_state() :
    game_state(),
    bmp_menu_bg(NULL),
    logo_data(logo_map()){
    
}


/* ----------------------------------------------------------------------------
 * Draws the main menu.
 */
void main_menu_state::do_drawing() {
    al_clear_to_color(al_map_rgb(0, 0, 0));
    draw_bitmap(
        bmp_menu_bg, point(game.win_w * 0.5, game.win_h * 0.5),
        point(game.win_w, game.win_h)
    );
    
    //Draw the logo Pikmin.
    logo_data.do_drawing(map_gray(255));
    
    draw_scaled_text(
        game.fonts.standard, al_map_rgb(255, 255, 255),
        point(8, game.win_h  - 8),
        point(0.6, 0.6),
        ALLEGRO_ALIGN_LEFT, 2,
        "Pikmin (c) Nintendo"
    );
    draw_scaled_text(
        game.fonts.standard, al_map_rgb(255, 255, 255),
        point(game.win_w - 8, game.win_h  - 8),
        point(0.6, 0.6),
        ALLEGRO_ALIGN_RIGHT, 2,
        game.config.name + " " + game.config.version +
        ", powered by Pikifen " +
        i2s(VERSION_MAJOR) + "." + i2s(VERSION_MINOR)  + "." + i2s(VERSION_REV)
    );
    
    gui.draw();
    
    game.fade_mgr.draw();
    
    al_flip_display();
}


/* ----------------------------------------------------------------------------
 * Ticks a frame's worth of logic.
 */
void main_menu_state::do_logic() {
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
string main_menu_state::get_name() const {
    return "main menu";
}


/* ----------------------------------------------------------------------------
 * Handles Allegro events.
 * ev:
 *   Event to handle.
 */
void main_menu_state::handle_allegro_event(ALLEGRO_EVENT &ev) {
    if(game.fade_mgr.is_fading()) return;
    
    gui.handle_event(ev);
}


/* ----------------------------------------------------------------------------
 * Loads the main menu into memory.
 */
void main_menu_state::load() {
    draw_loading_screen("", "", 1.0);
    al_flip_display();
    data_node settings_file(GUI_FILE_PATH);
    
    //Menu items.
    gui.register_coords("play",             50, 55, 50,  6);
    gui.register_coords("options",          50, 63, 50,  6);
    gui.register_coords("editors",          50, 71, 50,  6);
    gui.register_coords("exit",             50, 87, 50,  6);
    gui.read_coords(settings_file.get_child_by_name("positions"));
    
    //Play button.
    button_gui_item* play_button =
        new button_gui_item("Play", game.fonts.area_name);
    play_button->on_activate =
    [] (const point &) {
        game.fade_mgr.start_fade(false, [] () {
            game.change_state(game.states.area_menu);
        });
    };
    gui.add_item(play_button, "play");
    
    //Options button.
    button_gui_item* options_button =
        new button_gui_item("Options", game.fonts.area_name);
    options_button->on_activate =
    [] (const point &) {
        game.fade_mgr.start_fade(false, [] () {
            game.change_state(game.states.options_menu);
        });
    };
    gui.add_item(options_button, "options");
    
    //Editors button.
    button_gui_item* editors_button =
        new button_gui_item("Editors", game.fonts.area_name);
    editors_button->on_activate =
    [] (const point &) {
        game.fade_mgr.start_fade(false, [] () {
            game.change_state(game.states.editors_menu);
        });
    };
    gui.add_item(editors_button, "editors");
    
    //Exit button.
    gui.back_item =
        new button_gui_item("Exit", game.fonts.area_name);
    gui.back_item->on_activate =
    [] (const point &) {
        game.is_game_running = false;
    };
    gui.add_item(gui.back_item, "exit");
    
    //Resources.
    bmp_menu_bg = load_bmp(game.asset_file_names.main_menu);
    
    //Logo pikmin.
    logo_data.load(settings_file.get_child_by_name("logo"));
    
    //Finishing touches.
    gui.set_selected_item(play_button);
    game.fade_mgr.start_fade(true, nullptr);
    
}


/* ----------------------------------------------------------------------------
 * Unloads the main menu from memory.
 */
void main_menu_state::unload() {

    //Resources.
    al_destroy_bitmap(bmp_menu_bg);
    
    //Menu items.
    gui.destroy();
    
    //Misc.
    logo_data.logo_pikmin.clear();
    
}
