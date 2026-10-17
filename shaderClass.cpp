#include "shaderClass.h"
#include <iostream>

// ============================================================
//  Shader root directory
// ============================================================
#ifdef SHADER_DIR
    static const std::string gShaderRoot = SHADER_DIR;
#else
    static const std::string gShaderRoot = "shaders/";
#endif

// ============================================================
//  resolvePath — shaders/ 
// ============================================================
std::string Shader::resolvePath(const char* filename)
{
    std::string name = filename;

    if (name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos)
        return name;

    std::string root = gShaderRoot;
    if (!root.empty() && root.back() != '/' && root.back() != '\\')
        root += '/';
    return root + name;
}

// ============================================================
//  get_file_contents
// ============================================================
std::string get_file_contents(const char* filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "[Shader] FAILED TO OPEN: " << filename << std::endl;
        return "";
    }
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    return contents;
}

// ============================================================
//  Constructor — vertex + fragment
// ============================================================
Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
    std::string vPath = resolvePath(vertexFile);
    std::string fPath = resolvePath(fragmentFile);

    std::string vertexCode   = get_file_contents(vPath.c_str());
    std::string fragmentCode = get_file_contents(fPath.c_str());

    const char* vertexSource   = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    compileErrors(vertexShader, "VERTEX", vPath.c_str());

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    compileErrors(fragmentShader, "FRAGMENT", fPath.c_str());

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    compileErrors(ID, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "[Shader] " << vPath << " + " << fPath
              << " → ID=" << ID << std::endl;
}

// ============================================================
//  Constructor — vertex + fragment + geometry
// ============================================================
Shader::Shader(const char* vertexFile, const char* fragmentFile, const char* geometryFile)
{
    std::string vPath = resolvePath(vertexFile);
    std::string fPath = resolvePath(fragmentFile);
    std::string gPath = resolvePath(geometryFile);

    std::string vertexCode   = get_file_contents(vPath.c_str());
    std::string fragmentCode = get_file_contents(fPath.c_str());
    std::string geometryCode = get_file_contents(gPath.c_str());

    const char* vertexSource   = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();
    const char* geometrySource = geometryCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    compileErrors(vertexShader, "VERTEX", vPath.c_str());

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    compileErrors(fragmentShader, "FRAGMENT", fPath.c_str());

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometrySource, NULL);
    glCompileShader(geometryShader);
    compileErrors(geometryShader, "GEOMETRY", gPath.c_str());

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glAttachShader(ID, geometryShader);
    glLinkProgram(ID);
    compileErrors(ID, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    std::cout << "[Shader] " << vPath << " + " << fPath
              << " + " << gPath << " → ID=" << ID << std::endl;
}

void Shader::Activate() {
    glUseProgram(ID);
}

void Shader::Delete() {
    if (ID) {
        glDeleteProgram(ID);
        ID = 0;
    }
}

void Shader::compileErrors(unsigned int shader, const char* type, const char* filename) {
    GLint hasCompiled;
    char infoLog[2048];

    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE) {
            glGetShaderInfoLog(shader, 2048, NULL, infoLog);
            std::cerr << "\n========================================\n"
                      << "SHADER COMPILE ERROR [" << type << "]\n"
                      << "File: " << (filename && *filename ? filename : "(unknown)") << "\n"
                      << "----------------------------------------\n"
                      << infoLog
                      << "========================================\n" << std::endl;

#ifdef _DEBUG
            __debugbreak();
#endif
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE) {
            glGetProgramInfoLog(shader, 2048, NULL, infoLog);
            std::cerr << "\n========================================\n"
                      << "SHADER LINK ERROR\n"
                      << "----------------------------------------\n"
                      << infoLog
                      << "========================================\n" << std::endl;

#ifdef _DEBUG
            __debugbreak();
#endif
        }
    }
}