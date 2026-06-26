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
     - numpy array / list
     - yes
     - numpy array if T has a numpy dtype, else list (if T is convertible)

   * - std::tuple<T...> 
     - tuple
     - yes
     - if T... are convertible

   * - std::pair<T1, T2>   
     - tuple
     - yes
     - if T1, T2 are convertible

   * - std::function<R(T...)>   
     - Callable
     - yes
     - if R, T... are convertible

   * - std::variant<T...>
     - T... (first match)
     - yes
     - converts to/from the first matching alternative

   * - std::map<K, V> 
     - dict
     - yes
     - if K, V are convertible

   * - std::set<K>
     - set
     - yes
     - if K is convertible


   * - std::array<T, N>
     - list
     - yes
     - if T is convertible

   * - std::optional<T>
     - Conversion of T or None
     - yes
     - If T is convertible

   * - std::span<std::byte>
     - bytes
     - Python -> C++ only
     -

Note that convertibility is composable: :code:`std::vector<T>` is convertible if :code:`T` is, 
so :code:`std::vector<std::tuple<T, U , W>>` is if :code:`T,U,W` are.

Custom converters for user-defined types
----------------------------------------

In some cases, it may be necessary to define custom converters for user-defined types
into some *existing* Python type. 
Note that this is different from wrapping a C++ class to a Python, which creates a **new** Python type. 

In order to define a custom converter, specialize the :code:`c2py::py_converter` struct.

.. code-block:: cpp

   template <typename T> struct py_converter {
   
     // [Optional] Name of the Python type, used in error messages and docstrings.
     // Can be a static constexpr string or a static function returning std::string.
     static constexpr const char *tp_name = "...";
     // or: static std::string tp_name() { return "..."; }
   
     // C++ to Python. 
     static PyObject *c2py(auto &&x);
     
     // Python to C++ 
     // Return true iff the object can A PRIORI be converted.
     // This is normally a type check.
     // An error may still occur in the conversion, e.g. int overflow.
     // raise_exception: in case of failure, sets an error message as a Python exception.
     static bool is_convertible(PyObject *ob, bool raise_exception) noexcept;
     
     // Python to C++ 
     // Convert, assuming that is_convertible is true.
     // Can still throw C++ exceptions.
     // Returns a T or a T& (for wrapped types).
     static [T& | T] py2c(PyObject *ob);
   };
 

.. _third_party_lib:

Set up for third party libraries
--------------------------------

If you are developing a third-party C++ library and 
want to provide specializations for some of the types defined by the library, 
we recommend placing them in a separate header file (e.g., `converters.hpp`). 
To avoid polluting your library's main header for non-Python users, conditionally include this file only when building with clair/c2py:

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

.. note::

  The c2py library must be included **first**, hence before any library header that defines custom converters.
  ``clair-c2py`` will *reject* the code otherwise, in order to ensure that the C2PY_INCLUDED macros is properly 
  defined before any custom library headers are included.
