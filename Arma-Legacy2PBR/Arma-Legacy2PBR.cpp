#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

cv::Mat prepareImage(const cv::Mat& src, const cv::Size& targetSize, int depth) {
    cv::Mat dest;
    if (src.size() != targetSize) {
        cv::resize(src, dest, targetSize); // Resizing
    }
    else {
        dest = src.clone();
    }
    if (dest.depth() != depth) {
        dest.convertTo(dest, depth); // Depth conversion
    }
    return dest;
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
    fs::path currentPath = fs::current_path();
    for (const auto& entry : fs::directory_iterator(currentPath)) {
        if (entry.path().extension() == ".png" && entry.path().filename().string().find(suffix) != std::string::npos) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

int main() {
    std::vector<std::string> nohqFiles = findFilesWithSuffix("_nohq.png");
    std::vector<std::string> smdiFiles = findFilesWithSuffix("_smdi.png");
    std::vector<std::string> asFiles = findFilesWithSuffix("_as.png");
    std::vector<std::string> coFiles = findFilesWithSuffix("_co.png");

    if (nohqFiles.empty() || smdiFiles.empty() || asFiles.empty() || coFiles.empty()) {
        std::cout << "Failed to load one or more images." << std::endl;
        return -1;
    }

    for (size_t i = 0; i < nohqFiles.size(); ++i) {
        cv::Mat nohq = cv::imread(nohqFiles[i]);
        cv::Mat smdi = cv::imread(smdiFiles[i % smdiFiles.size()]);
        cv::Mat as = cv::imread(asFiles[i % asFiles.size()]);
        cv::Mat co = cv::imread(coFiles[i % coFiles.size()]);

        cv::Size targetSize = nohq.size();
        int depth = nohq.depth();

        cv::Mat nohqPrepared = prepareImage(nohq, targetSize, depth);
        cv::Mat smdiPrepared = prepareImage(smdi, targetSize, depth);
        cv::Mat asPrepared = prepareImage(as, targetSize, depth);
        cv::Mat coPrepared = prepareImage(co, targetSize, depth);

        std::vector<cv::Mat> nohqChannels, smdiChannels, asChannels, coChannels;
        cv::split(nohqPrepared, nohqChannels);
        cv::split(smdiPrepared, smdiChannels);
        cv::split(asPrepared, asChannels);
        cv::split(coPrepared, coChannels);

        std::vector<cv::Mat> nmoChannels = { smdiChannels[1], nohqChannels[1], nohqChannels[2], asChannels[0] };
        cv::Mat nmo(targetSize, CV_8UC4);
        cv::merge(nmoChannels, nmo);

        std::vector<cv::Mat> bcrChannels = { coChannels[0], coChannels[1], coChannels[2], smdiChannels[0] };
        cv::Mat bcr(targetSize, CV_8UC4);
        cv::merge(bcrChannels, bcr);
        std::string baseName = getBaseName(nohqFiles[i]);
        std::string nmoName = baseName + "_NMO.png";
        std::string bcrName = baseName + "_BCR.png";
        cv::imwrite(nmoName, nmo);
        cv::imwrite(bcrName, bcr);

        std::cout << "NMO and BCR textures created successfully: " << nmoName << " and " << bcrName << std::endl;
    }

    return 0;
}