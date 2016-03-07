/*
 * Copyright (c) Andre 'Espyo' Silva 2013-2016.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included
 * README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the leader class and leader-related functions.
 */

#ifndef LEADER_INCLUDED
#define LEADER_INCLUDED

#include <vector>

class pikmin;

#include "../const.h"
#include "leader_type.h"
#include "mob.h"
#include "../misc_structs.h"
#include "pikmin.h"
#include "../sector.h"

using namespace std;

/* ----------------------------------------------------------------------------
 * A leader controls Pikmin, and
 * is controlled by the player.
 */
class leader : public mob {
public:
    leader_type* lea_type;
    
    mob* holding_pikmin;
    
    pikmin* auto_pluck_pikmin; //-1 = not plucking.
    
    bool is_in_walking_anim;
    
    leader(const float x, const float y, leader_type* type, const float angle, const string &vars);
    
    virtual void draw();
    
    void signal_group_move_start();
    void signal_group_move_end();
    
};



void dismiss();
float get_leader_to_group_center_dist(mob* l);
void swap_pikmin(mob* new_pik);
void switch_to_leader(leader* new_leader_ptr);



enum LEADER_STATES {
    LEADER_STATE_IDLE,
    LEADER_STATE_ACTIVE,
    LEADER_STATE_WHISTLING,
    LEADER_STATE_HOLDING,
    LEADER_STATE_DISMISSING,
    LEADER_STATE_SPRAYING,
    LEADER_STATE_PAIN,
    LEADER_STATE_INACTIVE_PAIN,
    LEADER_STATE_KNOCKED_BACK,
    LEADER_STATE_INACTIVE_KNOCKED_BACK,
    LEADER_STATE_DYING,
    LEADER_STATE_INACTIVE_DYING,
    LEADER_STATE_IN_GROUP_CHASING,
    LEADER_STATE_IN_GROUP_STOPPED,
    LEADER_STATE_GOING_TO_PLUCK,
    LEADER_STATE_PLUCKING,
    LEADER_STATE_INACTIVE_GOING_TO_PLUCK,
    LEADER_STATE_INACTIVE_PLUCKING,
    LEADER_STATE_SLEEPING_WAITING,
    LEADER_STATE_SLEEPING_MOVING,
    LEADER_STATE_INACTIVE_SLEEPING_WAITING,
    LEADER_STATE_INACTIVE_SLEEPING_MOVING,
    LEADER_STATE_WAKING_UP, //Time during which the leader is getting up.
    LEADER_STATE_INACTIVE_WAKING_UP, //Time during which the leader is getting up.
    LEADER_STATE_HELD,
    LEADER_STATE_THROWN,
    
    N_LEADER_STATES,
    
};

enum LEADER_ANIMATIONS {
    LEADER_ANIM_IDLE,
    LEADER_ANIM_WALK,
    LEADER_ANIM_PLUCK,
    LEADER_ANIM_GET_UP,
    LEADER_ANIM_DISMISS,
    LEADER_ANIM_THROW,
    LEADER_ANIM_WHISTLING,
    LEADER_ANIM_LIE,
    LEADER_ANIM_PAIN,
    LEADER_ANIM_KNOCKED_DOWN,
    LEADER_ANIM_SPRAYING,
};

const float LEADER_INVULN_PERIOD = 1.5f;
const float LEADER_ZAP_ANIM_PARTS = 20;

#endif //ifndef LEADER_INCLUDED