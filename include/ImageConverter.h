// ImageConverter.h
#pragma once
#include <string>

class ImageConverter {
public:
    // V�rifie si un fichier est au format JPEG
    static bool isJpegFile(const std::string& path);

    // V�rifie si un fichier est au format PNG
    static bool isPngFile(const std::string& path);

    // Convertit une image PNG en JPEG et retourne le chemin du fichier JPEG
    // Si keepSource est true, le fichier PNG original est conserv�
    static std::string convertPngToJpeg(const std::string& pngPath, bool keepSource = true);
};