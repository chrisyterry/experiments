# a simple wrapper class to emulate C/C++ pointer semantics, taken from https://stackoverflow.com/questions/1145722/simulating-pointers-in-python
class reference:
    def __init__(self, obj): self.obj = obj
    def get(self):    return self.obj
    def set(self, obj):      self.obj = obj