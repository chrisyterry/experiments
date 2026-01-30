from llama_cpp import Llama

llm = Llama(
    model_path = 
    n_gpu_layers = -1 # offload all layers to GPU
    n_ctx = 2048 # context size
)