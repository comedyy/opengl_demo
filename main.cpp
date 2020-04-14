/******************************************************************************\
| OpenGL 4 Example Code.                                                       |
| Accompanies written series "Anton's OpenGL 4 Tutorials"                      |
| Email: anton at antongerdelan dot net                                        |
| First version 27 Jan 2014                                                    |
| Copyright Dr Anton Gerdelan, Trinity College Dublin, Ireland.                |
| See individual libraries separate legal notices                              |
|******************************************************************************|
| Cube Maps                                                                    |
| You can swap the "reflect_vs.glsl" and "reflect_fs.glsl" for the refraction  |
| versions. Comment one set out and uncomment the other                        |
\******************************************************************************/
#include "gl_utils.h"    // common opengl functions and small utilities like logs
#include "maths_funcs.h" // my maths functions
#include "obj_parser.h"  // my little Wavefront .obj mesh loader
#include "stb_image.h"   // Sean Barrett's image loader - nothings.org
#include "GL/glew.h"     // include GLEW and new version of GL on Windows
#include "GLFW/glfw3.h"  // GLFW helper library
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#define MESH_FILE "suzanne.obj"

/* choose pure reflection or pure refraction here. */
#define MONKEY_VERT_FILE "reflect_vs.glsl"
#define MONKEY_FRAG_FILE "reflect_fs.glsl"
//#define MONKEY_VERT_FILE "refract_vs.glsl"
//#define MONKEY_FRAG_FILE "refract_fs.glsl"

#define CUBE_VERT_FILE "cube_vs.glsl"
#define CUBE_FRAG_FILE "cube_fs.glsl"
#define FRONT "negz.jpg"
#define BACK "posz.jpg"
#define TOP "posy.jpg"
#define BOTTOM "negy.jpg"
#define LEFT "negx.jpg"
#define RIGHT "posx.jpg"

// keep track of window size for things like the viewport and the mouse cursor
int g_gl_width      = 640;
int g_gl_height     = 480;
GLFWwindow* g_window = NULL;

/* big cube. returns Vertex Array Object */
GLuint make_big_cube() {
  float points[] = {
		-10.0f, 10.0f,	-10.0f, -10.0f, -10.0f, -10.0f, 10.0f,	-10.0f, -10.0f,
		10.0f,	-10.0f, -10.0f, 10.0f,	10.0f,	-10.0f, -10.0f, 10.0f,	-10.0f,

		-10.0f, -10.0f, 10.0f,	-10.0f, -10.0f, -10.0f, -10.0f, 10.0f,	-10.0f,
		-10.0f, 10.0f,	-10.0f, -10.0f, 10.0f,	10.0f,	-10.0f, -10.0f, 10.0f,

		10.0f,	-10.0f, -10.0f, 10.0f,	-10.0f, 10.0f,	10.0f,	10.0f,	10.0f,
		10.0f,	10.0f,	10.0f,	10.0f,	10.0f,	-10.0f, 10.0f,	-10.0f, -10.0f,

		-10.0f, -10.0f, 10.0f,	-10.0f, 10.0f,	10.0f,	10.0f,	10.0f,	10.0f,
		10.0f,	10.0f,	10.0f,	10.0f,	-10.0f, 10.0f,	-10.0f, -10.0f, 10.0f,

		-10.0f, 10.0f,	-10.0f, 10.0f,	10.0f,	-10.0f, 10.0f,	10.0f,	10.0f,
		10.0f,	10.0f,	10.0f,	-10.0f, 10.0f,	10.0f,	-10.0f, 10.0f,	-10.0f,

		-10.0f, -10.0f, -10.0f, -10.0f, -10.0f, 10.0f,	10.0f,	-10.0f, -10.0f,
		10.0f,	-10.0f, -10.0f, -10.0f, -10.0f, 10.0f,	10.0f,	-10.0f, 10.0f
	};
	GLuint vbo;
  glGenBuffers( 1, &vbo );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glBufferData( GL_ARRAY_BUFFER, 3 * 36 * sizeof( GLfloat ), &points, GL_STATIC_DRAW );

  GLuint vao;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );
  glEnableVertexAttribArray( 0 );
  glBindBuffer( GL_ARRAY_BUFFER, vbo );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
  return vao;
}

/* use stb_image to load an image file into memory, and then into one side of
a cube-map texture. */
bool load_cube_map_side( GLuint texture, GLenum side_target, const char* file_name ) {
  glBindTexture( GL_TEXTURE_CUBE_MAP, texture );

  int x, y, n;
  int force_channels        = 4;
  unsigned char* image_data = stbi_load( file_name, &x, &y, &n, force_channels );
  if ( !image_data ) {
    fprintf( stderr, "ERROR: could not load %s\n", file_name );
    return false;
  }
  // non-power-of-2 dimensions check
  if ( ( x & ( x - 1 ) ) != 0 || ( y & ( y - 1 ) ) != 0 ) { fprintf( stderr, "WARNING: image %s is not power-of-2 dimensions\n", file_name ); }

  // copy image data into 'target' side of cube map
  glTexImage2D( side_target, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data );
  free( image_data );
  return true;
}

/* load all 6 sides of the cube-map from images, then apply formatting to the
final texture */
void create_cube_map( const char* front, const char* back, const char* top, const char* bottom, const char* left, const char* right, GLuint* tex_cube ) {
  // generate a cube-map texture to hold all the sides
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, tex_cube );

  // load each image and copy into a side of the cube-map texture
  load_cube_map_side( *tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front );
  load_cube_map_side( *tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back );
  load_cube_map_side( *tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top );
  load_cube_map_side( *tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom );
  load_cube_map_side( *tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left );
  load_cube_map_side( *tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right );
  // format cube map texture
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

// camera matrices. it's easier if they are global
mat4 view_mat;
mat4 proj_mat;
vec3 cam_pos( 0.0f, 0.0f, 5.0f );

bool is_in_shoot_game_state = false;
double x_diff = 0;
double y_diff = 0;
double forward_move_diff = 0;
double back_move_diff = 0;
double right_move_diff = 0;
double left_move_diff = 0;
double x_pre = 0;
double y_pre = 0;
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(is_in_shoot_game_state){
        x_diff = xpos - x_pre;
        y_diff = ypos - y_pre;
        x_diff *= -10;
        y_diff *= -10;
        x_pre = xpos;
        y_pre = ypos;
        //std::cout<< xpos << " " << ypos << " " << x_diff << " " << y_diff << std::endl;
        //glfwSetCursorPos(g_window, g_gl_width / 2, g_gl_height / 2);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
        is_in_shoot_game_state = true;
			  glfwGetCursorPos( g_window, &x_pre, &y_pre );
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        is_in_shoot_game_state = false;   
    }

    if ( key == GLFW_KEY_A) {
      right_move_diff = action == GLFW_RELEASE ? 0 : 1;
    }
    else if ( key == GLFW_KEY_D) {
      left_move_diff = action == GLFW_RELEASE ? 0 : 1;
    }
    else if ( key == GLFW_KEY_W) {
      forward_move_diff = action == GLFW_RELEASE ? 0 : 1;
    }
    else if ( key == GLFW_KEY_S) {
      back_move_diff = action == GLFW_RELEASE ? 0 : 1;
    }
}

int main() {
  /*--------------------------------START
   * OPENGL--------------------------------*/
  restart_gl_log();
  // start GL context and O/S window using the GLFW helper library
  start_gl();
    glfwSetCursorPosCallback(g_window, cursor_position_callback);
    glfwSetMouseButtonCallback(g_window, mouse_button_callback);
    glfwSetKeyCallback(g_window, key_callback);

  /*---------------------------------CUBE
   * MAP-----------------------------------*/
  GLuint cube_vao = make_big_cube();
  GLuint cube_map_texture;
  create_cube_map( FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT, &cube_map_texture );
  /*------------------------------CREATE
   * GEOMETRY-------------------------------*/
  GLfloat* vp       = NULL; // array of vertex points
  GLfloat* vn       = NULL; // array of vertex normals
  GLfloat* vt       = NULL; // array of texture coordinates
  int g_point_count = 0;
  ( load_obj_file( MESH_FILE, vp, vt, vn, g_point_count ) );

  GLuint vao;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );

  GLuint points_vbo, normals_vbo;
  if ( NULL != vp ) {
    glGenBuffers( 1, &points_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, points_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * g_point_count * sizeof( GLfloat ), vp, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
  }
  if ( NULL != vn ) {
    glGenBuffers( 1, &normals_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, normals_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * g_point_count * sizeof( GLfloat ), vn, GL_STATIC_DRAW );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 1 );
  }

  /*-------------------------------CREATE
   * SHADERS-------------------------------*/
  // shaders for "Suzanne" mesh
  GLuint monkey_sp      = create_programme_from_files( MONKEY_VERT_FILE, MONKEY_FRAG_FILE );
  int monkey_M_location = glGetUniformLocation( monkey_sp, "M" );
  int monkey_V_location = glGetUniformLocation( monkey_sp, "V" );
  int monkey_P_location = glGetUniformLocation( monkey_sp, "P" );

  // cube-map shaders
  GLuint cube_sp = create_programme_from_files( CUBE_VERT_FILE, CUBE_FRAG_FILE );
  // note that this view matrix should NOT contain camera translation.
  int cube_V_location = glGetUniformLocation( cube_sp, "V" );
  int cube_P_location = glGetUniformLocation( cube_sp, "P" );

/*-------------------------------CREATE CAMERA--------------------------------*/
#define ONE_DEG_IN_RAD ( 2.0 * M_PI ) / 360.0 // 0.017444444
  // input variables
  float cam_near = 0.1f;                                     // clipping plane
  float cam_far  = 100.0f;                                   // clipping plane
  float fovy     = 67.0f;                                    // 67 degrees
  float aspect   = (float)g_gl_width / (float)g_gl_height; // aspect ratio
  proj_mat       = perspective( fovy, aspect, cam_near, cam_far );

  float cam_speed         = 3.0f;  // 1 unit per second
  float cam_heading_speed = 50.0f; // 30 degrees per second
  float cam_heading       = 0.0f;  // y-rotation in degrees
  mat4 T                  = translate( identity_mat4(), vec3( -cam_pos.v[0], -cam_pos.v[1], -cam_pos.v[2] ) );
  mat4 R                  = rotate_y_deg( identity_mat4(), -cam_heading );
  versor q                = quat_from_axis_deg( -cam_heading, 0.0f, 1.0f, 0.0f );
  view_mat                = R * T;
  // keep track of some useful vectors that can be used for keyboard movement
  vec4 fwd( 0.0f, 0.0f, -1.0f, 0.0f );
  vec4 rgt( 1.0f, 0.0f, 0.0f, 0.0f );
  vec4 up( 0.0f, 1.0f, 0.0f, 0.0f );

  /*---------------------------SET RENDERING
   * DEFAULTS---------------------------*/
  glUseProgram( monkey_sp );
  glUniformMatrix4fv( monkey_V_location, 1, GL_FALSE, view_mat.m );
  glUniformMatrix4fv( monkey_P_location, 1, GL_FALSE, proj_mat.m );
  glUseProgram( cube_sp );
  glUniformMatrix4fv( cube_V_location, 1, GL_FALSE, R.m );
  glUniformMatrix4fv( cube_P_location, 1, GL_FALSE, proj_mat.m );
  // unique model matrix for each sphere
  mat4 model_mat = identity_mat4();

  glEnable( GL_DEPTH_TEST );          // enable depth-testing
  glDepthFunc( GL_LESS );             // depth-testing interprets a smaller value as "closer"
  glEnable( GL_CULL_FACE );           // cull face
  glCullFace( GL_BACK );              // cull back face
  glFrontFace( GL_CCW );              // set counter-clock-wise vertex order to mean the front
  glClearColor( 0.2, 0.2, 0.2, 1.0 ); // grey background to help spot mistakes

  /*-------------------------------RENDERING
   * LOOP-------------------------------*/
  while ( !glfwWindowShouldClose( g_window ) ) {
    // update timers
    static double previous_seconds = glfwGetTime();
    double current_seconds         = glfwGetTime();
    double elapsed_seconds         = current_seconds - previous_seconds;
    previous_seconds               = current_seconds;
    _update_fps_counter( g_window );

    int fb_width, fb_height;
    glfwGetFramebufferSize( g_window, &fb_width, &fb_height );
    glViewport( 0, 0, fb_width, fb_height );
    float aspect = (float)fb_width / (float)fb_height; // aspect ratio
    proj_mat     = perspective( fovy, aspect, cam_near, cam_far );

    // wipe the drawing surface clear
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // render a sky-box using the cube-map texture
    glDepthMask( GL_FALSE );
    glUseProgram( cube_sp );
  	glUniformMatrix4fv( cube_P_location, 1, GL_FALSE, proj_mat.m );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, cube_map_texture );
    glBindVertexArray( cube_vao );
    glDrawArrays( GL_TRIANGLES, 0, 36 );
    glDepthMask( GL_TRUE );

    glUseProgram( monkey_sp );
    glBindVertexArray( vao );
    glUniformMatrix4fv( monkey_M_location, 1, GL_FALSE, model_mat.m );
  	glUniformMatrix4fv( monkey_P_location, 1, GL_FALSE, proj_mat.m );
    glDrawArrays( GL_TRIANGLES, 0, g_point_count );
    // update other events like input handling
    glfwPollEvents();

    // control keys
    bool cam_moved = false;
    vec3 move( 0.0, 0.0, 0.0 );
    float cam_yaw   = 0.0f; // y-rotation in degrees
    float cam_pitch = 0.0f;

    if (right_move_diff != 0 || left_move_diff != 0){
        move.v[0] += (right_move_diff -left_move_diff) * cam_speed * elapsed_seconds;
    }

    if (forward_move_diff != 0 || back_move_diff != 0){
        move.v[2] += (forward_move_diff - back_move_diff) * cam_speed * elapsed_seconds;
    }

    if (x_diff != 0 )
    {
        cam_yaw += x_diff * elapsed_seconds;
        versor q_yaw = quat_from_axis_deg( cam_yaw, up.v[0], up.v[1], up.v[2] );
        q = q_yaw * q;
    }

    if (y_diff != 0)
    {
        cam_pitch += y_diff * elapsed_seconds;
        versor q_pitch = quat_from_axis_deg( cam_pitch, rgt.v[0], rgt.v[1], rgt.v[2] );
        q = q_pitch * q;
    }

    cam_moved = x_diff != 0 || y_diff != 0 || forward_move_diff != 0 || right_move_diff != 0 || left_move_diff != 0 || back_move_diff != 0;

    // update view matrix
    if ( cam_moved ) {
      cam_heading += cam_yaw;

      // re-calculate local axes so can move fwd in dir cam is pointing
      R   = quat_to_mat4( q );
      fwd = R * vec4( 0.0, 0.0, -1.0, 0.0 );
      rgt = R * vec4( 1.0, 0.0, 0.0, 0.0 );
      up  = R * vec4( 0.0, 1.0, 0.0, 0.0 );

      cam_pos = cam_pos + vec3( fwd ) * -move.v[2];
      cam_pos = cam_pos + vec3( up ) * move.v[1];
      cam_pos = cam_pos + vec3( rgt ) * move.v[0];
      mat4 T  = translate( identity_mat4(), vec3( cam_pos ) );

      view_mat = inverse( R ) * inverse( T );
      glUseProgram( monkey_sp );
      glUniformMatrix4fv( monkey_V_location, 1, GL_FALSE, view_mat.m );

      // cube-map view matrix has rotation, but not translation
      glUseProgram( cube_sp );
      glUniformMatrix4fv( cube_V_location, 1, GL_FALSE, inverse( R ).m );
    }


    //std::cout<<cam_yaw << " " << cam_pitch<<std::endl;
    x_diff = 0;
    y_diff = 0;
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers( g_window );
  }

  // close GL context and any other GLFW resources
  glfwTerminate();
  return 0;
}