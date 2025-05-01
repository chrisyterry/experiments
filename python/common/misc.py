import numpy as np

# a simple wrapper class to emulate C/C++ pointer semantics, taken from https://stackoverflow.com/questions/1145722/simulating-pointers-in-python
class reference:
    def __init__(self, obj): self.obj = obj
    def get(self):    return self.obj
    def set(self, obj):      self.obj = obj

# normalize a numpy array
def nomralizeArray(array, axis=-1, order = 2):
    norm = np.atleast_1d(np.linalg.norm(a, order, axis))
    norm[norm==0] = 1
    return array / np.expand_dims(norm, axis)