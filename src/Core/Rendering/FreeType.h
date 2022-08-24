﻿#pragma once

#include <ft2build.h>
#include <Core/File.h>

#include FT_FREETYPE_H

#include "Types.h"

class FreeType
{
    std::vector<unsigned char> fontData;

public:
    void loadChar(FT_Face face, char c)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            LOG_ERROR("FreeType failed to load glyph \'%c\'!", c);
            return;
        }

        // create the texture for this character
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
#ifdef USE_GLFM
        constexpr GLint internalFormat = GL_ALPHA;
#else
        constexpr GLint internalFormat = GL_RED;
#endif

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            internalFormat,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        FontCharacter character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };

        loadedCharacters.insert(std::pair<char, FontCharacter>(c, character));
    }

    FreeType()
    {
        LOG("Initializing FreeType...");

        FT_Library ft;
        if (FT_Init_FreeType(&ft))
        {
            LOG_ERROR("Failed to initialize FreeType!");
            return;
        }

        File fontFile = File(Resource("ObjectData/UI/", "octopuzzlerType.otf"));

        fontData = fontFile.readAllBytes();

        FT_Face face;
        if (FT_New_Memory_Face(ft, fontData.data(), fontData.size(), 0, &face))
        {
            LOG("Failed to load ObjectData/UI/octopuzzlerType.otf!");
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

        for (unsigned char c = 'a'; c < 'a' + 26; c++) // haha
            loadChar(face, c);

        for (unsigned char c = 'A'; c < 'A' + 26; c++)
            loadChar(face, c);

        for (unsigned char c = '0'; c <= '9'; c++)
            loadChar(face, c);

        loadChar(face, ' ');
        loadChar(face, '.');
        loadChar(face, '/');
        loadChar(face, ':');
        loadChar(face, '-');

        // arrows
        loadChar(face, '*'); // up
        loadChar(face, ','); // down
        loadChar(face, '('); // left
        loadChar(face, ')'); // right
        loadChar(face, '^'); // dash up
        loadChar(face, '_'); // dash down
        loadChar(face, '<'); // dash left
        loadChar(face, '>'); // dash right

        loadChar(face, '#'); // empty box

        // eyes
        loadChar(face, '$'); // circle
        loadChar(face, '%'); // square
        loadChar(face, '&'); // triangle
    }

    std::unordered_map<char, FontCharacter> loadedCharacters;
};
