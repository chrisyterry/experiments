
#include <iostream>
#include <memory>
#include <vector>
#include "llm_wrapper.hpp"
#include "commandline_args.hpp"

int main(int argc, char* argv[]) {

    std::unique_ptr<CommandLineArgs> arg_parser = std::make_unique<CommandLineArgs>("LLM chat", "Simple LLM chat using model specified with command line parameters");
    arg_parser->addArgument<std::string>("model_path", "path to the .gguf file for the model to use", "mp");
    arg_parser->parse(argc, argv);
    
    std::string user_input = "hello computer, it is nice to finally speak with you!";
    std::unique_ptr<LLM> llm = std::make_unique<LLM>(arg_parser->getArgument<std::string>("model_path"));
    
    std::cout << "User: " << user_input << std::endl;

    // get the response from the LLM
    std::string response = llm->getChatResponse(user_input);

    std::cout << "Machine: " << response << std::endl;

    return 0;
}