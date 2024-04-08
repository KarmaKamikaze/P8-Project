#ifndef P8_PROJECT_TRAJECTORY_H
#define P8_PROJECT_TRAJECTORY_H

#include <vector>
#include <sqlite3.h>
#include <tuple>
#include "../data/trajectory_structure.hpp"
#include "sqlite_querying.hpp"


namespace trajectory_data_handling {
    enum db_table {
        original_trajectories,
        simplified_trajectories
    };

    class trajectory_manager {
    public:
        static void load_trajectories_into_rtree(query_purpose rtree_table);
        static void insert_trajectories_into_trajectory_table(std::vector<data_structures::Trajectory> &all_trajectories, db_table table);
        static void spatial_range_query_on_rtree_table(query_purpose purpose, std::tuple<float, float> longitudeRange, std::tuple<float, float> latitudeRange, std::tuple<float, float> timestampRange);
        static void load_database_into_datastructure(query_purpose purpose, std::vector<std::string> const& id);
        static void print_trajectories(std::vector<data_structures::Trajectory> &all_trajectories);
        static void create_database();
        static void create_rtree_table();
        static void reset_all_data();
    };
}

#endif
