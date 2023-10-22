/**
 * Author: Safin Shihab
 * Assignment: Pong Clone
 * Date due: 2023-10-21, 11:59pm
 *idea:
 *- Pong with ash an pickachu
 *- ball is a pokeball
 *- game quits when a player loses
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 **/

#define GL_GLEXT_PROTOTYPES 1
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

// —— NEW STUFF —— //
#include <ctime>   //
#include "cmath"   //
// ——————————————— //

const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char  V_SHADER_PATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/shaders/vertex_textured.glsl",
            F_SHADER_PATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/shaders/fragment_textured.glsl";

const char  PLAYER_SPRITE_FILEPATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/ash.png",
            POKE_SPRITE_FILEPATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/poke.png",
            BALL_SPRITE_FILEPATH[] = "/Users/safin/NYU/CS-Intro to game/SDLSimple/SDLSimple/ball.png";



const float MILLISECONDS_IN_SECOND     = 1000.0;
const float MINIMUM_COLLISION_DISTANCE = .5f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

SDL_Window* g_display_window;
bool  g_game_is_running = true;
float g_previous_ticks  = 0.0f;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix,
              g_player1_matrix,
              g_player2_matrix,
              g_ball_matrix,
              g_projection_matrix;
                

GLuint  g_player1_texture_id,
        g_player2_texture_id,
        g_ball_texture_id;

//initial positions
glm::vec3 g_player1_init_position = glm::vec3(-4.0f, 0.0f, 0.0f);
glm::vec3 g_player2_init_position = glm::vec3(4.0f, 0.0f, 0.0f);

glm::vec3 g_player1_position = glm::vec3(-4.0f, 0.0f, 0.0f); // Start player1 on the left side
glm::vec3 g_player1_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_player2_position = glm::vec3(4.0f, 0.0f, 0.0f); // Start player2 on the right side
glm::vec3 g_player2_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f); // Start the ball in the middle
glm::vec3 g_ball_movement = glm::vec3(-.5f, -.04f, 0.0f);

// player ai
bool g_player2_ai            = false;
glm::vec3 g_player2_ai_movement = glm::vec3(0.0f, -1.0f, 0.0f);  // move down

// winning
bool g_player1_won = false;
bool g_player2_won = false;

// collision handling flags
bool player1_collided = false;

float g_player_speed = 5.0f;

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

    stbi_image_free(image);
    
    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Collisions!",
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
    
    g_player1_matrix = glm::mat4(1.0f);
    g_player2_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    // random x movement
//    player1_collided = (rand() % 2) ? true : false;
        
    g_player1_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_player2_texture_id  = load_texture(POKE_SPRITE_FILEPATH);
    g_ball_texture_id  = load_texture(BALL_SPRITE_FILEPATH);
    

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input() {
    // if nothing is pressed
    g_player1_movement = glm::vec3(0.0f);
    g_player2_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:  // quit game
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    // Player 1
                    case SDLK_w:
                        g_player1_movement.y = -1.0f;
                        break;
                    case SDLK_s:
                        g_player1_movement.y = 1.0f;
                        break;

                    // Player 2
                    case SDLK_UP:
                        g_player2_movement.y = -1.0f;
                        break;
                    case SDLK_DOWN:
                        g_player2_movement.y = 1.0f;
                        break;

                    // Toggle AI
                    case SDLK_t:
                        g_player2_ai = !g_player2_ai;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }

    // Key hold for smoother movement
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    // player 1
    if (key_state[SDL_SCANCODE_W]) {
        g_player1_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_S]) {
        g_player1_movement.y = -1.0f;
    }

    // player 2
    if (key_state[SDL_SCANCODE_UP]) {
        g_player2_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_DOWN]) {
        g_player2_movement.y = -1.0f;
    }

    // Normalize movement
    if (glm::length(g_player1_movement) > 1.0f) {
        g_player1_movement = glm::normalize(g_player1_movement);
    }
    if (glm::length(g_player2_movement) > 1.0f) {
        g_player2_movement = glm::normalize(g_player2_movement);
    }
}




void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks; // Delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // Bound paddle to top and bottom screen edges
    if (g_player1_position.y > 3.0f) {
        g_player1_position.y = 3.0f;
    } else if (g_player1_position.y < -3.0f) {
        g_player1_position.y = -3.0f;
    }
    if (g_player2_position.y > 3.0f) {
        g_player2_position.y = 3.0f;
    } else if (g_player2_position.y < -3.0f) {
        g_player2_position.y = -3.0f;
    }

    g_player1_matrix = glm::mat4(1.0f);
    g_player1_position += g_player1_movement * g_player_speed * delta_time;

    g_player2_matrix = glm::mat4(1.0f);
    if (g_player2_ai) {
        if (g_player2_position.y >= 3.0f) {
            g_player2_ai_movement.y = -1.0f;
        } else if (g_player2_position.y <= -3.0f) {
            g_player2_ai_movement.y = 1.0f;
        }
        g_player2_position += g_player2_ai_movement * g_player_speed * delta_time;
    } else {
        g_player2_position += g_player2_movement * g_player_speed * delta_time;
    }

    g_player1_matrix = glm::translate(g_player1_matrix, g_player1_position);
    g_player2_matrix = glm::translate(g_player2_matrix, g_player2_position);

    g_ball_matrix = glm::mat4(1.0f);

    g_ball_position += g_ball_movement * g_player_speed * delta_time;

    float collision_factor = 1.0f; // Adjust this factor for collision accuracy

    float x1_distance = fabs(g_player1_position.x - g_ball_position.x) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float y1_distance = fabs(g_player1_position.y - g_ball_position.y) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float x2_distance = fabs(g_player2_position.x - g_ball_position.x) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);
    float y2_distance = fabs(g_player2_position.y - g_ball_position.y) - ((1 * collision_factor + 1 * collision_factor) / 2.0f);

    // Collision with player 1
    if (x1_distance < 0.0f && y1_distance < 0.0f) {
        g_ball_movement.x = fabs(g_ball_movement.x); // Reverse the ball's horizontal direction
    }

    // Collision with player 2
    if (x2_distance < 0.0f && y2_distance < 0.0f) {
        g_ball_movement.x = -fabs(g_ball_movement.x); // Reverse the ball's horizontal direction
    }

    if (g_ball_position.y > 2.8f || g_ball_position.y < -2.8f) {
        g_ball_movement.y *= -1.0f; // Reverse the ball's vertical direction
    }

    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    if (g_ball_position.x < -6.0f || g_ball_position.x > 6.0f) {
        g_game_is_running = false;
    }
}




void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_player1_matrix, g_player1_texture_id);
    draw_object(g_player2_matrix, g_player2_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


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
