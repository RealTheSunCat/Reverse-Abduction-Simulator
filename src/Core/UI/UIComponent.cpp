#include "UIComponent.h"

#include <utility>

#include "Outrospection.h"
#include "Util.h"


UITransform::UITransform(int posX, int posY, int sizeX, int sizeY,
                         const glm::vec2& fbRes, UIAlign _alignment)
    : defaultRes(fbRes), pos(posX, posY), size(sizeX, sizeY), alignment(_alignment)
{
    switch(alignment) // TODO no idea if this works, never used it
    {
    case UIAlign::CENTER:
        pos -= size / 2.f;
        break;

    case UIAlign::TOP_LEFT:

        break;

    case UIAlign::TOP_RIGHT:
        pos.x -= size.x;
        break;

    case UIAlign::BOT_LEFT:
        pos.y -= size.y;
        break;

    case UIAlign::BOT_RIGHT:
        pos -= size;
        break;
    }
}

UITransform::UITransform(int posX, int posY, int radius,
    const glm::vec2& fbRes, UIAlign _alignment) : UITransform(posX, posY, radius, 0, fbRes, _alignment)
{}

glm::vec2 UITransform::getPos() const
{
    return pos * getSizeRatio();
}

glm::vec2 UITransform::getSize() const
{
    return size * getSizeRatio();
}

glm::vec2 UITransform::getSizeRatio() const
{
    return *Outrospection::get().curFbResolution / defaultRes;
}

void UITransform::setPos(glm::vec2 _pos)
{
    pos = _pos;
}

void UITransform::setPos(int x, int y)
{
    pos = glm::vec2(x, y);
}

void UITransform::setSize(int x, int y)
{
    size = glm::vec2(x, y);
}

GLuint UIComponent::quadVAO = 0;

UIComponent::UIComponent(const std::string& _texName, const GLint& texFilter, const UITransform& _transform)
    : UIComponent(_texName, simpleTexture({"ObjectData/UI/", _texName}, texFilter), _transform)
{
}

UIComponent::UIComponent(std::string _name, const Resource& _res, const UITransform& _transform)
    : text(std::move(_name)), textColor(0.0f), transform(_transform)
{
    animations.insert(std::make_pair("default", _res));

    // we need to create our quad the first time!
    if (quadVAO == 0)
    {
        // configure VAO/VBO
        GLuint VBO;
        float vertices[] = {
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,

            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(quadVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void UIComponent::tick()
{
    if(glm::length(m_goal) != 0 && transform.getPos() != m_goal)
    {
        if(moveLinearly)
            transform.setPos(transform.getPos() + (2 * glm::normalize(m_goal - transform.getPos())));
        else
            transform.setPos(Util::lerp(transform.getPos(), m_goal, animationSpeed));
    }

    opacity = Util::lerp(opacity, opacityGoal, animationSpeed);
}

void UIComponent::addAnimation(const std::string& anim, const Resource& _res)
{
    animations.insert(std::make_pair(anim, _res));
}

void UIComponent::setAnimation(const std::string& anim)
{
    Outrospection::get().textureManager.get(animations.at(curAnimation)).shouldTick = false;
    Outrospection::get().textureManager.get(animations.at(curAnimation)).reset();

    if(animations.find(anim) == animations.end())
    {
        LOG_ERROR("Animation %s has not been loaded!", anim);
        curAnimation = "default";
    } else {
        curAnimation = anim;
    }

    Outrospection::get().textureManager.get(animations.at(curAnimation)).reset();
    Outrospection::get().textureManager.get(animations.at(curAnimation)).shouldTick = true;
}

void UIComponent::setPosition(int x, int y)
{
    transform.setPos(x, y);
}

void UIComponent::setScale(int px)
{
    transform.setSize(px, px);
}

void UIComponent::setScale(int x, int y)
{
    transform.setSize(x, y);
}

void UIComponent::draw(Shader& shader, const Shader& glyphShader) const
{
    if (!visible)
        return;

    shader.use();

    glm::vec2 pos = transform.getPos();

    if(bobUpAndDown)
    {
        pos.y += transform.getSize().y * 0.1f * sin((Util::currentTimeMillis() % 100000) / 1000.f + float(Util::hashBytes(text.c_str(), text.length()) % 10000) / 100.f);

        //printf("%f\n", Util::currentTimeMillis() % 100000);
    }

    // TODO maybe we should cache this
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos, 0));
    model = glm::scale(model, glm::vec3(transform.getSize(), 0));

    shader.setMat4("model", model);

    shader.setFloat("opacity", opacity);
    shader.setBool("flip", flip);

    glActiveTexture(GL_TEXTURE0);
    Outrospection::get().textureManager.get(animations.at(curAnimation)).bind();

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (textSize > 0 && !text.empty()) // TODO make a proper text class
    {
        drawText(text, glyphShader);
    }
}

void UIComponent::drawText(const std::string& text, const Shader& glyphShader) const
{
    glyphShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(quadVAO);

    glm::vec2 textScale = transform.getSizeRatio() * 1.5f * textSize; // TODO sketchy scale?

    glm::vec2 textPos = transform.getPos();
    textPos.y += (transform.getSize().y) / 2 + (10 * textScale.y);

    // add an artificial space at the beginning
    textPos.x += textScale.x * 10;

    glm::vec2 startPos = textPos;

    for (char c : text)
    {
        if (c <= '\0' || c == ' ')
        {
            textPos.x += textScale.x * 10;
            continue;
        }

        if(c == '\n') {
            startPos.y += transform.getSize().y / 2 * textSize;
            textPos = startPos;
            continue;
        }

        if (!Outrospection::get().fontCharacters.contains(c)) {
            LOG_ERROR("Character %c not found!", c);

            // assume a space
            textPos.x += textScale.x * 10;
            continue;
        }

        FontCharacter fontCharacter = Outrospection::get().fontCharacters[c];

        // calculate model matrix
        glm::mat4 charModel = glm::mat4(1.0f);

        glm::vec2 charPos = textPos;
        charPos.x += fontCharacter.bearing.x * textScale.x;
        charPos.y -= fontCharacter.bearing.y * textScale.y;

        glm::vec2 charSize = fontCharacter.size * textScale;


        glBindTexture(GL_TEXTURE_2D, fontCharacter.textureId);

        if(textShadow) {
            charModel = glm::translate(charModel, glm::vec3(charPos, 0.0f));
            charModel = glm::scale(charModel, glm::vec3(charSize, 1.0f));

            glyphShader.setMat4("model", charModel);
            glyphShader.setVec3("textColor", 0.765f * textColor);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            charModel = glm::mat4(1.0f);
        }

        charPos.y -= (transform.getSize().y) / 16 * (textScale.y / 3.5f);

        charModel = glm::translate(charModel, glm::vec3(charPos, 0.0f));
        charModel = glm::scale(charModel, glm::vec3(charSize, 1.0f));

        glyphShader.setMat4("model", charModel);
        glyphShader.setVec3("textColor", textColor);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        textPos.x += (fontCharacter.advance >> 6) * textScale.x;
    }

    glBindVertexArray(0);
}

void UIComponent::setGoal(int x, int y)
{
    m_goal = glm::vec2(x, y);
}

bool UIComponent::hasGoal()
{
    return (1 - abs(glm::dot(m_goal, transform.getPos()))) > 0.1;
}

void UIComponent::warpToGoal()
{
    if(glm::length(m_goal) > 0)
        transform.setPos(m_goal);

    opacity = opacityGoal;
}
