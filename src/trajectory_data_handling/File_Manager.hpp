#include <iostream>
#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include "../data/Trajectory.hpp"

namespace trajectory_data_handling {
   class File_Manager {
   private:
       using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

       static std::filesystem::path TDRIVE_PATH;
       static std::filesystem::path GEOLIFE_PATH;
       static char delimiter;
       static long stringToTime(const std::string& timeString);
   public:
       static void load_tdrive_dataset();
       static void load_geolife_dataset();
   };
}