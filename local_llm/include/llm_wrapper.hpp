#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "llama.h"

 /**
  * @brief wrapper class for LLM using llama.cpp
  */
 class LLM {
    public:
        /**
         * @brief LLM class constructor
         * 
         * @param model_path path to the model
         * @param temperature [optional] temperature to use for sampling 
         * @param print_debug whether to print llama and class debug messages
         */
        LLM(std::string model_path, float temperature = 0.0f, bool print_debug = false);

        /**
         * @brief LLM class destructor
         */
        ~LLM();

        /**
         * @brief clear the chat history and associated caches
         */
        void clearChat();

        /**
         * @brief set the temperature for the network
         * 
         * @param temperature temperature for token sampling, should be in range 0.1-1.0
         */
        void setTemperature(float temperature) {
          m_temperature = std::clamp(temperature, 0.1f, 1.0f);
        }

        /**
         * @brief get the network response to the specified prompt using a chat-style interaction
         */
        std::string getChatResponse(std::string prompt);

      private:

        /**
         * @brief generates a response to the specified string
         * 
         * @param prompt the prompt to process with the LLM
         * 
         * @return the response from the LLM
         */
        std::string getResponseString(std::string prompt);

        // model parameters
        float m_temperature = 0.1; ///< temperature for the LLM

        // model components
        llama_model*       m_model   = nullptr;  ///< the llama model, static weights loaded from the disk into memory Usually shared between contexts
        const llama_vocab* m_vocab   = nullptr;  ///< the vocabulary, matching of strings to token IDs
        llama_sampler*     m_sampler = nullptr;  ///< selects token ID from raw "logits" score for each possible token

        // chat elements
        llama_context*                  m_context            = nullptr;  ///< the context for the current session
        int                             m_prev_prompt_length = 0;  ///< length of prompt from previous iteration
        std::vector<llama_chat_message> m_chat_history;  ///< current chat messages
        std::vector<char>               m_formatted_chat_messages;  ///< llama format chat messages
        const char*                     m_chat_template;  ///< template for chat
 };