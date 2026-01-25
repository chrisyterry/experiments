#include "commandline_args.hpp"
#include <bits/stdc++.h>
#include <unordered_set>

CommandLineArgs::CommandLineArgs(std::string name, std::string app_description) {
    m_app_name        = name;
    m_app_description = app_description;
}

void CommandLineArgs::parse(int argc, char* argv[]) {

    std::string last_arg           = "";  // the last non-value arg encountered
    uint8_t     last_arg_index     = 0;  // last arg index
    bool        last_arg_abrv      = false;  // whether the last encountered non-value argument was abbreviated
    bool        last_arg_was_value = true;  // whether the last argument was a value
    bool        last_arg_was_flag  = false; // whether the last argument was a flag

    uint8_t arguments_read = 0;

    // for each input argument
    for (uint8_t arg = 1; arg < argc; ++arg) {

        std::string current_arg = argv[arg];
        bool        is_value    = true;
        bool        is_abrv     = false;

        // if the arg starts with a - or a -- strip this and take it as the argument name
        if (current_arg.substr(0, 2) == "--") {
            current_arg = current_arg.substr(2, current_arg.size());
            is_value    = false;
        } else if (current_arg[0] == '-') {
            // if it's not a negative number
            if (!std::isdigit(current_arg[1])) {
                current_arg = current_arg.substr(1, current_arg.size());
                is_value    = false;
                is_abrv     = true;
            }
        }

        uint8_t arg_index = 0;

        // determine if the argument is known or not
        if (!is_value) {

            // change the arg to lower case
            std::transform(current_arg.begin(), current_arg.end(), current_arg.begin(), [](unsigned char c){ return std::tolower(c); });

            // if the argument is a help command
            if (isHelp(current_arg)) {
                printHelp();
            }

            bool unknown_arg = false;
            if (is_abrv) {
                // if this is one of the specified arguments
                unknown_arg = m_argument_abrv_indexes.count(current_arg) <= 0;
                if (!unknown_arg) {
                    arg_index = m_argument_abrv_indexes.at(current_arg);
                }
            } else {
                unknown_arg = m_argument_name_indexes.count(current_arg) <= 0;
                if (!unknown_arg) {
                    arg_index = m_argument_name_indexes.at(current_arg);
                }
            }
            if (unknown_arg) {
                std::cout << "Unknown argument '" + current_arg + "' specified!" << std::endl;
                printHelp(false);
            }
        }

        // if the current argument is a value
        if (is_value) {
            // if the last arg was a value or flag
            if (last_arg_was_value || last_arg_was_flag) {
                throw std::runtime_error(("unexpected value " + current_arg + "!"));
            // if the last arg was not a value
            } else {
                setValue(last_arg_index, current_arg);
                last_arg_was_flag = false;
            }
        // if the current arg is not a value
        } else {
            // if the current arg type is a flag
            if (m_expected_arguments.at(arg_index).type == ArgType::BOOL) {
                last_arg_was_flag = true;
                setValue(arg_index, "true");
            // if the last argument was not a value or a flag
            } else if (!(last_arg_was_value || last_arg_was_flag)) {
                throw std::runtime_error(("no value specified for " + last_arg + "!"));
            } else {
                last_arg_was_flag = false;
            }
        }

        std::cout << "current arg: " << current_arg << std::endl;
        std::cout << "is value: " << is_value << std::endl;

        // args after flag broken

        if (!is_value) {
            // record this iteration
            last_arg           = current_arg;
            last_arg_abrv      = is_abrv;
            last_arg_index     = arg_index;
        }
        last_arg_was_value = is_value;
    }

    // check that all required arguments were present
    for (uint8_t arg = 0; arg < m_expected_arguments.size(); ++arg) {
        if (!m_expected_arguments.at(arg).read && !m_expected_arguments.at(arg).has_default) {
            // needs to print the name of the argument
            std::cout << m_expected_arguments.at(arg).name + " was not specified!" << std::endl;
            printHelp(false);
        }
    }
}

void CommandLineArgs::setValue(uint8_t arg_index, std::string value) {

    static const std::unordered_set<std::string> true_strings  = { "1", "true", "tru", "t" };
    static const std::unordered_set<std::string> false_strings = { "0", "false", "fls", "f" };

    if (arg_index < m_expected_arguments.size()) {
        bool success = true;
        switch (m_expected_arguments.at(arg_index).type) {
            case ArgType::BOOL:
                {
                    std::optional<bool> bool_value = std::nullopt;
                    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
                    if (true_strings.count(value) > 0) {
                        bool_value = true;
                    } else if (false_strings.count(value) > 0) {
                        bool_value = false;
                    } else {
                        success = false;
                        break;
                    }
                    m_expected_arguments.at(arg_index).value = bool_value.value();
                    break;
                }
            case ArgType::INT_8:
                m_expected_arguments.at(arg_index).value = (int8_t)std::stoi(value);
                break;
            case ArgType::UINT_8:
                m_expected_arguments.at(arg_index).value = (uint8_t)std::stoul(value);
                break;
            case ArgType::INT_16:
                m_expected_arguments.at(arg_index).value = (int16_t)std::stoi(value);
                break;
            case ArgType::UINT_16:
                m_expected_arguments.at(arg_index).value = (uint16_t)std::stoul(value);
                break;
            case ArgType::INT_32:
                m_expected_arguments.at(arg_index).value = (int32_t)std::stol(value);
                break;
            case ArgType::UINT_32:
                m_expected_arguments.at(arg_index).value = (uint32_t)std::stoul(value);
                break;
            case ArgType::INT_64:
                m_expected_arguments.at(arg_index).value = (int64_t)std::stoll(value);
                break;
            case ArgType::UINT_64:
                m_expected_arguments.at(arg_index).value = (uint64_t)std::stoull(value);
                break;
            case ArgType::FLOAT:
                m_expected_arguments.at(arg_index).value = std::stof(value);
                break;
            case ArgType::DOUBLE:
                m_expected_arguments.at(arg_index).value = std::stod(value);
                break;
            case ArgType::STRING:
                m_expected_arguments.at(arg_index).value = value;
                break;
            default:
                break;
        }

        if (!success) {
            throw std::runtime_error(("Could not convert '" + value + " to " + m_type_to_string.at(m_expected_arguments.at(arg_index).type) + "!"));
        } else {
            // record that the argument ahs been read
            m_expected_arguments.at(arg_index).read = true;
        }

    } else {
        throw std::runtime_error(("Argument index '" + std::to_string(int(arg_index)) + " >= arguments size (" + std::to_string(m_expected_arguments.size()) + ")!"));
    }
}

void CommandLineArgs::printHelp(bool app_details) {
    if (app_details) {
        std::cout << "Application: " << m_app_name << "\nDescription: " << m_app_description << "\n\n";
    }
    
    std::cout << "Arguments: " << std::endl;
    // for each configured argument
    for (uint8_t arg = 0; arg < m_expected_arguments.size(); ++arg) {
        std::cout << "\t ";
        if (m_expected_arguments.at(arg).name_abrv != "") {
            std::cout << "-" << m_expected_arguments.at(arg).name_abrv << ", ";
        } 
        std::cout << "--" << m_expected_arguments.at(arg).name << " - (" << m_type_to_string.at(m_expected_arguments.at(arg).type) << ") ";
        
        if (m_expected_arguments.at(arg).has_default) {
            std::cout << "[optional] ";
        }
        std::cout << m_expected_arguments.at(arg).help << std::endl;
    }
    // exit the program
    std::exit(0);
}

bool CommandLineArgs::isHelp(std::string argument) {
    // static speed sup reading but it will stick around after we're done parsing
    const static std::unordered_set<std::string> help_strings = {
        "help", "h", "hlp"
    };
    return help_strings.count(argument) > 0; 
}