#include <FreeImage.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

FREE_IMAGE_FORMAT getFreeImageFormat(const std::string& filename) {
    std::string extension = fs::path(filename).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension == ".tga") {
        return FIF_TARGA;
    }
    else if (extension == ".tif") {
        return FIF_TIFF;
    }
    else if (extension == ".png") {
        return FIF_PNG;
    }
    return FIF_UNKNOWN;
}

FIBITMAP* loadImage(const std::string& filename) {
    FREE_IMAGE_FORMAT format = getFreeImageFormat(filename);
    if (format == FIF_UNKNOWN) {
        std::cerr << "Unknown image format: " << filename << std::endl;
        return nullptr;
    }

    FIBITMAP* dib = FreeImage_Load(format, filename.c_str(), 0);
    if (!dib) {
        std::cerr << "Failed to load image: " << filename << std::endl;
    }
    return dib;
}

bool saveImage(const std::string& filename, FIBITMAP* dib) {
    FREE_IMAGE_FORMAT format = getFreeImageFormat(filename);
    if (format == FIF_UNKNOWN) {
        std::cerr << "Unknown image format: " << filename << std::endl;
        return false;
    }

    int flags = 0;
    if (format == FIF_TARGA) {
        // TGA compression settings
        // flags = TARGA_LOAD_RGB888; // 24-bit RGB format
        // flags = TARGA_LOAD_ARGB8888; // 32-bit ARGB format
        flags = TARGA_DEFAULT; // Default settings
        // flags = TARGA_SAVE_RLE; // Apply RLE compression
    }
    else if (format == FIF_TIFF) {
        // TIFF compression settings
        // flags = TIFF_DEFLATE; // Use Deflate compression
        flags = TIFF_NONE; // No compression
        // flags = TIFF_LZW; // LZW compression (default)
        // flags = TIFF_CCITTFAX3; // CCITT Group 3 compression (for black and white images)
        // flags = TIFF_CCITTFAX4; // CCITT Group 4 compression (for black and white images)
        // flags = TIFF_JPEG; // JPEG compression (lossy)
    }
    else if (format == FIF_PNG) {
        // PNG compression settings
        // flags = PNG_Z_BEST_COMPRESSION; // Best compression ratio
        // flags = PNG_Z_DEFAULT_COMPRESSION; // Default compression
        flags = PNG_Z_NO_COMPRESSION; // No compression
    }
    if (FreeImage_Save(format, dib, filename.c_str(), flags) == FALSE) {
        std::cerr << "Failed to save image: " << filename << std::endl;
        return false;
    }

    return true;
}

std::string getBaseName(const std::string& filename) {
    size_t pos = filename.find_last_of("_");
    if (pos != std::string::npos) {
        return filename.substr(0, pos);
    }
    return filename;
}

std::vector<std::string> findFilesWithSuffix(const std::string& suffix) {
    std::vector<std::string> files;
    try {
        fs::path targetPath = fs::current_path() / "TGA_Result";
        for (const auto& entry : fs::directory_iterator(targetPath)) {
            if (entry.path().extension() == ".tga" || entry.path().extension() == ".tif" || entry.path().extension() == ".png") {
                if (entry.path().filename().string().find(suffix) != std::string::npos) {
                    files.push_back(entry.path().string());
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << '\n';
    }
    catch (...) {
        std::cerr << "An unknown error occurred." << '\n';
    }
    return files;
}

void moveFilesToPBRFolder(const std::vector<std::string>&filenames) {

    try {
        fs::path pbrFolderPath = fs::current_path() / "PBR_Result";

        if (!fs::exists(pbrFolderPath)) {
            fs::create_directory(pbrFolderPath);
        }

        for (const auto& filename : filenames) {
            fs::path filePath = filename;
            fs::path newFilePath = pbrFolderPath / filePath.filename();
            fs::rename(filePath, newFilePath);
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << '\n';
    }
    catch (...) {
        std::cerr << "An unknown error occurred." << '\n';
    }
}

bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

int main() {
    FreeImage_Initialise();

    std::vector<std::string> nohqFiles = findFilesWithSuffix("_nohq");
    std::vector<std::string> smdiFiles = findFilesWithSuffix("_smdi");
    std::vector<std::string> asFiles = findFilesWithSuffix("_as");
    std::vector<std::string> coFiles = findFilesWithSuffix("_co");

    // We separate each type into TGA, TIFF, and PNG files.
    std::vector<std::string> nohqFilesTGA, nohqFilesTIFF, nohqFilesPNG;
    std::vector<std::string> smdiFilesTGA, smdiFilesTIFF, smdiFilesPNG;
    std::vector<std::string> asFilesTGA, asFilesTIFF, asFilesPNG;
    std::vector<std::string> coFilesTGA, coFilesTIFF, coFilesPNG;

    for (const auto& file : nohqFiles) {
        if (endsWith(file, ".tga")) {
            nohqFilesTGA.push_back(file);
        }
        else if (endsWith(file, ".tif")) {
            nohqFilesPNG.push_back(file);
        }
        else if (endsWith(file, ".png")) {
            nohqFilesTIFF.push_back(file);
        }
    }

    for (const auto& file : smdiFiles) {
        if (endsWith(file, ".tga")) {
            smdiFilesTGA.push_back(file);
        }
        else if (endsWith(file, ".tif")) {
            smdiFilesPNG.push_back(file);
        }
        else if (endsWith(file, ".png")) {
            smdiFilesTIFF.push_back(file);
        }
    }

    for (const auto& file : asFiles) {
        if (endsWith(file, ".tga")) {
            asFilesTGA.push_back(file);
        }
        else if (endsWith(file, ".tif")) {
            asFilesPNG.push_back(file);
        }
        else if (endsWith(file, ".png")) {
            asFilesTIFF.push_back(file);
        }
    }

    for (const auto& file : coFiles) {
        if (endsWith(file, ".tga")) {
            coFilesTGA.push_back(file);
        }
        else if (endsWith(file, ".tif")) {
            coFilesPNG.push_back(file);
        }
        else if (endsWith(file, ".png")) {
            coFilesTIFF.push_back(file);
        }
    }

    if (nohqFiles.empty() || smdiFiles.empty() || asFiles.empty() || coFiles.empty()) {
        std::cout << "Failed to load one or more images." << std::endl;
        FreeImage_DeInitialise();
        return -1;
    }

    for (size_t i = 0; i < nohqFiles.size(); ++i) {
        FIBITMAP* nohq = loadImage(nohqFiles[i]);
        FIBITMAP* smdi = loadImage(smdiFiles[i % smdiFiles.size()]);
        FIBITMAP* as = loadImage(asFiles[i % asFiles.size()]);
        FIBITMAP* co = loadImage(coFiles[i % coFiles.size()]);

        if (!nohq || !smdi || !as || !co) {
            std::cout << "Failed to load one or more images." << std::endl;
            FreeImage_Unload(nohq);
            FreeImage_Unload(smdi);
            FreeImage_Unload(as);
            FreeImage_Unload(co);
            FreeImage_DeInitialise();
            return -1;
        }

        // Querying image dimensions and depth
        unsigned int width = FreeImage_GetWidth(nohq);
        unsigned int height = FreeImage_GetHeight(nohq);
        unsigned int bpp = FreeImage_GetBPP(nohq);

        // Preparing images
        FIBITMAP* nohqPrepared = FreeImage_ConvertTo32Bits(nohq);
        FIBITMAP* smdiPrepared = FreeImage_ConvertTo32Bits(smdi);
        FIBITMAP* asPrepared = FreeImage_ConvertTo32Bits(as);
        FIBITMAP* coPrepared = FreeImage_ConvertTo32Bits(co);

        // Checking dimensions and doubling the size of AS if necessary
        unsigned int asWidth = FreeImage_GetWidth(asPrepared);
        unsigned int asHeight = FreeImage_GetHeight(asPrepared);

        if (asWidth != width || asHeight != height) {
            // Resized
            FIBITMAP* asResized = FreeImage_Rescale(asPrepared, width, height);
            FreeImage_Unload(asPrepared);
            asPrepared = asResized;
        }

        // Querying channels
        BYTE* nohqBits = FreeImage_GetBits(nohqPrepared);
        BYTE* smdiBits = FreeImage_GetBits(smdiPrepared);
        BYTE* asBits = FreeImage_GetBits(asPrepared);
        BYTE* coBits = FreeImage_GetBits(coPrepared);

        // Creating NMO and BCR images
        FIBITMAP* nmo = FreeImage_Allocate(width, height, 32);
        FIBITMAP* bcr = FreeImage_Allocate(width, height, 32);

        BYTE* nmoBuffer = FreeImage_GetBits(nmo);
        BYTE* bcrBuffer = FreeImage_GetBits(bcr);

        for (unsigned int y = 0; y < height; y++) {
            for (unsigned int x = 0; x < width; x++) {
                unsigned int nohqIndex = (x + y * width) * 4;
                unsigned int smdiIndex = (x + y * width) * 4;
                unsigned int asIndex = (x + y * width) * 4;
                unsigned int coIndex = (x + y * width) * 4;
                unsigned int nmoIndex = (x + y * width) * 4;
                unsigned int bcrIndex = (x + y * width) * 4;

                // NMO imaging
                nmoBuffer[nmoIndex + 0] = smdiBits[smdiIndex + 1]; // The G channel is from the SMDI
                nmoBuffer[nmoIndex + 1] = nohqBits[nohqIndex + 1]; // The G channel is from the NOHQ
                nmoBuffer[nmoIndex + 2] = nohqBits[nohqIndex + 2]; // The R channel is from the NOHQ
                nmoBuffer[nmoIndex + 3] = asBits[asIndex + 1];     // The G channel is from the AS

                // BCR imaging
                bcrBuffer[bcrIndex + 0] = coBits[coIndex + 0]; // The R channel is from the CO
                bcrBuffer[bcrIndex + 1] = coBits[coIndex + 1]; // The G channel is from the CO
                bcrBuffer[bcrIndex + 2] = coBits[coIndex + 2]; // The B channel is from the CO
                bcrBuffer[bcrIndex + 3] = smdiBits[smdiIndex + 0]; // The B channel is from the SMDI
            }
        }

        std::string baseName = getBaseName(nohqFiles[i]);
        std::string nmoNameTGA = baseName + "_NMO.tga";
        std::string bcrNameTGA = baseName + "_BCR.tga";
        std::string nmoNameTIFF = baseName + "_NMO.tif";
        std::string bcrNameTIFF = baseName + "_BCR.tif";
        std::string nmoNamePNG = baseName + "_NMO.png";
        std::string bcrNamePNG = baseName + "_BCR.png";

        saveImage(nmoNameTGA, nmo);
        saveImage(bcrNameTGA, bcr);
        saveImage(nmoNameTIFF, nmo);
        saveImage(bcrNameTIFF, bcr);
        saveImage(nmoNamePNG, nmo);
        saveImage(bcrNamePNG, bcr);

        std::cout << "NMO and BCR textures created successfully: " << nmoNameTGA << ", " << bcrNameTGA << ", " << nmoNameTIFF << ", " << bcrNameTIFF << ", " << nmoNamePNG << ", " << bcrNamePNG << std::endl;

        // Release
        FreeImage_Unload(nohq);
        FreeImage_Unload(smdi);
        FreeImage_Unload(as);
        FreeImage_Unload(co);
        FreeImage_Unload(nohqPrepared);
        FreeImage_Unload(smdiPrepared);
        FreeImage_Unload(asPrepared);
        FreeImage_Unload(coPrepared);
        FreeImage_Unload(nmo);
        FreeImage_Unload(bcr);
    }

    std::vector<std::string> nmoFilesTGA = findFilesWithSuffix("_NMO.tga");
    std::vector<std::string> bcrFilesTGA = findFilesWithSuffix("_BCR.tga");
    std::vector<std::string> nmoFilesTIFF = findFilesWithSuffix("_NMO.tif");
    std::vector<std::string> bcrFilesTIFF = findFilesWithSuffix("_BCR.tif");
    std::vector<std::string> nmoFilesPNG = findFilesWithSuffix("_NMO.png");
    std::vector<std::string> bcrFilesPNG = findFilesWithSuffix("_BCR.png");

    moveFilesToPBRFolder(nmoFilesTGA);
    moveFilesToPBRFolder(bcrFilesTGA);
    moveFilesToPBRFolder(nmoFilesTIFF);
    moveFilesToPBRFolder(bcrFilesTIFF);
    moveFilesToPBRFolder(nmoFilesPNG);
    moveFilesToPBRFolder(bcrFilesPNG);

    std::cout << "NMO and BCR textures moved to PBR_Result folder successfully." << std::endl;

    FreeImage_DeInitialise();
    return 0;
}
