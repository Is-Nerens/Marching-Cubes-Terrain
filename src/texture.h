#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GL/glew.h>

typedef struct Texture {
    int width;
    int height;
    unsigned char* buffer;
    GLuint handle;
} Texture;

void TextureLoadFile(Texture* texture, char* filepath)
{
    int bpp;
    texture->buffer = stbi_load(filepath, &texture->width, &texture->height, &bpp, 4);
    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->buffer);
}

void TextureFree(Texture* texture)
{
    glDeleteTextures(1, &texture->handle);
    free(texture->buffer);
    texture->buffer = NULL;
    texture->width = 0;
    texture->height = 0;
}