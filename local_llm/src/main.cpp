
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include "llm_wrapper.hpp"

int main() {
   
    std::string model_path = "/home/chriz/Downloads/Llama-3.2-3B-Instruct-uncensored-Q6_K.gguf";

    std::unique_ptr<LLM> model = std::make_unique<LLM>(model_path);

    std::string test_prompt = "Hello computer, it is nice to finally speak to you! how are you feeling?";
    std::cout << "User: " << test_prompt << std::endl;

    std::string response = model->getChatResponse(test_prompt);

    std::cout << "Machine: " << response << std::endl;

    return 0;
}