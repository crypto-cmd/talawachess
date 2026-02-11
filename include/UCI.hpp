#include "Board.hpp"
#include "Bot.hpp"
#include <string>
namespace talawachess {
class UCI {
  private:
    const std::string ENGINE_NAME= "Talawa";
    Bot _bot;

  public:
    UCI()= default;
    void listen();
};
} // namespace talawachess
