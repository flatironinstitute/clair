namespace c2py_module {

  // Name of the package if any. Default is ""
  auto package_name = "Package"; // the module will be Package.MyModule

  // The documentation string of the module. Default = ""
  auto documentation = "Module documentation";

  // An function (or lambda) `module_init` to be executed at the start of the module
  // Signature must be () -> void
  auto module_init = []() {};

} // namespace c2py_module
