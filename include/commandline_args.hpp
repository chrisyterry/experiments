
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <variant>
#include <iostream>
#include "misc.hpp"
/**
 * @brief class to parse command line arguments process of usage:
 * 
 *  1) instantiate class with application name and description
 *  2) use addArgument to add the desired arguments
 *  3) call parse with the inputs given to the main/entry function to load values into arguments
 *  4) use getArgument to obtain the value read for the argument
 *
 */
class CommandLineArgs {

    using ArgValue = std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string>;

    public:

    /**
     * @brief constructor for command line arguments parser
     * 
     * @param app_name name of the app
     * @param app_description description of the app
     */
      CommandLineArgs(std::string name, std::string app_description);

    /**
     * @brief add an expected argument
     * @note templated methods should be in the include file as the compiler can't see the cpp and hpp header templates simultaneously
     * 
     * @param name the full name of the argument
     * @param help help string for argument
     * @param abbreviation [optional] abbreviated name of the argument
     * @param default_val [optional] default value of the argument; if this is set the argument is considered optional
     */
      template <typename T>
      void addArgument(std::string name, std::string help, std::string abbreviation = "", std::optional<T> default_val = std::nullopt) {
          Argument new_argument;

          // get name and help
          new_argument.name = name;
          new_argument.help = help;
          if (abbreviation != "") {
              new_argument.name_abrv = abbreviation;
          }

          // set the type for the argument
          std::string type_name = getTypeString<T>();
          if (m_string_to_type.count(type_name) > 0) {
            new_argument.type = m_string_to_type.at(type_name);
          } else {
            new_argument.type = ArgType::STRING;
          }

          // if a default value has been specified
          if (default_val.has_value()) {
              new_argument.value       = default_val.value();
              new_argument.has_default = true;
          }
          // add entries in the argument name index lookups
          m_argument_name_indexes.insert({ name, m_expected_arguments.size() });
          if (abbreviation != "") {
              m_argument_abrv_indexes.insert({ abbreviation, m_expected_arguments.size() });
          }

          // add the argument to the list of expected arguments
          m_expected_arguments.emplace_back(new_argument);
      }

      /**
       * @brief add a flag to the commandline parser
       */
      void addFlag(std::string name, std::string help, std::string abbreviation = "") {
        addArgument<bool>(name, help, abbreviation, false);
      }

    /**
     * @brief parse the commandline arguments
     * 
     * @param argc the number of input arguments (pass 'argc' from main)
     * @param argv the command line arguments array (pass 'argv' from main)
     */
    void parse(const int argc, char* argv[]);

    /**
     * @brief get the value for the argument for the specified name
     *
     * @param name the name of the argument
     *
     * @return value of the argument (if found)
     */
    template <typename T>
    T getArgument(std::string name) {
        // get the name as lower case
        std::string lowered_name = name;
        std::transform(lowered_name.begin(), lowered_name.end(), lowered_name.begin(), [](unsigned char c) { return std::tolower(c); });
        uint8_t arg_index = 0;
        if (m_argument_name_indexes.count(lowered_name) > 0) {
            arg_index = m_argument_name_indexes.at(lowered_name);
        } else if (m_argument_abrv_indexes.count(lowered_name) > 0) {
            arg_index = m_argument_abrv_indexes.at(lowered_name);
        } else {
            throw std::runtime_error(("Could not find argument with name '" + name + "'!"));
        }

        // get the value held for the argument
        if (std::holds_alternative<T>(m_expected_arguments.at(arg_index).value)) {
            return std::get<T>(m_expected_arguments.at(arg_index).value);
        } else {
            throw std::runtime_error(("Argument '" + name + "' type " + m_type_to_string.at(m_expected_arguments.at(arg_index).type) + " does not match provided template!"));
        }
    }

    private:
      /**
       * @brief types for input arguments
       */
      enum class ArgType : uint8_t {
          BOOL,
          INT_8,
          UINT_8,
          INT_16,
          UINT_16,
          INT_32,
          UINT_32,
          INT_64,
          UINT_64,
          FLOAT,
          DOUBLE,
          STRING,
      };

      const std::unordered_map<std::string, ArgType> m_string_to_type = {
          { "bool", ArgType::BOOL },
          { "signed char", ArgType::INT_8 },
          { "unsigned char", ArgType::UINT_8 },
          { "short", ArgType::INT_16 },
          { "unsigned short", ArgType::UINT_16 },
          { "int", ArgType::INT_32 },
          { "unsigned int", ArgType::UINT_32 },
          { "long", ArgType::INT_64 },
          { "unsigned long", ArgType::UINT_64 },
          { "float", ArgType::FLOAT },
          { "double", ArgType::DOUBLE },
          { "string", ArgType::STRING },
      };  ///< conversion between string and argument type

      const std::unordered_map<ArgType, std::string> m_type_to_string = {
          { ArgType::BOOL, "bool" },
          { ArgType::INT_8, "signed char" },
          { ArgType::UINT_8, "unsigned char" },
          { ArgType::INT_16, "short" },
          { ArgType::UINT_16, "unsigned short" },
          { ArgType::INT_32, "int" },
          { ArgType::UINT_32, "unsigned int" },
          { ArgType::INT_64, "long" },
          { ArgType::UINT_64, "unsigned long" },
          { ArgType::FLOAT, "float" },
          { ArgType::DOUBLE, "double" },
          { ArgType::STRING, "string" },
      };  ///< conversion between argument type and string

      /**
       * @brief struct holding command line argument
       */
      struct Argument {
          std::string name;  ///< name of argument
          std::string name_abrv = "";  ///< abbreviated name
          std::string help      = "";  ///< help string
          ArgType     type;  ///< the type of the value
          ArgValue    value;  ///< value for the argument
          bool        has_default = false;  ///< whether this argument has a default set
          bool        read        = false;  ///< whether the argument has been read from the commandline
      };

      std::string                              m_app_name;  ///< name of the app
      std::string                              m_app_description;  ///< description of the app
      std::vector<Argument>                    m_expected_arguments;  ///< expected arguments
      std::unordered_map<std::string, uint8_t> m_argument_name_indexes;  ///< mapping between argument name and data index
      std::unordered_map<std::string, uint8_t> m_argument_abrv_indexes;  ///< mapping between abbreviated argument name and data

      /**
       * @brief set the value for the specified string at the specified index
       * 
       * @param arg_index the index of the argument to be set
       * @param value string containing the value at the index
       */
      void setValue(uint8_t arg_index, std::string value);

      /**
       * @brief print help for the commandline arguments and exit the program
       * 
       * @param app_details [optional] whether to include app details in the help console output
       */
      void printHelp(bool app_details = true);

      /**
       * @brief check if the specified argument is a call for help
       * 
       * @param argument the argument to check; assumed to be lower case
       * 
       * @return wether the argument is a request for help
       */
      bool isHelp(std::string argument);
};