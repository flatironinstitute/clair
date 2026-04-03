#pragma once
#include "./wdata.hpp"

/// Analyze all classes collected in the module.
/// For each class:
///  - find all methods and fields, including those inherited from non-wrapped base classes,
///  - find all nested classes,
///  - determines HDF5/serialization support
void scan_classes(wdata_t &wd);
