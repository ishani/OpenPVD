//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// an experimental PhysX visual debugger
//

#include "pch.h"

#include <glbinding/glbinding.h>
#include <glbinding/Version.h>
#include <glbinding/FunctionCall.h>
#include <glbinding/CallbackMask.h>
#include <glbinding-aux/ContextInfo.h>

#include <glbinding/gl43core/gl.h>
#include <glbinding/getProcAddress.h>

#include <glm/glm.hpp>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include "imgui_internal.h"

#include <globjects/globjects.h>
#include <globjects/base/File.h>
#include <globjects/logging.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include "viewer/ScreenAlignedQuad.h"

using namespace gl43core;

namespace
{
    std::unique_ptr<ScreenAlignedQuad> g_quad = nullptr;
    std::unique_ptr<globjects::Buffer> g_buffer = nullptr;

    std::unique_ptr<globjects::Program> g_program = nullptr;

    std::unique_ptr<globjects::AbstractStringSource> g_vertexShaderSource = nullptr;
    std::unique_ptr<globjects::AbstractStringSource> g_vertexShaderTemplate = nullptr;
    std::unique_ptr<globjects::Shader> g_vertexShader = nullptr;

    std::unique_ptr<globjects::AbstractStringSource> g_fragmentShaderSource = nullptr;
    std::unique_ptr<globjects::AbstractStringSource> g_fragmentShaderTemplate = nullptr;
    std::unique_ptr<globjects::Shader> g_fragmentShader = nullptr;

    auto g_size = glm::ivec2{};
}


void initialize()
{
    g_vertexShaderSource = ScreenAlignedQuad::vertexShaderSource();
    g_vertexShaderTemplate = globjects::Shader::applyGlobalReplacements( g_vertexShaderSource.get() );
    g_vertexShader = globjects::Shader::create( GL_VERTEX_SHADER, g_vertexShaderTemplate.get() );

    g_fragmentShaderSource = ScreenAlignedQuad::fragmentShaderSource();
    g_fragmentShaderTemplate = globjects::Shader::applyGlobalReplacements( g_fragmentShaderSource.get() );
    g_fragmentShader = globjects::Shader::create( GL_FRAGMENT_SHADER, g_fragmentShaderTemplate.get() );

    g_program = globjects::Program::create();
    g_program->attach( g_vertexShader.get(), g_fragmentShader.get() );

    g_quad = ScreenAlignedQuad::create( g_program.get() );

    g_program->setUniform( "maximum", 10 );
    g_program->setUniform( "rowCount", 10 );
    g_program->setUniform( "columnCount", 10 );

    static const auto data = std::array<int, 100> { {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                10, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                9, 10, 1, 2, 3, 4, 5, 6, 7, 8,
                8, 9, 10, 1, 2, 3, 4, 5, 6, 7,
                7, 8, 9, 10, 1, 2, 3, 4, 5, 6,
                6, 7, 8, 9, 10, 1, 2, 3, 4, 5,
                5, 6, 7, 8, 9, 10, 1, 2, 3, 4,
                4, 5, 6, 7, 8, 9, 10, 1, 2, 3,
                3, 4, 5, 6, 7, 8, 9, 10, 1, 2,
                2, 3, 4, 5, 6, 7, 8, 9, 10, 1 }};

    g_buffer = globjects::Buffer::create();
    g_buffer->setData( sizeof( data ), data.data(), GL_STATIC_DRAW );

    g_buffer->bindBase( GL_SHADER_STORAGE_BUFFER, 1 );
}

void deinitialize()
{
    g_quad.reset( nullptr );
    g_buffer.reset( nullptr );

    g_program.reset( nullptr );
    g_vertexShader.reset( nullptr );
    g_vertexShaderTemplate.reset( nullptr );
    g_vertexShaderSource.reset( nullptr );
    g_fragmentShader.reset( nullptr );
    g_fragmentShaderTemplate.reset( nullptr );
    g_fragmentShaderSource.reset( nullptr );
}


static void glfwErrorCallback( int error, const char* description )
{
    spdlog::error( "[glfw] error {} : {}", error, description );
}



// ---------------------------------------------------------------------------------------------------------------------
int main( int argc, char** argv )
{
    glfwSetErrorCallback( glfwErrorCallback );
    if ( !glfwInit() )
    {
        spdlog::error( "[glfw] failed to initialise" );
        exit( 1 );
    }

    glfwDefaultWindowHints();
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, 1 );
    glfwWindowHint( GLFW_SAMPLES, 2 );
    glfwWindowHint( GLFW_DOUBLEBUFFER, 1 );

    GLFWwindow* m_GlfwWindow;
    m_GlfwWindow = glfwCreateWindow(
        1024,
        768,
        "OpenPVD viewer",
        nullptr,
        nullptr
    );
    if ( !m_GlfwWindow )
    {
        glfwTerminate();
        spdlog::error( "[glfw] unable to create main window" );
        exit( 1 );
    }
    glfwMakeContextCurrent( m_GlfwWindow );
    glfwSwapInterval( 1 );
    glfwSetWindowCenter( m_GlfwWindow );

    glbinding::initialize( glfwGetProcAddress, false );
    globjects::init( glfwGetProcAddress );

    globjects::DebugMessage::enable();
    globjects::DebugMessage::setCallback( []( const globjects::DebugMessage& message ) 
        {
            spdlog::debug( "GL : {}", message.toString() );
        });

    spdlog::info( "OpenGL {} | {} | {}", 
        glbinding::aux::ContextInfo::version().toString(),
        glbinding::aux::ContextInfo::vendor(),
        glbinding::aux::ContextInfo::renderer()
    );


    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL( m_GlfwWindow, true );
    ImGui_ImplOpenGL3_Init( nullptr );

    initialize();

    for ( ;; )
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if ( ImGui::Begin( "Test" ) )
        {
            ImGui::Text( "Wotcha" );
        }
        ImGui::End();

        ImGui::Render();

        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        glViewport( 0, 0, 1024, 768 );
        glClearColor( 0.0f, 0.0f, 0.5f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );
        g_quad->draw();

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

        glfwSwapBuffers( m_GlfwWindow );
        glfwPollEvents();

        if ( glfwWindowShouldClose( m_GlfwWindow ) != 0 )
            break;
    }

    deinitialize();

    glfwTerminate();
}
