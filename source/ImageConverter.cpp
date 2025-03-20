// ImageConverter.cpp
#include "ImageConverter.h"
#include <cstdio>
#include <cstring>

// Seulement inclure les en-têtes sans définir les implémentations
// Les implémentations sont dans stb_implementation.cpp
#include "stb_image.h"
#include "stb_image_write.h"

bool ImageConverter::isJpegFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    // Les fichiers JPEG commencent par les octets FF D8
    uint8_t signature[2] = { 0 };
    size_t read = fread(signature, 1, 2, f);
    fclose(f);

    if (read != 2) return false;
    return (signature[0] == 0xFF && signature[1] == 0xD8);
}

bool ImageConverter::isPngFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    // Les fichiers PNG commencent par les octets 89 50 4E 47 0D 0A 1A 0A
    const uint8_t pngSignature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    uint8_t signature[8] = { 0 };
    size_t read = fread(signature, 1, 8, f);
    fclose(f);

    if (read != 8) return false;
    return (memcmp(signature, pngSignature, 8) == 0);
}

std::string ImageConverter::convertPngToJpeg(const std::string& pngPath, bool keepSource) {
    // Vérifier si c'est bien un PNG
    if (!isPngFile(pngPath)) {
        return pngPath; // Si ce n'est pas un PNG, retourner le chemin original
    }

    // Générer le nom du fichier JPEG de sortie
    std::string jpegPath = pngPath;
    size_t extPos = jpegPath.find_last_of('.');
    if (extPos != std::string::npos) {
        jpegPath = jpegPath.substr(0, extPos);
    }
    jpegPath += ".jpg";

    // Charger l'image PNG avec stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(pngPath.c_str(), &width, &height, &channels, 3); // Forcer 3 canaux (RGB)

    if (!imageData) {
        return pngPath; // En cas d'échec du chargement, retourner le chemin original
    }

    // Écrire l'image au format JPEG avec stb_image_write
    int quality = 90; // Qualité JPEG (0-100)
    int success = stbi_write_jpg(jpegPath.c_str(), width, height, 3, imageData, quality);

    // Libérer la mémoire
    stbi_image_free(imageData);

    // Si la conversion a réussi et que keepSource est false, supprimer le PNG original
    if (success && !keepSource) {
        remove(pngPath.c_str());
    }

    return success ? jpegPath : pngPath;
}