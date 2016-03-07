/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2016.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Pikmin type class and Pikmin type-related functions.
 */

#include "pikmin_type.h"

#include "../const.h"
#include "../functions.h"
#include "leader.h"
#include "../mob_script.h"
#include "../vars.h"

/* ----------------------------------------------------------------------------
 * Creates a Pikmin type.
 */
pikmin_type::pikmin_type() :
    mob_type(),
    carry_strength(1),
    attack_power(1),
    carry_speed(1),
    attack_interval(0.8),
    size(DEF_PIKMIN_SIZE),
    throw_height_mult(1.0),
    has_onion(true),
    can_dig(false),
    can_fly(false),
    can_swim(false),
    can_latch(true),
    can_carry_bomb_rocks(false) {
    
    bmp_top[0] = NULL;
    bmp_top[1] = NULL;
    bmp_top[2] = NULL;
    bmp_icon[0] = NULL;
    bmp_icon[1] = NULL;
    bmp_icon[2] = NULL;
    
    weight = 1;
    show_health = false;
    
    init_script();
}


void pikmin_type::load_from_file(data_node* file, const bool load_resources, vector<pair<size_t, string> >* anim_conversions) {
    attack_power = s2f(file->get_child_by_name("attack_power")->value);
    attack_interval = s2f(file->get_child_by_name("attack_interval")->get_value_or_default("0.8"));
    throw_height_mult = s2f(file->get_child_by_name("throw_height_mult")->get_value_or_default("1"));
    can_carry_bomb_rocks = s2b(file->get_child_by_name("can_carry_bomb_rocks")->value);
    can_dig = s2b(file->get_child_by_name("can_dig")->value);
    can_latch = s2b(file->get_child_by_name("can_latch")->value);
    can_swim = s2b(file->get_child_by_name("can_swim")->value);
    carry_speed = s2f(file->get_child_by_name("carry_speed")->value);
    carry_strength = s2f(file->get_child_by_name("carry_strength")->value);
    has_onion = s2b(file->get_child_by_name("has_onion")->value);
    
    if(load_resources) {
        bmp_top[0] =  bitmaps.get(file->get_child_by_name("top_leaf")->value,    file);
        bmp_top[1] =  bitmaps.get(file->get_child_by_name("top_bud")->value,     file);
        bmp_top[2] =  bitmaps.get(file->get_child_by_name("top_flower")->value,  file);
        bmp_icon[0] = bitmaps.get(file->get_child_by_name("icon_leaf")->value,   file);
        bmp_icon[1] = bitmaps.get(file->get_child_by_name("icon_bud")->value,    file);
        bmp_icon[2] = bitmaps.get(file->get_child_by_name("icon_flower")->value, file);
    }
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_IDLE,     "idle"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_WALK,     "walk"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_THROWN,   "thrown"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_ATTACK,   "attack"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_GRAB,     "grab"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_CARRY,    "carry"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_BURROWED, "burrowed"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_PLUCKING, "plucking"));
    anim_conversions->push_back(make_pair(PIKMIN_ANIM_LYING,    "lying"));
}


void pikmin_type::init_script() {

    easy_fsm_creator efc;
    efc.new_state("buried", PIKMIN_STATE_BURIED); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_become_buried);
        }
        efc.new_event(MOB_EVENT_PLUCKED); {
            efc.run_function(pikmin::fsm_begin_pluck);
            efc.change_state("plucked");
        }
        efc.new_event(MOB_EVENT_LANDED); {
            efc.run_function(pikmin::fsm_stand_still);
        }
    }
    
    efc.new_state("plucked", PIKMIN_STATE_PLUCKING); {
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.run_function(pikmin::fsm_end_pluck);
            efc.change_state("in_group_chasing");
        }
    }
    
    efc.new_state("in_group_chasing", PIKMIN_STATE_IN_GROUP_CHASING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_chase_leader);
        }
        efc.new_event(MOB_EVENT_GRABBED_BY_FRIEND); {
            efc.run_function(pikmin::fsm_be_grabbed_by_friend);
            efc.change_state("grabbed_by_leader");
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.change_state("in_group_stopped");
        }
        efc.new_event(MOB_EVENT_GROUP_MOVE_STARTED); {
            efc.change_state("group_move_chasing");
        }
        efc.new_event(MOB_EVENT_DISMISSED); {
            efc.run_function(pikmin::fsm_be_dismissed);
            efc.change_state("going_to_dismiss_spot");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("in_group_stopped", PIKMIN_STATE_IN_GROUP_STOPPED); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_stop_in_group);
        }
        efc.new_event(MOB_EVENT_GRABBED_BY_FRIEND); {
            efc.run_function(pikmin::fsm_be_grabbed_by_friend);
            efc.change_state("grabbed_by_leader");
        }
        efc.new_event(MOB_EVENT_SPOT_IS_FAR); {
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_GROUP_MOVE_STARTED); {
            efc.change_state("group_move_chasing");
        }
        efc.new_event(MOB_EVENT_DISMISSED); {
            efc.run_function(pikmin::fsm_be_dismissed);
            efc.change_state("going_to_dismiss_spot");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("group_move_chasing", PIKMIN_STATE_GROUP_MOVE_CHASING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_chase_leader);
        }
        efc.new_event(MOB_EVENT_ON_TICK); {
            efc.run_function(pikmin::fsm_chase_leader);
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.change_state("group_move_stopped");
        }
        efc.new_event(MOB_EVENT_GROUP_MOVE_ENDED); {
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_DISMISSED); {
            efc.run_function(pikmin::fsm_be_dismissed);
            efc.change_state("going_to_dismiss_spot");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_NEAR_OPPONENT); {
            efc.run_function(pikmin::fsm_go_to_opponent);
            efc.change_state("going_to_opponent");
        }
        efc.new_event(MOB_EVENT_NEAR_CARRIABLE_OBJECT); {
            efc.run_function(pikmin::fsm_go_to_carriable_object);
            efc.change_state("going_to_carriable_object");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("group_move_stopped", PIKMIN_STATE_GROUP_MOVE_STOPPED); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_stop_in_group);
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.change_state("group_move_stopped");
        }
        efc.new_event(MOB_EVENT_SPOT_IS_FAR); {
            efc.change_state("group_move_chasing");
        }
        efc.new_event(MOB_EVENT_GROUP_MOVE_ENDED); {
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_DISMISSED); {
            efc.run_function(pikmin::fsm_be_dismissed);
            efc.change_state("going_to_dismiss_spot");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_NEAR_OPPONENT); {
            efc.run_function(pikmin::fsm_go_to_opponent);
            efc.change_state("going_to_opponent");
        }
        efc.new_event(MOB_EVENT_NEAR_CARRIABLE_OBJECT); {
            efc.run_function(pikmin::fsm_go_to_carriable_object);
            efc.change_state("going_to_carriable_object");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("grabbed_by_leader", PIKMIN_STATE_GRABBED_BY_LEADER); {
        efc.new_event(MOB_EVENT_ON_LEAVE); {
            efc.run_function(pikmin::fsm_be_released);
        }
        efc.new_event(MOB_EVENT_THROWN); {
            efc.run_function(pikmin::fsm_be_thrown);
            efc.change_state("thrown");
        }
        efc.new_event(MOB_EVENT_RELEASED); {
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("thrown", PIKMIN_STATE_THROWN); {
        efc.new_event(MOB_EVENT_LANDED); {
            efc.run_function(pikmin::fsm_land);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_A_N); {
            efc.run_function(pikmin::fsm_land_on_mob);
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("going_to_dismiss_spot", PIKMIN_STATE_GOING_TO_DISMISS_SPOT); {
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.run_function(pikmin::fsm_reach_dismiss_spot);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_NEAR_OPPONENT); {
            efc.run_function(pikmin::fsm_go_to_opponent);
            efc.change_state("going_to_opponent");
        }
        efc.new_event(MOB_EVENT_NEAR_CARRIABLE_OBJECT); {
            efc.run_function(pikmin::fsm_go_to_carriable_object);
            efc.change_state("going_to_carriable_object");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("idle", PIKMIN_STATE_IDLE); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_become_idle);
        }
        efc.new_event(MOB_EVENT_ON_LEAVE); {
            efc.run_function(pikmin::fsm_stop_being_idle);
        }
        efc.new_event(MOB_EVENT_NEAR_OPPONENT); {
            efc.run_function(pikmin::fsm_go_to_opponent);
            efc.change_state("going_to_opponent");
        }
        efc.new_event(MOB_EVENT_NEAR_CARRIABLE_OBJECT); {
            efc.run_function(pikmin::fsm_go_to_carriable_object);
            efc.change_state("going_to_carriable_object");
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_TOUCHED_LEADER); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("going_to_opponent", PIKMIN_STATE_GOING_TO_OPPONENT); {
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.change_state("attacking_grounded");
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_LOST_FOCUSED_MOB); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_FOCUSED_MOB_DIED); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("going_to_carriable_object", PIKMIN_STATE_GOING_TO_CARRIABLE_OBJECT); {
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.run_function(pikmin::fsm_reach_carriable_object);
            efc.change_state("carrying");
        }
        efc.new_event(MOB_EVENT_FOCUSED_MOB_UNCARRIABLE); {
            efc.run_function(pikmin::fsm_forget_carriable_object);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_TIMER); {
            efc.run_function(pikmin::fsm_forget_carriable_object);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("carrying", PIKMIN_STATE_CARRYING); {
        efc.new_event(MOB_EVENT_ON_LEAVE); {
            efc.run_function(pikmin::fsm_stop_carrying);
            efc.run_function(pikmin::fsm_stand_still);
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_FINISHED_CARRYING); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_LOST_FOCUSED_MOB); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_FOCUSED_MOB_UNCARRIABLE); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("attacking_grounded", PIKMIN_STATE_ATTACKING_GROUNDED); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_prepare_to_attack);
        }
        efc.new_event(MOB_EVENT_ON_TICK); {
            efc.run_function(pikmin::fsm_tick_attacking_grounded);
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.run_function(pikmin::fsm_rechase_opponent);
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
    }
    
    efc.new_state("attacking_latched", PIKMIN_STATE_ATTACKING_LATCHED); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(pikmin::fsm_prepare_to_attack);
        }
        efc.new_event(MOB_EVENT_ON_TICK); {
            efc.run_function(pikmin::fsm_tick_latched);
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(pikmin::fsm_called);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_FOCUSED_MOB_DIED); {
            efc.run_function(pikmin::fsm_lose_latched_mob);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(pikmin::fsm_get_knocked_down);
            efc.change_state("knocked_back");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("grabbed_by_enemy", PIKMIN_STATE_GRABBED_BY_ENEMY); {
        efc.new_event(MOB_EVENT_RELEASED); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_ON_TICK); {
            efc.run_function(pikmin::fsm_tick_grabbed_by_enemy);
        }
    }
    
    efc.new_state("knocked_back", PIKMIN_STATE_KNOCKED_BACK); {
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.run_function(pikmin::fsm_stand_still);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_LANDED); {
            efc.run_function(pikmin::fsm_stand_still);
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_EAT); {
            efc.run_function(pikmin::fsm_be_grabbed_by_enemy);
            efc.change_state("grabbed_by_enemy");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(pikmin::fsm_fall_down_pit);
        }
    }
    
    efc.new_state("celebrating", PIKMIN_STATE_CELEBRATING); {
        //TODO
    }
    
    states = efc.finish();
    first_state_nr = fix_states(states, "idle");
    
    if(states.size() != N_PIKMIN_STATES) {
        error_log(
            "ENGINE WARNING: Number of Pikmin states on the FSM (" + i2s(states.size()) +
            ") and the enum (" + i2s(N_PIKMIN_STATES) + ") do not match.");
    }
}