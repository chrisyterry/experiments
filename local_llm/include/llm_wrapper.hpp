#pragma once
#include <string>
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
         */
        LLM(std::string model_path, float temperature = 0.0f);

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
         * @brief get the entire chat history
         * 
         * @return vector containing chat history
         */
        std::vector<std::string> getChatHistory() {return m_chat_history;};

        /**
         * @brief get the network response to the specified prompt using a chat-style interaction
         */
        std::string getChatResponse(std::string prompt);

      private:

        /**
         * @brief generates a response to the specified string
         * 
         * @param prompt the prompt to process with the LLM
         */
        void getResponseString(std::string prompt);

        // model parameters
        float m_temperature = 0.1; ///< temperature for the LLM

        // model components
        llama_model*   m_model   = nullptr;  ///< the llama model, static weights loaded from the disk into memory Usually shared between contexts
        llama_vocab*   m_vocab   = nullptr;  ///< the vocabulary, matching of strings to token IDs
        llama_sampler* m_sampler = nullptr;  ///< selects token ID from raw "logits" score for each possible token

        // chat elements
        int                      m_prev_prompt_length = 0;  ///< length of prompt from previous iteration
        std::vector<std::string> m_chat_history;  ///< current chat messages
        std::vector<char>        m_formatted_chat_messages;  ///< llama format chat messages
        const char*              m_chat_template;  ///< template for chat
 };