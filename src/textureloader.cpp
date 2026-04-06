#include "textureloader.h"

unordered_map<string, Texture2D> TextureLoader::loadedTextures;

Texture2D* TextureLoader::LoadTextureFromFile(const string& filename) {
    if(filename.empty()) return nullptr;

    auto it = loadedTextures.find(filename);
    if (it != loadedTextures.end()) {
        return &it->second; // Return existing texture
    }

    string filepath = "src/assets/Images/" + filename;
    Texture2D texture = LoadTexture(filepath.c_str());
    if(texture.id != 0){
        loadedTextures[filename] = texture; // Store the loaded texture
        return &loadedTextures[filename];
    } 
    return nullptr; // Return nullptr if loading failed
}

void TextureLoader::DeallocTexture() {
    for (auto& pair : loadedTextures) {
        UnloadTexture(pair.second); // Unload each texture
    }
    loadedTextures.clear(); // Clear the map
}