/*
 * Copyright (c) Andr� 'Espyo' Silva 2014.
 * The following source file belongs to the open-source project
 * Pikmin fangame engine. Please read the included README file
 * for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Mob class and mob-related functions.
 */

#include <algorithm>

#include "const.h"
#include "functions.h"
#include "mob.h"
#include "pikmin.h"
#include "vars.h"

/* ----------------------------------------------------------------------------
 * Creates a mob.
 */
mob::mob(const float x, const float y, const float z, mob_type* t, sector* sec) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->sec = sec;
    
    type = t;
    anim = animation_instance(&t->anims);
    
    to_delete = false;
    reached_destination = false;
    
    speed_x = speed_y = speed_z = 0;
    home_x = x; home_y = y;
    angle = intended_angle = 0;
    affected_by_gravity = true;
    
    health = t->max_health;
    invuln_period = 0;
    knockdown_period = 0;
    team = MOB_TEAM_NONE;
    
    go_to_target = false;
    gtt_instant = false;
    target_x = x;
    target_y = y;
    target_rel_x = NULL;
    target_rel_y = NULL;
    
    focused_prey = NULL;
    for(unsigned char e = 0; e < N_MOB_EVENTS; e++) events_queued[e] = 0;
    events_queued[MOB_EVENT_SPAWN] = 1;
    
    timer = timer_interval = 0;
    script_wait = 0;
    script_wait_event = NULL;
    dead = false;
    state = MOB_STATE_IDLE;
    time_in_state = 0;
    
    following_party = NULL;
    was_thrown = false;
    unwhistlable_period = 0;
    untouchable_period = 0;
    party = NULL;
    
    carrier_info = NULL;
}

/* ----------------------------------------------------------------------------
 * Makes the mob follow a game tick.
 */
void mob::tick() {
    //Movement.
    bool was_airborne = z > sec->floors[0].z;
    x += delta_t* speed_x;
    y += delta_t* speed_y;
    z += delta_t* speed_z;
    
    if(z <= sec->floors[0].z) {
        z = sec->floors[0].z;
        if(was_airborne) {
            speed_x = 0;
            speed_y = 0;
            speed_z = 0;
            was_thrown = false;
        }
    }
    
    //Gravity.
    if(z > sec->floors[0].z && affected_by_gravity) {
        speed_z += delta_t* (GRAVITY_ADDER);
    }
    
    //Chasing a target.
    if(go_to_target && ((speed_z == 0 && knockdown_period == 0) || gtt_instant)) {
        float final_target_x = target_x, final_target_y = target_y;
        if(target_rel_x) final_target_x += *target_rel_x;
        if(target_rel_y) final_target_y += *target_rel_y;
        
        if(gtt_instant) {
        
            x = final_target_x;
            y = final_target_y;
            speed_x = speed_y = 0;
            
        } else if(x != final_target_x || y != final_target_y) {
            float new_angle = angle;
            move_point(x, y, final_target_x, final_target_y, get_base_speed(), 0.001, &speed_x, &speed_y, &new_angle, &reached_destination);
            if(!reached_destination && state != MOB_STATE_BEING_CARRIED) {
                //Only face the way the mob wants to go if it's still going. Otherwise, let other code turn them whichever way it wants.
                face(new_angle);
            }
        } else reached_destination = true;
    }
    
    //ToDo collisions
    
    //Other things.
    if(unwhistlable_period > 0) {
        unwhistlable_period -= delta_t;
        unwhistlable_period = max(unwhistlable_period, 0);
    }
    if(untouchable_period > 0) {
        untouchable_period -= delta_t;
        untouchable_period = max(untouchable_period, 0);
    }
    
    if(party) {
        float party_center_mx = 0, party_center_my = 0;
        move_point(
            party->party_center_x, party->party_center_y,
            x, y,
            type->move_speed,
            get_leader_to_group_center_dist(this),
            &party_center_mx, &party_center_my, NULL, NULL
        );
        party->party_center_x += party_center_mx * delta_t;
        party->party_center_y += party_center_my * delta_t;
        
        size_t n_members = party->members.size();
        for(size_t m = 0; m < n_members; m++) {
            party->members[m]->face(atan2(y - party->members[m]->y, x - party->members[m]->x));
        }
    }
    
    time_in_state += delta_t;
    
    if(invuln_period > 0) {
        invuln_period -= delta_t;
        invuln_period = max(invuln_period, 0);
    }
    
    if(speed_z == 0) {
        if(knockdown_period > 0) {
            knockdown_period -= delta_t;
            knockdown_period = max(knockdown_period, 0);
        }
    }
    
    
    //Change the facing angle to the angle the mob wants to face.
    if(angle > M_PI)  angle -= M_PI * 2;
    if(angle < -M_PI) angle += M_PI * 2;
    if(intended_angle > M_PI)  intended_angle -= M_PI * 2;
    if(intended_angle < -M_PI) intended_angle += M_PI * 2;
    
    float angle_dif = intended_angle - angle;
    if(angle_dif > M_PI)  angle_dif -= M_PI * 2;
    if(angle_dif < -M_PI) angle_dif += M_PI * 2;
    
    angle += sign(angle_dif) * min(type->rotation_speed * delta_t, fabs(angle_dif));
    
    //Scripts.
    if(
        get_mob_event(this, MOB_EVENT_SEE_PREY, true) ||
        get_mob_event(this, MOB_EVENT_LOSE_PREY, true) ||
        get_mob_event(this, MOB_EVENT_NEAR_PREY, true)
    ) {
        mob* actual_prey = NULL;
        if(focused_prey) if(!focused_prey->dead) actual_prey = focused_prey;
        
        if(actual_prey) {
            float d = dist(x, y, actual_prey->x, actual_prey->y);
            
            //Prey is near.
            if(d <= type->near_radius && script_wait == 0) {
                focused_prey_near = true;
                events_queued[MOB_EVENT_SEE_PREY] = 0;
                events_queued[MOB_EVENT_LOSE_PREY] = 0;
                events_queued[MOB_EVENT_NEAR_PREY] = 2;
            }
            
            //Prey is suddenly out of sight.
            if(d > type->sight_radius) {
                unfocus_mob(this, actual_prey, true);
                
            } else {
            
                //Prey was near, but is now far.
                if(focused_prey_near) {
                    if( d > type->near_radius) {
                        focused_prey_near = false;
                        events_queued[MOB_EVENT_NEAR_PREY] = 0;
                        events_queued[MOB_EVENT_LOSE_PREY] = 0;
                        events_queued[MOB_EVENT_SEE_PREY] = 1;
                    }
                }
            }
            
        } else {
        
            //Find a Pikmin.
            if(!actual_prey) {
                size_t n_pikmin = pikmin_list.size();
                for(size_t p = 0; p < n_pikmin; p++) {
                    pikmin* pik_ptr = pikmin_list[p];
                    if(pik_ptr->dead) continue;
                    
                    float d = dist(x, y, pik_ptr->x, pik_ptr->y);
                    if(d <= type->sight_radius) {
                        focus_mob(this, pik_ptr, d < type->near_radius, true);
                        break;
                    }
                }
            }
            
            if(!focused_prey) {
                //Try the captains now.
                size_t n_leaders = leaders.size();
                for(size_t l = 0; l < n_leaders; l++) {
                    leader* leader_ptr = leaders[l];
                    if(leader_ptr->dead) continue;
                    
                    float d = dist(x, y, leader_ptr->x, leader_ptr->y);
                    if(d <= type->sight_radius) {
                        focus_mob(this, leader_ptr, d < type->near_radius, true);
                        break;
                    }
                }
            }
        }
        
    }
    
    if(get_mob_event(this, MOB_EVENT_TIMER, true)) {
        if(timer > 0 && timer_interval > 0) {
            timer -= delta_t;
            if(timer <= 0) {
                timer = timer_interval;
                events_queued[MOB_EVENT_TIMER] = 1;
            }
        }
    }
    
    if(get_mob_event(this, MOB_EVENT_REACH_HOME, true)) {
        if(reached_destination && target_code == MOB_TARGET_HOME) {
            target_code = MOB_TARGET_NONE;
            events_queued[MOB_EVENT_REACH_HOME] = 1;
        }
    }
    
    if(!dead && health <= 0) {
        dead = true;
        if(get_mob_event(this, MOB_EVENT_DEATH, true)) {
            events_queued[MOB_EVENT_DEATH] = 1;
        }
    }
    
    //Actually run the scripts, if possible.
    bool ran_event = false;
    for(unsigned char e = 0; e < N_MOB_EVENTS; e++) {
        if(events_queued[e] == 1) {
            mob_event* ev_ptr = get_mob_event(this, e);
            if(ev_ptr) {
                ev_ptr->run(this, 0);
                ran_event = true;
                events_queued[e] = 0;
            }
        }
    }
    if(!ran_event) { //Try the low priority ones now.
        for(unsigned char e = 0; e < N_MOB_EVENTS; e++) {
            if(events_queued[e] == 2) {
                mob_event* ev_ptr = get_mob_event(this, e);
                if(ev_ptr) {
                    ev_ptr->run(this, 0);
                    events_queued[e] = 0;
                }
            }
        }
    } else {
        for(unsigned char e = 0; e < N_MOB_EVENTS; e++) {
            if(events_queued[e] == 2) events_queued[e] = 0;
        }
    }
    
    if(script_wait > 0) {
        script_wait -= delta_t;
        if(script_wait <= 0) {
            script_wait = 0;
            
            script_wait_event->run(this, script_wait_action); //Continue the waiting event.
        }
    }
    
    //Animation.
    bool finished_anim = anim.tick(delta_t);
    
    if(script_wait == -1 && finished_anim) { //Waiting for the animation to end.
        script_wait = 0;
        script_wait_event->run(this, script_wait_action); //Continue the waiting event.
    }
}

/* ----------------------------------------------------------------------------
 * Sets a target for the mob to follow.
 * target_*:     Coordinates of the target, relative to either the world origin,
   * or another point, specified in the next parameters.
 * target_rel_*: Pointers to moving coordinates. If NULL, it's the world origin.
   * Use this to make the mob follow another mob wherever they go, for instance.
 * instant:      If true, the mob teleports to that spot, instead of walking to it.
 */
void mob::set_target(float target_x, float target_y, float* target_rel_x, float* target_rel_y, bool instant) {
    this->target_x = target_x; this->target_y = target_y;
    this->target_rel_x = target_rel_x; this->target_rel_y = target_rel_y;
    this->gtt_instant = instant;
    
    go_to_target = true;
    reached_destination = false;
    target_code = MOB_TARGET_NONE;
}

/* ----------------------------------------------------------------------------
 * Makes a mob not follow any target.
 * stop: If true, the mob stops dead on its tracks.
 */
void mob::remove_target(bool stop) {
    go_to_target = false;
    reached_destination = false;
    
    if(stop) {
        speed_x = 0;
        speed_y = 0;
    }
}

/* ----------------------------------------------------------------------------
 * Makes a mob gradually face a new angle.
 */
void mob::face(float new_angle) {
    intended_angle = new_angle;
}

/* ----------------------------------------------------------------------------
 * Changes a mob's state.
 * new_state: The new state ID. Use MOB_STATE_*.
 */
void mob::set_state(unsigned char new_state) {
    state = new_state;
    time_in_state = 0;
}

/* ----------------------------------------------------------------------------
 * Returns the base speed for this mob.
 * This is overwritten by some child classes.
 */
float mob::get_base_speed() {
    return this->type->move_speed;
}

mob::~mob() {}

/* ----------------------------------------------------------------------------
 * Creates a structure with info about carrying.
 * m:             The mob this info belongs to.
 * max_carriers:  The maximum number of carrier Pikmin.
 * carry_to_ship: If true, this mob is delivered to a ship. Otherwise, an Onion.
 */
carrier_info_struct::carrier_info_struct(mob* m, unsigned int max_carriers, bool carry_to_ship) {
    this->max_carriers = max_carriers;
    this->carry_to_ship = carry_to_ship;
    
    current_carrying_strength = 0;
    current_n_carriers = 0;
    decided_type = NULL;
    
    for(size_t c = 0; c < max_carriers; c++) {
        carrier_spots.push_back(NULL);
        float angle = (M_PI * 2) / max_carriers * c;
        carrier_spots_x.push_back(cos(angle) * m->type->size * 0.5);
        carrier_spots_y.push_back(sin(angle) * m->type->size * 0.5);
    }
}

/* ----------------------------------------------------------------------------
 * Deletes a carrier info structure.
 * Makes all carrying Pikmin drop it in the process.
 */
carrier_info_struct::~carrier_info_struct() {
    for(size_t s = 0; s < max_carriers; s++) {
        if(carrier_spots[s]) {
            if(typeid(*carrier_spots[s]) == typeid(pikmin)) {
                drop_mob((pikmin*) carrier_spots[s]);
            }
        }
    }
}

/* ----------------------------------------------------------------------------
 * Adds a mob to another mob's party.
 */
void add_to_party(mob* party_leader, mob* new_member) {
    if(new_member->following_party == party_leader) return; //Already following, never mind.
    
    new_member->following_party = party_leader;
    party_leader->party->members.push_back(new_member);
    
    //Find a spot.
    if(party_leader->party) {
        if(party_leader->party->party_spots) {
            float spot_x = 0, spot_y = 0;
            
            party_leader->party->party_spots->add(new_member, &spot_x, &spot_y);
            
            new_member->set_target(
                spot_x, spot_y,
                &party_leader->party->party_center_x, &party_leader->party->party_center_y,
                false
            );
        }
    }
    
    make_uncarriable(new_member);
}

/* ----------------------------------------------------------------------------
 * Makes m1 attack m2.
 * Stuff like status effects and maturity (Pikmin only) are taken into account.
 */
void attack(mob* m1, mob* m2, const bool m1_is_pikmin, const float damage, const float angle, const float knockback, const float new_invuln_period, const float new_knockdown_period) {
    if(m2->invuln_period > 0) return;
    
    pikmin* p_ptr = NULL;
    float total_damage = damage;
    if(m1_is_pikmin) {
        p_ptr = (pikmin*) m1;
        total_damage += p_ptr->maturity * damage * MATURITY_POWER_MULT;
    }
    
    m2->invuln_period = new_invuln_period;
    m2->knockdown_period = new_knockdown_period;
    m2->health -= damage;
    
    if(knockback != 0) {
        m2->speed_z = 500;
        m2->speed_x = cos(angle) * knockback;
        m2->speed_y = sin(angle) * knockback;
    }
    
    //If before taking damage, the interval was dividable X times, and after it's only dividable by Y (X>Y), an interval was crossed.
    if(m2->type->big_damage_interval > 0 && m2->health != m2->type->max_health) {
        if(floor((m2->health + damage) / m2->type->big_damage_interval) > floor(m2->health / m2->type->big_damage_interval)) {
            if(get_mob_event(m2, MOB_EVENT_BIG_DAMAGE, true)) {
                m2->events_queued[MOB_EVENT_BIG_DAMAGE] = 1;
            }
        }
    }
}

/* ----------------------------------------------------------------------------
 * Creates a mob, adding it to the corresponding vectors.
 */
void create_mob(mob* m) {
    mobs.push_back(m);
    
    if(typeid(*m) == typeid(pikmin)) {
        pikmin_list.push_back((pikmin*) m);
        
    } else if(typeid(*m) == typeid(leader)) {
        leaders.push_back((leader*) m);
        
    } else if(typeid(*m) == typeid(onion)) {
        onions.push_back((onion*) m);
        
    } else if(typeid(*m) == typeid(nectar)) {
        nectars.push_back((nectar*) m);
        
    } else if(typeid(*m) == typeid(pellet)) {
        pellets.push_back((pellet*) m);
        
    } else if(typeid(*m) == typeid(ship)) {
        ships.push_back((ship*) m);
        
    } else if(typeid(*m) == typeid(treasure)) {
        treasures.push_back((treasure*) m);
        
    } else if(typeid(*m) == typeid(info_spot)) {
        info_spots.push_back((info_spot*) m);
        
    } else if(typeid(*m) == typeid(enemy)) {
        enemies.push_back((enemy*) m);
        
    }
}

/* ----------------------------------------------------------------------------
 * Deletes a mob from the relevant vectors.
 * It's always removed from the vector of mobs, but it's
 * also removed from the vector of Pikmin if it's a Pikmin,
 * leaders if it's a leader, etc.
 */
void delete_mob(mob* m) {
    remove_from_party(m);
    vector<mob*> focusers = m->focused_by;
    for(size_t m_nr = 0; m_nr < focusers.size(); m_nr++) {
        unfocus_mob(focusers[m_nr], m, true);
    }
    
    mobs.erase(find(mobs.begin(), mobs.end(), m));
    
    if(typeid(*m) == typeid(pikmin)) {
        pikmin* p_ptr = (pikmin*) m;
        drop_mob(p_ptr);
        pikmin_list.erase(find(pikmin_list.begin(), pikmin_list.end(), p_ptr));
        
    } else if(typeid(*m) == typeid(leader)) {
        leaders.erase(find(leaders.begin(), leaders.end(), (leader*) m));
        
    } else if(typeid(*m) == typeid(onion)) {
        onions.erase(find(onions.begin(), onions.end(), (onion*) m));
        
    } else if(typeid(*m) == typeid(nectar)) {
        nectars.erase(find(nectars.begin(), nectars.end(), (nectar*) m));
        
    } else if(typeid(*m) == typeid(pellet)) {
        pellets.erase(find(pellets.begin(), pellets.end(), (pellet*) m));
        
    } else if(typeid(*m) == typeid(ship)) {
        ships.erase(find(ships.begin(), ships.end(), (ship*) m));
        
    } else if(typeid(*m) == typeid(treasure)) {
        treasures.erase(find(treasures.begin(), treasures.end(), (treasure*) m));
        
    } else if(typeid(*m) == typeid(info_spot)) {
        info_spots.erase(find(info_spots.begin(), info_spots.end(), (info_spot*) m));
        
    } else if(typeid(*m) == typeid(enemy)) {
        enemies.erase(find(enemies.begin(), enemies.end(), (enemy*) m));
        
    } else {
        //ToDo warn somehow.
        
    }
    
    delete m;
}

/* ----------------------------------------------------------------------------
 * Makes m1 focus on m2.
 */
void focus_mob(mob* m1, mob* m2, const bool is_near, const bool call_event) {
    unfocus_mob(m1, m1->focused_prey, false);
    
    m1->focused_prey = m2;
    m1->focused_prey_near = true;
    m2->focused_by.push_back(m1);
    
    if(call_event) {
        m1->focused_prey_near = is_near;
        m1->events_queued[MOB_EVENT_LOSE_PREY] = 0;
        m1->events_queued[MOB_EVENT_NEAR_PREY] = (is_near ? 1 : 0);
        m1->events_queued[MOB_EVENT_SEE_PREY] = (is_near ? 0 : 1);
    }
}

/* ----------------------------------------------------------------------------
 * Returns the closest hitbox to a point, belonging to a mob's current frame of animation and position.
 * x, y: Point.
 * m:    The mob.
 */
hitbox_instance* get_closest_hitbox(const float x, const float y, mob* m) {
    frame* f = m->anim.get_frame();
    if(!f) return NULL;
    hitbox_instance* closest_hitbox = NULL;
    float closest_hitbox_dist = 0;
    
    for(size_t h = 0; h < f->hitbox_instances.size(); h++) {
        hitbox_instance* h_ptr = &f->hitbox_instances[h];
        float hx, hy;
        rotate_point(h_ptr->x, h_ptr->y, m->angle, &hx, &hy);
        float d = dist(x - m->x, y - m->y, hx, hy) - h_ptr->radius;
        if(h == 0 || d < closest_hitbox_dist) {
            closest_hitbox_dist = d;
            closest_hitbox = h_ptr;
        }
    }
    
    return closest_hitbox;
}

/* ----------------------------------------------------------------------------
 * Returns the hitbox instance in the current animation with the specified name.
 */
hitbox_instance* get_hitbox_instance(mob* m, const size_t nr) {
    frame* f = m->anim.get_frame();
    for(size_t h = 0; h < f->hitbox_instances.size(); h++) {
        hitbox_instance* h_ptr = &f->hitbox_instances[h];
        if(h_ptr->hitbox_nr == nr) return h_ptr;
    }
    return NULL;
}

/* ----------------------------------------------------------------------------
 * Makes a mob impossible to be carried, and makes the Pikmin carrying it drop it.
 */
void make_uncarriable(mob* m) {
    if(!m->carrier_info) return;
    
    delete m->carrier_info;
    m->carrier_info = NULL;
}

/* ----------------------------------------------------------------------------
 * Removes a mob from its leader's party.
 */
void remove_from_party(mob* member) {
    if(!member->following_party) return;
    
    member->following_party->party->members.erase(find(
                member->following_party->party->members.begin(),
                member->following_party->party->members.end(),
                member));
                
    if(member->following_party->party->party_spots) {
        member->following_party->party->party_spots->remove(member);
    }
    
    member->following_party = NULL;
    member->remove_target(false);
    member->unwhistlable_period = UNWHISTLABLE_PERIOD;
    member->untouchable_period = UNTOUCHABLE_PERIOD;
}

/* ----------------------------------------------------------------------------
 * Should m1 attack m2? Teams are used to decide this.
 */
bool should_attack(mob* m1, mob* m2) {
    if(m2->team == MOB_TEAM_DECORATION) return false;
    if(m1->team == MOB_TEAM_NONE) return true;
    if(m1->team == m2->team) return false;
    return true;
}

/* ----------------------------------------------------------------------------
 * Makes m1 lose focus on m2.
 */
void unfocus_mob(mob* m1, mob* m2, const bool call_event) {
    if(m2) {
        if(m1->focused_prey != m2) return;
        
        for(size_t m = 0; m < m2->focused_by.size();) {
            if(m2->focused_by[m] == m1) m2->focused_by.erase(m2->focused_by.begin() + m);
            else m++;
        }
    }
    
    m1->focused_prey = NULL;
    m1->focused_prey_near = false;
    if(call_event) {
        m1->events_queued[MOB_EVENT_SEE_PREY] = 0;
        m1->events_queued[MOB_EVENT_NEAR_PREY] = 0;
        m1->events_queued[MOB_EVENT_LOSE_PREY] = 1;
    }
}
