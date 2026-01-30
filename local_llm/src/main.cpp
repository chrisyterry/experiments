
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include "llm_wrapper.hpp"
#include "llm_utils.hpp"
#include "commandline_args.hpp"

int main(int argc, char* argv[]) {

    // get arguments
    std::unique_ptr<CommandLineArgs> arg_parser = std::make_unique<CommandLineArgs>("LLM chat", "Simple LLM chat using model specified with command line parameters. Type 'clear' in chat to reset the context/conversation");
    arg_parser->addArgument<std::string>("model_path", "path to the .gguf file for the model to use", "mp");
    arg_parser->parse(argc, argv);

    // setup LLM
    std::unique_ptr<LLM> llm = std::make_unique<LLM>(arg_parser->getArgument<std::string>("model_path"), 0.1, true);
    
    // setup chat
    std::unique_ptr<ConsoleInput> console_input = std::make_unique<ConsoleInput>();

    std::cout << "----- Chat Start -----\n" << std::endl;
    while (true) {
        // get user input
        std::string user_input = console_input->getInput();
        std::cout << std::endl;

        if (user_input == "clear") {
            llm->clearChat();
            std::cout << "---- Reset model ----" << std::endl;
            std::cout << "----- Chat Start -----\n" << std::endl;
        } else {
            std::cout << "processing request";
            // get the response from the LLM
            std::string response = llm->getChatResponse(user_input);
            std::cout << "LLM: \n" << response << "\n" << std::endl;
        }
    }

    return 0;
}