#define TINYOBJLOADER_IMPLEMENTATION

#ifdef VULKAN_BUILD
#define GLFW_INCLUDE_VULKAN
#include "Renderer/vkRenderer.h"
#endif

#include <cstdlib>
#include <filesystem>
#include <string>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Config.h"
#include "Util/Logger.h"
#include "Scene/Track.h"
#include "Loaders/TrackLoader.h"
#include "Loaders/CarLoader.h"
#include "Loaders/MusicLoader.h"
#include "Renderer/Renderer.h"
#include "Race/RaceSession.h"

using namespace std::filesystem;

class OpenNFSEngine {
public:
    explicit OpenNFSEngine(std::shared_ptr<Logger> &onfs_logger) : logger(onfs_logger) {
        if (Config::get().renameAssets) {
            RenameAssetsToLowercase();
        }
        InitDirectories();
        PopulateAssets();

        if (Config::get().vulkanRender) {
#ifdef VULKAN_BUILD
            vkRenderer renderer;
            renderer.run();
#else
            ASSERT(false, "This build of OpenNFS was not compiled with Vulkan support!");
#endif
        } else {
            run();
        }
    }

    void run() {
        LOG(INFO) << "OpenNFS Version " << ONFS_VERSION;

        // Must initialise OpenGL here as the Loaders instantiate meshes which create VAO's
        std::shared_ptr<GLFWwindow> window = Renderer::InitOpenGL(Config::get().resX, Config::get().resY, "OpenNFS v" + ONFS_VERSION);
        AssetData loadedAssets             = {get_enum(Config::get().carTag), Config::get().car, get_enum(Config::get().trackTag), Config::get().track};

        // TODO: TEMP FIX UNTIL I DO A PROPER RETURN from race session
        ASSERT(loadedAssets.trackTag != NFSVersion::UNKNOWN, "Unknown track type!");

        /*------- Render --------*/
        while (loadedAssets.trackTag != NFSVersion::UNKNOWN) {
            /*------ ASSET LOAD ------*/
            // Load Track Data
            auto const &track = TrackLoader::LoadTrack(loadedAssets.trackTag, loadedAssets.track);
            // Load Car data from unpacked NFS files (TODO: Track first (for now), silly dependence on extracted sky texture for car environment map)
            auto const &car = CarLoader::LoadCar(loadedAssets.carTag, loadedAssets.car);

            // Load Music
            // MusicLoader musicLoader("F:\\NFS3\\nfs3_modern_base_eng\\gamedata\\audio\\pc\\atlatech");

            RaceSession race(window, logger, installedNFS, track, car);
            loadedAssets = race.Simulate();
        }

        // Close OpenGL window and terminate GLFW
        glfwTerminate();
    }

private:
    std::shared_ptr<Logger> logger;

    std::vector<NfsAssetList> installedNFS;

    static void InitDirectories() {
        if (!(exists(CAR_PATH))) {
            create_directories(CAR_PATH);
        }
        if (!(exists(TRACK_PATH))) {
            create_directories(TRACK_PATH);
        }
    }

    void PopulateAssets() {
        path basePath(RESOURCE_PATH);
        bool hasMisc = false;
        bool hasUI   = false;

        for (directory_iterator itr(basePath); itr != directory_iterator(); ++itr) {
            NfsAssetList currentNFS;
            currentNFS.tag = NFSVersion::UNKNOWN;

            if (itr->path().filename().string() == get_string(NFSVersion::NFS_2_SE)) {
                currentNFS.tag = NFSVersion::NFS_2_SE;

                std::stringstream trackBasePathStream;
                trackBasePathStream << itr->path().string() << NFS_2_SE_TRACK_PATH;
                std::string trackBasePath(trackBasePathStream.str());
                ASSERT(exists(trackBasePath), "NFS 2 Special Edition track folder: " << trackBasePath << " is missing");

                for (directory_iterator trackItr(trackBasePath); trackItr != directory_iterator(); ++trackItr) {
                    if (trackItr->path().filename().string().find(".trk") != std::string::npos) {
                        currentNFS.tracks.emplace_back(trackItr->path().filename().replace_extension("").string());
                    }
                }

                std::stringstream carBasePathStream;
                carBasePathStream << itr->path().string() << NFS_2_SE_CAR_PATH;
                std::string carBasePath(carBasePathStream.str());
                ASSERT(exists(carBasePath), "NFS 2 Special Edition car folder: " << carBasePath << " is missing");

                // TODO: Work out where NFS2 SE Cars are stored
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_2)) {
                currentNFS.tag = NFSVersion::NFS_2;

                std::stringstream trackBasePathStream;
                trackBasePathStream << itr->path().string() << NFS_2_TRACK_PATH;
                std::string trackBasePath(trackBasePathStream.str());
                ASSERT(exists(trackBasePath), "NFS 2 track folder: " << trackBasePath << " is missing");

                for (directory_iterator trackItr(trackBasePath); trackItr != directory_iterator(); ++trackItr) {
                    if (trackItr->path().filename().string().find(".trk") != std::string::npos) {
                        currentNFS.tracks.emplace_back(trackItr->path().filename().replace_extension("").string());
                    }
                }

                std::stringstream carBasePathStream;
                carBasePathStream << itr->path().string() << NFS_2_CAR_PATH;
                std::string carBasePath(carBasePathStream.str());
                ASSERT(exists(carBasePath), "NFS 2 car folder: " << carBasePath << " is missing");

                for (directory_iterator carItr(carBasePath); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find(".geo") != std::string::npos) {
                        currentNFS.cars.emplace_back(carItr->path().filename().replace_extension("").string());
                    }
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_2_PS1)) {
                currentNFS.tag = NFSVersion::NFS_2_PS1;

                for (directory_iterator trackItr(itr->path().string()); trackItr != directory_iterator(); ++trackItr) {
                    if (trackItr->path().filename().string().find(".trk") != std::string::npos) {
                        currentNFS.tracks.emplace_back(trackItr->path().filename().replace_extension("").string());
                    }
                }

                for (directory_iterator carItr(itr->path().string()); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find(".geo") != std::string::npos) {
                        currentNFS.cars.emplace_back(carItr->path().filename().replace_extension("").string());
                    }
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_3_PS1)) {
                currentNFS.tag = NFSVersion::NFS_3_PS1;

                for (directory_iterator trackItr(itr->path().string()); trackItr != directory_iterator(); ++trackItr) {
                    if (trackItr->path().filename().string().find(".trk") != std::string::npos) {
                        currentNFS.tracks.emplace_back(trackItr->path().filename().replace_extension("").string());
                    }
                }

                for (directory_iterator carItr(itr->path().string()); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find(".geo") != std::string::npos) {
                        currentNFS.cars.emplace_back(carItr->path().filename().replace_extension("").string());
                    }
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_3)) {
                currentNFS.tag = NFSVersion::NFS_3;

                std::string sfxPath = itr->path().string() + "/gamedata/render/pc/sfx.fsh";
                ASSERT(exists(sfxPath), "NFS 3 SFX Resource: " << sfxPath << " is missing");
                ASSERT(ImageLoader::ExtractQFS(sfxPath, RESOURCE_PATH + "sfx/"), "Unable to extract SFX textures from " << sfxPath);

                std::stringstream trackBasePathStream;
                trackBasePathStream << itr->path().string() << NFS_3_TRACK_PATH;
                std::string trackBasePath(trackBasePathStream.str());
                ASSERT(exists(trackBasePath), "NFS 3 Hot Pursuit track folder: " << trackBasePath << " is missing");

                for (directory_iterator trackItr(trackBasePath); trackItr != directory_iterator(); ++trackItr) {
                    currentNFS.tracks.emplace_back(trackItr->path().filename().string());
                }

                std::stringstream carBasePathStream;
                carBasePathStream << itr->path().string() << NFS_3_CAR_PATH;
                std::string carBasePath(carBasePathStream.str());
                ASSERT(exists(carBasePath), "NFS 3 Hot Pursuit car folder: " << carBasePath << " is missing");

                for (directory_iterator carItr(carBasePath); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find("traffic") == std::string::npos) {
                        currentNFS.cars.emplace_back(carItr->path().filename().string());
                    }
                }

                carBasePathStream << "traffic/";
                for (directory_iterator carItr(carBasePathStream.str()); carItr != directory_iterator(); ++carItr) {
                    currentNFS.cars.emplace_back("traffic/" + carItr->path().filename().string());
                }

                carBasePathStream << "pursuit/";
                for (directory_iterator carItr(carBasePathStream.str()); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find("pursuit") == std::string::npos) {
                        currentNFS.cars.emplace_back("traffic/pursuit/" + carItr->path().filename().string());
                    }
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_4_PS1)) {
                currentNFS.tag = NFSVersion::NFS_4_PS1;

                for (directory_iterator dirItr(itr->path().string()); dirItr != directory_iterator(); ++dirItr) {
                    if (dirItr->path().filename().string().find("zzz") == 0 && dirItr->path().filename().string().find(".viv") != std::string::npos) {
                        currentNFS.cars.emplace_back(dirItr->path().filename().replace_extension("").string());
                    } else if (dirItr->path().filename().string().find("ztr") == 0 && dirItr->path().filename().string().find(".grp") != std::string::npos) {
                        currentNFS.tracks.emplace_back(dirItr->path().filename().replace_extension("").string());
                    }
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_4)) {
                currentNFS.tag = NFSVersion::NFS_4;

                std::stringstream trackBasePathStream;
                trackBasePathStream << itr->path().string() << NFS_4_TRACK_PATH;
                std::string trackBasePath(trackBasePathStream.str());
                ASSERT(exists(trackBasePath), "NFS 4 High Stakes track folder: " << trackBasePath << " is missing");

                for (directory_iterator trackItr(trackBasePath); trackItr != directory_iterator(); ++trackItr) {
                    currentNFS.tracks.emplace_back(trackItr->path().filename().string());
                }

                std::stringstream carBasePathStream;
                carBasePathStream << itr->path().string() << NFS_4_CAR_PATH;
                std::string carBasePath(carBasePathStream.str());
                ASSERT(exists(carBasePath), "NFS 4 High Stakes car folder: " << carBasePath << " is missing");

                for (directory_iterator carItr(carBasePath); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find("traffic") == std::string::npos) {
                        currentNFS.cars.emplace_back(carItr->path().filename().string());
                    }
                }

                carBasePathStream << "traffic/";
                for (directory_iterator carItr(carBasePathStream.str()); carItr != directory_iterator(); ++carItr) {
                    if ((carItr->path().filename().string().find("choppers") == std::string::npos) && (carItr->path().filename().string().find("pursuit") == std::string::npos)) {
                        currentNFS.cars.emplace_back("traffic/" + carItr->path().filename().string());
                    }
                }

                carBasePathStream << "choppers/";
                for (directory_iterator carItr(carBasePathStream.str()); carItr != directory_iterator(); ++carItr) {
                    currentNFS.cars.emplace_back("traffic/choppers/" + carItr->path().filename().string());
                }

                carBasePathStream.str(std::string());
                carBasePathStream << itr->path().string() << NFS_4_CAR_PATH << "traffic/"
                                  << "pursuit/";
                for (directory_iterator carItr(carBasePathStream.str()); carItr != directory_iterator(); ++carItr) {
                    currentNFS.cars.emplace_back("traffic/pursuit/" + carItr->path().filename().string());
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::MCO)) {
                currentNFS.tag = NFSVersion::MCO;

                std::string trackBasePath = itr->path().string() + MCO_TRACK_PATH;
                ASSERT(exists(trackBasePath), "Motor City Online track folder: " << trackBasePath << " is missing");

                for (directory_iterator trackItr(trackBasePath); trackItr != directory_iterator(); ++trackItr) {
                    currentNFS.tracks.emplace_back(trackItr->path().filename().string());
                }

                std::string carBasePath = itr->path().string() + MCO_CAR_PATH;
                ASSERT(exists(carBasePath), "Motor City Online car folder: " << carBasePath << " is missing");
                for (directory_iterator carItr(carBasePath); carItr != directory_iterator(); ++carItr) {
                    currentNFS.cars.emplace_back(carItr->path().filename().replace_extension("").string());
                }
            } else if (itr->path().filename().string() == get_string(NFSVersion::NFS_5)) {
                currentNFS.tag = NFSVersion::NFS_5;

                std::stringstream trackBasePathStream;
                trackBasePathStream << itr->path().string() << NFS_5_TRACK_PATH;
                std::string trackBasePath(trackBasePathStream.str());
                ASSERT(exists(trackBasePath), "NFS 5 track folder: " << trackBasePath << " is missing");

                for (directory_iterator trackItr(trackBasePath); trackItr != directory_iterator(); ++trackItr) {
                    if (trackItr->path().filename().string().find(".crp") != std::string::npos) {
                        currentNFS.tracks.emplace_back(trackItr->path().filename().replace_extension("").string());
                    }
                }

                std::stringstream carBasePathStream;
                carBasePathStream << itr->path().string() << NFS_5_CAR_PATH;
                std::string carBasePath(carBasePathStream.str());
                ASSERT(exists(carBasePath), "NFS 5 car folder: " << carBasePath << " is missing");

                for (directory_iterator carItr(carBasePath); carItr != directory_iterator(); ++carItr) {
                    if (carItr->path().filename().string().find(".crp") != std::string::npos) {
                        currentNFS.cars.emplace_back(carItr->path().filename().replace_extension("").string());
                    }
                }
            } else if (itr->path().filename().string() == "misc") {
                hasMisc = true;
                continue;
            } else if (itr->path().filename().string() == "ui") {
                hasUI = true;
                continue;
            } else if (itr->path().filename().string() == "asset") {
                continue;
            } else {
                LOG(WARNING) << "Unknown folder in resources directory: " << itr->path().filename().string();
                continue;
            }
            installedNFS.emplace_back(currentNFS);
        }

        ASSERT(hasMisc, "Missing \'misc\' folder in resources directory");
        ASSERT(hasUI, "Missing \'ui\' folder in resources directory");
        ASSERT(installedNFS.size(), "No Need for Speed games detected in resources directory");

        for (auto nfs : installedNFS) {
            LOG(INFO) << "Detected: " << get_string(nfs.tag);
        }
    }

    static bool FilePathSortByDepthReverse(path a, path b) {
        return (a.string().size() > b.string().size());
    }

    void RenameAssetsToLowercase() {
        LOG(INFO) << "Renaming all available NFS resource files to lowercase for cross-platform ONFS resource load";

        // Get paths of all available NFS versions
        std::vector<path> baseNfsPaths;
        for (directory_iterator itr(RESOURCE_PATH); itr != directory_iterator(); ++itr) {
            // Yucky way of iterating Enum
            for (uint8_t uNfsIdx = 0; uNfsIdx < 11; ++uNfsIdx) {
                NFSVersion version = (NFSVersion) uNfsIdx;
                if (itr->path().filename().string() == get_string(version)) {
                    baseNfsPaths.push_back(itr->path());
                }
            }
        }

        // Perform the renames, store the paths to avoid modifying paths whilst being iterated on
        std::vector<path> originalPaths;
        std::vector<path> lowercasePaths;
        for (auto &baseNfsPath : baseNfsPaths) {
            for (recursive_directory_iterator iter(baseNfsPath), end; iter != end; ++iter) {
                // Convert the filename to lowercase using transform() function and ::tolower in STL, then add it back to path for full, relative path
                path originalPath            = iter->path();
                std::string lowerNfsFileName = originalPath.filename().string();
                transform(lowerNfsFileName.begin(), lowerNfsFileName.end(), lowerNfsFileName.begin(), ::tolower);
                path lowerNfsFilePath(originalPath.parent_path().string() + "/" + lowerNfsFileName);

                // Store the original path and lowercase path for later rename
                if (originalPath != lowerNfsFilePath) {
                    originalPaths.push_back(originalPath);
                    lowercasePaths.push_back(lowerNfsFilePath);
                }
            }
        }
        // Sort the paths by depth, deepest first, so the file references remain valid at base as we traverse and rename
        std::sort(originalPaths.begin(), originalPaths.end(), FilePathSortByDepthReverse);
        std::sort(lowercasePaths.begin(), lowercasePaths.end(), FilePathSortByDepthReverse);

        // Perform the renaming
        for (uint32_t uNfsFileIdx = 0; uNfsFileIdx < originalPaths.size(); ++uNfsFileIdx) {
            rename(originalPaths[uNfsFileIdx], lowercasePaths[uNfsFileIdx]);
            LOG(INFO) << "Renaming " << originalPaths[uNfsFileIdx].string() << " to " << lowercasePaths[uNfsFileIdx].string();
        }

        LOG(INFO) << "Renaming complete on " << originalPaths.size() << " files";
    }
};

int main(int argc, char **argv) {
    Config::get().InitFromCommandLine(argc, argv);
    std::shared_ptr<Logger> logger = std::make_shared<Logger>();

    try {
        OpenNFSEngine game(logger);
    } catch (const std::runtime_error &e) {
        LOG(WARNING) << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
