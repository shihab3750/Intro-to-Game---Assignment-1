/**
* Author: Safin Shihab
* Assignment: Simple 2D Scene
* Date due: 2023-09-30, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char  V_SHADER_PATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/shaders/vertex_textured.glsl",
            F_SHADER_PATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;


const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero
const char PLAYER_SPRITE_FILEPATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/ash.png";
const char POKE_SPRITE_FILEPATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/poke.png";
GLuint g_player_texture_id, g_poke_texture_id;


SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_is_growing = true;
const float TRIANGLE_INIT_ANGLE = glm::radians(45.0);
const float ROT_ANGLE = 90.0f;
int g_frame_counter = 0;
float TRAN_VALUE = 0.025f;
float g_triangle_x = 0.0f;
float g_triangle_rotate = 0.0f;
float g_previous_ticks = 0.0f;

ShaderProgram g_shader_program;
glm::mat4   view_matrix,
            m_model_matrix,
            m_model_matrix2,
            m_projection_matrix,
            m_trans_matrix;

float m_previous_ticks = 0.0f;

const float GROWTH_FACTOR = 1.01f;  // growth rate of 1.0% per frame
const float SHRINK_FACTOR = 0.99f;  // growth rate of -1.0% per frame
const int MAX_FRAME = 40;           // this value is, of course, up to you



float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * 10.0f ) - (10.0f / 2.0f);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
        default:
            return 0.0f;
    }
}

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{

    SDL_Init(SDL_INIT_VIDEO);
    
    g_display_window = SDL_CreateWindow("Let's Catch Pokemon",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix2 = glm::mat4(1.0f);
    m_model_matrix2 = glm::rotate(m_model_matrix, TRIANGLE_INIT_ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));

    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    m_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    g_shader_program.set_projection_matrix(m_projection_matrix);
    g_shader_program.set_view_matrix(view_matrix);
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    //Loading textures
    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_poke_texture_id = load_texture(POKE_SPRITE_FILEPATH);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_game_is_running = false;
        }
    }
}


void update()
{
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    g_frame_counter += 1;

    if (g_frame_counter >= MAX_FRAME)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }

    // Initialize identity matrix for clean state
    m_model_matrix = glm::mat4(1.0f);

    // Scale m_model_matrix
    glm::vec3 scale_vector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                                       g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                                       1.0f);

    m_model_matrix = glm::scale(m_model_matrix, scale_vector);

    // Translate m_model_matrix left and right
    g_triangle_x += 7.0f * TRAN_VALUE * delta_time;
    
    // Checking if m_model_matrix leaves the window
    if (g_triangle_x > 1.0f) {
        g_triangle_x = 1.0f;
        TRAN_VALUE = -TRAN_VALUE;
    } else if (g_triangle_x < -1.0f) {
        g_triangle_x = -1.0f;
        TRAN_VALUE = -TRAN_VALUE;
    }

    g_triangle_rotate += ROT_ANGLE * delta_time; // 90 degrees per second

    // Calculate the position of m_model_matrix2 relative to m_model_matrix
    float distance = 0.5f;
    float angle = glm::radians(g_triangle_rotate);
    float tri_x = g_triangle_x + distance * cos(angle);
    float tri_y = distance * sin(angle);

//    Update translation
    glm::vec3 translation_vector(g_triangle_x, 0.0f, 0.0f);
    m_model_matrix = glm::translate(m_model_matrix, translation_vector);

//    Update m_model_matrix2 position and rotation in relation to m_model_matrix
    glm::vec3 model2_position(tri_x, tri_y + 2.0f, 0.0f);
    m_model_matrix2 = glm::translate(m_model_matrix, model2_position);
    m_model_matrix2 = glm::rotate(m_model_matrix2, glm::radians(g_triangle_rotate), glm::vec3(0.0f, 0.0f, 1.0f));
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClearColor(0.0f, 0.4f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_shader_program.set_model_matrix(m_model_matrix);
    g_shader_program.set_model_matrix(m_model_matrix2);
    
    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    // Build texture
    draw_object(m_model_matrix2, g_poke_texture_id);
    draw_object(m_model_matrix, g_player_texture_id);
    
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
