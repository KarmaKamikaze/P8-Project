#ifndef P8_PROJECT_TRAJECTORY_H
#define P8_PROJECT_TRAJECTORY_H

#include <vector>
#include <sqlite3.h>
#include <pqxx/pqxx>
#include "../data/Trajectory.hpp"
#include "Query_Handler.hpp"
#include "../querying/Range_Query.hpp"


namespace trajectory_data_handling {
    enum class db_table {
        original_trajectories,
        simplified_trajectories
    };

    class Trajectory_Manager {
    public:
        static void insert_trajectory(data_structures::Trajectory const& trajectory, db_table table);
        static void remove_from_trajectories(std::shared_ptr<std::vector<data_structures::Trajectory>> const& trajectories,
                                             spatial_queries::Range_Query::Window const& window);
        static void spatial_range_query_on_rtree_table(query_purpose purpose, spatial_queries::Range_Query::Window const& window);
        static void load_into_data_structure(query_purpose purpose, std::vector<std::string> const& id);
        static void print_trajectories(std::vector<data_structures::Trajectory> const& all_trajectories);
        static void create_database();
        static void reset_all_data();
        static void replace_trajectory(data_structures::Trajectory const& trajectory);
    private:
        static void add_query_file_to_transaction(std::string const& query_file_path, pqxx::work &transaction);
        static std::string connection_string;
    };
}

#endif