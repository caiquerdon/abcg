#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include "abcgOpenGL.hpp"

#include "camera.hpp"
#include "ground.hpp"
#include <chrono>
#include <random>
// #include <thread>
#include <tuple>
#include <unordered_map>
#include <set>

struct Vertex {
  glm::vec3 position;

  friend bool operator==(Vertex const &, Vertex const &) = default;
};

class Window : public abcg::OpenGLWindow {
protected:
  void onEvent(SDL_Event const &event) override;
  void onCreate() override;
  void onPaint() override;
  void onPaintUI() override;
  void onResize(glm::ivec2 const &size) override;
  void onDestroy() override;
  void onUpdate() override;

private:
  struct Target {
    GLuint m_vao{};
    GLuint m_vbo{};
    GLuint m_ebo{};
    std::vector<Vertex> m_vertices;
    std::vector<GLuint> m_indices;
    glm::vec4 m_color{};
    std::string m_name{};
    bool m_captured{false};
    glm::vec3 m_position{0, 0, 0};
  };


  std::unordered_map<std::string, Target> m_targets_list;
  std::vector<std::string> m_modelPaths = {"target.obj"};

  int m_num_targets{5};
  Target m_target[5];
  std::set<std::string> m_notes_targets;

  bool m_showNotes{false};
  bool m_restarted{false};


  glm::ivec2 m_viewportSize{};

  ImFont *m_font{};

  GLuint m_VAO{};
  GLuint m_VBO{};
  GLuint m_EBO{};

  GLuint m_VAO_arrow{};
  GLuint m_VBO_arrow{};
  GLuint m_EBO_arrow{};

  GLuint m_program{};

  std::default_random_engine m_randomEngine;

  GLint m_viewMatrixLocation{};
  GLint m_projMatrixLocation{};
  GLint m_modelMatrixLocation{};
  GLint m_colorLocation{};

  Camera m_camera;
  float m_dollySpeed{};
  float m_truckSpeed{};
  float m_panSpeed{};

  Ground m_ground;

  std::vector<Vertex> m_vertices;
  std::vector<GLuint> m_indices;

  std::vector<Vertex> m_vertices_arrow;
  std::vector<GLuint> m_indices_arrow;

  // void loadModelFromFile(std::string_view path);

  std::tuple<std::vector<Vertex>, std::vector<GLuint>>
  loadModelFromFile(std::string_view path);

  // Estados da Flecha
  glm::vec3 m_arrowPosition{};
  glm::vec3 m_arrowVelocity{};
  bool m_arrowLaunched{false};

  enum class TargetState { Captured, Escaped, Live };
  TargetState m_currentState{TargetState::Live};

  void launchArrow();
  void updateArrowPosition();
  // void checkTargetCapture();

  void backToLive();
  void restartGame();

  int frameTimer{0};
};

#endif