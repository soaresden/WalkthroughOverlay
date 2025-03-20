//StbImageElement.cpp
#include "StbImageElement.hpp"
#include "stb_image.h"
#include "Log.hpp"  // Pour le débogage
#include <cstring>
#include <algorithm>

// Constantes globales
#define IMAGE_MAX_WIDTH 450
#define IMAGE_MAX_HEIGHT 450

StbImageElement::StbImageElement(const std::string& path)
    : tsl::elm::Element(),
    m_path(path),
    m_imageData(nullptr),
    m_width(0),
    m_height(0),
    m_originalWidth(0),
    m_originalHeight(0),
    m_zoomLevel(0.0f),
    m_offsetX(0),
    m_offsetY(0),
    m_state(LoadState::NotStarted) {

    // Log pour déboguer
    Log::log("StbImageElement créé pour %s", path.c_str());
}

StbImageElement::~StbImageElement() {
    if (m_imageData) {
        stbi_image_free(m_imageData);
        m_imageData = nullptr;
    }
    Log::log("StbImageElement détruit");
}

void StbImageElement::loadImage() {
    // Si déjà chargé, ne rien faire
    if (m_state == LoadState::Ready) {
        return;
    }

    // Libérer l'ancienne image si nécessaire
    if (m_imageData != nullptr) {
        stbi_image_free(m_imageData);
        m_imageData = nullptr;
    }

    // Log de débogage
    Log::log("Chargement de l'image: %s", m_path.c_str());

    // Charger l'image - forcer 4 canaux
    int width, height, channels;
    m_imageData = stbi_load(m_path.c_str(), &width, &height, &channels, 4);

    if (!m_imageData) {
        m_state = LoadState::Error;
        m_errorMessage = stbi_failure_reason();
        Log::log("Erreur de chargement: %s", m_errorMessage.c_str());
        return;
    }

    // Sauvegarder les dimensions originales
    m_originalWidth = width;
    m_originalHeight = height;
    Log::log("Dimensions originales: %dx%d", width, height);

    // Redimensionner si nécessaire
    if (width > IMAGE_MAX_WIDTH || height > IMAGE_MAX_HEIGHT) {
        // Calculer le ratio pour maintenir les proportions
        float ratio = std::min((float)IMAGE_MAX_WIDTH / width, (float)IMAGE_MAX_HEIGHT / height);
        m_width = static_cast<int>(width * ratio);
        m_height = static_cast<int>(height * ratio);

        // Créer une nouvelle image redimensionnée
        unsigned char* resizedData = new unsigned char[m_width * m_height * 4];

        // Redimensionner avec un algorithme simple
        for (int y = 0; y < m_height; y++) {
            for (int x = 0; x < m_width; x++) {
                int srcX = static_cast<int>((float)x / m_width * width);
                int srcY = static_cast<int>((float)y / m_height * height);

                for (int c = 0; c < 4; c++) {
                    resizedData[(y * m_width + x) * 4 + c] =
                        m_imageData[(srcY * width + srcX) * 4 + c];
                }
            }
        }

        // Remplacer l'ancienne image
        stbi_image_free(m_imageData);
        m_imageData = resizedData;
        Log::log("Image redimensionnée à %dx%d", m_width, m_height);
    }
    else {
        // Utiliser les dimensions originales
        m_width = width;
        m_height = height;
    }

    m_state = LoadState::Ready;
    Log::log("Image chargée avec succès");
}

void StbImageElement::draw(tsl::gfx::Renderer* renderer) {
    // Fond noir
    renderer->fillScreen(renderer->a({ 0x0, 0x0, 0x0, 0xF }));

    // Gestion des états
    if (m_state == LoadState::NotStarted) {
        renderer->drawString("Chargement...", false, 20, 100, 18, renderer->a(0xFFFF));
        m_state = LoadState::Loading;
        return;
    }
    else if (m_state == LoadState::Loading) {
        loadImage();
        return;
    }
    else if (m_state == LoadState::Error) {
        renderer->drawString("Erreur de chargement!", false, 20, 100, 18, renderer->a(0xFFFF));
        renderer->drawString(m_path.c_str(), false, 20, 130, 15, renderer->a(0xFFFF));
        renderer->drawString(m_errorMessage.c_str(), false, 20, 160, 15, renderer->a(0xFFFF));
        renderer->drawString("B: Retour", false, 20, 200, 18, renderer->a(0xFFFF));
        return;
    }

    // Si l'image est chargée
    if (m_imageData && m_state == LoadState::Ready) {
        // Calculer le facteur de zoom (simplifié)
        float zoom = 1.0f + m_zoomLevel;
        int zoomedWidth = static_cast<int>(m_width * zoom);
        int zoomedHeight = static_cast<int>(m_height * zoom);

        // Position centrée
        int centerX = getX() + getWidth() / 2;
        int centerY = getY() + getHeight() / 2;
        int startX = centerX - zoomedWidth / 2 + m_offsetX;
        int startY = centerY - zoomedHeight / 2 + m_offsetY;

        // Dessiner l'image
        renderer->drawBitmap(startX, startY, zoomedWidth, zoomedHeight, m_imageData);

        // Afficher les informations
        char info[128];
        snprintf(info, sizeof(info), "Taille: %dx%d | Zoom: %.1fx",
            m_originalWidth, m_originalHeight, zoom);
        renderer->drawString(info, false, 20, 630, 15, renderer->a(0xFFFF));

        // Afficher les contrôles
        renderer->drawString("X: Cacher | Y: Réinitialiser | StickR: Zoom",
            false, 20, 650, 15, renderer->a(0xFFFF));
    }
}

void StbImageElement::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    // Utiliser tout l'espace disponible
    this->setBoundaries(parentX, parentY, parentWidth, parentHeight);
}

bool StbImageElement::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
    HidAnalogStickState leftJoystick, HidAnalogStickState rightJoystick) {
    // Log pour débogage
    if (keysDown)
        Log::log("StbImageElement: touches enfoncées: 0x%016llX", keysDown);

    // Si l'image n'est pas encore chargée, ignorer les entrées
    if (m_state != LoadState::Ready)
        return false;

    // Masquer l'overlay
    if (keysDown & HidNpadButton_X) {
        Log::log("Touche X: Masquer l'overlay");
        tsl::Overlay::get()->hide();
        return true;
    }

    // Réinitialiser la vue
    if (keysDown & HidNpadButton_Y) {
        Log::log("Touche Y: Réinitialiser la vue");
        m_zoomLevel = 0.0f;
        m_offsetX = 0;
        m_offsetY = 0;
        return true;
    }

    // Zoom avec les boutons du joystick droit
    if (keysHeld & HidNpadButton_StickRUp) {
        m_zoomLevel += 0.05f;
        if (m_zoomLevel > 5.0f) m_zoomLevel = 5.0f;
        Log::log("StickRUp: zoom = %.2f", m_zoomLevel);
        return true;
    }

    if (keysHeld & HidNpadButton_StickRDown) {
        m_zoomLevel -= 0.05f;
        if (m_zoomLevel < -0.5f) m_zoomLevel = -0.5f;
        Log::log("StickRDown: zoom = %.2f", m_zoomLevel);
        return true;
    }

    // Déplacement avec les touches directionnelles
    bool moved = false;
    if (keysHeld & HidNpadButton_Left) {
        m_offsetX += 5;
        moved = true;
    }

    if (keysHeld & HidNpadButton_Right) {
        m_offsetX -= 5;
        moved = true;
    }

    if (keysHeld & HidNpadButton_Up) {
        m_offsetY += 5;
        moved = true;
    }

    if (keysHeld & HidNpadButton_Down) {
        m_offsetY -= 5;
        moved = true;
    }

    // Déplacement avec le joystick gauche
    if (std::abs(leftJoystick.x) > 0.3f) {
        m_offsetX -= static_cast<int>(leftJoystick.x * 5);
        moved = true;
    }

    if (std::abs(leftJoystick.y) > 0.3f) {
        m_offsetY -= static_cast<int>(leftJoystick.y * 5);
        moved = true;
    }

    if (moved) {
        Log::log("Déplacement: offset = (%d, %d)", m_offsetX, m_offsetY);
        return true;
    }

    return false;
}