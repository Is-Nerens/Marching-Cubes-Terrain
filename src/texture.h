#ifndef TEXTURE_H
#define TEXTURE_H
#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <string>
#include <iostream>
#include "vendor/stb_image.h"

class Texture
{
public:
    
    Texture(const std::string& path) : filepath(path), width(0), height(0)
    {
        stbi_set_flip_vertically_on_load(1);
        localbuffer = stbi_load(path.c_str(), &width, &height, &bpp, 4);

        if (localbuffer != nullptr) 
        {
            glGenTextures(1, &rendererID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localbuffer);

            stbi_image_free(localbuffer);
        } 
        else 
        {
            std::cerr << "[Texture Class] Failed to load image: " << path << std::endl;
        }
    }

    ~Texture() {
        glDeleteTextures(1, &rendererID);
    }

    int Width()  const { return width;  }
    int Height() const { return height; }

    void Bind(unsigned int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, rendererID);
    }

    void Unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:
    std::string filepath;
    unsigned char* localbuffer;
    int width, height, bpp;
    unsigned int rendererID;
};

#endif