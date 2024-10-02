/**
* Author:  Yanka Sikder
* Assignment: Simple 2D Scene
* Date due: 2024-10-28, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 600 * 2,
              WINDOW_HEIGHT = 360 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char BG_SPRITE_FILEPATH[]    = "SV_BG.png",
               CAT1_SPRITE_FILEPATH[]    = "cat1.png",
               CAT2_SPRITE_FILEPATH[]    = "cat2.png",
               BALL_SPRITE_FILEPATH[]    = "strawb.png";
            

constexpr glm::vec3 INIT_SCALE       = glm::vec3(2.0f, 1.98f, 0.0f),
                    BG_SCALE       = glm::vec3(10.5f, 8.98f, 0.0f),
                    BALL_SCALE       = glm::vec3(-1.0f, 1.0f, 0.0f),
                    INIT_POS_BG    = glm::vec3(0.0f, .5f, 0.0f),
                    INIT_POS_CAT1    = glm::vec3(-4.0f, 0.0f, 0.0f),
                    INIT_POS_CAT2    = glm::vec3(4.0f, 0.0f, 0.0f),
                    INIT_POS_BALL    = glm::vec3(0.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

// global variables needed
glm::vec3 g_cat1_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_cat2_position = glm::vec3(0.0f, 0.0f, 0.0f);

// for the ball
glm::vec3 g_ball_position = INIT_POS_BALL;
glm::vec3 g_ball_velocity = glm::vec3(1.0f, 1.0f, 0.0f); // spawn & start with a diagonal movement

float g_ball_speed = 3.5f;  // Ball speed multiplier



SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
            g_bg_matrix,
            g_cat1_matrix,
            g_cat2_matrix,
            g_ball_matrix,
            g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_bg    = glm::vec3(0.0f, 0.0f, 0.0f),
            g_rotation_cat1    = glm::vec3(0.0f, 0.0f, 0.0f),
            g_rotation_cat2    = glm::vec3(0.0f, 0.0f, 0.0f),
            g_rotation_ball    = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_bg_texture_id,
       g_cat1_texture_id,
       g_cat2_texture_id,
       g_ball_texture_id;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Moana, Maui, HeiHei & the Kakamoras!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_bg_matrix       = glm::mat4(1.0f);
    g_cat1_matrix       = glm::mat4(1.0f);
    g_cat2_matrix       = glm::mat4(1.0f);
    g_ball_matrix       = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_bg_texture_id   = load_texture(BG_SPRITE_FILEPATH);
    g_cat1_texture_id   = load_texture(CAT1_SPRITE_FILEPATH);
    g_cat2_texture_id   = load_texture(CAT2_SPRITE_FILEPATH);
    g_ball_texture_id   = load_texture(BALL_SPRITE_FILEPATH);
    
    // generating the background here so it isnt affected in the update fx with all the other items
    g_bg_matrix = glm::translate(glm::mat4(1.0f), INIT_POS_BG);
    g_bg_matrix = glm::scale(g_bg_matrix, BG_SCALE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input(float delta_time)
{
    // Poll events for quit and close events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Cat1 (WASD Controls)
    glm::vec3 cat1_movement = glm::vec3(0.0f);
    // Vertical movement
    if (key_state[SDL_SCANCODE_W] && g_cat1_position.y + INIT_POS_CAT1.y + 1.0f < 3.75f) {
        cat1_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_S] && g_cat1_position.y + INIT_POS_CAT1.y - 1.0f > -3.75f) {
        cat1_movement.y = -1.0f;
    }
    // Horizontal movement, constrained by central boundary
    if (key_state[SDL_SCANCODE_A] && g_cat1_position.x + INIT_POS_CAT1.x - 1.0f > -5.0f) {
        cat1_movement.x = -1.0f;
    } else if (key_state[SDL_SCANCODE_D] && g_cat1_position.x + INIT_POS_CAT1.x + 1.0f < -0.5f) {
        cat1_movement.x = 1.0f;
    }

    // Cat2 (Arrow Keys)
    glm::vec3 cat2_movement = glm::vec3(0.0f);
    // Vertical movement
    if (key_state[SDL_SCANCODE_UP] && g_cat2_position.y + INIT_POS_CAT2.y + 1.0f < 3.75f) {
        cat2_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_DOWN] && g_cat2_position.y + INIT_POS_CAT2.y - 1.0f > -3.75f) {
        cat2_movement.y = -1.0f;
    }
    // Horizontal movement, constrained by central boundary
    if (key_state[SDL_SCANCODE_LEFT] && g_cat2_position.x + INIT_POS_CAT2.x - 1.0f > 0.5f) {
        cat2_movement.x = -1.0f;
    } else if (key_state[SDL_SCANCODE_RIGHT] && g_cat2_position.x + INIT_POS_CAT2.x + 1.0f < 5.0f) {
        cat2_movement.x = 1.0f;
    }

    // Update global movement variables based on delta_time for smooth movement
    g_cat1_position += cat1_movement * delta_time;
    g_cat2_position += cat2_movement * delta_time;
}


void update() {
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    // Calculate the next position
    glm::vec3 next_ball_position = g_ball_position + g_ball_velocity * g_ball_speed * delta_time;

    // Paddle 1 (Cat1) collision
    if (next_ball_position.x - 0.5f < g_cat1_position.x + INIT_POS_CAT1.x + 0.5f &&
        next_ball_position.x + 0.5f > g_cat1_position.x + INIT_POS_CAT1.x - 0.5f &&
        next_ball_position.y < g_cat1_position.y + INIT_POS_CAT1.y + 1.0f &&
        next_ball_position.y > g_cat1_position.y + INIT_POS_CAT1.y - 1.0f)
    {
        g_ball_velocity.x = fabs(g_ball_velocity.x);  // Ensure positive x-velocity
        g_ball_velocity.y = g_ball_velocity.y * -1.0f;  // Invert y-velocity to reflect bounce direction
        next_ball_position.x = g_cat1_position.x + INIT_POS_CAT1.x + 0.6f;  // Adjust position to avoid overlap
    }

    // Paddle 2 (Cat2) collision
    if (next_ball_position.x + 0.5f > g_cat2_position.x + INIT_POS_CAT2.x - 0.5f &&
        next_ball_position.x - 0.5f < g_cat2_position.x + INIT_POS_CAT2.x + 0.5f &&
        next_ball_position.y < g_cat2_position.y + INIT_POS_CAT2.y + 1.0f &&
        next_ball_position.y > g_cat2_position.y + INIT_POS_CAT2.y - 1.0f)
    {
        g_ball_velocity.x = -fabs(g_ball_velocity.x);  // Ensure negative x-velocity
        g_ball_velocity.y = g_ball_velocity.y * -1.0f;  // Invert y-velocity for proper bounce
        next_ball_position.x = g_cat2_position.x + INIT_POS_CAT2.x - 0.6f;  // Adjust position to avoid overlap
    }

    // Ball collision with top and bottom boundaries
    if (next_ball_position.y + 0.5f > 3.75f || next_ball_position.y - 0.5f < -3.75f) {
        g_ball_velocity.y = -g_ball_velocity.y;  // Reverse y-velocity
    }

    // Ball goes out of bounds (off-screen)
    if (next_ball_position.x + 0.5f > 5.0f || next_ball_position.x - 0.5f < -5.0f) {
        g_ball_position = INIT_POS_BALL;
        g_ball_velocity = glm::vec3((rand() % 2 == 0 ? 1.0f : -1.0f), (rand() % 2 == 0 ? 1.0f : -1.0f), 0.0f);
        return;
    }

    // Update ball position
    g_ball_position = next_ball_position;

    // Update ball matrix
    g_ball_matrix = glm::translate(glm::mat4(1.0f), g_ball_position);
    g_ball_matrix = glm::scale(g_ball_matrix, BALL_SCALE);

    // Update paddle positions as before
    g_cat1_matrix = glm::translate(glm::mat4(1.0f), INIT_POS_CAT1 + g_cat1_position);
    g_cat2_matrix = glm::translate(glm::mat4(1.0f), INIT_POS_CAT2 + g_cat2_position);

    g_cat1_matrix = glm::scale(g_cat1_matrix, INIT_SCALE);
    g_cat2_matrix = glm::scale(g_cat2_matrix, INIT_SCALE);
}



void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_bg_matrix, g_bg_texture_id);
    draw_object(g_cat1_matrix, g_cat1_texture_id);
    draw_object(g_cat2_matrix, g_cat2_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        // Delta time calculations
        float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
        float delta_time = ticks - g_previous_ticks;
        g_previous_ticks = ticks;

        process_input(delta_time); // Pass delta_time here
        update();
        render();
    }

    shutdown();
    return 0;
}
