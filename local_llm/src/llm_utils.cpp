#include "llm_utils.hpp"
#include <cstdio>
#include <iostream>

std::string ConsoleInput::getInput() {

    bool prev_empty = false;

    std::string user_input = "";

    while (true) {
        // get the current input
        std::string current_line;
        std::getline(std::cin, current_line);

        // if the user hit enter this line
        if (current_line == "") {
            // if they had hit enter last line
            if (prev_empty) {
                break;
            } else {
                prev_empty = true;
            }
        } else {
            // if the last input was empty
            if (prev_empty) {
                // new line
                user_input += "\n";
            }
            user_input += current_line;
            prev_empty = false;
        }
    }

    return user_input;
}