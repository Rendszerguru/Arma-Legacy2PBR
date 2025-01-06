#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <FreeImage.h>

namespace fs = std::filesystem;

void ensurePBRFolderExists() {
    fs::path pbrFolderPath = fs::current_path() / "PBR_Result";
    if (!fs::exists(pbrFolderPath)) {
        if (!fs::create_directories(pbrFolderPath)) {
            std::cerr << "Failed to create PBR_Result folder!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

FREE_IMAGE_FORMAT getFreeImageFormat(const std::string& filename) {
    return FreeImage_GetFIFFromFilename(filename.c_str());
}

FIBITMAP* loadImage(const std::string& filename) {
    FREE_IMAGE_FORMAT format = getFreeImageFormat(filename);
    if (format == FIF_UNKNOWN) {
        std::cerr << "Unknown image format: " << filename << std::endl;
        return nullptr;
    }
    FIBITMAP* dib = FreeImage_Load(format, filename.c_str());
    if (!dib) {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return nullptr;
    }
    // Check bit depth
    if (FreeImage_GetBPP(dib) < 32) {
        FIBITMAP* converted = FreeImage_ConvertTo32Bits(dib);
        FreeImage_Unload(dib);
        if (!converted) {
            std::cerr << "Failed to convert image to 32-bit: " << filename << std::endl;
            return nullptr;
        }
        dib = converted;
    }
    return dib;
}

std::string getBaseName(const std::string& filename) {
    return fs::path(filename).stem().string();
}

std::vector<std::string> findFilesWithSuffix(const std::string& suffix) {
    std::vector<std::string> result;
    try {
        for (const auto& entry : fs::directory_iterator(fs::current_path() / "TGA_Result")) {
            if (entry.path().extension() == ".tga" && entry.path().stem().string().ends_with(suffix)) {
                result.push_back(entry.path().string());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    return result;
}

bool saveImage(const std::string& baseName, const std::string& suffix, FIBITMAP* dib, const std::vector<std::string>& extensions) {
    fs::path pbrFolderPath = fs::current_path() / "PBR_Result";
    for (const auto& ext : extensions) {
        std::string filename = (pbrFolderPath / (baseName + suffix + ext)).string();
        FREE_IMAGE_FORMAT format = getFreeImageFormat(filename);

        if (format == FIF_UNKNOWN) {
            std::cerr << "Unknown image format: " << filename << std::endl;
            return false;
        }

        int flags = (format == FIF_TARGA) ? TARGA_DEFAULT :
            (format == FIF_TIFF) ? TIFF_NONE :
            (format == FIF_PNG) ? PNG_Z_NO_COMPRESSION : 0;

        if (!FreeImage_Save(format, dib, filename.c_str(), flags)) {
            std::cerr << "Failed to save image: " << filename << std::endl;
            return false;
        }

        std::cout << "Image saved to: " << filename << std::endl;
    }
    return true;
}

int main() {
    FreeImage_Initialise();
    ensurePBRFolderExists();

    std::vector<std::string> nohqFiles = findFilesWithSuffix("_nohq");
    std::vector<std::string> smdiFiles = findFilesWithSuffix("_smdi");
    std::vector<std::string> asFiles = findFilesWithSuffix("_as");
    std::vector<std::string> coFiles = findFilesWithSuffix("_co");

    if (nohqFiles.empty() || smdiFiles.empty() || asFiles.empty() || coFiles.empty()) {
        std::cerr << "Failed to load one or more image sets." << std::endl;
        FreeImage_DeInitialise();
        return -1;
    }

    for (size_t i = 0; i < nohqFiles.size(); ++i) {
        FIBITMAP* nohq = loadImage(nohqFiles[i]);
        FIBITMAP* smdi = loadImage(smdiFiles[i % smdiFiles.size()]);
        FIBITMAP* as = loadImage(asFiles[i % asFiles.size()]);
        FIBITMAP* co = loadImage(coFiles[i % coFiles.size()]);

        if (!nohq || !smdi || !as || !co) {
            std::cerr << "Failed to load or process one or more images." << std::endl;
            FreeImage_Unload(nohq);
            FreeImage_Unload(smdi);
            FreeImage_Unload(as);
            FreeImage_Unload(co);
            FreeImage_DeInitialise();
            return -1;
        }

        unsigned int width = FreeImage_GetWidth(nohq);
        unsigned int height = FreeImage_GetHeight(nohq);

        if (FreeImage_GetBPP(nohq) < 32 || FreeImage_GetBPP(smdi) < 32 ||
            FreeImage_GetBPP(as) < 32 || FreeImage_GetBPP(co) < 32) {
            std::cerr << "One or more images do not have sufficient channels for processing." << std::endl;
            FreeImage_Unload(nohq);
            FreeImage_Unload(smdi);
            FreeImage_Unload(as);
            FreeImage_Unload(co);
            FreeImage_DeInitialise();
            return -1;
        }

        FIBITMAP* nohqPrepared = FreeImage_ConvertTo32Bits(nohq);
        FIBITMAP* smdiPrepared = FreeImage_ConvertTo32Bits(smdi);
        FIBITMAP* asPrepared = FreeImage_ConvertTo32Bits(as);
        FIBITMAP* coPrepared = FreeImage_ConvertTo32Bits(co);

        // Check image dimensions and rescale if necessary
        if (FreeImage_GetWidth(asPrepared) != width || FreeImage_GetHeight(asPrepared) != height) {
            FIBITMAP* resized = FreeImage_Rescale(asPrepared, width, height);
            FreeImage_Unload(asPrepared);
            asPrepared = resized;
        }

        // Create new images for NMO and BCR
        FIBITMAP* nmo = FreeImage_Allocate(width, height, 32);
        FIBITMAP* bcr = FreeImage_Allocate(width, height, 32);

        BYTE* nohqBits = FreeImage_GetBits(nohqPrepared);
        BYTE* smdiBits = FreeImage_GetBits(smdiPrepared);
        BYTE* asBits = FreeImage_GetBits(asPrepared);
        BYTE* coBits = FreeImage_GetBits(coPrepared);
        BYTE* nmoBuffer = FreeImage_GetBits(nmo);
        BYTE* bcrBuffer = FreeImage_GetBits(bcr);

        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                unsigned int index = (x + y * width) * 4;

                nmoBuffer[index + 0] = smdiBits[index + 1];
                nmoBuffer[index + 1] = nohqBits[index + 1];
                nmoBuffer[index + 2] = nohqBits[index + 2];
                nmoBuffer[index + 3] = (asBits[index + 1] + nohqBits[index + 2]) / 2;

                bcrBuffer[index + 0] = coBits[index + 0];
                bcrBuffer[index + 1] = coBits[index + 1];
                bcrBuffer[index + 2] = coBits[index + 2];
                bcrBuffer[index + 3] = smdiBits[index + 0];
            }
        }

        std::string baseName = getBaseName(nohqFiles[i]);
        std::vector<std::string> extensions = { ".tga", ".tif", ".png" };

        saveImage(baseName, "_NMO", nmo, extensions);
        saveImage(baseName, "_BCR", bcr, extensions);

        FreeImage_Unload(nohq);
        FreeImage_Unload(smdi);
        FreeImage_Unload(as);
        FreeImage_Unload(co);
        FreeImage_Unload(nmo);
        FreeImage_Unload(bcr);
    }

    FreeImage_DeInitialise();
    return 0;
}
