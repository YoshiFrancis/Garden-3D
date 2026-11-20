// CS334 / ECE30834 -- Assignment 4 student starter.
// TODO1: input parsing (30%).
// TODO2: one-step rule application (30%).
// TODO3: 2D geometry, with 0.75 branch-step scaling (30%).
// TODO4: angle-jitter parsing and per-turn sampling (10%).
// TODO5: optional weighted production rules (Bonus 15%); also see lsystem.hpp.
// The window, shaders, preprocessing, iteration cache, and buffer upload are
// provided.

#define NOMINMAX
#include "lsystem.hpp"
#include "util.hpp"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <random>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <utility>

// Stream processing helper functions
std::stringstream preprocessStream(std::istream &istr);
std::string getNextLine(std::istream &istr);
std::string trim(const std::string &line);

// Static L-System members
unsigned int LSystem::refcount = 0;
GLuint LSystem::shader = 0;
GLuint LSystem::xformLoc = 0;

// Constructor
LSystem::LSystem() : angle(0.0f), jitter(0.0f), vao(0), vbo(0), bufSize(0) {

  // Initialize random number generator with random seed
  std::random_device rd;
  rng = std::mt19937(rd());

  // Create shader if we're the first object
  if (refcount == 0)
    initShader();
  refcount++;
}

// Destructor
LSystem::~LSystem() {
  // Destroy vertex buffer and array
  if (vao) {
    glDeleteVertexArrays(1, &vao);
    vao = 0;
  }
  if (vbo) {
    glDeleteBuffers(1, &vbo);
    vbo = 0;
  }
  bufSize = 0;

  refcount--;
  // Destroy shader if we're the last object
  if (refcount == 0) {
    if (shader) {
      glDeleteProgram(shader);
      shader = 0;
    }
  }
}

// Move constructor
LSystem::LSystem(LSystem &&other)
    : strings(std::move(other.strings)), rules(std::move(other.rules)),
      angle(other.angle), jitter(other.jitter), vao(other.vao), vbo(other.vbo),
      iterData(std::move(other.iterData)), bufSize(other.bufSize) {

  // Initialize random number generator with random seed
  std::random_device rd;
  rng = std::mt19937(rd());

  other.vao = 0;
  other.vbo = 0;
  other.bufSize = 0;
  // Increment reference count (temp will decrement upon destructor)
  refcount++;
}

// Move assignment operator
LSystem &LSystem::operator=(LSystem &&other) {
  strings = std::move(other.strings);
  rules = std::move(other.rules);
  angle = other.angle;
  jitter = other.jitter;
  iterData = std::move(other.iterData);
  bufSize = other.bufSize;

  // Release any existing buffers
  if (vao) {
    glDeleteVertexArrays(1, &vao);
  }
  if (vbo) {
    glDeleteBuffers(1, &vbo);
  }
  // Acquire other's buffers
  vao = other.vao;
  vbo = other.vbo;

  other.vao = 0;
  other.vbo = 0;
  other.bufSize = 0;
  // Refcount stays the same

  return *this;
}

// Parse preprocessed input and replace the current L-system.
// parseFile()/parseString() remove comments and empty lines before calling
// here.
void LSystem::parse(std::istream &istr) {
  // PROVIDED defaults keep the unimplemented starter safe to run.
  float inAngle = 0.0f;
  float inJitter = 0.0f;
  unsigned int inIters = 1;
  std::string inAxiom;

  // TODO5 (BONUS, rule storage): if you implement weighted alternatives,
  // update this temporary type AND the rules member in lsystem.hpp together.
  std::map<char, std::string> inRules;

  // TODO1 BEGIN -- Input parsing (30%).
  // Read A, N, the axiom, and the deterministic rules from istr.
  // Store them in inAngle, inIters, inAxiom, and inRules respectively.
  // A rule has one predecessor character, a colon, and a successor string.
  // Ignore whitespace as specified in the handout. Do not rewrite strings
  // here: the PROVIDED framework below manages the iteration sequence.
  //
  // Write your parser here. Remove the diagnostic below when implemented.
  std::string angleAndJitter;
  istr >> angleAndJitter;
  auto begin = angleAndJitter.find_first_of('<');
  auto end = angleAndJitter.find_first_of('>');
  if (begin != std::string::npos) {
    std::cout << "found...\n";
    inJitter = std::stof(angleAndJitter.substr(begin + 1, end));
    inAngle = std::stof(angleAndJitter.substr(0, begin));
  } else {
    std::cout << "not found...\n";
    inAngle = std::stof(angleAndJitter);
  }
  istr >> inIters;
  istr >> inAxiom;

  std::cout << "begin: " << begin << " | " << end
            << " | angleAndJitter: " << angleAndJitter << "\n";
  std::cout << "angle: " << inAngle << " | jitter: " << inJitter << "\n";
  {
    char rule_char;
    std::string tmp;
    std::string rule;
    while (istr >> rule_char >> tmp >> rule) {
      std::cout << "rule " << inRules.size() << "\n";
      std::cout << rule_char << ":tmp:" << tmp << ":rule:" << rule << "\n";
      inRules[rule_char] = rule;
    }
  }
  // // // TODO1 END

  // TODO4 BEGIN -- Read optional angle jitter (part of the required 10%).
  // Extend the first-line parsing above to accept A<J>, such as 25.7<10.0>.
  // Store J in inJitter; without <J>, leave inJitter at 0.
  // This parsing belongs with TODO1's first-line reader, not after the rules
  // have consumed the stream. Mark your integrated implementation TODO4.
  // TODO4 END

  // TODO5 BEGIN -- Weighted-rule parsing (BONUS 15%).
  // Support several successors for one predecessor, each with an optional
  // weight before ':'. An omitted weight means 1. Keep deterministic inputs
  // compatible. Integrate this with the rule reader from TODO1.
  // Complete the matching TODO5 locations in applyRules() and lsystem.hpp.
  // TODO5 END

  // PROVIDED state update and iteration management. Keep this section.
  angle = inAngle;
  jitter = inJitter;
  strings = {inAxiom};
  rules = std::move(inRules);
  iterData.clear();
  auto verts = createGeometry(strings.back());
  addVerts(verts);

  // N is the number of initially cached states, INCLUDING the axiom (T_0).
  // For positive N=6 this caches T_0 through T_5; Right can request T_6.
  // Do not add a second rewriting loop in your input parser.
  try {
    while (strings.size() < inIters)
      iterate();
  } catch (const std::exception &e) {
    std::cerr << "Too many iterations: geometry exceeds maximum buffer size"
              << std::endl;
  }
}

// Parse contents of source string
void LSystem::parseString(std::string string) {
  std::stringstream ss(string);

  // Preprocess to remove comments & whitespace
  ss = preprocessStream(ss);
  parse(ss);
}

// Parse a file
void LSystem::parseFile(std::string filename) {
  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error("failed to open " + filename);

  // Preprocess to remove comments & whitespace
  std::stringstream ss = preprocessStream(file);
  parse(ss);
}

// Apply rules to the latest string to generate the next string
unsigned int LSystem::iterate() {
  // Apply rules to last string
  std::string newString = applyRules(strings.back());
  // Get geometry of new iteration
  auto verts = createGeometry(newString);

  // Check for too-large buffer
  auto &id = iterData.back();
  if ((id.first + id.count + verts.size()) * sizeof(glm::vec2) > MAX_BUF)
    throw std::runtime_error("geometry exceeds maximum buffer size");

  // Store new iteration
  strings.push_back(newString);
  addVerts(verts);

  return getNumIter();
}

// Draw the latest iteration of the L-System
void LSystem::draw(glm::mat3 viewProj) {
  if (!getNumIter())
    return;
  drawIter(getNumIter() - 1, viewProj);
}

// Draw a specific iteration of the L-System
void LSystem::drawIter(unsigned int iter, glm::mat3 viewProj) {
  IterData &id = iterData.at(iter);
  if (id.count == 0 || !vao)
    return; // No geometry until TODO3 is implemented.

  glUseProgram(shader);
  glBindVertexArray(vao);

  // Send matrix to shader
  glm::mat3 xform = viewProj * id.bbfix;
  glUniformMatrix3fv(xformLoc, 1, GL_FALSE, glm::value_ptr(xform));
  // Draw L-System
  glDrawArrays(GL_LINES, id.first, id.count);

  glBindVertexArray(0);
  glUseProgram(0);
}

// Apply one simultaneous rewriting step to the input string.
std::string LSystem::applyRules(std::string string) {
  std::string result;

  // TODO2 BEGIN -- Rule application (30%).
  // Process each character of the INPUT string once. Append its successor
  // when a rule exists, or the unchanged character when no rule exists.
  // Newly appended characters must not be rewritten during this same step.
  // Write your implementation here.
  // TODO2 END
  for (char c : string) {
    if (rules.count(c) > 0) {
      result += rules[c];
    } else {
      result += c;
    }
  }

  // TODO5 BEGIN -- Weighted rule selection (BONUS 15%).
  // Extend the replacement lookup above to choose among all alternatives
  // for this predecessor, with probabilities proportional to their weights.
  // Make a fresh choice for each rewritten occurrence, not once per file.
  // Use the rng member; deterministic rules must still work.
  // TODO5 END

  // Placeholder: no expansion is implemented yet.
  return result;
}

// Generate GL_LINES vertices: every TWO vertices form a separate segment.
std::vector<glm::vec2> LSystem::createGeometry(std::string string) {
  std::vector<glm::vec2> verts;
  constexpr float branchScale = 0.75f; // PROVIDED assignment parameter.

  // TODO3 BEGIN -- Geometry generation (30%, including branch scaling).
  // Set up a drawing state starting at the origin, pointing along +Y,
  // with a positive initial step (the examples use 1). Keep position,
  // heading, and step length together when saving/restoring a branch.
  // You may use explicit state variables or transformation matrices.
  // Set up a local stack for the saved states. Each function call starts fresh.

  struct state {
    glm::vec2 pos;
    glm::vec2 dir;
    float step;
  };

  std::stack<state> s{};

  state curr_state = {glm::vec2(0, 0), glm::vec2(0, 1), 1};

  for (char c : string) {

    glm::vec2 &pos = curr_state.pos; // initiall start at the origin
    glm::vec2 &dir = curr_state.dir; // initially face the positive y-dir
    float &step = curr_state.step;

    switch (c) {
    case 'f': {
      verts.push_back(pos);
      verts.push_back(pos + dir * step);
      pos += dir * step;
      break;
    }

    case 'F': {
      verts.push_back(pos);
      verts.push_back(pos + dir * step);
      pos += dir * step;
      break;
    }
    case 'g': {
      verts.push_back(pos);
      verts.push_back(pos + dir * step);
      pos += dir * step;
      break;
    }
    case 'G': {
      verts.push_back(pos);
      verts.push_back(pos + dir * step);
      pos += dir * step;
      break;
    }

    case 's': {
      pos += dir * step;
      break;
    }
    case 'S': {
      pos += dir * step;
      break;
    }
    case '+': {
      float turnAngle = angle;
      // TODO4 -- Per-turn angle jitter for '+' (part of required 10%).
      // Adjust turnAngle with a fresh uniform offset in [-jitter,+jitter].
      // Use rng. Leave the turn unchanged when jitter is zero.

      float random_jitter = ((std::rand() * 2.0f * jitter) - jitter);
      if (jitter == 0)
        random_jitter = 1;
      // TODO3 -- Turn counterclockwise (5%).
      // Update the heading by turnAngle degrees without moving.
      // Preserve the current branch's step length.
      float radians = glm::radians(turnAngle * random_jitter);
      float cos = glm::cos(radians);
      float sin = glm::sin(radians);
      float oldX = dir.x;
      float oldY = dir.y;
      dir.x = cos * oldX - sin * oldY;
      dir.y = sin * oldX + cos * oldY;
      break;
    }

    case '-': {
      float turnAngle = angle;
      // TODO4 -- Per-turn angle jitter for '-' (part of required 10%).
      // Adjust turnAngle with a fresh uniform offset in [-jitter,+jitter].
      // Then use the resulting magnitude for this clockwise turn.

      float random_jitter = ((std::rand() * 2.0f * jitter) - jitter);
      if (jitter == 0)
        random_jitter = 1;
      // TODO3 -- Turn clockwise (5%).
      // Update the heading by turnAngle degrees without moving.
      // Preserve the current branch's step length.
      float radians = -glm::radians(turnAngle * random_jitter);
      float cos = glm::cos(radians);
      float sin = glm::sin(radians);
      float oldX = dir.x;
      float oldY = dir.y;
      dir.x = cos * oldX - sin * oldY;
      dir.y = sin * oldX + cos * oldY;
      break;
    }

    case '[': {
      s.emplace(curr_state);
      state new_state = curr_state;
      new_state.step *= branchScale;
      curr_state = new_state;
      break;
    }
      // TODO3 -- Save state and enter a branch (5%).
      // Save the COMPLETE parent state BEFORE shortening the step.
      // The new branch starts at the same position and heading;
      // only its step is multiplied by branchScale.

    case ']': {
      if (s.size() > 0) {
        curr_state = s.top();
        s.pop();
      }
      break;
    }
      // TODO3 -- Restore state (5%).
      // Pop the most recently saved state, restoring position,
      // heading, AND step length. Do not draw a connecting line.
      // Avoid accessing an empty stack.

    default:
      // Unrecognized drawing symbols have no geometric effect.
      std::cerr << "Unrecognized symbol has no geometric effect: " << c << "\n";
      break;
    }
  }
  // TODO3 END

  (void)branchScale; // Remove when used by your implementation.
  // Placeholder: empty until drawing commands are implemented.
  return verts;
}

// Add given geometry to the OpenGL vertex buffer and update state accordingly
void LSystem::addVerts(std::vector<glm::vec2> &verts) {
  // Add iteration data
  IterData id;
  if (iterData.empty())
    id.first = 0;
  else {
    auto &lastID = iterData.back();
    id.first = lastID.first + lastID.count;
  }
  id.count = verts.size();

  // PROVIDED: an unfinished geometry TODO may return an empty vector.
  // Record that iteration but do not compute an empty bounding box or upload
  // zero bytes into a buffer that has not yet been created.
  id.bbfix = glm::mat3(1.0f);
  if (verts.empty()) {
    iterData.push_back(id);
    return;
  }

  // Calculate bounding box and create adjustment matrix.
  glm::vec2 minBB = glm::vec2(std::numeric_limits<float>::max());
  glm::vec2 maxBB = glm::vec2(std::numeric_limits<float>::lowest());
  for (auto &v : verts) {
    minBB = glm::min(minBB, v);
    maxBB = glm::max(maxBB, v);
  }
  glm::vec2 diag = maxBB - minBB;
  const float extent = glm::max(diag.x, diag.y);
  const float scale = extent > 0.0f ? 1.9f / extent : 1.0f;
  id.bbfix[0][0] = scale;
  id.bbfix[1][1] = scale;
  id.bbfix[2] = glm::vec3(-(minBB + maxBB) * scale / 2.0f, 1.0f);
  iterData.push_back(id);

  GLsizei newSize = (id.first + id.count) * sizeof(glm::vec2);
  if (newSize > bufSize) {
    // Create a new vertex buffer to hold vertex data
    GLuint tempBuf;
    glGenBuffers(1, &tempBuf);
    glBindBuffer(GL_ARRAY_BUFFER, tempBuf);
    glBufferData(GL_ARRAY_BUFFER, newSize, nullptr, GL_STATIC_DRAW);

    // Copy data from existing buffer
    if (vbo) {
      glBindBuffer(GL_COPY_READ_BUFFER, vbo);
      glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0, bufSize);
      glBindBuffer(GL_COPY_READ_BUFFER, 0);
      glDeleteBuffers(1, &vbo);
    }

    vbo = tempBuf;
    bufSize = newSize;

  } else
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // Upload new vertex data
  glBufferSubData(GL_ARRAY_BUFFER, id.first * sizeof(glm::vec2),
                  id.count * sizeof(glm::vec2), verts.data());

  // Reset vertex data source (format)
  if (!vao) {
    glGenVertexArrays(1, &vao);
  }
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
                        (GLvoid *)0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Compile and link shader
void LSystem::initShader() {
  std::vector<GLuint> shaders;
  shaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/v.glsl"));
  shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/f.glsl"));
  shader = linkProgram(shaders);
  // Cleanup extra state
  for (auto s : shaders)
    glDeleteShader(s);
  shaders.clear();

  // Get uniform locations
  xformLoc = glGetUniformLocation(shader, "xform");
}

// Remove empty lines, comments, and trim leading and trailing whitespace
std::stringstream preprocessStream(std::istream &istr) {
  istr.exceptions(istr.badbit | istr.failbit);
  std::stringstream ss;

  try {
    while (true) {
      std::string line = getNextLine(istr);
      ss << line << std::endl; // Add newline after each line
    }                          // Stream always ends with a newline

  } catch (const std::exception &e) {
    if (!istr.eof())
      throw e;
  }

  return ss;
}

// Reads lines from istream, stripping whitespace and comments,
// until it finds a nonempty line
std::string getNextLine(std::istream &istr) {
  const std::string comment = "#";
  std::string line = "";
  while (line == "") {
    std::getline(istr, line);
    // Skip comments and empty lines
    auto found = line.find(comment);
    if (found != std::string::npos)
      line = line.substr(0, found);
    line = trim(line);
  }
  return line;
}

// Trim leading and trailing whitespace from a line
std::string trim(const std::string &line) {
  const std::string whitespace = " \t\r\n";
  auto first = line.find_first_not_of(whitespace);
  if (first == std::string::npos)
    return "";
  auto last = line.find_last_not_of(whitespace);
  auto range = last - first + 1;
  return line.substr(first, range);
}
