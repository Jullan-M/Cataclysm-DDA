#include "magic.h"

#include "avatar.h"
#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "magic_teleporter_list.h"
#include "translations.h"
#include "ui.h"

#include <set>

namespace
{
const std::map<std::string, valid_target> target_map = {
    { "ally", valid_target::target_ally },
    { "hostile", valid_target::target_hostile },
    { "self", valid_target::target_self },
    { "ground", valid_target::target_ground },
    { "none", valid_target::target_none }
};
const std::map<std::string, body_part> bp_map = {
    { "TORSO", body_part::bp_torso },
    { "HEAD", body_part::bp_head },
    { "EYES", body_part::bp_eyes },
    { "MOUTH", body_part::bp_mouth },
    { "ARM_L", body_part::bp_arm_l },
    { "ARM_R", body_part::bp_arm_r },
    { "HAND_L", body_part::bp_hand_l },
    { "HAND_R", body_part::bp_hand_r },
    { "LEG_L", body_part::bp_leg_l },
    { "LEG_R", body_part::bp_leg_r },
    { "FOOT_L", body_part::bp_foot_l },
    { "FOOT_R", body_part::bp_foot_r }
};
const std::map<std::string, spell_flag> flag_map = {
    { "PERMANENT", spell_flag::PERMANENT },
    { "IGNORE_WALLS", spell_flag::IGNORE_WALLS },
    { "HOSTILE_SUMMON", spell_flag::HOSTILE_SUMMON },
    { "HOSTILE_50", spell_flag::HOSTILE_50 },
    { "SILENT", spell_flag::SILENT },
    { "LOUD", spell_flag::LOUD },
    { "VERBAL", spell_flag::VERBAL },
    { "SOMATIC", spell_flag::SOMATIC },
    { "NO_HANDS", spell_flag::NO_HANDS },
    { "NO_LEGS", spell_flag::NO_LEGS },
    { "CONCENTRATE", spell_flag::CONCENTRATE }
};
} // namespace

namespace io
{
template<>
valid_target string_to_enum<valid_target>( const std::string &trigger )
{
    return string_to_enum_look_up( target_map, trigger );
}
template<>
body_part string_to_enum<body_part>( const std::string &trigger )
{
    return string_to_enum_look_up( bp_map, trigger );
}
template<>
spell_flag string_to_enum<spell_flag>( const std::string &trigger )
{
    return string_to_enum_look_up( flag_map, trigger );
}
} // namespace io

// LOADING
// spell_type

namespace
{
generic_factory<spell_type> spell_factory( "spell" );
} // namespace

template<>
const spell_type &string_id<spell_type>::obj() const
{
    return spell_factory.obj( *this );
}

template<>
bool string_id<spell_type>::is_valid() const
{
    return spell_factory.is_valid( *this );
}

void spell_type::load_spell( JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

static energy_type energy_source_from_string( const std::string &str )
{
    if( str == "MANA" ) {
        return mana_energy;
    } else if( str == "HP" ) {
        return hp_energy;
    } else if( str == "BIONIC" ) {
        return bionic_energy;
    } else if( str == "STAMINA" ) {
        return stamina_energy;
    } else if( str == "FATIGUE" ) {
        return fatigue_energy;
    } else if( str == "NONE" ) {
        return none_energy;
    } else {
        debugmsg( _( "ERROR: Invalid energy string.  Defaulting to NONE" ) );
        return none_energy;
    }
}

static damage_type damage_type_from_string( const std::string &str )
{
    if( str == "fire" ) {
        return DT_HEAT;
    } else if( str == "acid" ) {
        return DT_ACID;
    } else if( str == "bash" ) {
        return DT_BASH;
    } else if( str == "bio" ) {
        return DT_BIOLOGICAL;
    } else if( str == "cold" ) {
        return DT_COLD;
    } else if( str == "cut" ) {
        return DT_CUT;
    } else if( str == "electric" ) {
        return DT_ELECTRIC;
    } else if( str == "stab" ) {
        return DT_STAB;
    } else if( str == "none" || str == "NONE" ) {
        return DT_TRUE;
    } else {
        debugmsg( _( "ERROR: Invalid damage type string.  Defaulting to none" ) );
        return DT_TRUE;
    }
}

void spell_type::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "description", description, translated_string_reader );
    mandatory( jo, was_loaded, "effect", effect );

    const auto trigger_reader = enum_flags_reader<valid_target> { "valid_targets" };
    mandatory( jo, was_loaded, "valid_targets", valid_targets, trigger_reader );

    const auto bp_reader = enum_flags_reader<body_part> { "affected_bps" };
    optional( jo, was_loaded, "affected_body_parts", affected_bps, bp_reader );
    const auto flag_reader = enum_flags_reader<spell_flag> { "flags" };
    optional( jo, was_loaded, "flags", spell_tags, flag_reader );

    optional( jo, was_loaded, "effect_str", effect_str, "" );

    optional( jo, was_loaded, "min_damage", min_damage, 0 );
    optional( jo, was_loaded, "damage_increment", damage_increment, 0.0f );
    optional( jo, was_loaded, "max_damage", max_damage, 0 );

    optional( jo, was_loaded, "min_range", min_range, 0 );
    optional( jo, was_loaded, "range_increment", range_increment, 0.0f );
    optional( jo, was_loaded, "max_range", max_range, 0 );

    optional( jo, was_loaded, "min_aoe", min_aoe, 0 );
    optional( jo, was_loaded, "aoe_increment", aoe_increment, 0.0f );
    optional( jo, was_loaded, "max_aoe", max_aoe, 0 );

    optional( jo, was_loaded, "min_dot", min_dot, 0 );
    optional( jo, was_loaded, "dot_increment", dot_increment, 0.0f );
    optional( jo, was_loaded, "max_dot", max_dot, 0 );

    optional( jo, was_loaded, "min_duration", min_duration, 0 );
    optional( jo, was_loaded, "duration_increment", duration_increment, 0.0f );
    optional( jo, was_loaded, "max_duration", max_duration, 0 );

    optional( jo, was_loaded, "min_pierce", min_pierce, 0 );
    optional( jo, was_loaded, "pierce_increment", pierce_increment, 0.0f );
    optional( jo, was_loaded, "max_pierce", max_pierce, 0 );

    optional( jo, was_loaded, "base_energy_cost", base_energy_cost, 0 );
    optional( jo, was_loaded, "final_energy_cost", final_energy_cost, base_energy_cost );
    optional( jo, was_loaded, "energy_increment", energy_increment, 0.0f );

    std::string temp_string;
    optional( jo, was_loaded, "spell_class", temp_string, "NONE" );
    spell_class = trait_id( temp_string );
    optional( jo, was_loaded, "energy_source", temp_string, "NONE" );
    energy_source = energy_source_from_string( temp_string );
    optional( jo, was_loaded, "damage_type", temp_string, "NONE" );
    dmg_type = damage_type_from_string( temp_string );
    optional( jo, was_loaded, "difficulty", difficulty, 0 );
    optional( jo, was_loaded, "max_level", max_level, 0 );

    optional( jo, was_loaded, "base_casting_time", base_casting_time, 0 );
    optional( jo, was_loaded, "final_casting_time", final_casting_time, base_casting_time );
    optional( jo, was_loaded, "casting_time_increment", casting_time_increment, 0.0f );
}

void spell_type::check_consistency()
{
    for( const spell_type &sp_t : get_all() ) {
        if( sp_t.min_aoe > sp_t.max_aoe ) {
            debugmsg( string_format( "ERROR: %s has higher min_aoe than max_aoe", sp_t.id.c_str() ) );
        }
        if( abs( sp_t.min_damage ) > abs( sp_t.max_damage ) ) {
            debugmsg( string_format( "ERROR: %s has higher min_damage than max_damage", sp_t.id.c_str() ) );
        }
        if( sp_t.min_range > sp_t.max_range ) {
            debugmsg( string_format( "ERROR: %s has higher min_range than max_range", sp_t.id.c_str() ) );
        }
        if( sp_t.min_dot > sp_t.max_dot ) {
            debugmsg( string_format( "ERROR: %s has higher min_dot than max_dot", sp_t.id.c_str() ) );
        }
        if( sp_t.min_duration > sp_t.max_duration ) {
            debugmsg( string_format( "ERROR: %s has higher min_dot_time than max_dot_time", sp_t.id.c_str() ) );
        }
        if( sp_t.min_pierce > sp_t.max_pierce ) {
            debugmsg( string_format( "ERROR: %s has higher min_pierce than max_pierce", sp_t.id.c_str() ) );
        }
        if( sp_t.casting_time_increment < 0.0f && sp_t.base_casting_time < sp_t.final_casting_time ) {
            debugmsg( string_format( "ERROR: %s has negative increment and base_casting_time < final_casting_time",
                                     sp_t.id.c_str() ) );
        }
        if( sp_t.casting_time_increment > 0.0f && sp_t.base_casting_time > sp_t.final_casting_time ) {
            debugmsg( string_format( "ERROR: %s has positive increment and base_casting_time > final_casting_time",
                                     sp_t.id.c_str() ) );
        }
    }
}

const std::vector<spell_type> &spell_type::get_all()
{
    return spell_factory.get_all();
}

void spell_type::reset_all()
{
    spell_factory.reset();
}

bool spell_type::is_valid() const
{
    return spell_factory.is_valid( this->id );
}

// spell

spell::spell( const spell_type *sp, int xp )
{
    type = sp;
    experience = xp;
}

spell::spell( spell_id sp, int xp ) : spell( &sp.obj(), xp ) {}

spell_id spell::id() const
{
    return type->id;
}

trait_id spell::spell_class() const
{
    return type->spell_class;
}

int spell::damage() const
{
    if( type->min_damage >= 0 ) {
        return std::min( static_cast<int>( type->min_damage + round( get_level() *
                                           type->damage_increment ) ), type->max_damage );
    } else { // if it's negative, min and max work differently
        return std::max( static_cast<int>( type->min_damage + round( get_level() *
                                           type->damage_increment ) ), type->max_damage );
    }
}

int spell::aoe() const
{
    return std::min( static_cast<int>( type->min_aoe + round( get_level() * type->aoe_increment ) ),
                     type->max_aoe );
}

int spell::range() const
{
    return std::min( static_cast<int>( type->min_range + round( get_level() * type->range_increment ) ),
                     type->max_range );
}

int spell::duration() const
{
    return std::min( static_cast<int>( type->min_duration + round( get_level() *
                                       type->duration_increment ) ),
                     type->max_duration );
}

time_duration spell::duration_turns() const
{
    return 1_turns * duration() / 100;
}

void spell::gain_level()
{
    gain_exp( exp_to_next_level() );
}

bool spell::is_max_level() const
{
    return get_level() >= type->max_level;
}

bool spell::can_learn( const player &p ) const
{
    if( type->spell_class == trait_id( "NONE" ) ) {
        return true;
    }
    return p.has_trait( type->spell_class );
}

int spell::energy_cost( const player &p ) const
{
    int cost;
    if( type->base_energy_cost < type->final_energy_cost ) {
        cost = std::min( type->final_energy_cost,
                         static_cast<int>( round( type->base_energy_cost + type->energy_increment * get_level() ) ) );
    } else if( type->base_energy_cost > type->final_energy_cost ) {
        cost = std::max( type->final_energy_cost,
                         static_cast<int>( round( type->base_energy_cost + type->energy_increment * get_level() ) ) );
    } else {
        cost = type->base_energy_cost;
    }
    if( !has_flag( spell_flag::NO_HANDS ) ) {
        // the first 10 points of combined encumbrance is ignored, but quickly adds up
        const int hands_encumb = std::max( 0, p.encumb( bp_hand_l ) + p.encumb( bp_hand_r ) - 10 );
        cost += 10 * hands_encumb;
    }
    return cost;
}

bool spell::has_flag( const spell_flag &flag ) const
{
    return type->spell_tags[flag];
}

bool spell::is_spell_class( const trait_id &mid ) const
{
    return mid == type->spell_class;
}

bool spell::can_cast( const player &p ) const
{
    if( !p.magic.knows_spell( type->id ) ) {
        // how in the world can this happen?
        debugmsg( "ERROR: owner of spell does not know spell" );
        return false;
    }
    switch( type->energy_source ) {
        case mana_energy:
            return p.magic.available_mana() >= energy_cost( p );
        case stamina_energy:
            return p.stamina >= energy_cost( p );
        case hp_energy: {
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( energy_cost( p ) < p.hp_cur[i] ) {
                    return true;
                }
            }
            return false;
        }
        case bionic_energy:
            return p.power_level >= energy_cost( p );
        case fatigue_energy:
            return p.get_fatigue() < EXHAUSTED;
        case none_energy:
        default:
            return true;
    }
}

int spell::get_difficulty() const
{
    return type->difficulty;
}

int spell::casting_time( const player &p ) const
{
    // casting time in moves
    int casting_time = 0;
    if( type->base_casting_time < type->final_casting_time ) {
        casting_time = std::min( type->final_casting_time,
                                 static_cast<int>( round( type->base_casting_time + type->casting_time_increment * get_level() ) ) );
    } else if( type->base_casting_time > type->final_casting_time ) {
        casting_time = std::max( type->final_casting_time,
                                 static_cast<int>( round( type->base_casting_time + type->casting_time_increment * get_level() ) ) );
    } else {
        casting_time = type->base_casting_time;
    }
    if( !has_flag( spell_flag::NO_LEGS ) ) {
        // the first 20 points of encumbrance combined is ignored
        const int legs_encumb = std::max( 0, p.encumb( bp_leg_l ) + p.encumb( bp_leg_r ) - 20 );
        casting_time += legs_encumb * 3;
    }
    if( has_flag( spell_flag::SOMATIC ) ) {
        // the first 20 points of encumbrance combined is ignored
        const int arms_encumb = std::max( 0, p.encumb( bp_arm_l ) + p.encumb( bp_arm_r ) - 20 );
        casting_time += arms_encumb * 2;
    }
    return casting_time;
}

std::string spell::name() const
{
    return _( type->name );
}

float spell::spell_fail( const player &p ) const
{
    // formula is based on the following:
    // exponential curve
    // effective skill of 0 or less is 100% failure
    // effective skill of 8 (8 int, 0 spellcraft, 0 spell level, spell difficulty 0) is ~50% failure
    // effective skill of 30 is 0% failure
    const float effective_skill = 2 * ( get_level() - get_difficulty() ) + p.get_int() +
                                  p.get_skill_level( skill_id( "spellcraft" ) );
    // add an if statement in here because sufficiently large numbers will definitely overflow because of exponents
    if( effective_skill > 30.0f ) {
        return 0.0f;
    } else if( effective_skill < 0.0f ) {
        return 1.0f;
    }
    float fail_chance = pow( ( effective_skill - 30.0f ) / 30.0f, 2 );
    if( has_flag( spell_flag::SOMATIC ) ) {
        // the first 20 points of encumbrance combined is ignored
        const int arms_encumb = std::max( 0, p.encumb( bp_arm_l ) + p.encumb( bp_arm_r ) - 20 );
        // each encumbrance point beyond the "gray" color counts as half an additional fail %
        fail_chance += arms_encumb / 200.0f;
    }
    if( has_flag( spell_flag::VERBAL ) ) {
        // a little bit of mouth encumbrance is allowed, but not much
        const int mouth_encumb = std::max( 0, p.encumb( bp_mouth ) - 5 );
        fail_chance += mouth_encumb / 100.0f;
    }
    // concentration spells work better than you'd expect with a higher focus pool
    if( has_flag( spell_flag::CONCENTRATE ) ) {
        if( p.focus_pool <= 0 ) {
            return 0.0f;
        }
        fail_chance /= p.focus_pool / 100.0f;
    }
    return clamp( fail_chance, 0.0f, 1.0f );
}

std::string spell::colorized_fail_percent( const player &p ) const
{
    const float fail_fl = spell_fail( p ) * 100.0f;
    std::string fail_str;
    fail_fl == 100.0f ? fail_str = _( "Too Difficult!" ) : fail_str =  string_format( "%.1f %% %s",
                                   fail_fl, _( "Failure Chance" ) );
    nc_color color;
    if( fail_fl > 90.0f ) {
        color = c_magenta;
    } else if( fail_fl > 75.0f ) {
        color = c_red;
    } else if( fail_fl > 60.0f ) {
        color = c_light_red;
    } else if( fail_fl > 35.0f ) {
        color = c_yellow;
    } else if( fail_fl > 15.0f ) {
        color = c_green;
    } else {
        color = c_light_green;
    }
    return colorize( fail_str, color );
}

int spell::xp() const
{
    return experience;
}

void spell::gain_exp( int nxp )
{
    experience += nxp;
}

std::string spell::energy_string() const
{
    switch( type->energy_source ) {
        case hp_energy:
            return _( "health" );
        case mana_energy:
            return _( "mana" );
        case stamina_energy:
            return _( "stamina" );
        case bionic_energy:
            return _( "bionic power" );
        case fatigue_energy:
            return _( "fatigue" );
        default:
            return "";
    }
}

std::string spell::energy_cost_string( const player &p ) const
{
    if( energy_source() == none_energy ) {
        return _( "none" );
    }
    if( energy_source() == bionic_energy || energy_source() == mana_energy ) {
        return colorize( to_string( energy_cost( p ) ), c_light_blue );
    }
    if( energy_source() == hp_energy ) {
        auto pair = get_hp_bar( energy_cost( p ), p.get_hp_max() / num_hp_parts );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == stamina_energy ) {
        auto pair = get_hp_bar( energy_cost( p ), p.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == fatigue_energy ) {
        return colorize( to_string( energy_cost( p ) ), c_cyan );
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

std::string spell::energy_cur_string( const player &p ) const
{
    if( energy_source() == none_energy ) {
        return _( "infinite" );
    }
    if( energy_source() == bionic_energy ) {
        return colorize( to_string( p.power_level ), c_light_blue );
    }
    if( energy_source() == mana_energy ) {
        return colorize( to_string( p.magic.available_mana() ), c_light_blue );
    }
    if( energy_source() == stamina_energy ) {
        auto pair = get_hp_bar( p.stamina, p.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == hp_energy ) {
        return "";
    }
    if( energy_source() == fatigue_energy ) {
        const std::pair<std::string, nc_color> pair = p.get_fatigue_description();
        return colorize( pair.first, pair.second );
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

bool spell::is_valid() const
{
    if( type == nullptr ) {
        return false;
    }
    return type->is_valid();
}

bool spell::bp_is_affected( body_part bp ) const
{
    return type->affected_bps[bp];
}

void spell::make_sound( const tripoint &target ) const
{
    if( !has_flag( spell_flag::SILENT ) ) {
        int loudness = damage() / 3;
        if( has_flag( spell_flag::LOUD ) ) {
            loudness += 1 + damage() / 3;
        }
        sounds::sound( target, loudness, sounds::sound_t::combat, _( "an explosion" ), false );
    }
}

std::string spell::effect() const
{
    return type->effect;
}

energy_type spell::energy_source() const
{
    return type->energy_source;
}

bool spell::is_valid_target( valid_target t ) const
{
    return type->valid_targets[t];
}

bool spell::is_valid_target( const tripoint &p ) const
{
    bool valid = false;
    if( Creature *const cr = g->critter_at<Creature>( p ) ) {
        Creature::Attitude cr_att = cr->attitude_to( g->u );
        valid = valid || ( cr_att != Creature::A_FRIENDLY && is_valid_target( target_hostile ) ) ||
                ( cr_att == Creature::A_FRIENDLY && is_valid_target( target_ally ) );
    } else {
        valid = is_valid_target( target_ground );
    }
    if( p == g->u.pos() ) {
        valid = valid || is_valid_target( target_self );
    }
    return valid;
}

std::string spell::description() const
{
    return _( type->description );
}

nc_color spell::damage_type_color() const
{
    switch( dmg_type() ) {
        case DT_HEAT:
            return c_red;
        case DT_ACID:
            return c_light_green;
        case DT_BASH:
            return c_magenta;
        case DT_BIOLOGICAL:
            return c_green;
        case DT_COLD:
            return c_white;
        case DT_CUT:
            return c_light_gray;
        case DT_ELECTRIC:
            return c_light_blue;
        case DT_STAB:
            return c_light_red;
        case DT_TRUE:
            return c_dark_gray;
        default:
            return c_black;
    }
}

std::string spell::damage_type_string() const
{
    switch( dmg_type() ) {
        case DT_HEAT:
            return "heat";
        case DT_ACID:
            return "acid";
        case DT_BASH:
            return "bashing";
        case DT_BIOLOGICAL:
            return "biological";
        case DT_COLD:
            return "cold";
        case DT_CUT:
            return "cutting";
        case DT_ELECTRIC:
            return "electric";
        case DT_STAB:
            return "stabbing";
        case DT_TRUE:
            // not *really* force damage
            return "force";
        default:
            return "error";
    }
}

// constants defined below are just for the formula to be used,
// in order for the inverse formula to be equivalent
static const float a = 6200.0;
static const float b = 0.146661;
static const float c = -62.5;

int spell::get_level() const
{
    // you aren't at the next level unless you have the requisite xp, so floor
    return std::max( static_cast<int>( floor( log( experience + a ) / b + c ) ), 0 );
}

int spell::get_max_level() const
{
    return type->max_level;
}

// helper function to calculate xp needed to be at a certain level
// pulled out as a helper function to make it easier to either be used in the future
// or easier to tweak the formula
static int exp_for_level( int level )
{
    // level 0 never needs xp
    if( level == 0 ) {
        return 0;
    }
    return ceil( exp( ( level - c ) * b ) ) - a;
}

int spell::exp_to_next_level() const
{
    return exp_for_level( get_level() + 1 ) - xp();
}

std::string spell::exp_progress() const
{
    const int level = get_level();
    const int this_level_xp = exp_for_level( level );
    const int next_level_xp = exp_for_level( level + 1 );
    const int denominator = next_level_xp - this_level_xp;
    const float progress = static_cast<float>( xp() - this_level_xp ) / std::max( 1.0f,
                           static_cast<float>( denominator ) );
    return string_format( "%i%%", clamp( static_cast<int>( round( progress * 100 ) ), 0, 99 ) );
}

float spell::exp_modifier( const player &p ) const
{
    const float int_modifier = ( p.get_int() - 8.0f ) / 8.0f;
    const float difficulty_modifier = get_difficulty() / 20.0f;
    const float spellcraft_modifier = p.get_skill_level( skill_id( "spellcraft" ) ) / 10.0f;

    return ( int_modifier + difficulty_modifier + spellcraft_modifier ) / 5.0f + 1.0f;
}

int spell::casting_exp( const player &p ) const
{
    // the amount of xp you would get with no modifiers
    const int base_casting_xp = 75;

    return round( p.adjust_for_focus( base_casting_xp * exp_modifier( p ) ) );
}

std::string spell::enumerate_targets() const
{
    std::vector<std::string> all_valid_targets;
    for( const std::pair<std::string, valid_target> pair : target_map ) {
        if( is_valid_target( pair.second ) && pair.second != target_none ) {
            all_valid_targets.emplace_back( pair.first );
        }
    }
    if( all_valid_targets.size() == 1 ) {
        return all_valid_targets[0];
    }
    std::string ret;
    for( auto iter = all_valid_targets.begin(); iter != all_valid_targets.end(); iter++ ) {
        if( iter + 1 == all_valid_targets.end() ) {
            ret = string_format( "%s and %s", ret, *iter );
        } else if( iter == all_valid_targets.begin() ) {
            ret = string_format( "%s", *iter );
        } else {
            ret = string_format( "%s, %s", ret, *iter );
        }
    }
    return ret;
}

damage_type spell::dmg_type() const
{
    return type->dmg_type;
}

damage_instance spell::get_damage_instance() const
{
    damage_instance dmg;
    dmg.add_damage( dmg_type(), damage() );
    return dmg;
}

dealt_damage_instance spell::get_dealt_damage_instance() const
{
    dealt_damage_instance dmg;
    dmg.set_damage( dmg_type(), damage() );
    return dmg;
}

std::string spell::effect_data() const
{
    return type->effect_str;
}

int spell::heal( const tripoint &target ) const
{
    monster *const mon = g->critter_at<monster>( target );
    if( mon ) {
        return mon->heal( -damage() );
    }
    player *const p = g->critter_at<player>( target );
    if( p ) {
        p->healall( -damage() );
        return -damage();
    }
    return -1;
}

// player

known_magic::known_magic()
{
    mana_base = 1000;
    mana = mana_base;
}

void known_magic::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "mana", mana );

    json.member( "spellbook" );
    json.start_array();
    for( auto pair : spellbook ) {
        json.start_object();
        json.member( "id", pair.second.id() );
        json.member( "xp", pair.second.xp() );
        json.end_object();
    }
    json.end_array();

    json.end_object();
}

void known_magic::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "mana", mana );

    JsonArray parray = data.get_array( "spellbook" );
    while( parray.has_more() ) {
        JsonObject jo = parray.next_object();
        std::string id = jo.get_string( "id" );
        spell_id sp = spell_id( id );
        int xp = jo.get_int( "xp" );
        spellbook.emplace( sp, spell( sp, xp ) );
    }
}

bool known_magic::knows_spell( const std::string &sp ) const
{
    return knows_spell( spell_id( sp ) );
}

bool known_magic::knows_spell( const spell_id &sp ) const
{
    return spellbook.count( sp ) == 1;
}

void known_magic::learn_spell( const std::string &sp, player &p, bool force )
{
    learn_spell( spell_id( sp ), p, force );
}

void known_magic::learn_spell( const spell_id &sp, player &p, bool force )
{
    learn_spell( &sp.obj(), p, force );
}

void known_magic::learn_spell( const spell_type *sp, player &p, bool force )
{
    if( !sp->is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    spell temp_spell( sp );
    if( !temp_spell.is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    if( !force && sp->spell_class != trait_id( "NONE" ) ) {
        if( can_learn_spell( p, sp->id ) && !p.has_trait( sp->spell_class ) ) {
            std::string trait_cancel;
            for( const trait_id &cancel : sp->spell_class->cancels ) {
                if( cancel == sp->spell_class->cancels.back() &&
                    sp->spell_class->cancels.back() != sp->spell_class->cancels.front() ) {
                    trait_cancel = string_format( "%s and %s", trait_cancel, cancel->name() );
                } else if( cancel == sp->spell_class->cancels.front() ) {
                    trait_cancel = cancel->name();
                } else {
                    trait_cancel = string_format( "%s, %s", trait_cancel, cancel->name() );
                }
                if( cancel == sp->spell_class->cancels.back() ) {
                    trait_cancel += ".";
                }
            }
            if( query_yn(
                    _( "Learning this spell will make you a %s and lock you out of %s\nContinue?" ),
                    sp->spell_class.obj().name(), trait_cancel ) ) {
                p.set_mutation( sp->spell_class );
                p.add_msg_if_player( sp->spell_class.obj().desc() );
            } else {
                return;
            }
        }
    }
    if( force || can_learn_spell( p, sp->id ) ) {
        spellbook.emplace( sp->id, temp_spell );
        p.add_msg_if_player( m_good, _( "You learned %s!" ), _( sp->name ) );
    } else {
        p.add_msg_if_player( m_bad, _( "You can't learn this spell." ) );
    }
}

void known_magic::forget_spell( const std::string &sp )
{
    forget_spell( spell_id( sp ) );
}

void known_magic::forget_spell( const spell_id &sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "Can't forget a spell you don't know!" );
        return;
    }
    add_msg( m_bad, _( "All knowledge of %s leaves you." ), sp->name );
    spellbook.erase( sp );
}

bool known_magic::can_learn_spell( const player &p, const spell_id &sp ) const
{
    const spell_type sp_t = sp.obj();
    if( sp_t.spell_class == trait_id( "NONE" ) ) {
        return true;
    }
    return !p.has_opposite_trait( sp_t.spell_class );
}

spell &known_magic::get_spell( const spell_id &sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "ERROR: Tried to get unknown spell" );
    }
    spell &temp_spell = spellbook[ sp ];
    return temp_spell;
}

std::vector<spell *> known_magic::get_spells()
{
    std::vector<spell *> spells;
    for( auto &spell_pair : spellbook ) {
        spells.emplace_back( &spell_pair.second );
    }
    return spells;
}

int known_magic::available_mana() const
{
    return mana;
}

void known_magic::set_mana( int new_mana )
{
    mana = new_mana;
}

void known_magic::mod_mana( const player &p, int add_mana )
{
    set_mana( clamp( mana + add_mana, 0, max_mana( p ) ) );
}

int known_magic::max_mana( const player &p ) const
{
    const float int_bonus = ( ( 0.2f + p.get_int() * 0.1f ) - 1.0f ) * mana_base;
    return std::max( 0.0f, ( ( mana_base + int_bonus ) * p.mutation_value( "mana_multiplier" ) ) +
                     p.mutation_value( "mana_modifier" ) - p.power_level );
}

void known_magic::update_mana( const player &p, float turns )
{
    // mana should replenish in 8 hours.
    const float full_replenish = to_turns<float>( 8_hours );
    const float ratio = turns / full_replenish;
    mod_mana( p, floor( ratio * max_mana( p ) * p.mutation_value( "mana_regen_multiplier" ) ) );
}

std::vector<spell_id> known_magic::spells() const
{
    std::vector<spell_id> spell_ids;
    for( auto pair : spellbook ) {
        spell_ids.emplace_back( pair.first );
    }
    return spell_ids;
}

// does the player have enough energy (of the type of the spell) to cast the spell?
bool known_magic::has_enough_energy( const player &p, spell &sp ) const
{
    int cost = sp.energy_cost( p );
    switch( sp.energy_source() ) {
        case mana_energy:
            return available_mana() >= cost;
        case bionic_energy:
            return p.power_level >= cost;
        case stamina_energy:
            return p.stamina >= cost;
        case hp_energy:
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( p.hp_cur[i] > cost ) {
                    return true;
                }
            }
            return false;
        case fatigue_energy:
            return p.get_fatigue() < EXHAUSTED;
        case none_energy:
            return true;
        default:
            return false;
    }
}

int known_magic::time_to_learn_spell( const player &p, const std::string &str ) const
{
    return time_to_learn_spell( p, spell_id( str ) );
}

int known_magic::time_to_learn_spell( const player &p, const spell_id &sp ) const
{
    const int base_time = to_moves<int>( 30_minutes );
    return base_time * ( 1.0 + sp.obj().difficulty / ( 1.0 + ( p.get_int() - 8.0 ) / 8.0 ) +
                         ( p.get_skill_level( skill_id( "spellcraft" ) ) / 10.0 ) );
}

size_t known_magic::get_spellname_max_width()
{
    size_t width = 0;
    for( const spell *sp : get_spells() ) {
        width = std::max( width, sp->name().length() );
    }
    return width;
}

class spellcasting_callback : public uilist_callback
{
    private:
        std::vector<spell *> known_spells;
        void draw_spell_info( const spell &sp, const uilist *menu );
    public:
        bool casting_ignore;

        spellcasting_callback( std::vector<spell *> &spells,
                               bool casting_ignore ) : known_spells( spells ),
            casting_ignore( casting_ignore ) {}
        bool key( const input_context &, const input_event &event, int /*entnum*/,
                  uilist * /*menu*/ ) override {
            if( event.get_first_input() == 'I' ) {
                casting_ignore = !casting_ignore;
                return true;
            }
            return false;
        }

        void select( int entnum, uilist *menu ) override {
            mvwputch( menu->window, 0, menu->w_width - menu->pad_right, c_magenta, LINE_OXXX );
            mvwputch( menu->window, menu->w_height - 1, menu->w_width - menu->pad_right, c_magenta, LINE_XXOX );
            for( int i = 1; i < menu->w_height - 1; i++ ) {
                mvwputch( menu->window, i, menu->w_width - menu->pad_right, c_magenta, LINE_XOXO );
            }
            std::string ignore_string =  casting_ignore ? _( "Ignore Distractions" ) :
                                         _( "Popup Distractions" );
            mvwprintz( menu->window, 0, menu->w_width - menu->pad_right + 2,
                       casting_ignore ? c_red : c_light_green, string_format( "%s %s", "[I]", ignore_string ) );
            draw_spell_info( *known_spells[entnum], menu );
        }
};

static std::string moves_to_string( const int moves )
{
    const int turns = moves / 100;
    if( moves < 200 ) {
        return _( string_format( "%d %s", moves, "moves" ) );
    } else if( moves < to_moves<int>( 2_minutes ) ) {
        return _( string_format( "%d %s", turns, "turns" ) );
    } else if( moves < to_moves<int>( 2_hours ) ) {
        return _( string_format( "%d %s", to_minutes<int>( turns * 1_turns ), "minutes" ) );
    } else {
        return _( string_format( "%d %s", to_hours<int>( turns * 1_turns ), "hours" ) );
    }
}

static bool casting_time_encumbered( const spell &sp, const player &p )
{
    int encumb = 0;
    if( !sp.has_flag( spell_flag::NO_LEGS ) ) {
        // the first 20 points of encumbrance combined is ignored
        encumb += std::max( 0, p.encumb( bp_leg_l ) + p.encumb( bp_leg_r ) - 20 );
    }
    if( sp.has_flag( spell_flag::SOMATIC ) ) {
        // the first 20 points of encumbrance combined is ignored
        encumb += std::max( 0, p.encumb( bp_arm_l ) + p.encumb( bp_arm_r ) - 20 );
    }
    if( encumb > 0 ) {
        return true;
    }
    return false;
}

static bool energy_cost_encumbered( const spell &sp, const player &p )
{
    if( !sp.has_flag( spell_flag::NO_HANDS ) ) {
        return std::max( 0, p.encumb( bp_hand_l ) + p.encumb( bp_hand_r ) - 10 ) > 0;
    }
    return false;
}

// this prints various things about the spell out in a list
// including flags and things like "goes through walls"
static std::string enumerate_spell_data( const spell &sp )
{
    std::vector<std::string> spell_data;
    if( sp.has_flag( spell_flag::CONCENTRATE ) ) {
        spell_data.emplace_back( _( "requires concentration" ) );
    }
    if( sp.has_flag( spell_flag::VERBAL ) ) {
        spell_data.emplace_back( _( "verbal" ) );
    }
    if( sp.has_flag( spell_flag::SOMATIC ) ) {
        spell_data.emplace_back( _( "somatic" ) );
    }
    if( !sp.has_flag( spell_flag::NO_HANDS ) ) {
        spell_data.emplace_back( _( "impeded by gloves" ) );
    } else {
        spell_data.emplace_back( _( "does not require hands" ) );
    }
    if( !sp.has_flag( spell_flag::NO_LEGS ) ) {
        spell_data.emplace_back( _( "requires mobility" ) );
    }
    if( sp.effect() == "target_attack" && sp.range() > 1 ) {
        spell_data.emplace_back( _( "can be cast through walls" ) );
    }
    return enumerate_as_string( spell_data );
}

void spellcasting_callback::draw_spell_info( const spell &sp, const uilist *menu )
{
    const int h_offset = menu->w_width - menu->pad_right + 1;
    // includes spaces on either side for readability
    const int info_width = menu->pad_right - 4;
    const int h_col1 = h_offset + 1;
    const int h_col2 = h_offset + ( info_width / 2 );
    const catacurses::window w_menu = menu->window;
    // various pieces of spell data mean different things depending on the effect of the spell
    const std::string fx = sp.effect();
    int line = 1;
    nc_color gray = c_light_gray;
    nc_color light_green = c_light_green;
    nc_color yellow = c_yellow;

    print_colored_text( w_menu, line++, h_col1, yellow, yellow,
                        sp.spell_class() == trait_id( "NONE" ) ? _( "Classless" ) : sp.spell_class()->name() );

    line += fold_and_print( w_menu, line, h_col1, info_width, gray, sp.description() );

    line++;

    line += fold_and_print( w_menu, line, h_col1, info_width, gray, enumerate_spell_data( sp ) );

    line++;

    print_colored_text( w_menu, line, h_col1, gray, gray,
                        string_format( "%s: %d %s", _( "Spell Level" ), sp.get_level(),
                                       sp.is_max_level() ? _( "(MAX)" ) : "" ) );
    print_colored_text( w_menu, line++, h_col2, gray, gray,
                        string_format( "%s: %d", _( "Max Level" ), sp.get_max_level() ) );

    print_colored_text( w_menu, line, h_col1, gray, gray,
                        sp.colorized_fail_percent( g->u ) );
    print_colored_text( w_menu, line++, h_col2, gray, gray,
                        string_format( "%s: %d", _( "Difficulty" ), sp.get_difficulty() ) );

    print_colored_text( w_menu, line, h_col1, gray, gray,
                        string_format( "%s: %s", _( "to Next Level" ), colorize( to_string( sp.xp() ), light_green ) ) );
    print_colored_text( w_menu, line++, h_col2, gray, gray,
                        string_format( "%s: %s", _( "to Next Level" ), colorize( to_string( sp.exp_to_next_level() ),
                                       light_green ) ) );

    line++;

    const bool cost_encumb = energy_cost_encumbered( sp, g->u );
    print_colored_text( w_menu, line++, h_col1, gray, gray,
                        string_format( "%s: %s %s%s", cost_encumb ? _( "Casting Cost (impeded)" ) : _( "Casting Cost" ),
                                       sp.energy_cost_string( g->u ), sp.energy_string(),
                                       sp.energy_source() == hp_energy ? "" :  string_format( " ( %s current )",
                                               sp.energy_cur_string( g->u ) ) ) );
    const bool c_t_encumb = casting_time_encumbered( sp, g->u );
    print_colored_text( w_menu, line++, h_col1, gray, gray, colorize(
                            string_format( "%s: %s", c_t_encumb ? _( "Casting Time (impeded)" ) : _( "Casting Time" ),
                                           moves_to_string( sp.casting_time( g->u ) ) ),
                            c_t_encumb  ? c_red : c_light_gray ) );

    line++;

    std::string targets = "";
    if( sp.is_valid_target( target_none ) ) {
        targets = "self";
    } else {
        targets = sp.enumerate_targets();
    }
    print_colored_text( w_menu, line++, h_col1, gray, gray,
                        string_format( "%s: %s", _( "Valid Targets" ), _( targets ) ) );

    line++;

    const int damage = sp.damage();
    std::string damage_string;
    std::string aoe_string;
    // if it's any type of attack spell, the stats are normal.
    if( fx == "target_attack" || fx == "projectile_attack" || fx == "cone_attack" ||
        fx == "line_attack" ) {
        if( damage > 0 ) {
            damage_string =  string_format( "%s: %s %s", _( "Damage" ), colorize( to_string( damage ),
                                            sp.damage_type_color() ),
                                            colorize( sp.damage_type_string(), sp.damage_type_color() ) );
        } else if( damage < 0 ) {
            damage_string = string_format( "%s: %s", _( "Healing" ), colorize( "+" + to_string( -damage ),
                                           light_green ) );
        }
        if( sp.aoe() > 0 ) {
            std::string aoe_string_temp = "Spell Radius";
            std::string degree_string = "";
            if( fx == "cone_attack" ) {
                aoe_string_temp = "Cone Arc";
                degree_string = "degrees";
            } else if( fx == "line_attack" ) {
                aoe_string_temp = "Line Width";
            }
            aoe_string = string_format( "%s: %d %s", _( aoe_string_temp ), sp.aoe(), degree_string );
        }
    } else if( fx == "teleport_random" ) {
        if( sp.aoe() > 0 ) {
            aoe_string = string_format( "%s: %d", _( "Variance" ), sp.aoe() );
        }
    } else if( fx == "spawn_item" ) {
        damage_string = string_format( "%s %d %s", _( "Spawn" ), sp.damage(), item::nname( sp.effect_data(),
                                       sp.damage() ) );
    } else if( fx == "summon" ) {
        damage_string = string_format( "%s %d %s", _( "Summon" ), sp.damage(),
                                       _( monster( mtype_id( sp.effect_data() ) ).get_name( ) ) );
        aoe_string =  string_format( "%s: %d", _( "Spell Radius" ), sp.aoe() );
    }

    print_colored_text( w_menu, line, h_col1, gray, gray, damage_string );
    print_colored_text( w_menu, line++, h_col2, gray, gray, aoe_string );

    print_colored_text( w_menu, line++, h_col1, gray, gray,
                        string_format( "%s: %s", _( "Range" ), sp.range() <= 0 ? _( "self" ) : to_string( sp.range() ) ) );

    // todo: damage over time here, when it gets implemeted

    print_colored_text( w_menu, line++, h_col2, gray, gray, sp.duration() <= 0 ? "" :
                        string_format( "%s: %s", _( "Duration" ), moves_to_string( sp.duration() ) ) );
}

int known_magic::get_invlet( const spell_id &sp, std::set<int> &used_invlets )
{
    auto found = invlets.find( sp );
    if( found != invlets.end() ) {
        return found->second;
    }
    for( const std::pair<spell_id, int> &invlet_pair : invlets ) {
        used_invlets.emplace( invlet_pair.second );
    }
    for( int i = 'a'; i <= 'z'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    for( int i = 'A'; i <= 'Z'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    for( int i = '!'; i <= '-'; i++ ) {
        if( used_invlets.count( i ) == 0 ) {
            used_invlets.emplace( i );
            return i;
        }
    }
    return 0;
}

int known_magic::select_spell( const player &p )
{
    // max width of spell names
    const size_t max_spell_name_length = get_spellname_max_width();
    std::vector<spell *> known_spells = get_spells();

    uilist spell_menu;
    spell_menu.w_height = 24;
    spell_menu.w_width = 80;
    spell_menu.w_x = ( TERMX - spell_menu.w_width ) / 2;
    spell_menu.w_y = ( TERMY - spell_menu.w_height ) / 2;
    spell_menu.pad_right = spell_menu.w_width - static_cast<int>( max_spell_name_length ) - 5;
    spell_menu.title = _( "Choose a Spell" );
    spellcasting_callback cb( known_spells, casting_ignore );
    spell_menu.callback = &cb;

    std::set<int> used_invlets;
    used_invlets.emplace( 'I' );

    for( size_t i = 0; i < known_spells.size(); i++ ) {
        spell_menu.addentry( static_cast<int>( i ), known_spells[i]->can_cast( p ),
                             get_invlet( known_spells[i]->id(), used_invlets ), known_spells[i]->name() );
    }

    spell_menu.query();

    casting_ignore = static_cast<spellcasting_callback *>( spell_menu.callback )->casting_ignore;

    return spell_menu.ret;
}

void known_magic::on_mutation_gain( const trait_id &mid, player &p )
{
    for( const std::pair<spell_id, int> &sp : mid->spells_learned ) {
        learn_spell( sp.first, p, true );
        spell &temp_sp = get_spell( sp.first );
        for( int level = 0; level <= sp.second; level++ ) {
            temp_sp.gain_level();
        }
    }
}

void known_magic::on_mutation_loss( const trait_id &mid )
{
    std::vector<spell_id> spells_to_forget;
    for( const spell *sp : get_spells() ) {
        if( sp->is_spell_class( mid ) ) {
            spells_to_forget.emplace_back( sp->id() );
        }
    }
    for( const spell_id &sp_id : spells_to_forget ) {
        forget_spell( sp_id );
    }
}

void spellbook_callback::add_spell( const spell_id &sp )
{
    spells.emplace_back( sp.obj() );
}

static std::string color_number( const int num )
{
    if( num > 0 ) {
        return colorize( to_string( num ), c_light_green );
    } else if( num < 0 ) {
        return colorize( to_string( num ), c_light_red );
    } else {
        return colorize( to_string( num ), c_white );
    }
}

static std::string color_number( const float num )
{
    if( num > 100 ) {
        return colorize( string_format( "+%.0f", num ), c_light_green );
    } else if( num < -100 ) {
        return colorize( string_format( "%.0f", num ), c_light_red );
    } else if( num > 0 ) {
        return colorize( string_format( "+%.2f", num ), c_light_green );
    } else if( num < 0 ) {
        return colorize( string_format( "%.2f", num ), c_light_red );
    } else {
        return colorize( "0", c_white );
    }
}

static void draw_spellbook_info( const spell_type &sp, uilist *menu )
{
    const int width = menu->pad_left - 4;
    const int start_x = 2;
    int line = 1;
    const catacurses::window w = menu->window;
    nc_color gray = c_light_gray;
    nc_color yellow = c_yellow;
    const spell fake_spell( &sp );

    const std::string spell_name = colorize( _( sp.name ), c_light_green );
    const std::string spell_class = sp.spell_class == trait_id( "NONE" ) ? _( "Classless" ) :
                                    sp.spell_class->name();
    print_colored_text( w, line, start_x, gray, gray, spell_name );
    print_colored_text( w, line++, menu->pad_left - spell_class.length() - 1, yellow, yellow,
                        spell_class );
    line++;
    line += fold_and_print( w, line, start_x, width, gray, _( sp.description ) );
    line++;

    mvwprintz( w, line, start_x, c_light_gray, string_format( "%s: %d", _( "Difficulty" ),
               sp.difficulty ) );
    mvwprintz( w, line++, start_x + width / 2, c_light_gray, string_format( "%s: %d", _( "Max Level" ),
               sp.max_level ) );

    const std::string fx = sp.effect;
    std::string damage_string;
    std::string aoe_string;
    bool has_damage_type = false;
    if( fx == "target_attack" || fx == "projectile_attack" || fx == "cone_attack" ||
        fx == "line_attack" ) {
        damage_string = _( "Damage" );
        aoe_string = _( "AoE" );
        has_damage_type = sp.min_damage > 0 && sp.max_damage > 0;
    } else if( fx == "spawn_item" || fx == "summon_monster" ) {
        damage_string = _( "Spawned" );
    } else if( fx == "recover_energy" ) {
        damage_string = _( "Recover" );
    } else if( fx == "teleport_random" ) {
        aoe_string = _( "Variance" );
    }

    if( has_damage_type ) {
        print_colored_text( w, line++, start_x, gray, gray, string_format( "%s: %s", _( "Damage Type" ),
                            colorize( fake_spell.damage_type_string(), fake_spell.damage_type_color() ) ) );
    }
    line++;

    print_colored_text( w, line++, start_x, gray, gray,
                        string_format( "%-10s %-7s %-7s %-7s", _( "Stat Gain" ), _( "lvl 0" ), _( "per lvl" ),
                                       _( "max lvl" ) ) );
    std::vector<std::tuple<std::string, int, float, int>> rows;

    if( sp.max_damage != 0 && sp.min_damage != 0 && !damage_string.empty() ) {
        rows.emplace_back( damage_string, sp.min_damage, sp.damage_increment, sp.max_damage );
    }

    if( sp.max_range != 0 && sp.min_range != 0 ) {
        rows.emplace_back( _( "Range" ), sp.min_range, sp.range_increment, sp.max_range );
    }

    if( sp.min_aoe != 0 && sp.max_aoe != 0 && !aoe_string.empty() ) {
        rows.emplace_back( aoe_string, sp.min_aoe, sp.range_increment, sp.max_aoe );
    }

    if( sp.min_duration != 0 && sp.max_duration != 0 ) {
        rows.emplace_back( _( "Duration" ), sp.min_duration, sp.duration_increment, sp.max_duration );
    }

    rows.emplace_back( _( "Cast Cost" ), sp.base_energy_cost, sp.energy_increment,
                       sp.final_energy_cost );
    rows.emplace_back( _( "Cast Time" ), sp.base_casting_time, sp.casting_time_increment,
                       sp.final_casting_time );

    for( std::tuple<std::string, int, float, int> &row : rows ) {
        mvwprintz( w, line, start_x, c_light_gray, std::get<0>( row ) );
        print_colored_text( w, line, start_x + 11, gray, gray, color_number( std::get<1>( row ) ) );
        print_colored_text( w, line, start_x + 19, gray, gray, color_number( std::get<2>( row ) ) );
        print_colored_text( w, line, start_x + 27, gray, gray, color_number( std::get<3>( row ) ) );
        line++;
    }
}

void spellbook_callback::select( int entnum, uilist *menu )
{
    mvwputch( menu->window, 0, menu->pad_left, c_magenta, LINE_OXXX );
    mvwputch( menu->window, menu->w_height - 1, menu->pad_left, c_magenta, LINE_XXOX );
    for( int i = 1; i < menu->w_height - 1; i++ ) {
        mvwputch( menu->window, i, menu->pad_left, c_magenta, LINE_XOXO );
    }
    draw_spellbook_info( spells[entnum], menu );
}
