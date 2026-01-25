#include "llm_wrapper.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>

LLM::LLM(std::string model_path, float temperature, bool print_debug) {

    if (!print_debug) {
        // only print errors
        llama_log_set([](enum ggml_log_level level, const char* text, void* /* user_data */) {
            if (level >= GGML_LOG_LEVEL_ERROR) {
                fprintf(stderr, "%s", text);
            }
        },
                      nullptr);
    }

    setTemperature(temperature);

    // initialize llama cpp backend
    ggml_backend_load_all();

    // setup model parameters
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers       = 100;  // offload all processing to GPU

    // load model
    m_model = llama_model_load_from_file("/home/chriz/Downloads/Llama-3.2-3B-Instruct-uncensored-Q6_K.gguf", model_params);
    if (!m_model) {
        std::cout << "Blyat comrade, model failed" << std::endl;
        std::exit(1);
    }

    // setup model vocabulary
    m_vocab = llama_model_get_vocab(m_model);

    // setup context
    auto context_parameters    = llama_context_default_params();
    context_parameters.n_ctx   = 2048;  // context size in tokens
    context_parameters.n_batch = context_parameters.n_ctx;  // number of tokens processed in each call to model
    m_context                  = llama_init_from_model(m_model, context_parameters);
    if (!m_context) {
        std::cout << "Blyat comrade, model context failed" << std::endl;
        std::exit(1);
    }

    // setup sampler
    m_sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(m_sampler, llama_sampler_init_min_p(0.05f, 1));  // filter low probability noise
    llama_sampler_chain_add(m_sampler, llama_sampler_init_temp(temperature));  // level of creativity
    llama_sampler_chain_add(m_sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    // get the chat template
    m_chat_template = llama_model_chat_template(m_model, nullptr);

    // setup vector for llama chat messages
    m_formatted_chat_messages = std::vector<char>(llama_n_ctx(m_context));
}

LLM::~LLM() {

    // clear chat messages
    for (auto& msg : m_chat_history) {
        free(const_cast<char*>(msg.content));
    }

    // free resources
    llama_sampler_free(m_sampler);
    llama_free(m_context);
    llama_model_free(m_model);
}

void LLM::clearChat() {
}

std::string LLM::getChatResponse(std::string prompt) {
    // add the user input to the message list and format it
    m_chat_history.push_back({ "user: ", strdup(prompt.c_str()) });
    // copy chat messages to raw chars in model format
    int new_len = llama_chat_apply_template(m_chat_template, m_chat_history.data(), m_chat_history.size(), true, m_formatted_chat_messages.data(), m_formatted_chat_messages.size());
    if (new_len > (int)m_formatted_chat_messages.size()) {
        m_formatted_chat_messages.resize(new_len);
        new_len = llama_chat_apply_template(m_chat_template, m_chat_history.data(), m_chat_history.size(), true, m_formatted_chat_messages.data(), m_formatted_chat_messages.size());
    }
    if (new_len < 0) {
        std::cout << "could not apply chat template!" << std::endl;
        std::exit(1);
    }

    // remove previous messages to obtain the prompt to give to the LLM to generate the response
    std::string llm_input(m_formatted_chat_messages.begin() + m_prev_prompt_length, m_formatted_chat_messages.begin() + new_len);

    // generate a response
    std::string response = getResponseString(llm_input);

    // add the response to the messages
    m_chat_history.push_back({ "machine: ", strdup(response.c_str()) });
    m_prev_prompt_length = llama_chat_apply_template(m_chat_template, m_chat_history.data(), m_chat_history.size(), false, nullptr, 0);
    if (m_prev_prompt_length < 0) {
        std::cout << "failed to apply chat template!" << std::endl;
        std::exit(1);
    }

    return m_chat_history.back().content;
}

std::string LLM::getResponseString(std::string prompt) {
    // check if this is the first turn
    const bool is_first = llama_memory_seq_pos_max(llama_get_memory(m_context), 0) == -1;

    // tokenize the input string
    const int num_tokens = std::abs(llama_tokenize(m_vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true));  // call first with null to get buffer size; a negative number means the buffer is too small
    std::cout << "num tokens: " << num_tokens << std::endl;
    std::vector<llama_token> prompt_tokens(num_tokens);
    llama_tokenize(m_vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true);  // fill out tokens

    // get batch to tell llama which tokens to process
    llama_batch token_batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
    llama_token new_token_id;

    // string to hold network response
    std::string response;

    // process the tokens
    while (true) {
        int n_ctx      = llama_n_ctx(m_context);  // total token capacity
        int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(m_context), 0) + 1;  // current position in input token vector

        // check that we still have context space available
        if (n_ctx_used + token_batch.n_tokens > n_ctx) {
            std::cout << "blyat commrade, context exceeded!" << std::endl;
        }

        // run the token through the network; updates KV-cache in context with batch tokens
        int ret = llama_decode(m_context, token_batch);
        if (ret != 0) {
            // decode failure!
            std::cout << "Decode failure" << std::endl;
            GGML_ABORT("failed to decode");
        }

        // take output from network and picks best token; -1 selects last token in batch
        new_token_id = llama_sampler_sample(m_sampler, m_context, -1);

        // check for end of output
        if (llama_vocab_is_eog(m_vocab, new_token_id)) {
            break;
        }

        // convert integer token back into string
        char        char_buffer[256];
        int         n = llama_token_to_piece(m_vocab, new_token_id, char_buffer, sizeof(char_buffer), 0, true);
        std::string response_piece(char_buffer, n);

        // add the string to the output
        response += response_piece;

        // take generated token and put it into new batch to feed back to the model
        token_batch = llama_batch_get_one(&new_token_id, 1);
    }

    return response;
}