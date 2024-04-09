#include <iostream>
#include <fstream>
#include <pqxx/pqxx>
#include "trajectory_sql.hpp"

namespace trajectory_data_handling {

    void trajectory_manager::insert_trajectories(
            std::vector<data_structures::Trajectory> const& all_trajectories, db_table table) const {
        std::string table_name{};
        switch(table) {
            case db_table::original_trajectories:
                table_name = "original_trajectories";
                break;
            case db_table::simplified_trajectories:
                table_name = "simplified_trajectories";
                break;
        }

        pqxx::connection c{connection_string};
        pqxx::work txn{c};

        for (const auto& trajectory : all_trajectories) {
            std::stringstream first_query{};
            first_query << "INSERT INTO " << table_name << "(trajectory_id, coordinates, time) " << " VALUES("
                  << trajectory.id << ", point(" << std::to_string(trajectory[0].longitude) << ", " << std::to_string(trajectory[0].latitude) << "), "
                  << std::to_string(trajectory[0].timestamp) << ");";

            txn.exec0(first_query.str());
            for (int i = 1; i < trajectory.size(); i++) {
                if(!(trajectory[i] == trajectory[i-1])) {
                    std::stringstream query{};
                    query << "INSERT INTO " << table_name << "(trajectory_id, coordinates, time) " << " VALUES("
                          << trajectory.id << ", point(" << std::to_string(trajectory[i].longitude) << ", " << std::to_string(trajectory[i].latitude) << "), "
                          << std::to_string(trajectory[i].timestamp) << ");";

                    txn.exec0(query.str());
                }
            }
        }

        txn.commit();
    }

    void trajectory_manager::load_into_data_structure(query_purpose purpose, std::vector<std::string> const& id) {
        std::string table_name{};
        std::stringstream query{};

        switch(purpose) {
            case query_purpose::load_original_trajectory_information_into_data_structure:
                table_name = "trajectory_information";
                break;
            case query_purpose::load_simplified_trajectory_information_into_data_structure:
                table_name = "simplified_trajectory_information";
                break;

            default:
                std::cout << "Error in switch statement in load_into_data_structure." << '\n';
        }

        if(id.empty()) {
            query << "SELECT trajectory_id, coordinates, time FROM " << table_name << ";";
        }
        else {
            query << "SELECT trajectory_id, coordinates, time FROM " << table_name << " WHERE trajectory_id IN (";
            for(auto i = id.begin(); i != id.end(); ++i) {
                query << *i;
                if (std::next(i) != id.end()) {
                    query << ",";
                }
            }
            query << ");";
        }
        trajectory_data_handling::query_handler::run_sql(query.str(), purpose);
    }

    void trajectory_manager::remove_from_trajectories(
            std::shared_ptr<std::vector<data_structures::Trajectory>> &trajectories,
            spatial_queries::Range_Query::Window window) {
        for(int i = 0; i < trajectories->size(); ) {
            auto &trajectory = (*trajectories)[i];
            if (trajectory.id != 0) {
                auto keep_trajectory = spatial_queries::Range_Query::in_range(trajectory, window);
                if (!keep_trajectory) {
                    trajectories->erase(trajectories->begin() + i);
                } else {
                    ++i;
                }
            } else {
                ++i;
            }
        }
    }

    void trajectory_manager::spatial_range_query_on_rtree_table(query_purpose purpose, spatial_queries::Range_Query::Window window) {
        std::string table_name{};
        std::stringstream query{};
        trajectory_data_handling::query_handler::original_trajectories = std::make_shared<std::vector<data_structures::Trajectory>>();
        trajectory_data_handling::query_handler::simplified_trajectories = std::make_shared<std::vector<data_structures::Trajectory>>();

        switch(purpose) {
            case query_purpose::load_original_rtree_into_datastructure:
                table_name = "trajectory_rtree";
                break;
            case query_purpose::load_simplified_rtree_into_datastructure:
                table_name = "simplified_trajectory_rtree";
                break;

            default:
                std::cout << "Error in switch statement in spatial_query_on_rtree_table." << '\n';
        }

        query << "SELECT id FROM " << table_name << " WHERE minLongitude<=" << window.x_high << " AND maxLongitude>="
        << window.x_low << " AND minLatitude<=" << window.y_high << " AND maxLatitude>=" << window.y_low
        << " AND minTimestamp<=" << window.t_high << " AND maxTimestamp>=" << window.t_low << ";";
        trajectory_data_handling::query_handler::run_sql(query.str().c_str(), purpose);


        if (!trajectory_data_handling::query_handler::trajectory_ids_in_range.empty()){
            switch(purpose) {
                case query_purpose::load_original_rtree_into_datastructure:
                    load_into_data_structure(query_purpose::load_original_trajectory_information_into_data_structure,
                                             trajectory_data_handling::query_handler::trajectory_ids_in_range);
                    remove_from_trajectories(trajectory_data_handling::query_handler::original_trajectories, window);
                    break;
                case query_purpose::load_simplified_rtree_into_datastructure:
                    load_into_data_structure(query_purpose::load_simplified_trajectory_information_into_data_structure,
                                             trajectory_data_handling::query_handler::trajectory_ids_in_range);
                    remove_from_trajectories(trajectory_data_handling::query_handler::simplified_trajectories, window);
                    break;
            }
        }
    }

    void trajectory_manager::print_trajectories(std::vector<data_structures::Trajectory> &all_trajectories) {
        for (const auto & trajectory : all_trajectories) {
            std::cout << "id: " << trajectory.id << std::endl;
            for (const auto &location: trajectory.locations) {
                std::cout << "order: " << location.order << '\n';
                std::cout << "timestamp: " << std::fixed <<  location.timestamp << '\n';
                std::cout << "longitude, latitude: " << location.longitude << " " << location.latitude << '\n';
            }
        }
    }


    void trajectory_manager::create_database() const {
        pqxx::connection c{connection_string};
        pqxx::work txn{c};

        add_query_file_to_transaction("../../sql/create_table_original.sql", txn);
        add_query_file_to_transaction("../../sql/create_index_original.sql", txn);
        add_query_file_to_transaction("../../sql/create_table_simplified.sql", txn);
        add_query_file_to_transaction("../../sql/create_index_simplified.sql", txn);

        txn.commit();
    }

    void trajectory_manager::reset_all_data() const {
        pqxx::connection c{connection_string};
        pqxx::work txn{c};
        txn.exec0("DROP INDEX original_trajectories_index;");
        txn.exec0("DROP TABLE original_trajectories;");
        txn.exec0("DROP INDEX simplified_trajectories_index;");
        txn.exec0("DROP TABLE simplified_trajectories;");

        txn.commit();
        create_database();
    }

    void trajectory_manager::replace_trajectory(data_structures::Trajectory const& trajectory) {

    }

    void trajectory_manager::add_query_file_to_transaction(std::string const& query_file_path,
                                                           pqxx::work &transaction) {
        std::ifstream ifs{query_file_path};
        std::string query(std::istreambuf_iterator<char>{ifs}, {});
        transaction.exec0(query);
    }
}







