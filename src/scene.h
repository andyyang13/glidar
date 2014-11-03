/*
 * Copyright (c) 2014, John O. Woods, Ph.D.
 *   West Virginia University Applied Space Exploration Lab
 *   West Virginia Robotic Technology Center
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#ifndef SCENE_H
# define SCENE_H

#define GLM_FORCE_RADIANS

#include <Magick++.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cmath>
#include "mesh.h"

#define _USE_MATH_DEFINES

const float ASPECT_RATIO = 1.0;
const float CAMERA_Y     = 0.05;
const unsigned int BOX_HALF_DIAGONAL = 174;
// const GLfloat MIN_NEAR_PLANE = 0.01; // now defined in mesh.h

const double RADIANS_PER_DEGREE = M_PI / 180.0;
const float NEAR_PLANE_FACTOR = 0.99;
const float FAR_PLANE_FACTOR = 1.01;


class Scene {
public:
  Scene(const std::string& filename, float scale_factor_, float camera_d_)
  : scale_factor(scale_factor_),
    projection(1.0),
    camera_d(camera_d_),
    near_plane_bound(camera_d_ - BOX_HALF_DIAGONAL),
    real_near_plane(std::max(MIN_NEAR_PLANE, camera_d_-BOX_HALF_DIAGONAL)),
    far_plane(camera_d_+BOX_HALF_DIAGONAL)
  {
    mesh.load_mesh(filename);

    glm::vec3 dimensions = mesh.dimensions();
    std::cerr << "Object dimensions as modeled: " << dimensions.x << '\t' << dimensions.y << '\t' << dimensions.z << std::endl;
    glm::vec3 centroid = mesh.centroid();
    std::cerr << "Center of object as modeled: " << centroid.x << '\t' << centroid.y << '\t' << centroid.z << std::endl;
    std::cerr << "NOTE: Object will be re-centered prior to rendering." << std::endl;
  }


  void gl_setup() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D); // Probably has no meaning since we're using shaders.
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    //glBlendFuncSeparate(GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_SRC_ALPHA);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDepthFunc(GL_LEQUAL);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glPolygonMode( GL_FRONT, GL_FILL );
  }



  void projection_setup(float fov, float model_rx, float model_ry, float model_rz, float cam_x, float cam_y, float cam_z, float cam_rx, float cam_ry, float cam_rz) {
    gl_setup();

    // Figure out where the near plane belongs.
    //glGetFloatv(GL_MODELVIEW_MATRIX, static_cast<GLfloat*>(glm::value_ptr(model_view)));
    glm::mat4 model = get_model_matrix(model_rx, model_ry, model_rz);

    // Get the camera position in model coordinates so we can find the near plane.
    glm::mat4 inverse_model = glm::inverse(model);
    glm::vec4 camera_pos    = glm::vec4(cam_x, cam_y, cam_z, 1.0f);
    glm::vec4 camera_pos_mc = inverse_model * camera_pos;
    //glm::vec4 camera_dir_oc = inverse_model_view * camera_dir;

    near_plane_bound = mesh.near_plane_bound(model, camera_pos_mc);
    real_near_plane = near_plane_bound * NEAR_PLANE_FACTOR;
    far_plane = mesh.far_plane_bound(model, camera_pos_mc) * FAR_PLANE_FACTOR;
    std::cerr << "Near plane: " << near_plane_bound << "\tFar plane: " << far_plane << std::endl;

    projection =  glm::perspective((float)(fov * M_PI / 180.0f), (float)(ASPECT_RATIO), (float)(real_near_plane), (float)(far_plane));
    //gluPerspective(fov, ASPECT_RATIO, real_near_plane, far_plane);

    // Store a copy of the current projection matrix.
    //glGetFloatv( GL_PROJECTION_MATRIX, glm::value_ptr(projection));
  }


  void projection_setup(float fov, const glm::mat4& inverse_model, const glm::mat4& view_physics) {
    gl_setup();

    glm::mat4 inverse_view = glm::inverse(view_physics);

    glm::vec4 camera_pos_mc = inverse_model * inverse_view * glm::vec4(0.0, 0.0, 0.0, 1.0);
    
    glm::mat4 model = glm::inverse(inverse_model);
    
    near_plane_bound = mesh.near_plane_bound(model, camera_pos_mc);
    real_near_plane = near_plane_bound * NEAR_PLANE_FACTOR;
    far_plane = mesh.far_plane_bound(model, camera_pos_mc) * FAR_PLANE_FACTOR;
    std::cerr << "Near plane: " << near_plane_bound << "\tFar plane: " << far_plane << std::endl;

    projection = glm::perspective<float>(fov * M_PI / 180.0, ASPECT_RATIO, real_near_plane, far_plane);
  }


  void gl_setup_lighting(Shader* shader_program) {
    float light_position[] = {0.0, 0.0, 0.0, 1.0};
    float light_direction[] = {0.0, 0.0, 1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 10.0);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0001f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00000001f);

    // Use an identity matrix for lighting.
    glm::mat4 light_matrix(1);
    GLint light_matrix_id = glGetUniformLocation(shader_program->id(), "LightModelViewMatrix");
    glUniformMatrix4fv(light_matrix_id, 1, false, static_cast<GLfloat*>(glm::value_ptr(light_matrix)));
  }


  void render(Shader* shader_program, float fov, const glm::mat4& inverse_model_physics, const glm::mat4& view_physics) {
    glm::mat4 inverse_model = glm::scale(glm::mat4(1), glm::vec3(1.0/scale_factor, 1.0/scale_factor, 1.0/scale_factor)) *
      inverse_model_physics;
    projection_setup(fov, inverse_model, view_physics);

    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_program->id());

    gl_setup_lighting(shader_program);

    glm::mat4 model = glm::inverse(inverse_model);//model_physics * glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor, scale_factor, scale_factor));
    std::cerr << "Model: " << glm::to_string(model) << std::endl;
    std::cerr << "View: " << glm::to_string(view_physics) << std::endl;

    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");
    GLint near_plane_id = glGetUniformLocation(shader_program->id(), "near_plane");

    glUniform1fv(far_plane_id, 1, &far_plane);
    glUniform1fv(near_plane_id, 1, &real_near_plane);

    GLint v_id = glGetUniformLocation(shader_program->id(), "ViewMatrix");
    glUniformMatrix4fv(v_id, 1, GL_FALSE, &view_physics[0][0]);
    
    glm::mat4 model_view = view_physics * model;
    GLint mv_id = glGetUniformLocation(shader_program->id(), "ModelViewMatrix");
    glUniformMatrix4fv(mv_id, 1, GL_FALSE, &model_view[0][0]);

    glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(model_view));
    GLint normal_id = glGetUniformLocation(shader_program->id(), "NormalMatrix");
    glUniformMatrix3fv(normal_id, 1, false, static_cast<GLfloat*>(glm::value_ptr(normal_matrix)));

    std::cerr << "Model view: " << glm::to_string(model_view) << std::endl;
    
    glm::mat4 model_view_projection = projection * model_view;
    
    GLint mvp_id = glGetUniformLocation(shader_program->id(), "ModelViewProjectionMatrix");
    glUniformMatrix4fv(mvp_id, 1, GL_FALSE, &model_view_projection[0][0]);

    mesh.render(shader_program);

    check_gl_error();

    glFlush();
  }
  

  void render(Shader* shader_program, float fov, const glm::dquat& model_q, const glm::dvec3& translate, const glm::dquat& camera_q) {
    glm::mat4 view_physics  = glm::mat4(glm::mat4_cast(camera_q) * glm::translate(glm::dmat4(1.0), translate));
    glm::mat4 inverse_model = glm::mat4(glm::mat4_cast(glm::inverse(model_q)));

    render(shader_program, fov, inverse_model, view_physics);
  }

  
  void render(Shader* shader_program, float fov, float model_rx, float model_ry, float model_rz, float camera_x, float camera_y, float camera_z, float camera_rx, float camera_ry, float camera_rz) {
    projection_setup(fov, model_rx, model_ry, model_rz, camera_x, camera_y, camera_z, camera_rx, camera_ry, camera_rz);

    // clear window with the current clearing color, and clear the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup model view matrix: All future transformations will affect our models (what we draw).
    glUseProgram(shader_program->id());

    gl_setup_lighting(shader_program);

    GLint far_plane_id = glGetUniformLocation(shader_program->id(), "far_plane");
    GLint near_plane_id = glGetUniformLocation(shader_program->id(), "near_plane");

    glUniform1fv(far_plane_id, 1, &far_plane);
    glUniform1fv(near_plane_id, 1, &real_near_plane);

    glm::mat4 view = get_view_matrix(camera_x, camera_y, camera_z, camera_rx, camera_ry, camera_rz);
    GLint v_id = glGetUniformLocation(shader_program->id(), "ViewMatrix");
    glUniformMatrix4fv(v_id, 1, GL_FALSE, &view[0][0]);

    std::cerr << "View: " << glm::to_string(view) << std::endl;

    glm::mat4 model_view = get_model_view_matrix(model_rx, model_ry, model_rz, camera_x, camera_y, camera_z, camera_rx, camera_ry, camera_rz);

    GLint mv_id = glGetUniformLocation(shader_program->id(), "ModelViewMatrix");
    glUniformMatrix4fv(mv_id, 1, GL_FALSE, &model_view[0][0]);

    glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(model_view));
    GLint normal_id = glGetUniformLocation(shader_program->id(), "NormalMatrix");
    glUniformMatrix3fv(normal_id, 1, false, static_cast<GLfloat*>(glm::value_ptr(normal_matrix)));
    
    std::cerr << "Model view: " << glm::to_string(model_view) << std::endl;
    //std::cerr << "Projection: " << glm::to_string(projection) << std::endl;
    glm::mat4 model_view_projection = projection * model_view;
    GLint mvp_id = glGetUniformLocation(shader_program->id(), "ModelViewProjectionMatrix");
    glUniformMatrix4fv(mvp_id, 1, GL_FALSE, &model_view_projection[0][0]);

    // Render the mesh.
    mesh.render(shader_program);

    check_gl_error();

    // flush drawing commands
    glFlush();
  }


  /*
   * Write the translation and rotation information to a file.
   */
  void save_transformation_metadata(const std::string& basename, float model_rx, float model_ry, float model_rz, float camera_x, float camera_y, float camera_z, float camera_rx, float camera_ry, float camera_rz) {
    std::string filename = basename + ".transform";
    std::ofstream out(filename.c_str());

    out << camera_x << '\t' << camera_y << '\t' << camera_z << std::endl;
    out << model_rx << '\t' << model_ry << '\t' << model_rz << std::endl;
    out << camera_rx << '\t' << camera_ry << '\t' << camera_rz << std::endl;

    out.close();
  }


  /*
   * Return the transformation metadata as a 4x4 homogeneous matrix.
   */
  Eigen::Matrix4f get_pose(float mod_rx, float mod_ry, float mod_rz, float cam_rx, float cam_ry, float cam_rz) {
    using Eigen::Vector3f;
    using Eigen::Affine;
    using Eigen::Transform;
    using Eigen::Translation3f;
    using Eigen::AngleAxisf;
    using Eigen::Matrix4f;
    using Eigen::Scaling;
   
    AngleAxisf model_rx(mod_rx * M_PI / 180.0, Vector3f::UnitX());
    AngleAxisf model_ry(mod_ry * M_PI / 180.0, Vector3f::UnitY());
    AngleAxisf model_rz(mod_rz * M_PI / 180.0, Vector3f::UnitZ());

    AngleAxisf camera_rx(cam_rx * M_PI / 180.0, Vector3f::UnitX());
    AngleAxisf camera_ry(cam_ry * M_PI / 180.0, Vector3f::UnitY());
    AngleAxisf camera_rz(cam_rz * M_PI / 180.0, Vector3f::UnitZ());
    
    Translation3f model_to_camera_translate(Vector3f(0.0, 0.0, -camera_d));
    AngleAxisf    model_to_camera_rotate(M_PI, Vector3f::UnitY());
  
    Eigen::Transform<float,3,Affine> result;
    result =  camera_rz * camera_ry * camera_rx *
              model_to_camera_rotate *
              model_to_camera_translate *
              model_rz * model_ry * model_rx;
  
    return result.matrix();
  }

  /*
   * Get the model view matrix before the scene is rendered.
   */
  glm::mat4 get_model_view_matrix(float model_rx, float model_ry, float model_rz, float camera_x, float camera_y, float camera_z, float camera_rx, float camera_ry, float camera_rz) {
    return get_view_matrix(camera_x, camera_y, camera_z, camera_rx, camera_ry, camera_rz) * get_model_matrix(model_rx, model_ry, model_rz);
  }


  glm::dmat4 get_model_view_matrix(const glm::dquat& model, const glm::dvec3& translate, const glm::dquat& camera) {
    return get_view_matrix(translate, camera) * get_model_matrix(model);
  }

  glm::dmat4 get_view_matrix(const glm::dvec3& translate, const glm::dquat& camera) {
    return glm::mat4_cast(camera) * glm::translate(glm::dmat4(1.0), translate);
  }

  glm::dmat4 get_inverse_model_matrix(const glm::dquat& model) {
    return glm::scale(glm::dmat4(1.0), glm::dvec3(1.0/scale_factor, 1.0/scale_factor, 1.0/scale_factor)) *
      glm::mat4_cast(glm::inverse(model));
  }

  glm::dmat4 get_model_matrix(const glm::dquat& model) {
    return glm::mat4_cast(model) * glm::scale(glm::dmat4(1.0), glm::dvec3(scale_factor, scale_factor, scale_factor));
  }
  
  /*
   * Get the view matrix before the scene is rendered.
   */
  glm::mat4 get_view_matrix(float camera_x, float camera_y, float camera_z, float rx, float ry, float rz) {
    return glm::rotate(glm::mat4(1.0f), (float)(rz), glm::vec3(0.0, 0.0, 1.0)) *
      glm::rotate(glm::mat4(1.0f), (float)(ry), glm::vec3(0.0, 1.0, 0.0)) *
      glm::rotate(glm::mat4(1.0f), (float)(rx), glm::vec3(1.0, 0.0, 0.0)) *
      glm::translate(glm::mat4(1.0f), glm::vec3(-camera_x, -camera_y, -camera_z));
  }


  /*
   * Get the model matrix before the scene is rendered.
   */
  glm::mat4 get_model_matrix(float rx, float ry, float rz) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), (float)(rz), glm::vec3(0.0, 0.0, 1.0)) *
      glm::rotate(glm::mat4(1.0f), (float)(ry), glm::vec3(0.0, 1.0, 0.0)) *
      glm::rotate(glm::mat4(1.0f), (float)(rx), glm::vec3(1.0, 0.0, 0.0)) *
      glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor, scale_factor, scale_factor));
    std::cerr << "Model: " << glm::to_string(model) << std::endl;
    return model;
  }


  /*
   * Writes the point cloud to a buffer as x,y,z,i. Returns a size_t
   * indicating the number of floating point entries written (note:
   * not the number of bytes written).
   */
  size_t write_point_cloud(float model_rx, float model_ry, float model_rz, float camera_x, float camera_y, float camera_z, float camera_rx, float camera_ry, float camera_rz, float* data, unsigned int width, unsigned int height) {
    // Get matrices we need for reversing the model-view-projection-clip-viewport transform.
    glm::ivec4 viewport;
    glm::mat4 model_view_matrix = get_model_view_matrix(model_rx, model_ry, model_rz, camera_x, camera_y, camera_z, camera_rx, camera_ry, camera_rz);

    glGetIntegerv( GL_VIEWPORT, (int*)&viewport );

    size_t data_count = 0;
    
    unsigned char rgba[4*width*height];

    glm::mat4 axis_flip = glm::scale(glm::mat4(1.0), glm::vec3(-1.0, 1.0, 1.0));

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)(rgba));

    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        size_t pos = 4*(j*height+i);
        
        int gb = rgba[pos + 1] * 255 + rgba[pos + 2];
        if (gb == 0) continue;
        double t = gb / 65536.0; // 2.0 * gb / 65536.0 - 1.0;
        double d = t * (far_plane - real_near_plane) + real_near_plane;

        glm::vec3 win(i,j,t);
	glm::vec4 position(glm::unProject(win, model_view_matrix, projection, viewport), 0.0);
        //gluUnProject(i, j, t, glm::value_ptr(model_view_matrix), glm::value_ptr(projection), (int*)&viewport, &(position[0]), &(position[1]), &(position[2]) );

	// Transform back into camera coordinates
	glm::vec4 position_cc = axis_flip * model_view_matrix * position;
	position_cc.z = d;

	//std::cerr << glm::to_string(position) << "    ->    " << glm::to_string(position_cc) << std::endl; 

        data[data_count]   =  position_cc[0];
        data[data_count+1] =  position_cc[1];
        data[data_count+2] =  position_cc[2];
        data[data_count+3] =  rgba[pos + 0] / 256.0;

        data_count += 4;
      }
    }

    return data_count;    
  }
  


  /*
  * Write the current color buffer as a PCD (point cloud file) (binary non-organized version).
  */
  void save_point_cloud(float mrx, float mry, float mrz, float cx, float cy, float cz, float crx, float cry, float crz, const std::string& basename, unsigned int width, unsigned int height) {
    std::string filename = basename + ".pcd";

    std::cerr << "Saving point cloud..." << std::endl;

    float* data       = new float[4*width*height + 4];
    size_t data_count = write_point_cloud(mrx, mry, mrz, cx, cy, cz, crx, cry, crz, data, width, height);
    
    std::ofstream out(filename.c_str());
    
    // Print PCD header
    out << "VERSION .7\nFIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n";
    out << "WIDTH " << data_count / 4 << std::endl;
    out << "HEIGHT " << 1 << std::endl;
    out << "VIEWPOINT 0 0 0 1 0 0 0" << std::endl;
    out << "POINTS " << data_count / 4 << std::endl;
    out << "DATA binary" << std::endl;
    out.write((char*)(data), sizeof(float)*data_count);

    out.close();

    std::cerr << "Saved '" << filename << "'" << std::endl;
  }

  float get_near_plane() const { return real_near_plane; }
  float get_far_plane() const { return far_plane; }

private:
  Mesh mesh;
  float scale_factor;

  glm::mat4 projection;
  float camera_d;
  GLfloat near_plane_bound;
  GLfloat real_near_plane;
  GLfloat far_plane;
};

#endif
