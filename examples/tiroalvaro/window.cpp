#include "window.hpp"

// Explicit specialization of std::hash for Vertex
template <> struct std::hash<Vertex> {
  size_t operator()(Vertex const &vertex) const noexcept {
    auto const h1{std::hash<glm::vec3>()(vertex.position)};
    return h1;
  }
};

void Window::onEvent(SDL_Event const &event) {
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_SPACE) {
      launchArrow();
    }

    if (event.key.keysym.sym == SDLK_b) {
      m_showNotes = !m_showNotes;
    }

    if (event.key.keysym.sym == SDLK_r) {
      // std::thread restartGameThread(&Window::restartGame, this);
      // restartGameThread.detach();
      restartGame();
    }

    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
      m_dollySpeed = 1.0f;
    if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)
      m_dollySpeed = -1.0f;
    if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)
      m_panSpeed = -1.0f;
    if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d)
      m_panSpeed = 1.0f;
    if (event.key.keysym.sym == SDLK_q)
      m_truckSpeed = -1.0f;
    if (event.key.keysym.sym == SDLK_e)
      m_truckSpeed = 1.0f;
  }
  if (event.type == SDL_KEYUP) {
    if ((event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) &&
        m_dollySpeed > 0)
      m_dollySpeed = 0.0f;
    if ((event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) &&
        m_dollySpeed < 0)
      m_dollySpeed = 0.0f;
    if ((event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a) &&
        m_panSpeed < 0)
      m_panSpeed = 0.0f;
    if ((event.key.keysym.sym == SDLK_RIGHT ||
         event.key.keysym.sym == SDLK_d) &&
        m_panSpeed > 0)
      m_panSpeed = 0.0f;
    if (event.key.keysym.sym == SDLK_q && m_truckSpeed < 0)
      m_truckSpeed = 0.0f;
    if (event.key.keysym.sym == SDLK_e && m_truckSpeed > 0)
      m_truckSpeed = 0.0f;
  }
}

void Window::onCreate() {
  auto const &assetsPath{abcg::Application::getAssetsPath()};

  // Load a new font
  auto const filename{assetsPath + "Inconsolata-Medium.ttf"};
  m_font = ImGui::GetIO().Fonts->AddFontFromFileTTF(filename.c_str(), 30.0f);
  if (m_font == nullptr) {
    throw abcg::RuntimeError("Cannot load font file");
  }

  abcg::glClearColor(0, 0, 0, 1);

  // Enable depth buffering
  abcg::glEnable(GL_DEPTH_TEST);

  // Create program
  m_program =
      abcg::createOpenGLProgram({{.source = assetsPath + "lookat.vert",
                                  .stage = abcg::ShaderStage::Vertex},
                                 {.source = assetsPath + "lookat.frag",
                                  .stage = abcg::ShaderStage::Fragment}});

  m_ground.create(m_program);

  // Get location of uniform variables
  m_viewMatrixLocation = abcg::glGetUniformLocation(m_program, "viewMatrix");
  m_projMatrixLocation = abcg::glGetUniformLocation(m_program, "projMatrix");
  m_modelMatrixLocation = abcg::glGetUniformLocation(m_program, "modelMatrix");
  m_colorLocation = abcg::glGetUniformLocation(m_program, "color");

  for (size_t i = 0; i < m_modelPaths.size(); i++) {
    auto color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    std::string name = "";

    if (m_modelPaths[i] == "target.obj") {
      color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
      name = "Alvaro";
    } 

    auto const [vertices_target, indices_target] =
        loadModelFromFile(assetsPath + m_modelPaths[i]);

    GLuint tmp_VAO{};
    GLuint tmp_VBO{};
    GLuint tmp_EBO{};

    // Generate VBO
    abcg::glGenBuffers(1, &tmp_VBO);
    abcg::glBindBuffer(GL_ARRAY_BUFFER, tmp_VBO);
    abcg::glBufferData(GL_ARRAY_BUFFER,
                       sizeof(vertices_target.at(0)) * vertices_target.size(),
                       vertices_target.data(), GL_STATIC_DRAW);
    abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Generate EBO
    abcg::glGenBuffers(1, &tmp_EBO);
    abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp_EBO);
    abcg::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                       sizeof(indices_target.at(0)) * indices_target.size(),
                       indices_target.data(), GL_STATIC_DRAW);
    abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create VAO
    abcg::glGenVertexArrays(1, &tmp_VAO);

    // Bind vertex attributes to current VAO
    abcg::glBindVertexArray(tmp_VAO);

    abcg::glBindBuffer(GL_ARRAY_BUFFER, tmp_VBO);
    auto const positionAttribute{
        abcg::glGetAttribLocation(m_program, "inPosition")};
    abcg::glEnableVertexAttribArray(positionAttribute);
    abcg::glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE,
                                sizeof(Vertex), nullptr);
    abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

    abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp_EBO);

    // End of binding to current VAO
    abcg::glBindVertexArray(0);

    m_targets_list[m_modelPaths[i]] =
        Target{tmp_VAO,         tmp_VBO, tmp_EBO, vertices_target,
                indices_target, color,   name};
  }

  // build arrow
  auto const [vertices_arrow, indices_arrow] =
      loadModelFromFile(assetsPath + "arrow.obj");
  m_vertices_arrow = vertices_arrow;
  m_indices_arrow = indices_arrow;

  // Generate VBO
  abcg::glGenBuffers(1, &m_VBO_arrow);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO_arrow);
  abcg::glBufferData(GL_ARRAY_BUFFER,
                     sizeof(m_vertices_arrow.at(0)) *
                         m_vertices_arrow.size(),
                     m_vertices_arrow.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Generate EBO
  abcg::glGenBuffers(1, &m_EBO_arrow);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_arrow);
  abcg::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(m_indices_arrow.at(0)) *
                         m_indices_arrow.size(),
                     m_indices_arrow.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Create VAO
  abcg::glGenVertexArrays(1, &m_VAO_arrow);

  // Bind vertex attributes to current VAO
  abcg::glBindVertexArray(m_VAO_arrow);

  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBO_arrow);
  auto const positionAttribute_arrow{
      abcg::glGetAttribLocation(m_program, "inPosition")};
  abcg::glEnableVertexAttribArray(positionAttribute_arrow);
  abcg::glVertexAttribPointer(positionAttribute_arrow, 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_arrow);

  abcg::glBindVertexArray(0);

  // Definindo posição inicial dos Alvos
  m_randomEngine.seed(
      std::chrono::steady_clock::now().time_since_epoch().count());

  std::uniform_real_distribution<float> rd_alvaro_position(-5.0f, 5.0f);
  std::uniform_int_distribution<int> rd_alvaro_model(0, m_modelPaths.size() - 1);

  // inicializando alvos
  for (int i = 0; i < m_num_targets; ++i) {
    m_target[i] = m_targets_list[m_modelPaths[rd_alvaro_model(m_randomEngine)]];
    m_target[i].m_position = glm::vec3(rd_alvaro_position(m_randomEngine), 0,
                                        rd_alvaro_position(m_randomEngine));
  }
}

// https://stackoverflow.com/questions/321068/returning-multiple-values-from-a-c-function
std::tuple<std::vector<Vertex>, std::vector<GLuint>>
Window::loadModelFromFile(std::string_view path) {
  tinyobj::ObjReader reader;
  std::vector<Vertex> vertices;
  std::vector<GLuint> indices;

  if (!reader.ParseFromFile(path.data())) {
    if (!reader.Error().empty()) {
      throw abcg::RuntimeError(
          fmt::format("Failed to load model {} ({})", path, reader.Error()));
    }
    throw abcg::RuntimeError(fmt::format("Failed to load model {}", path));
  }

  if (!reader.Warning().empty()) {
    fmt::print("Warning: {}\n", reader.Warning());
  }

  auto const &attributes{reader.GetAttrib()};
  auto const &shapes{reader.GetShapes()};

  vertices.clear();
  indices.clear();

  // A key:value map with key=Vertex and value=index
  std::unordered_map<Vertex, GLuint> hash{};

  // Loop over shapes
  for (auto const &shape : shapes) {
    // Loop over indices
    for (auto const offset : iter::range(shape.mesh.indices.size())) {
      // Access to vertex
      auto const index{shape.mesh.indices.at(offset)};

      // Vertex position
      auto const startIndex{3 * index.vertex_index};
      auto const vx{attributes.vertices.at(startIndex + 0)};
      auto const vy{attributes.vertices.at(startIndex + 1)};
      auto const vz{attributes.vertices.at(startIndex + 2)};

      Vertex const vertex{.position = {vx, vy, vz}};

      // If map doesn't contain this vertex
      if (!hash.contains(vertex)) {
        // Add this index (size of m_vertices)
        hash[vertex] = vertices.size();
        // Add this vertex
        vertices.push_back(vertex);
      }

      indices.push_back(hash[vertex]);
    }
  }

  return std::make_tuple(vertices, indices);
}

void Window::onPaint() {
  // Clear color buffer and depth buffer
  abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  abcg::glViewport(0, 0, m_viewportSize.x, m_viewportSize.y);

  abcg::glUseProgram(m_program);

  // Set uniform variables for viewMatrix and projMatrix
  // These matrices are used for every scene object
  abcg::glUniformMatrix4fv(m_viewMatrixLocation, 1, GL_FALSE,
                           &m_camera.getViewMatrix()[0][0]);
  abcg::glUniformMatrix4fv(m_projMatrixLocation, 1, GL_FALSE,
                           &m_camera.getProjMatrix()[0][0]);

  // renderizando cada alvo
  for (int i = 0; i < m_num_targets; ++i) {
    auto selectedTarget = m_target[i];

    abcg::glBindVertexArray(selectedTarget.m_vao);

    glm::mat4 model{1.0f};
    // renderizacao condicional caso nao tenha sido capturado
    if (selectedTarget.m_captured == false) {
      model = glm::translate(model, selectedTarget.m_position);
      model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
      model = glm::scale(model, glm::vec3(0.02f));

      abcg::glUniformMatrix4fv(m_modelMatrixLocation, 1, GL_FALSE,
                               &model[0][0]);
      abcg::glUniform4f(m_colorLocation, selectedTarget.m_color.r,
                        selectedTarget.m_color.g, selectedTarget.m_color.b,
                        selectedTarget.m_color.a);
      abcg::glDrawElements(GL_TRIANGLES, selectedTarget.m_indices.size(),
                           GL_UNSIGNED_INT, nullptr);
    }
  }

  // DRAW Arrow
  if (m_arrowLaunched == true) {

    abcg::glBindVertexArray(m_VAO_arrow);

    // model = glm::mat4(1.0);
    glm::mat4 model_arrow{1.0f};
    model_arrow = glm::translate(model_arrow, m_arrowPosition);
    model_arrow =
        glm::rotate(model_arrow, glm::radians(90.0f), glm::vec3(0, 1, 0));
    model_arrow = glm::scale(model_arrow, glm::vec3(0.002f));

    abcg::glUniformMatrix4fv(m_modelMatrixLocation, 1, GL_FALSE,
                             &model_arrow[0][0]);
    abcg::glUniform4f(m_colorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    abcg::glDrawElements(GL_TRIANGLES, m_indices_arrow.size(),
                         GL_UNSIGNED_INT, nullptr);
  }

  abcg::glBindVertexArray(0);

  // Draw ground
  m_ground.paint();

  abcg::glUseProgram(0);
}

void Window::onPaintUI() {
  abcg::OpenGLWindow::onPaintUI();
  {
    // TEXT WINDOW
    auto const size{ImVec2(300, 85)};
    auto const position{ImVec2((m_viewportSize.x - size.x) / 2.0f,
                               (m_viewportSize.y - size.y) / 2.0f)};
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGuiWindowFlags const flags{ImGuiWindowFlags_NoBackground |
                                 ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoInputs};
    ImGui::Begin(" ", nullptr, flags);
    ImGui::PushFont(m_font);

    std::string text = "";
    float windowWidth = ImGui::GetWindowWidth();
    float textWidth = 0;

    // https://stackoverflow.com/questions/64653747/how-to-center-align-text-horizontally
    if (m_currentState == TargetState::Captured) {
      frameTimer += 1;
      // ou seja, passou 1.5 segundo (90 frames)
      if (frameTimer > 90.0f) {
        backToLive();
      }

      text = "Alvaro Atingido!";
      textWidth = ImGui::CalcTextSize(text.c_str()).x;
      ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
      ImGui::TextUnformatted(text.c_str());

    } else if (m_currentState == TargetState::Escaped) {
      frameTimer += 1;
      // ou seja, passou 1.5 segundo (90 frames)
      if (frameTimer > 90.0f) {
        backToLive();
      }

      text = "Errou!";
      textWidth = ImGui::CalcTextSize(text.c_str()).x;
      ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
      ImGui::TextUnformatted(text.c_str());
    }

    if (m_restarted == true) {
      frameTimer += 1;
      // ou seja, passou 1.5 segundo (90 frames)
      if (frameTimer > 90.0f) {
        m_restarted = false;
        frameTimer = 0.0f;
      }

      text = "Jogo reiniciado";
      textWidth = ImGui::CalcTextSize(text.c_str()).x;
      ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
      ImGui::TextUnformatted(text.c_str());
    }

    ImGui::PopFont();
    ImGui::End();

    text = "";

    // JANELA DA AGENDA
    if (m_showNotes) {
      ImGui::Begin("Notes", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
      ImGui::Text("Alvaros Atingidos:");

      for (const auto &target : m_notes_targets) {
        // ImGui::Text(target.c_str());
        ImGui::TextUnformatted(target.c_str());
      }

      // Adicione mais informações se necessário
      ImGui::End();
    }
  }
}

void Window::onResize(glm::ivec2 const &size) {
  m_viewportSize = size;
  m_camera.computeProjectionMatrix(size);
}

void Window::onDestroy() {
  m_ground.destroy();

  abcg::glDeleteProgram(m_program);
  abcg::glDeleteBuffers(1, &m_EBO);
  abcg::glDeleteBuffers(1, &m_VBO);
  abcg::glDeleteVertexArrays(1, &m_VAO);
}

void Window::onUpdate() {
  auto const deltaTime{gsl::narrow_cast<float>(getDeltaTime())};

  // Update LookAt camera
  m_camera.dolly(m_dollySpeed * deltaTime);
  m_camera.truck(m_truckSpeed * deltaTime);
  m_camera.pan(m_panSpeed * deltaTime);

  // Atualiza a posição da Flecha
  updateArrowPosition();
}

void Window::launchArrow() {
  if (!m_arrowLaunched) {
    m_currentState = TargetState::Live;

    fmt::print("Flecha Lançada!\n");

    m_arrowPosition = m_camera.getEyePosition();

    glm::vec3 launchDirection =
        glm::normalize(m_camera.getLookAtPoint() - m_camera.getEyePosition());
    float launchSpeed = 2.0f;
    m_arrowVelocity = launchDirection * launchSpeed;
    m_arrowLaunched = true;
  }
}

void Window::updateArrowPosition() {
  if (m_arrowLaunched) {
    auto const deltaTime{gsl::narrow_cast<float>(getDeltaTime())};

    const float arrowRadius = 0.1f;
    const float targetRadius = 0.5f;
    

    m_arrowPosition += m_arrowVelocity * deltaTime;

// Verifica se saiu da tela
    if ((m_arrowPosition.x - arrowRadius) < -5.0f ||
        (m_arrowPosition.x - arrowRadius) > 5.0f ||
        (m_arrowPosition.z - arrowRadius) < -5.0f ||
        (m_arrowPosition.z - arrowRadius) > 5.0f) {
      m_arrowLaunched = false;
      fmt::print("Flecha parou!\n");
    }
}


    // Verifica se colidiu com algum alvo
    for (int i = 0; i < m_num_targets; ++i) {
      if (!m_target[i].m_captured) {
        float distance =
            glm::distance(m_arrowPosition, m_target[i].m_position);
                const float arrowRadius = 0.1f;
                const float targetRadius = 0.5f;
        if ((distance - targetRadius - arrowRadius) < 0.02f) {

          // Colisão detectada
          fmt::print("Flecha atingiu o Alvaro {}!\n", i + 1);
          // probabilidade de captura 45%
          std::uniform_real_distribution<float> rd_alvaro_capture(0.0f, 1.0f);

          if (rd_alvaro_capture(m_randomEngine) < 0.45f) {
            m_target[i].m_captured = true;
            m_notes_targets.insert(m_target[i].m_name);

            m_currentState = TargetState::Captured;
          } else {
            m_currentState = TargetState::Escaped;
          }

          m_arrowLaunched = false;
          break;
        }
      }
    }
  }

void Window::backToLive() {
  m_currentState = TargetState::Live;
  frameTimer = 0.0f;
}

void Window::restartGame() {
  std::uniform_real_distribution<float> rd_alvaro_position(-5.0f, 5.0f);

  for (int i = 0; i < m_num_targets; ++i) {
    m_target[i].m_captured = false;
    m_target[i].m_position = glm::vec3(rd_alvaro_position(m_randomEngine), 0,
                                        rd_alvaro_position(m_randomEngine));
  }
  m_arrowLaunched = false;
  m_notes_targets.clear();

  m_showNotes = false;

  m_restarted = true;
  frameTimer = 0.0f;
}