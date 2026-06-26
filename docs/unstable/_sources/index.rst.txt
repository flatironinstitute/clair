.. _welcome:

clair/c2py
**********

.. sidebar:: clair |PROJECT_VERSION|

   This is the homepage of clair/c2py |PROJECT_VERSION|.
   
   For changes, see the :ref:`changelog page <changelog>`.
      
      .. image:: _static/logo_github.png
         :width: 75%
         :align: center
         :target: https://github.com/flatironinstitute/clair


clair-c2py is a Clang tool that automatically generates Python bindings for C++ code.
It is paired with **c2py**, a lightweight C++20 runtime library that the generated bindings rely on.
Its main characteristics are:  

* **Automatic**: discovers functions, classes in C++ source and generates the Python bindings.
  **No need to write any binding code manually !**

* **Documentation**: transcribes C++ Doxygen comments to Python/NumPy docstrings, including 
  LaTeX to RST translation. 
  No need to maintain separate documentation for C++ and Python!

* **Modern C++**: full support of recent C++ standards (C++20, C++23).

* **Easy CMake integration**.
* **Lightweight**: uses a small C++20 support library (c2py) for easy deployment.
* **Portable**: can be used in combination with any C++20 compiler.

*NB: this project is an evolution of the* `TRIQS/cpp2py <https://github.com/TRIQS/cpp2py>`_ *tool, used in the TRIQS library
since 2014. It is fully backward compatible.*

.. toctree::
   :maxdepth: 2
   :hidden:

   install
   getting_started/index
   gallery/index
   reference/index
   notebook
   issues
   changelog
   about
