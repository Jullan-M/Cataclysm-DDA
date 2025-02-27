#pragma once
#ifndef CREATURE_TRACKER_H
#define CREATURE_TRACKER_H

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "enums.h"
#include "point.h"

class monster;
class JsonIn;
class JsonOut;

class Creature_tracker
{
    public:
        Creature_tracker();
        ~Creature_tracker();
        /**
         * Returns the monster at the given location.
         * If there is no monster, it returns a `nullptr`.
         * Dead monsters are ignored and not returned.
         */
        std::shared_ptr<monster> find( const tripoint &pos ) const;
        /**
         * Returns a temporary id of the given monster (which must exist in the tracker).
         * The id is valid until monsters are added or removed from the tracker.
         * The id remains valid through serializing and deserializing.
         * Use @ref from_temporary_id to get the monster pointer back. (The later may
         * return a nullptr if the given id is not valid.)
         */
        int temporary_id( const monster &critter ) const;
        std::shared_ptr<monster> from_temporary_id( int id );
        /** Adds the given monster to the creature_tracker. Returns whether the operation was successful. */
        bool add( monster &critter );
        size_t size() const;
        /** Updates the position of the given monster to the given point. Returns whether the operation
         *  was successful. */
        bool update_pos( const monster &critter, const tripoint &new_pos );
        /** Removes the given monster from the Creature tracker, adjusting other entries as needed. */
        void remove( const monster &critter );
        void clear();
        void rebuild_cache();
        /** Swaps the positions of two monsters */
        void swap_positions( monster &first, monster &second );
        /** Kills 0 hp monsters. Returns if it killed any. */
        bool kill_marked_for_death();
        /** Removes dead monsters from. Their pointers are invalidated. */
        void remove_dead();

        const std::vector<std::shared_ptr<monster>> &get_monsters_list() const {
            return monsters_list;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

    private:
        std::vector<std::shared_ptr<monster>> monsters_list;
        std::unordered_map<tripoint, std::shared_ptr<monster>> monsters_by_location;
        /** Remove the monsters entry in @ref monsters_by_location */
        void remove_from_location_map( const monster &critter );
};

#endif
