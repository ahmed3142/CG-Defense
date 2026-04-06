#pragma once
#include <string>
#include <raylib.h>
#include <unordered_map>
#include <iostream>

using namespace std;

class TextureLoader {
    static unordered_map<string, Texture2D> loadedTextures;

    public:
        static Texture2D* LoadTextureFromFile(const string& filename);
        static void DeallocTexture();
};