
#include <iostream>
#include <memory>
#include <vector>
#include "llm_wrapper.hpp"

int main() {
    
    std::string user_input = "hello computer, it is nice to finally speak with you!";
    std::unique_ptr<LLM> llm = std::make_unique<LLM>("/home/chriz/Downloads/Llama-3.2-3B-Instruct-uncensored-Q6_K.gguf");
    
    std::cout << "User: " << user_input << std::endl;

    // get the response from the LLM
    std::string response = llm->getChatResponse(user_input);

    std::cout << "Machine: " << response << std::endl;

    return 0;
}