//
// Created by karma on 2/22/24.
//

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <ranges>
#include <deque>
#include "MRPA.h"

namespace simp_algorithms {
    std::vector<double> MRPA::error_tolerance_init(Trajectory const& traj) {
        std::vector<double> result{};

        auto number_of_tolerances = std::floor(std::log(traj.points.size()) / std::log(resolution_scale));

        for (auto k = 1; k <= number_of_tolerances; k++) {
            // Here we add 1 to the resolution and floor it in order to handle cases where the resolution calculation is exactly an int.
            auto resolution = std::floor(static_cast<double>(traj.points.size()) / std::pow(resolution_scale, k) + 1);
            double error_tolerance{};

            for (auto j = 1; j < resolution; j++) {
                auto range_start = static_cast<int>(std::floor((static_cast<double>(traj.points.size() - 1)) / (resolution - 1) * (j - 1) + 1));
                auto range_end = static_cast<int>(std::floor((static_cast<double>(traj.points.size() - 1)) / (resolution - 1) * j + 1));

                if (j == resolution - 1 && range_end < traj.points.size()) {
                    throw std::range_error("Last range_end variable is not equal to size of trajectory");
                }

                error_tolerance += error_SED_sum(traj, range_start, range_end);
            }

            error_tolerance *= 1 / (resolution - 1);
            result.emplace_back(error_tolerance);
        }

        return result;
    }

    Trajectory MRPA::simplify_subtrajectory(Trajectory const& trajectory, int i, int j) {
        Trajectory res {};

        for (int k = i + 1; k < j; ++k) {
            auto x = trajectory.points[i].x +
                    (trajectory.points[k].t - trajectory.points[i].t) /
                    (trajectory.points[j].t - trajectory.points[i].t) *
                    (trajectory.points[j].x - trajectory.points[i].x);
            auto y = trajectory.points[i].y +
                     (trajectory.points[k].t - trajectory.points[i].t) /
                     (trajectory.points[j].t - trajectory.points[i].t) *
                     (trajectory.points[j].y - trajectory.points[i].y);
            auto point = MRPA::Trajectory::Point(x,y,trajectory.points[k].t);
            res.points.emplace_back(point);
        }

        return res;
    }

    double MRPA::single_SED(Trajectory::Point const& original_point, Trajectory::Point const& approx_point)
    {
        return sqrt(pow(original_point.x - approx_point.x, 2) + pow(original_point.y - approx_point.y, 2));
    }

    double MRPA::error_SED_sum(const simp_algorithms::MRPA::Trajectory& trajectory, int i, int j)
    {
        // Here we subtract one from both i and j, due to 0-indexing
        i = i - 1;
        j = j - 1;
        auto approx_trajectory = simplify_subtrajectory(trajectory, i, j);
        double res {};
        for (int k = i + 1; k < j; ++k) {
            // Here we index the approx trajectory with the difference of k and i+1.
            // This is because the approx trajectory does not have as many points,
            // therefore it does not have values in the same indexes as the original trajectory.
            res += pow(single_SED(trajectory.points[k], approx_trajectory.points[k - (i + 1)]), 2);
        }

        return res;
    }

    Trajectory MRPA::simplify(Trajectory const& trajectory, double const& query_error, double simplification_error) {

    }

    std::vector<Trajectory> MRPA::mrpa(Trajectory const& trajectory) {
        std::vector<Trajectory> result{};
        auto error_tolerances = MRPA::error_tolerance_init(trajectory);

        auto first_tree = init_tree(trajectory, error_tolerances[0], error_tolerances[1]);
        auto first_approx = approximate(trajectory, first_tree, error_tolerances[0]);
        result.emplace_back(first_approx);

        for (int i = 1; i < error_tolerances.size(); ++i) {
            auto tree = apply_error_tolerance_scale_to_tree(result.back(), i, error_tolerances);
            auto approximation = approximate(result.back(), tree, error_tolerances[i]);
            result.emplace_back(approximation);
        }
        
        return result;
    }

    data_structures::Node<Trajectory::Point> MRPA::apply_error_tolerance_scale_to_tree(Trajectory const& trajectory,
                                                                                       int index,
                                                                                       const std::vector<double>& error_tolerances) {
        if (index == error_tolerances.size() - 1) {
            auto scale = error_tolerances[index] / error_tolerances[index - 1];
            return init_tree(trajectory, error_tolerances[index], error_tolerances[index] * scale);
        }
        else {
            return init_tree(trajectory, error_tolerances[index], error_tolerances[index + 1]);
        }
    }

    data_structures::Node<Trajectory::Point> MRPA::init_tree(Trajectory const& trajectory, double error_tol, double high_error_tol) {
        std::deque<Trajectory::Point> working_list{trajectory.points[0]};
        std::deque<Trajectory::Point> future_work{};
        std::vector<Trajectory::Point> unvisited{trajectory.points.begin() + 1, trajectory.points.end()};
        Node root{working_list.front()};

        while (std::ranges::find(working_list, trajectory.points.back()) == working_list.end()) {
            while(!working_list.empty()) {
                maintain_priority_queue(root, trajectory, error_tol, high_error_tol, working_list, future_work, unvisited);
            }

            working_list = std::move(future_work);
            future_work.clear();
        }

        return root;
    }

    void MRPA::maintain_priority_queue(Node& tree, Trajectory const& trajectory, double error_tol, double high_error_tol,
                                       std::deque<Trajectory::Point>& working_list,
                                       std::deque<Trajectory::Point>& future_work,
                                       std::vector<Trajectory::Point>& unvisited) {
        Trajectory::Point index1 = working_list.front();
        working_list.pop_front();

        for (const auto& index2: unvisited) {
            auto error = MRPA::error_SED_sum(trajectory, index1.order, index2.order);
            if (error <= error_tol) {
                unvisited.erase(unvisited.cbegin() + index2.order - 1); // minus one because order starts at 1
                future_work.push_front(index2);
                auto& parent = tree.find(index1);
                auto child = Node(index2);
                parent.children.emplace_back(child);
                child.parent = std::make_shared<Node>(parent);
            }
            else if (error > high_error_tol) {
                break;
            }
        }
        // Returns through side effects in parameters
    }


} // simp_algorithms