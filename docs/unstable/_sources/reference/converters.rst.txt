.. _converters:

Converters
************

Default converters
------------------

The following types are convertible

.. list-table:: Default converters
   :widths: 30 25 25 25
   :header-rows: 1

   * - C++ type   
     - Python type
     - Bidirectional 
     - Note

   * - int/long
     - int
     - yes
     -

   * - double
     - float
     - yes
     -

   * - std::complex<double>
     - complex
     - yes
     -
       
   * - std::string
     - str
     - yes
     -

   * - std::vector<T>
     - list
     - yes
     - if T is convertible

   * - std::tuple<T...> 
     - tuple
     - yes
     - if T... are convertible

   * - std::pair<T1, T2>   
     - tuple
     - yes
     - if T1, T2 are convertible

   * - std::function<R(T...)>   
     - lambda
     - yes
     - if R, T... are convertible

   * - std::variant<T...>
     - tuple
     - yes
     - if T... are convertible

   * - std::map<K, V> 
     - dict
     - yes
     - if K, V are convertible


   * - std::array<T, N>
     - list
     - yes
     - if T is convertible

   * - std::optional<T>
     - Convertion of T or None
     - yes
     - If T is convertible

   * - std::span<std::byte>
     - bytes
     - Python -> C++ only
     -

Note that convertibility is composable: :code:`std::vector<T>` is convertible is :code:`T` is, 
so :code:`std::vector<std::tuple<T, U , W>>` is if :code:`T,U,W` are.

py_converter
------------

In order to define a custom converter,  specialize the :code:`c2py::py_converter` struct.

.. code-block:: cpp

   template <typename T> struct py_converter {
   
     // C++ to python. 
     static PyObject *c2py(auto &&x);
     
     // Python to C++ 
     // Return true iif the object can A PRIORI be converted.
     // It is a normally type check.
     // An error may still occurr in the conversion, e.g. int overflow.
     // raise_exception : in case of failure, sets an error message as a python exception.
     static bool is_convertible(PyObject *ob, bool raise_exception) noexcept;
     
     // Python to C++ 
     // Convert, assuming that is_convertible is true
     // Can still throw C++ exception.
     // Returns a T or a T & (for wrapped types).
     static [T & | T] py2c(PyObject *ob);
   };
 

Third party library [developer corner]
--------------------------------------

In order for a third party library to provide converters

* Write the specialization of the converters for the types provided by the library in e.g. `converters.hpp`

* In the library includer e.g. `my_library/my_library.hpp`, include the converters conditionally

.. code-block:: cpp

  // my_library/my_library.hpp file  
  // ...
  //
  #ifdef C2PY_INCLUDED
  #include <my_library/converters.hpp>
  #endif

* The `c2py` library defines the `C2PY_INCLUDED` macro, so including the library like

.. code-block:: cpp

  #include <c2py/c2py.hpp>
  #include <my_library/my_library.hpp>

will **automatically** include the converters when `c2py` is included.
For a concrete example, cf e.g. the 
`TRIQS nda <https://github.com/TRIQS/nda>`_ library.


