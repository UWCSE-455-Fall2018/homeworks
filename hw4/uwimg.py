import sys, os
from ctypes import *
import math
import random

lib = CDLL(os.path.join(os.path.dirname(__file__), "libuwimg.so"), RTLD_GLOBAL)

def c_array(ctype, values):
    arr = (ctype*len(values))()
    arr[:] = values
    return arr

class IMAGE(Structure):
    _fields_ = [("w", c_int),
                ("h", c_int),
                ("c", c_int),
                ("data", POINTER(c_float))]
    def __add__(self, other):
        return add_image(self, other)
    def __sub__(self, other):
        return sub_image(self, other)

class POINT(Structure):
    _fields_ = [("x", c_float),
                ("y", c_float)]

class DESCRIPTOR(Structure):
    _fields_ = [("p", POINT),
                ("n", c_int),
                ("data", POINTER(c_float))]

class MATRIX(Structure):
    _fields_ = [("rows", c_int),
                ("cols", c_int),
                ("data", POINTER(POINTER(c_double))),
                ("shallow", c_int)]

class DATA(Structure):
    _fields_ = [("X", MATRIX),
                ("y", MATRIX)]

class LAYER(Structure):
    _fields_ = [("in", MATRIX),
                ("dw", MATRIX),
                ("w", MATRIX),
                ("v", MATRIX),
                ("out", MATRIX),
                ("activation", c_int)]

class MODEL(Structure):
    _fields_ = [("layers", POINTER(LAYER)),
                ("n", c_int)]


(LINEAR, LOGISTIC, TANH, RELU, LRELU, SOFTMAX) = range(6)


add_image = lib.add_image
add_image.argtypes = [IMAGE, IMAGE]
add_image.restype = IMAGE

sub_image = lib.sub_image
sub_image.argtypes = [IMAGE, IMAGE]
sub_image.restype = IMAGE

make_image = lib.make_image
make_image.argtypes = [c_int, c_int, c_int]
make_image.restype = IMAGE

free_image = lib.free_image
free_image.argtypes = [IMAGE]

load_image_lib = lib.load_image
load_image_lib.argtypes = [c_char_p]
load_image_lib.restype = IMAGE

def load_image(f):
    return load_image_lib(f.encode('ascii'))

save_png_lib = lib.save_png
save_png_lib.argtypes = [IMAGE, c_char_p]
save_png_lib.restype = None

def save_png(im, f):
    return save_png_lib(im, f.encode('ascii'))

save_image_lib = lib.save_image
save_image_lib.argtypes = [IMAGE, c_char_p]
save_image_lib.restype = None

def save_image(im, f):
    return save_image_lib(im, f.encode('ascii'))

same_image = lib.same_image
same_image.argtypes = [IMAGE, IMAGE]
same_image.restype = c_int

train_model = lib.train_model
train_model.argtypes = [MODEL, DATA, c_int, c_int, c_double, c_double, c_double]
train_model.restype = None

accuracy_model = lib.accuracy_model
accuracy_model.argtypes = [MODEL, DATA]
accuracy_model.restype = c_double

forward_model = lib.forward_model
forward_model.argtypes = [MODEL, MATRIX]
forward_model.restype = MATRIX

load_classification_data = lib.load_classification_data
load_classification_data.argtypes = [c_char_p, c_char_p, c_int]
load_classification_data.restype = DATA

make_layer = lib.make_layer
make_layer.argtypes = [c_int, c_int, c_int]
make_layer.restype = LAYER

def make_model(layers):
    m = MODEL()
    m.n = len(layers)
    m.layers = (LAYER*m.n) (*layers)
    return m

if __name__ == "__main__":
    im = load_image("data/dog.jpg")
    save_image(im, "hey")

