/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the enemy class and enemy-related functions.
 */

#ifndef ENEMY_INCLUDED
#define ENEMY_INCLUDED

#include "../mob_types/enemy_type.h"
#include "mob.h"

/* ----------------------------------------------------------------------------
 * I don't need to explain what an enemy is.
 */
class enemy : public mob {
public:
    //What type of enemy it is.
    enemy_type* ene_type;
    
    //Specific spoils that this enemy drops on death.
    vector<mob_type*> specific_spoils;
    //List of pellets it drops of random types. This is a list of pellet sizes.
    vector<size_t> random_pellet_spoils;
    
    //Constructor.
    enemy(const point &pos, enemy_type* type, const float angle);
    
    //Can the mob currently receive the specified status effect?
    virtual bool can_receive_status(status_type* s);
    //Mob drawing routine.
    virtual void draw_mob(bitmap_effect_manager* effect_manager = NULL);
    //Read script variables from the area data.
    virtual void read_script_vars(const string &vars);
    //Death class-specific logic.
    virtual void start_dying_class_specific();
};

#endif //ifndef ENEMY_INCLUDED
