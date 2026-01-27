#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <GL/glew.h> // Assuming GLEW or similar for now, will adjust if using GLAD

namespace Beam {

class Shader {
public:
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    void use() const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const float* matrix) const;

private:
    unsigned int m_id;
    void checkCompileErrors(unsigned int shader, std::string type);
};

} // namespace Beam

#endif // SHADER_HPP
