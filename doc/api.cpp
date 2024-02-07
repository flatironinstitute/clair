// ==========  Module declaration ==========

// Configuration of the module in the namespace c2py_module
// One module per file only
//
//  * Only the following variables, declarations, are authorized in this namespace,
//       anything else is an error (except "using namespace" clauses).
//
namespace c2py_module {

  // Name of the package if any. Default is ""
  auto package_name = "Package"; // the module will be Package.MyModule

  // The documentation string of the module. Default = ""
  auto documentation = "Module documentation";

  // -------- Automatic selection of function, classes, enums -----------
  //
  // If match_names/match_names are set, filter for all elements (functions, classes, enums).
  // Both fields are regex.
  //
  // Algorithm is (for an element X of fully qualified name Qname)
  // if match_names and not match_names match Qname : skip X
  // if reject_names and reject_names match Qname : skip X
  // else keep X.
  auto match_names = "a regex";

  auto reject_names = "a regex";

  // Only keep elements which are declared in files matching the pattern
  auto match_files = "a regex"; // TODO implement.

  // -----------------------------------------------------
  // An optional function `module_init` to be executed at the start of the module
  // Signature must be () -> void
  // nb : can be a function, or a lambda ...
  auto module_init = []() {};

  // -----------------------------------------------------
  // Add in this namespace the function, classes
  // to be manually added (or renamed) to the automatic detection.
  // in particular the template instantiation

  namespace add {

    // ----------------------------------------------------------
    // List of Python functions to be added explicitly to the module
    // in addition/replacement of the automatically detected ones.
    // The Python function is a dynamic dispatch of the list of function (ordinary or template instantiation)
    // The name of declaration is the name of the function in Python
    //
    // NB :
    // - The automatically detected functions are equivalent to an implicit declaration
    // auto f = c2py::dispatch< all accepted f overloads>
    // This implicit declaration is overruled by an explicit declaration of f

    // Examples:
    // declares a function h, dispatching the 2 instantiations.
    auto h = c2py::dispatch<N::h<int>, N::h<double>>;

    // A function fg in Python which dynamically dispatch N::f, and N::g
    // We can regroup any set of functions we want in a dispatch, mixing template instantiation, ordinary function, etc...
    auto fg = c2py::dispatch<N::f, N::g>;

    // Simply rename a function. NB: we should also reject N::f from automatic detection,
    // as this declaration does not remove the `f` function
    auto f_new_name = c2py::dispatch<N::f>;

    // ---------- Properties ----------------

    auto prop1 = c2py::property<x_getter, N::x_setter>;
    // 
    // Or tag a getter function with 
    // C2PY_PROPERTY 
    // C2PY_PROPERTY_SET(NAME)
    // 
    // getter_as_properties = "regex"; // for all classes matching the regex, the method () returning non void are transformed into a property
   

    // ----------------------------------------------------------
    // List of struct/classes to be wrapped, in addition the ones automatically detected

    // struct NameInPython = c2py::wrap<NameinC++>{};
    // some additional methods can be added to the class

    using ClsPythonName = ClsCppName;
    // .e.g ...
    using A  = N::A;
    using Bi = N::B<int>;
  
  } // namespace add

    /// Additional methods to add to the Python class
    /// e.g. method template instantiation, or new injected function h
    template <> struct add_methods_to<A> {
      // NB : for some obscure reason, C++ requires the & for a template method, but it can be omitted for regular functions
      static constexpr auto h = c2py::dispatch<&N::A<int>::h<int>, N::h<double>>;

      // A static method.
      // TODO : implement and test
      static constexpr auto h1 = c2py::dispatch_static<&N::A<int>::h<int>, N::h<double>>;
    };

    // Note. One can also make a derived struct in C++
    // with the additional methods, and wrap it, merging the parent methods by blacklisting the parent.
    // TODO : improve the test/example


  // ----------------------------------------------------------
  // Arithmetic
  // Must be declared manually, there are too many cases to auto detect.
  // TODO make the arithmetic depends on a string for clearer syntax.

  // Enumerate all the triplet A op B -> ReturnType for every op, and every class
  template <> auto arithmetic<A, "+"> = std::tuple<std::tuple<A, A, R>, ...>{};
  // ... other operators.

  // Short cuts
  // TODO implement
  template <auto op> auto arithmetic<A, op> = c2py::arithmetic::algebra<A, double, op>{};

} // namespace c2py_module
