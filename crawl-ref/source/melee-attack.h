#pragma once

#include <list>

#include "attack.h"
#include "fight.h"
#include "random-var.h"

enum unarmed_attack_type
{
    UNAT_NO_ATTACK,                    //    0
    UNAT_CONSTRICT,  // put constriction first so octopodes will use it
    UNAT_STAFF,
    UNAT_STAFF_SLAP,
    UNAT_KICK,
    UNAT_HEADBUTT,
    UNAT_PECK,
    UNAT_TAILSLAP,
    UNAT_PUNCH,
    UNAT_BITE,
    UNAT_PSEUDOPODS,
    UNAT_TENTACLES,
    UNAT_TENTACLES2,
    UNAT_TENTACLES3,
    UNAT_TENTACLES4,
    UNAT_TENDRIL1,
    UNAT_TENDRIL2,
    UNAT_FIRST_ATTACK = UNAT_CONSTRICT,
    UNAT_LAST_ATTACK = UNAT_TENDRIL2,
    NUM_UNARMED_ATTACKS,
};

class melee_attack : public attack
{
public:

    // For player:
    // Attack Number 0: Weapon 0.
    // Attack Number 1: Weapon 1
    // Attack Number 2/3: Mount Attacks. Scorpion Form's Bite and Sting.
    // Effective Attack 0: Runs attack delay; doesn't do auxillaries.
    // Effective Attack 1: Does Auxillaries and prints wounds; doesn't run attack delay.
    // Effective Attack 2: Does both Auxillaries, prints wounds and runs attack delay.
    // Effective Attack 3: No attack delay or auxilliaries.
    // Additionally (For a few logic checks):
    // 0 is treated as the first of multiple attacks in the turn
    // 1 is treated as the last of multiple attacks
    // 2 is treated as the only attack
    // 3 gets no special treatment and should be used for the middle of many attacks.

    // For enemies:
    // Attack Number - Which of the 4 mon_spec attacks run.
    // Effective Attack Number - used as a counter of how many times its hit this turn for some purposes.
    int       effective_attack_number;

    list<actor*> cleave_targets;
    bool         cleaving;          // additional attack from cleaving
    bool         is_riposte;        // long blade retaliation attack
    bool         cancel_remaining;  // if we enslave a target or attack with a non-attack cancel any auxillaries.
    wu_jian_attack_type wu_jian_attack;
    int wu_jian_number_of_targets;
    coord_def attack_position;

public:
    melee_attack(actor *attacker, actor *defender,
                 int attack_num = -1, int effective_attack_num = -1,
                 bool is_cleaving = false);

    // Applies attack damage and other effects.
    bool attack();

    // To-hit is a function of attacker/defender, inherited from attack
    int calc_to_hit(bool random = true, bool player_aux = false) override;

    static void chaos_affect_actor(actor *victim, bool md);

private:
    /* Attack phases */
    bool handle_phase_attempted() override;
    bool handle_phase_dodged() override;
    bool handle_phase_hit() override;
    bool handle_phase_damaged() override;
    bool handle_phase_blocked() override;
    bool handle_phase_aux(); // specific to melee attacks
    bool handle_phase_killed() override;
    bool handle_phase_end() override;

    /* Combat Calculations */
    bool using_weapon() const override;
    int weapon_damage() override;
    int calc_mon_to_hit_base() override;
    int apply_damage_modifiers(int damage) override;
    int calc_damage() override;
    bool apply_damage_brand(const char *what = nullptr) override;

    /* Attack effects */
    void check_autoberserk();
    bool check_unrand_effects() override;

    void rot_defender(int amount);

    bool consider_decapitation(int damage_done);
    bool attack_chops_heads(int damage_done, int damage_type);
    void decapitate(int dam_type);

    /* Axe cleaving */
    void cleave_setup();
    int cleave_damage_mod(int dam);

    /* Long blade riposte */
    void riposte(int which_weapon);

    /* Wu Jian martial attacks*/
    int martial_damage_mod(int dam);

    /* Mutation Effects */
    void do_spines();
    void do_passive_freeze();
#if TAG_MAJOR_VERSION == 34
    void do_passive_heat();
#endif
    void emit_foul_stench();
    /* Race Effects */
    void do_minotaur_retaliation();

    /* Brand / Attack Effects */
    bool do_knockback(bool trample = true);

    /* Output methods */
    void set_attack_verb(int damage) override;
    void announce_hit() override;

    /* Misc methods */
    void handle_noise(const coord_def & pos);
private:
    // Monster-attack specific stuff
    bool mons_attack_effects() override;
    void mons_apply_attack_flavour();
    string mons_attack_verb();
    string mons_attack_desc();
    // TODO: Unify do_poison and poison_monster
    bool mons_do_poison();
    void mons_do_napalm();
    void mons_do_eyeball_confusion();
    void mons_do_tendril_disarm();
    void apply_black_mark_effects();
private:
    // Player-attack specific stuff
    // Auxiliary unarmed attacks.
    bool player_aux_unarmed();
    bool player_gets_aux_punch();
    void player_aux_setup(unarmed_attack_type atk);
    bool player_aux_test_hit();
    bool player_aux_apply(unarmed_attack_type atk);

    int  player_apply_misc_modifiers(int damage) override;
    int  player_apply_final_multipliers(int damage) override;

    void player_exercise_combat_skills() override;
    bool player_monattk_hit_effects();
    void attacker_sustain_passive_damage();
    int  staff_damage(skill_type skill, bool max = false);
    bool apply_staff_damage();
    void player_stab_check() override;
    bool player_good_stab() override;
    void player_announce_aux_hit();
    string player_why_missed();
    void player_warn_miss();
    void player_weapon_upsets_god();
    void _defender_die();

    // Added in, were previously static methods of fight.cc
    bool _extra_aux_attack(unarmed_attack_type atk);
    bool _player_vampire_draws_blood(const monster* mon, const int damage,
                                     bool needs_bite_msg = false);
    bool _vamp_wants_blood_from_monster(const monster* mon);

    bool can_reach();
};
