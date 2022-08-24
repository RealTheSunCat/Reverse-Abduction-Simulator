#include "Shader.h"
#include "Core.h"

#include <fstream>

#include <ext/matrix_clip_space.hpp>

#include "Util.h"
#include "Core/File.h"

Shader::Shader(const GLchar* vertexName, const GLchar* fragmentName)
{
    std::string vName(vertexName);

#ifdef USE_GLFM
    vName = "ES_" + vName;
#endif

    std::string fName(fragmentName);
#ifdef USE_GLFM
    fName = "ES_" + fName;
#endif

    // read shader code from file
    File vertexFile = File({"ShaderData/", vName, "vert"});
    File fragmentFile = File({"ShaderData/", fName, "frag"});

    const std::vector<unsigned char> vCodeVector = vertexFile.readAllBytes();
    const std::vector<unsigned char> fCodeVector = fragmentFile.readAllBytes();

    std::string vCodeStr = std::string(vCodeVector.begin(), vCodeVector.end());
    std::string fCodeStr = std::string(fCodeVector.begin(), fCodeVector.end());

    const char* vCode = vCodeStr.c_str();
    const char* fCode = fCodeStr.c_str();

    // vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, nullptr);
    glCompileShader(vertex);

    // print compile errors if any
    int success;
    char errorLog[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, nullptr, errorLog);
        LOG_ERROR("Vertex shader %s failed to compile, error log:\n%s", vertexFile.path(), errorLog);
        LOG("Vertex shader code: \n\n%s\n\n", std::string(vCode));
    }

    // fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);
    // print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, nullptr, errorLog);
        LOG_ERROR("Fragment shader %s failed to compile, error log:\n%s", fragmentFile.path(), errorLog);
        LOG("Fragment shader code: \n\n%s\n\n", std::string(fCode));
    }

    // shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);

    glLinkProgram(ID);
    // print linking errors if any
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, nullptr, errorLog);
        LOG_ERROR("Failed to link shader programs %s and %s, error log:\n%s", vertexFile.path(), fragmentFile.path(), errorLog);
    }

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// use/activate the shader
void Shader::use() const
{
    glUseProgram(ID);
}

// utility uniform functions
void Shader::setBool(const char* name, const bool value) const
{
    glUniform1i(getUniformLocation(name), int(value));
}

// ------------------------------------------------------------------------
void Shader::setInt(const char* name, const int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

// ------------------------------------------------------------------------
void Shader::setFloat(const char* name, const float value) const
{
    glUniform1f(getUniformLocation(name), value);
}

// ------------------------------------------------------------------------
void Shader::setVec2(const char* name, const glm::vec2& value) const
{
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec2(const char* name, const float x, const float y) const
{
    glUniform2f(getUniformLocation(name), x, y);
}

// ------------------------------------------------------------------------
void Shader::setVec3(const char* name, const glm::vec3& value) const
{
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec3(const char* name, const float x, const float y, const float z) const
{
    glUniform3f(getUniformLocation(name), x, y, z);
}

// ------------------------------------------------------------------------
void Shader::setVec4(const char* name, const glm::vec4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec4(const char* name, const float x, const float y, const float z, const float w) const
{
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

// ------------------------------------------------------------------------
void Shader::setMat2(const char* name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

// ------------------------------------------------------------------------
void Shader::setMat3(const char* name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

// ------------------------------------------------------------------------
void Shader::setMat4(const char* name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

GLint Shader::getUniformLocation(const char* uniformName) const
{
    std::size_t hash = Util::hashBytes(uniformName, strlen(uniformName));

    const auto f = uniformCache.find(hash);

    GLint loc;

    if (f == uniformCache.end())
    {
        // get uniform location
        loc = glGetUniformLocation(ID, uniformName);

        std::pair<std::size_t, GLuint> newLoc(hash, loc);
        uniformCache.insert(newLoc);
    }
    else
    {
        loc = (*f).second;
    }

    return loc;
}

// utility function for checking shader compilation/linking errors
void Shader::checkCompileErrors(const GLuint shader, const std::string& type)
{
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            LOG_ERROR("Shader compilation error! Type: %s\n%s", type, infoLog);
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            LOG_ERROR("Shader linking error! Type: %s\n%s", type, infoLog);
        }
    }
}
