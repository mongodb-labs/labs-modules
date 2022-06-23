import os
import os.path
import SCons.Script.Main

def configure(conf, env):
    root = os.path.dirname(__file__)

    modules = [
#        "external",
        "stream"
    ]

    env['MONGO_LABS_FEATURES'] = modules


    # Compute a path to use for including files that are generated in the build directory.
    # Computes: $BUILD_DIR/mongo/db/modules/<module_name>/src
    src_include_path = os.path.join("$BUILD_DIR", str(env.Dir(root).Dir('src'))[4:] )

    def injectLabsModule(env):
        # Inject an include path so that idl generated files can be included
        env.Append(CPPPATH=[src_include_path])

        # Inject an import path so that idl files can import other idl files in the labs module repo
        # Computes: src/mongo/db/modules/<module_name>/src
        env.Append(IDLCFLAGS=["--include", str(env.Dir(root).Dir('src'))])

        return env

    env['MODULE_INJECTORS']['labs'] = injectLabsModule

    return modules
