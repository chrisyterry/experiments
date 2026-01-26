# pragma once
#include <string>

/**
 * @brief class to handle multi-line console input, useful for getting user input for an LLM chat
 */
class ConsoleInput {
  public:
    /**
     * @brief get the input text from the commandline; single enter will be treated as \n, double enter will end input
     */
    std::string getInput();
};