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

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include "imgui_internal.h"

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

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 2 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
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

    int gladErr = gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress );
    if ( gladErr == 0 )
    {
        spdlog::error( "[glfw] GLAD to initialise" );
        exit( 1 );
    }

    glfwSwapInterval( 1 );
    glfwSetWindowCenter( m_GlfwWindow );

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL( m_GlfwWindow, true );
    ImGui_ImplOpenGL3_Init( nullptr );

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

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

        glfwSwapBuffers( m_GlfwWindow );
        glfwPollEvents();

        if ( glfwWindowShouldClose( m_GlfwWindow ) != 0 )
            exit( 0 );
    }
}
