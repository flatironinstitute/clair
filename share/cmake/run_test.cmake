message(STATUS  "Running  ${exe} -v ${srcfile}.cpp -m ${module} -- ${opt} -I${src_cur_dir}/ -fcolor-diagnostics")

MESSAGe("bin_dir = ${bin_dir}")
# Calls cpp2py
execute_process(
 COMMAND cp ${src_cur_dir}/${module}.yml ${src_cur_dir}/${srcfile}.cpp ${bin_dir}
 COMMAND ${exe} -v ${srcfile}.cpp -m ${module} -- ${opt} -I${src_cur_dir}/ -fcolor-diagnostics
 RESULT_VARIABLE command_failed
 ERROR_VARIABLE error
 OUTPUT_FILE ${module}.out
 ERROR_FILE ${module}.err
 WORKING_DIRECTORY ${bin_dir}
 TIMEOUT 600
 )

if(command_failed)
  message(FATAL_ERROR "Error runing test '${module}'. Error output:\n ${error}")
endif()


# Compare the cxx file

execute_process(
 COMMAND diff -wB --strip-trailing-cr -I "^\\s*//" ${bin_dir}/${module}.cxx ${src_cur_dir}/${refcxx}
 RESULT_VARIABLE not_successful
 OUTPUT_FILE ${module}.diff.out
 ERROR_FILE ${module}.diff.err
 OUTPUT_VARIABLE out
 ERROR_VARIABLE err
 )

if(not_successful)
  file(READ ${module}.diff.out fileOUT)
  # Status prints new line correctly, FATAL_ERROR does not ... 
  message(STATUS "File produced does not match with reference : \n${bin_dir}/${module}.cxx \n${src_cur_dir}/${refcxx} \n ${fileOUT}")
  message(FATAL_ERROR "ERROR")
endif()

