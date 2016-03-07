/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2016.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Leader type class and leader type-related functions.
 */

#include "../functions.h"
#include "leader_fsm.h"
#include "leader_type.h"
#include "mob_fsm.h"
#include "../const.h"
#include "../vars.h"

leader_type::leader_type() :
    mob_type(),
    whistle_range(DEF_WHISTLE_RANGE),
    punch_strength(DEF_PUNCH_STRENGTH),
    pluck_delay(0.6),
    bmp_icon(nullptr) {
    
    main_color = al_map_rgb(128, 128, 128);
    show_health = false;
    
    init_script();
}

void leader_type::init_script() {

    easy_fsm_creator efc;
    efc.new_state("idle", LEADER_STATE_IDLE); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::enter_idle);
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(leader_fsm::join_group);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(LEADER_EVENT_FOCUSED); {
            efc.run_function(leader_fsm::focus);
            efc.change_state("active");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::inactive_lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("inactive_dying");
        }
        efc.new_event(LEADER_EVENT_INACTIVE_SEARCH_SEED); {
            efc.run_function(leader_fsm::inactive_search_seed);
        }
        efc.new_event(LEADER_EVENT_GO_PLUCK); {
            efc.run_function(leader_fsm::go_pluck);
            efc.change_state("inactive_going_to_pluck");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("active", LEADER_STATE_ACTIVE); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::enter_active);
        }
        efc.new_event(LEADER_EVENT_UNFOCUSED); {
            efc.run_function(leader_fsm::stop);
            efc.run_function(leader_fsm::unfocus);
            efc.change_state("idle");
        }
        efc.new_event(LEADER_EVENT_MOVE_START); {
            efc.run_function(leader_fsm::move);
            efc.run_function(leader_fsm::set_walk_anim);
        }
        efc.new_event(LEADER_EVENT_MOVE_END); {
            efc.run_function(leader_fsm::stop);
            efc.run_function(leader_fsm::set_stop_anim);
        }
        efc.new_event(LEADER_EVENT_HOLDING); {
            efc.run_function(leader_fsm::grab_mob);
            efc.change_state("holding");
        }
        efc.new_event(LEADER_EVENT_START_WHISTLE); {
            efc.change_state("whistling");
        }
        efc.new_event(LEADER_EVENT_DISMISS); {
            efc.change_state("dismissing");
        }
        efc.new_event(LEADER_EVENT_SPRAY); {
            efc.change_state("spraying");
        }
        efc.new_event(LEADER_EVENT_LIE_DOWN); {
            efc.run_function(leader_fsm::fall_asleep);
            efc.change_state("sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("dying");
        }
        efc.new_event(LEADER_EVENT_GO_PLUCK); {
            efc.run_function(leader_fsm::go_pluck);
            efc.change_state("going_to_pluck");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("whistling", LEADER_STATE_WHISTLING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::whistle);
        }
        efc.new_event(MOB_EVENT_ON_LEAVE); {
            efc.run_function(leader_fsm::stop_whistle);
        }
        efc.new_event(LEADER_EVENT_STOP_WHISTLE); {
            efc.change_state("active");
        }
        efc.new_event(MOB_EVENT_TIMER); {
            efc.change_state("active");
        }
        efc.new_event(LEADER_EVENT_MOVE_START); {
            efc.run_function(leader_fsm::move);
        }
        efc.new_event(LEADER_EVENT_MOVE_END); {
            efc.run_function(leader_fsm::stop);
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("holding", LEADER_STATE_HOLDING); {
        efc.new_event(LEADER_EVENT_THROW); {
            efc.run_function(leader_fsm::do_throw);
            efc.change_state("active");
        }
        efc.new_event(LEADER_EVENT_RELEASE); {
            efc.run_function(leader_fsm::release);
            efc.change_state("active");
        }
        efc.new_event(LEADER_EVENT_MOVE_START); {
            efc.run_function(leader_fsm::move);
            efc.run_function(leader_fsm::set_walk_anim);
        }
        efc.new_event(LEADER_EVENT_MOVE_END); {
            efc.run_function(leader_fsm::stop);
            efc.run_function(leader_fsm::set_stop_anim);
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::release);
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("dismissing", LEADER_STATE_DISMISSING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::dismiss);
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("active");
        }
        efc.new_event(LEADER_EVENT_MOVE_START); {
            efc.run_function(leader_fsm::move);
        }
        efc.new_event(LEADER_EVENT_MOVE_END); {
            efc.run_function(leader_fsm::stop);
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("dying");
        }
    }
    
    efc.new_state("spraying", LEADER_STATE_SPRAYING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::spray);
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("active");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("dying");
        }
    }
    
    efc.new_state("pain", LEADER_STATE_PAIN); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::suffer_pain);
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("active");
        }
    }
    
    efc.new_state("inactive_pain", LEADER_STATE_INACTIVE_PAIN); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::suffer_pain);
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.run_function(leader_fsm::be_dismissed);
            efc.change_state("idle");
        }
    }
    
    efc.new_state("knocked_back", LEADER_STATE_KNOCKED_BACK); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::get_knocked_back);
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("active");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("inactive_knocked_back", LEADER_STATE_INACTIVE_KNOCKED_BACK); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::get_knocked_back);
        }
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("dying", LEADER_STATE_DYING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::die);
        }
    }
    
    efc.new_state("inactive_dying", LEADER_STATE_INACTIVE_DYING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::inactive_die);
        }
    }
    
    efc.new_state("in_group_chasing", LEADER_STATE_IN_GROUP_CHASING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::chase_leader);
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.change_state("in_group_stopped");
        }
        efc.new_event(MOB_EVENT_DISMISSED); {
            efc.run_function(leader_fsm::be_dismissed);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_GRABBED_BY_FRIEND); {
            efc.run_function(leader_fsm::be_grabbed_by_friend);
            efc.change_state("held_by_leader");
        }
        efc.new_event(LEADER_EVENT_INACTIVE_SEARCH_SEED); {
            efc.run_function(leader_fsm::inactive_search_seed);
        }
        efc.new_event(LEADER_EVENT_GO_PLUCK); {
            efc.run_function(leader_fsm::go_pluck);
            efc.change_state("inactive_going_to_pluck");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::inactive_lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("inactive_dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("in_group_stopped", LEADER_STATE_IN_GROUP_STOPPED); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(leader_fsm::stop_in_group);
        }
        efc.new_event(MOB_EVENT_SPOT_IS_FAR); {
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_DISMISSED); {
            efc.run_function(leader_fsm::be_dismissed);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_GRABBED_BY_FRIEND); {
            efc.run_function(leader_fsm::be_grabbed_by_friend);
            efc.change_state("held_by_leader");
        }
        efc.new_event(LEADER_EVENT_INACTIVE_SEARCH_SEED); {
            efc.run_function(leader_fsm::inactive_search_seed);
        }
        efc.new_event(LEADER_EVENT_GO_PLUCK); {
            efc.run_function(leader_fsm::go_pluck);
            efc.change_state("inactive_going_to_pluck");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::inactive_lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("inactive_dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("going_to_pluck", LEADER_STATE_GOING_TO_PLUCK); {
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.run_function(leader_fsm::start_pluck);
            efc.change_state("plucking");
        }
        efc.new_event(LEADER_EVENT_CANCEL); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.change_state("active");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.change_state("dying");
        }
        efc.new_event(LEADER_EVENT_UNFOCUSED); {
            efc.run_function(leader_fsm::unfocus);
            efc.change_state("inactive_going_to_pluck");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("plucking", LEADER_STATE_PLUCKING); {
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.run_function(leader_fsm::search_seed);
        }
        efc.new_event(LEADER_EVENT_CANCEL); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.change_state("active");
        }
        efc.new_event(LEADER_EVENT_UNFOCUSED); {
            efc.run_function(leader_fsm::unfocus);
            efc.change_state("inactive_plucking");
        }
    }
    
    efc.new_state("inactive_going_to_pluck", LEADER_STATE_INACTIVE_GOING_TO_PLUCK); {
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.run_function(leader_fsm::start_pluck);
            efc.change_state("inactive_plucking");
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.run_function(leader_fsm::lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.change_state("inactive_dying");
        }
        efc.new_event(LEADER_EVENT_FOCUSED); {
            efc.run_function(leader_fsm::focus);
            efc.change_state("going_to_pluck");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("inactive_plucking", LEADER_STATE_INACTIVE_PLUCKING); {
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.run_function(leader_fsm::inactive_search_seed);
        }
        efc.new_event(MOB_EVENT_WHISTLED); {
            efc.run_function(leader_fsm::stop_pluck);
            efc.change_state("in_group_chasing");
        }
        efc.new_event(LEADER_EVENT_FOCUSED); {
            efc.run_function(leader_fsm::focus);
            efc.change_state("plucking");
        }
    }
    
    efc.new_state("sleeping_waiting", LEADER_STATE_SLEEPING_WAITING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(gen_mob_fsm::carry_stop_move);
        }
        efc.new_event(MOB_EVENT_CARRY_KEEP_GOING); {
            efc.run_function(gen_mob_fsm::check_carry_begin);
        }
        efc.new_event(MOB_EVENT_CARRIER_ADDED); {
            efc.run_function(gen_mob_fsm::handle_carrier_added);
            efc.run_function(gen_mob_fsm::check_carry_begin);
        }
        efc.new_event(MOB_EVENT_CARRIER_REMOVED); {
            efc.run_function(gen_mob_fsm::handle_carrier_removed);
        }
        efc.new_event(MOB_EVENT_CARRY_BEGIN_MOVE); {
            efc.change_state("sleeping_moving");
        }
        efc.new_event(LEADER_EVENT_CANCEL); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("waking_up");
        }
        efc.new_event(LEADER_EVENT_UNFOCUSED); {
            efc.run_function(leader_fsm::unfocus);
            efc.change_state("inactive_sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
            efc.run_function(leader_fsm::start_waking_up);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("dying");
        }
    }
    
    efc.new_state("sleeping_moving", LEADER_STATE_SLEEPING_MOVING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(gen_mob_fsm::carry_begin_move);
            efc.run_function(gen_mob_fsm::set_next_target);
        }
        efc.new_event(MOB_EVENT_CARRIER_REMOVED); {
            efc.run_function(gen_mob_fsm::handle_carrier_removed);
            efc.run_function(gen_mob_fsm::check_carry_stop);
        }
        efc.new_event(MOB_EVENT_CARRY_WAIT_UP); {
            efc.change_state("sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_CARRY_STOP_MOVE); {
            efc.change_state("sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_CARRY_BEGIN_MOVE); {
            efc.run_function(gen_mob_fsm::carry_begin_move);
            efc.run_function(gen_mob_fsm::set_next_target);
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.run_function(gen_mob_fsm::set_next_target);
        }
        efc.new_event(MOB_EVENT_CARRY_DELIVERED); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("waking_up");
        }
        efc.new_event(LEADER_EVENT_CANCEL); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("waking_up");
        }
        efc.new_event(LEADER_EVENT_UNFOCUSED); {
            efc.run_function(leader_fsm::unfocus);
            efc.change_state("inactive_sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
            efc.run_function(leader_fsm::start_waking_up);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("inactive_sleeping_waiting", LEADER_STATE_INACTIVE_SLEEPING_WAITING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(gen_mob_fsm::carry_stop_move);
        }
        efc.new_event(MOB_EVENT_CARRY_KEEP_GOING); {
            efc.run_function(gen_mob_fsm::check_carry_begin);
        }
        efc.new_event(MOB_EVENT_CARRIER_ADDED); {
            efc.run_function(gen_mob_fsm::handle_carrier_added);
            efc.run_function(gen_mob_fsm::check_carry_begin);
        }
        efc.new_event(MOB_EVENT_CARRIER_REMOVED); {
            efc.run_function(gen_mob_fsm::handle_carrier_removed);
        }
        efc.new_event(MOB_EVENT_CARRY_BEGIN_MOVE); {
            efc.change_state("inactive_sleeping_moving");
        }
        efc.new_event(LEADER_EVENT_CANCEL); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("inactive_waking_up");
        }
        efc.new_event(LEADER_EVENT_FOCUSED); {
            efc.run_function(leader_fsm::focus);
            efc.change_state("sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
            efc.run_function(leader_fsm::start_waking_up);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("dying");
        }
    }
    
    efc.new_state("inactive_sleeping_moving", LEADER_STATE_INACTIVE_SLEEPING_MOVING); {
        efc.new_event(MOB_EVENT_ON_ENTER); {
            efc.run_function(gen_mob_fsm::carry_begin_move);
            efc.run_function(gen_mob_fsm::set_next_target);
        }
        efc.new_event(MOB_EVENT_CARRIER_REMOVED); {
            efc.run_function(gen_mob_fsm::handle_carrier_removed);
            efc.run_function(gen_mob_fsm::check_carry_stop);
        }
        efc.new_event(MOB_EVENT_CARRY_WAIT_UP); {
            efc.change_state("inactive_sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_CARRY_STOP_MOVE); {
            efc.change_state("inactive_sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_CARRY_BEGIN_MOVE); {
            efc.run_function(gen_mob_fsm::carry_begin_move);
            efc.run_function(gen_mob_fsm::set_next_target);
        }
        efc.new_event(MOB_EVENT_REACHED_DESTINATION); {
            efc.run_function(gen_mob_fsm::set_next_target);
        }
        efc.new_event(MOB_EVENT_CARRY_DELIVERED); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("inactive_waking_up");
        }
        efc.new_event(LEADER_EVENT_CANCEL); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("inactive_waking_up");
        }
        efc.new_event(LEADER_EVENT_FOCUSED); {
            efc.run_function(leader_fsm::focus);
            efc.change_state("sleeping_waiting");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::lose_health);
            efc.run_function(leader_fsm::start_waking_up);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.run_function(leader_fsm::start_waking_up);
            efc.change_state("dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("waking_up", LEADER_STATE_WAKING_UP); {
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("active");
        }
    }
    
    efc.new_state("inactive_waking_up", LEADER_STATE_INACTIVE_WAKING_UP); {
        efc.new_event(MOB_EVENT_ANIMATION_END); {
            efc.change_state("idle");
        }
    }
    
    efc.new_state("held_by_leader", LEADER_STATE_HELD); {
        efc.new_event(MOB_EVENT_ON_LEAVE); {
            efc.run_function(leader_fsm::be_released);
        }
        efc.new_event(MOB_EVENT_THROWN); {
            efc.run_function(leader_fsm::be_thrown);
            efc.change_state("thrown");
        }
        efc.new_event(MOB_EVENT_RELEASED); {
            efc.change_state("in_group_chasing");
        }
        efc.new_event(MOB_EVENT_HITBOX_TOUCH_N_A); {
            efc.run_function(leader_fsm::inactive_lose_health);
        }
        efc.new_event(MOB_EVENT_DEATH); {
            efc.change_state("inactive_dying");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    efc.new_state("thrown", LEADER_STATE_THROWN); {
        efc.new_event(MOB_EVENT_LANDED); {
            efc.run_function(leader_fsm::land);
            efc.change_state("idle");
        }
        efc.new_event(MOB_EVENT_BOTTOMLESS_PIT); {
            efc.run_function(leader_fsm::fall_down_pit);
        }
    }
    
    states = efc.finish();
    first_state_nr = fix_states(states, "idle");
    
    if(states.size() != N_LEADER_STATES) {
        error_log(
            "ENGINE WARNING: Number of leader states on the FSM (" + i2s(states.size()) +
            ") and the enum (" + i2s(N_LEADER_STATES) + ") do not match.");
    }
}

void leader_type::load_from_file(data_node* file, const bool load_resources, vector<pair<size_t, string> >* anim_conversions) {
    pluck_delay = s2f(file->get_child_by_name("pluck_delay")->value);
    whistle_range = s2f(file->get_child_by_name("whistle_range")->get_value_or_default(f2s(DEF_WHISTLE_RANGE)));
    punch_strength = s2i(file->get_child_by_name("punch_strength")->value); //TODO default.
    
    if(load_resources) {
        sfx_dismiss = load_sample(file->get_child_by_name("dismiss_sfx")->value, mixer); //TODO don't use load_sample.
        sfx_name_call = load_sample(file->get_child_by_name("name_call_sfx")->value, mixer); //TODO don't use load_sample.
        sfx_whistle = load_sample(file->get_child_by_name("whistle_sfx")->value, mixer); //TODO don't use load_sample.
        bmp_icon = bitmaps.get(file->get_child_by_name("icon")->value, file);
    }
    
    anim_conversions->push_back(make_pair(LEADER_ANIM_IDLE,         "idle"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_WALK,         "walk"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_PLUCK,        "pluck"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_GET_UP,       "get_up"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_DISMISS,      "dismiss"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_THROW,        "thrown"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_WHISTLING,    "whistling"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_LIE,          "lie"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_PAIN,         "pain"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_KNOCKED_DOWN, "knocked_down"));
    anim_conversions->push_back(make_pair(LEADER_ANIM_SPRAYING,     "spraying"));
    
}