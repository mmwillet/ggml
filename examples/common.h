// Various helper functions and utilities

#pragma once

#include <string>
#include <map>
#include <vector>
#include <random>
#include <thread>

#define COMMON_SAMPLE_RATE 16000

//
// GPT CLI argument parsing
//

struct gpt_params {
    int32_t seed       = -1;   // RNG seed
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_predict  = 200;  // new tokens to predict
    int32_t n_parallel = 1;    // number of parallel streams
    int32_t n_batch    = 8;    // batch size for prompt processing
    int32_t n_ctx      = 2048; // context size (this is the KV cache max size)

    // sampling parameters
    int32_t top_k          = 40;
    float   top_p          = 0.9f;
    float   temp           = 0.9f;
    int32_t repeat_last_n  = 64;
    float   repeat_penalty = 1.00f;

    std::string model      = "models/gpt-2-117M/ggml-model.bin"; // model path
    std::string prompt     = "";
    std::string token_test = "";

    bool    interactive      = false;
    int32_t interactive_port = -1;

    int32_t n_gpu_layers     = 0;
};

bool gpt_params_parse(int argc, char ** argv, gpt_params & params);

void gpt_print_usage(int argc, char ** argv, const gpt_params & params);

std::string gpt_random_prompt(std::mt19937 & rng);

//
// Vocab utils
//

std::string trim(const std::string & s);

std::string replace(
        const std::string & s,
        const std::string & from,
        const std::string & to);

struct gpt_vocab {
    using id    = int32_t;
    using token = std::string;

    std::map<token, id> token_to_id;
    std::map<id, token> id_to_token;
    std::vector<std::string> special_tokens;

    void add_special_token(const std::string & token);
};

// poor-man's JSON parsing
std::map<std::string, int32_t> json_parse(const std::string & fname);

std::string convert_to_utf8(const std::wstring & input);

std::wstring convert_to_wstring(const std::string & input);

void gpt_split_words(std::string str, std::vector<std::string>& words);

// split text into tokens
//
// ref: https://github.com/openai/gpt-2/blob/a74da5d99abaaba920de8131d64da2862a8f213b/src/encoder.py#L53
//
// Regex (Python):
// r"""'s|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+"""
//
// Regex (C++):
// R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alpha:][:digit:]]+|\s+(?!\S)|\s+)"
//
std::vector<gpt_vocab::id> gpt_tokenize(const gpt_vocab & vocab, const std::string & text);

// test outputs of gpt_tokenize
//
//   - compare with tokens generated by the huggingface tokenizer
//   - test cases are chosen based on the model's main language (under 'prompt' directory)
//   - if all sentences are tokenized identically, print 'All tests passed.'
//   - otherwise, print sentence, huggingface tokens, ggml tokens
//
void test_gpt_tokenizer(gpt_vocab & vocab, const std::string & fpath_test);

// load the tokens from encoder.json
bool gpt_vocab_init(const std::string & fname, gpt_vocab & vocab);

// sample next token given probabilities for each embedding
//
//   - consider only the top K tokens
//   - from them, consider only the top tokens with cumulative probability > P
//
// TODO: not sure if this implementation is correct
// TODO: temperature is not implemented
//
gpt_vocab::id gpt_sample_top_k_top_p(
        const gpt_vocab & vocab,
        const float * logits,
        int    top_k,
        double top_p,
        double temp,
        std::mt19937 & rng);

gpt_vocab::id gpt_sample_top_k_top_p_repeat(
        const gpt_vocab & vocab,
        const float * logits,
        const int32_t * last_n_tokens_data,
        size_t last_n_tokens_data_size,
        int    top_k,
        double top_p,
        double temp,
        int repeat_last_n,
        float repeat_penalty,
        std::mt19937 & rng);

//
// Audio utils
//

// Read WAV audio file and store the PCM data into pcmf32
// The sample rate of the audio must be equal to COMMON_SAMPLE_RATE
// If stereo flag is set and the audio has 2 channels, the pcmf32s will contain 2 channel PCM
bool read_wav(
        const std::string & fname,
        std::vector<float> & pcmf32,
        std::vector<std::vector<float>> & pcmf32s,
        bool stereo);

// Apply a high-pass frequency filter to PCM audio
// Suppresses frequencies below cutoff Hz
void high_pass_filter(
        std::vector<float> & data,
        float cutoff,
        float sample_rate);

// Basic voice activity detection (VAD) using audio energy adaptive threshold
bool vad_simple(
        std::vector<float> & pcmf32,
        int   sample_rate,
        int   last_ms,
        float vad_thold,
        float freq_thold,
        bool  verbose);

// compute similarity between two strings using Levenshtein distance
float similarity(const std::string & s0, const std::string & s1);

//
// SAM argument parsing
//

struct sam_params {
    int32_t seed      = -1; // RNG seed
    int32_t n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());

    std::string model     = "models/sam-vit-b/ggml-model-f16.bin"; // model path
    std::string fname_inp = "img.jpg";
    std::string fname_out = "img.out";
};

bool sam_params_parse(int argc, char ** argv, sam_params & params);

void sam_print_usage(int argc, char ** argv, const sam_params & params);
