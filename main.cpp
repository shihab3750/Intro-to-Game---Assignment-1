/**
* Author: Safin Shihab
* Assignment: Lunar Lander
* Date due: 2023-11-08, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 10
#define ACCEL -2.0f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;

};

// ––––– CONSTANTS ––––– //
const int   WINDOW_WIDTH = 640,
            WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int   VIEWPORT_X = 0,
            VIEWPORT_Y = 0,
            VIEWPORT_WIDTH = WINDOW_WIDTH,
            VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char  V_SHADER_PATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/shaders/vertex_textured.glsl",
            F_SHADER_PATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND  = 1000.0;
const char  SPRITESHEET_FILEPATH[]  = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/assets/rocket_ship.png",
            PLATFORM_FILEPATH[]     = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/assets/platformPack_tile027.png",
         PLATFORM_TROPHY_FILEPATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/assets/platform_trophy.png",
            FONT_FILEPAHT[]         ="/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/assets/font1.png";


const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;
const int FONTBANK_SIZE = 16;

// ––––– GLOBAL VARIABLES ––––– //
GameState   g_game_state;
GLuint      g_font_texture_id;

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_game_is_actually_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;


// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}



void DrawText(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void initialise()
{
    // Initialising both the video
    // We did something similar when we talked about video game controllers
    SDL_Init(SDL_INIT_VIDEO);
        g_display_window = SDL_CreateWindow("Hello, Lunar Lander",
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

        g_view_matrix = glm::mat4(1.0f);
        g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

        g_shader_program.set_projection_matrix(g_projection_matrix);
        g_shader_program.set_view_matrix(g_view_matrix);

        glUseProgram(g_shader_program.get_program_id());

        glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);


    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);


    g_game_state.platforms = new Entity[PLATFORM_COUNT];

    for (int i = 0; i < PLATFORM_COUNT; i++)
    {   g_game_state.platforms[i].m_texture_id = load_texture(PLATFORM_FILEPATH);
        if (i==3)
        {
            g_game_state.platforms[i].m_texture_id = load_texture(PLATFORM_TROPHY_FILEPATH);
        }
        g_game_state.platforms[i].set_position(glm::vec3(i - 4.5f, -3.25f, 0.0f));
        g_game_state.platforms[i].update(0.0f, NULL, 0);
    }

    // ––––– PLAYER (GEORGE) ––––– //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(-2.5f, 3.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_speed(.76f);
    g_game_state.player->set_acceleration(glm::vec3(0.0f, -.95f, 0.0f));
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    //font
    g_font_texture_id = load_texture(FONT_FILEPAHT);
    // Jumping
    g_game_state.player->set_jumping_power(0.93f);


    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_UP:
                // Jump
                if (g_game_state.player->m_ship_fuel > 0) {
                    g_game_state.player->m_ship_fuel -= 90.0f;
                    if (g_game_state.player->m_ship_fuel < 0 )g_game_state.player->m_ship_fuel = 0.0f;
                    g_game_state.player->m_is_jumping = true;
                }
                else{
                    g_game_state.player->m_is_jumping = false;
                }
                break;
            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        if (g_game_state.player->m_ship_fuel > 0) {
            g_game_state.player->left_trigger = true;
            g_game_state.player->m_ship_fuel -= .5f;
            g_game_state.player->set_acceleration_x(-5.5f);
            g_game_state.player->move_left();
            if (g_game_state.player->m_ship_fuel < 0 )g_game_state.player->m_ship_fuel = 0.0f;
        }
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        if (g_game_state.player->m_ship_fuel > 0) {
            g_game_state.player->right_trigger = true;
            g_game_state.player->m_ship_fuel -= .5f;
            g_game_state.player->set_acceleration_x(5.5f);
            g_game_state.player->move_right();
            if (g_game_state.player->m_ship_fuel < 0 )g_game_state.player->m_ship_fuel = 0.0f;
        }
    }
    else
    {
        g_game_state.player->left_trigger = false;
        g_game_state.player->right_trigger = false;
    }
    
    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void sleep(float seconds){
//    To add delay ;
    clock_t startClock = clock();
    float secondsAhead = seconds * CLOCKS_PER_SEC;
    while(clock() < startClock+secondsAhead);
    return;
}

void update()
{
    if (!g_game_is_actually_running)
    {
        sleep(2);
        g_game_is_running = false;
    }
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_time_accumulator;
//    std::cout<< g_game_state.player->m_ship_fuel<<std::endl;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        
        // Left boundary
        if (g_game_state.player->get_position().x - g_game_state.player->get_width() / 2.0f < -5.0F)
        {
            g_game_state.player->set_position(glm::vec3(-5.0f + g_game_state.player->get_width() / 2.0f, g_game_state.player->get_position().y, 0.0f));
        }
        
        // Right boundary
        if (g_game_state.player->get_position().x + g_game_state.player->get_width() / 2.0f > 5.0F)
        {
            g_game_state.player->set_position(glm::vec3(5.0f - g_game_state.player->get_width() / 2.0f, g_game_state.player->get_position().y, 0.0f));
        }
        
        // Notice that we're using FIXED_TIMESTEP as our delta time
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_time_accumulator = delta_time;
}



void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    g_game_state.player->render(&g_shader_program);
    DrawText(&g_shader_program, g_font_texture_id, std::string("Fuel:"), .4f, 0.0f, glm::vec3(0.0f, 3.00f,0.0f));
    DrawText(&g_shader_program, g_font_texture_id, std::to_string(g_game_state.player->m_ship_fuel), .4f, 0.0f, glm::vec3(2.0f, 3.00f,0.0f));
    for (int i = 0; i < PLATFORM_COUNT; i++) g_game_state.platforms[i].render(&g_shader_program);
    
    // if landed
    if (g_game_state.player->m_collided_bottom){
        if(g_game_state.player->get_position().x >= -1.95f && g_game_state.player->get_position().x <= -1.1f){
            DrawText(&g_shader_program, g_font_texture_id, std::string("Mission Accomplished"), .4f, 0.0f, glm::vec3(-3.5f, 0.001f,0.0f));
            g_game_is_actually_running = false;
        }
        else
        {
            DrawText(&g_shader_program, g_font_texture_id, std::string("Mission Failed"), .4f, 0.0f, glm::vec3(-2.3f, 0.001f,0.0f));
            g_game_is_actually_running = false;
        }
        
    }
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

}

// ––––– GAME LOOP ––––– //
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
