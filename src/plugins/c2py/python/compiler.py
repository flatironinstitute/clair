import importlib, os, platform, sys, shutil, subprocess, hashlib, re, tempfile, copy

debug = True
dylib_ext = "dylib" if platform.system() == "Darwin" else "so"

def print_out (m, out) :
   print(m + out)
   #l = (70 - len(m))//2
   #print(l*'-' + m + l*'-' + '\n' + out)

def execute(command, message, replacements):
    try:
       out = subprocess.check_output(command, stderr=subprocess.STDOUT, shell=True)
       return
    except subprocess.CalledProcessError as E:
       err = E.output.decode('utf8')
       for (k,v) in replacements.items():
           err = err.replace(k,v)
       print_out ("Error: \n" + message, err)
    raise RuntimeError("Error in executing command: \n %s"%command)

#------------------------------------------

class ClangInvocation:
    """
       How to call the compiler 
    """
    def __init__(self, cpp_preamble = "", env_preamble = "", flags= "", command = None):
        # FIXME : should I expand clair_flags ?
        self.command =  command or "clang++ -fplugin=clair_c2py.%s `c2py_flags` -std=c++20 -shared -o {m}.so {m}.cpp -fdiagnostics-color=always %s "%(dylib_ext, flags)
        self.cpp_preamble = cpp_preamble
        # We export the (DY)LD_LIBRARY_PATH from the current shell to the subprocess...
        self.env_preamble = "export DYLD_LIBRARY_PATH=%s:$DYLD_LIBRARY_PATH"%os.environ["DYLD_LIBRARY_PATH"] + "\n" + env_preamble

    def copy(self):
        copy.deepcopy(self)

    def prepare_code(self, code):
        # Put in a specific namespace __cpp2py_anonymous ?
        lines = code.split('\n')
        #pos = next(n for n, l in enumerate(lines) if l.strip() and not l.strip().startswith('#'))
        return '\n'.join(["#include <c2py/c2py.hpp>"]  + self.cpp_preamble.split('\n')  + lines)
        # Seems not useful any more    
        #"#include <c2py/py_stream.hpp>"
        # code = re.sub("std::cout", "cpp2py::py_stream()", code)

# ------------------------------------------

compile_instructions = {'default' : ClangInvocation()}

# ------------------------------------------

def compile(code, verbosity = 0, compile_instruction_name = 'default', only=(), cxxflags= '', moduledir = '/tmp', recompile = False, no_clean = debug):
    """
    Takes the c++ code, call c++2py on it and compile the whole thing into a module.
    """
    # Additional Compiler Flags
    # if os.getenv('CXXFLAGS'): cxxflags = os.getenv('CXXFLAGS') + " " + cxxflags

    # Find the proper ClangInvocation
    try:
        cl_invoc = compile_instructions[compile_instruction_name]
    except:
        raise RuntimeError ("The compilation instructions are %s \n\n %s is not one of them"%(compile_instructions, compile_instruction_name))

    # prepare the code
    code = cl_invoc.prepare_code(code)
    #print(code)

    # Compute the temporary file name
    key = code, sys.version_info, sys.executable, cxxflags, only
    dir_name = hashlib.md5(str(key).encode('utf-8')).hexdigest().strip()
    module_name = "c2py_ipython_magic_%s"%dir_name
    module_dirname = moduledir + '/c2py_' + dir_name
    module_fullname = module_dirname + '/' + module_name

    if not os.path.exists(module_dirname) or recompile:
        try :
            os.mkdir(module_dirname)
        except :
            pass

        old_cwd = os.getcwd()
        try:
            os.chdir(module_dirname)

            with open('{}.cpp'.format(module_name), 'w') as f:
                f.write(code)
            cmd = '\n'.join([cl_invoc.env_preamble, cl_invoc.command])
            cmd_for_machine = cmd.format(m = module_name)
            cmd_for_log = "Compilation command: " + cmd.format(m = "[Cell]") + '\n'
            execute (cmd_for_machine, cmd_for_log if verbosity else "", {module_name:"[Cell]", module_dirname:""})

        except: # we clean if fail
            os.chdir(old_cwd)
            if not no_clean : shutil.rmtree(module_dirname)
            raise

        finally:
            os.chdir(old_cwd)

    sys.path.insert(0, module_dirname)
    module = importlib.import_module(module_name)
    sys.path = sys.path[1:]
    module.workdir = module_dirname
    
    # Clean anyway
    if not debug:
        shutil.rmtree(module_dirname)

    return module
