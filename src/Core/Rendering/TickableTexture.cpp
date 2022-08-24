#include "TickableTexture.h"

TickableTexture::TickableTexture(const std::vector<GLuint>& texIds,
                                 const unsigned int _frameLength, bool _shouldRestart)
    : SimpleTexture(texIds.at(0)), textures(texIds), frameLength(_frameLength)
{
    loop = _shouldRestart;
    shouldTick = true;

    texId = textures[0];
}

void TickableTexture::tick()
{
    if (!shouldTick)
        return;

    frameTally++;

    if (frameTally > frameLength)
    {
        frameTally = 0;
        nextFrame();
    }
}

void TickableTexture::tick(unsigned int& stepCount)
{
    for (unsigned int i = 0; i < stepCount; i++)
    {
        tick();
    }
}

void TickableTexture::nextFrame()
{
    if (curFrame < (textures.size() - 1))
        curFrame++;
    else if(loop)
        curFrame = 0;

    texId = textures.at(curFrame);
}

void TickableTexture::reset()
{
    curFrame = 0;
    texId = textures.at(curFrame);
}
