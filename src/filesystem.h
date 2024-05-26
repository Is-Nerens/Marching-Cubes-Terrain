#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include <iostream>
#include <filesystem> 
namespace fs = std::filesystem;

std::vector<std::string> GetFilepathsInFolder(std::string folderpath)
{
    std::vector<std::string> filenames;

    // check if the folder exists
    if (!fs::exists(folderpath)) {
        std::cerr << "[PrintFiles] Folder does not exist: " << folderpath << std::endl;
    }

    for (const auto& entry : fs::directory_iterator(folderpath))
    {
        if (fs::is_regular_file(entry.path()))
        {
            std::ifstream file(entry.path());
            {
                if (file.is_open())
                {
                    filenames.push_back(entry.path().string());
                    file.close();
                }
                else 
                {
                    std::cerr << "Failed to open file: " << entry.path() << std::endl;
                }
            }
        }
    }
    return filenames;
}

#endif