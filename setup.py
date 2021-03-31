from distutils.core import setup
from distutils.extension import Extension

setup(name="simple_graphs",
    ext_modules=[
        Extension("simple_graphs", ["Graph.cpp"])
    ])