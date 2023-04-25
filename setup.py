import os
import sys
import multiprocessing
import platform

def getNumberOfThreads(args:list[str]):
    val = multiprocessing.cpu_count()
    try:
        i = args.index("-j")
        v = int(args[i+1])
        if v > 1:
            val = v
    except ValueError:
        pass # just let val stay at cpu count
    return val

def getCmakeInstallPrefix(args:list[str]):
    try:
        i = args.index("--install")
        v = args[i+1]
        return "-DCMAKE_INSTALL_PREFIX={0}".format(v)
    except ValueError:
        return ""


def setup(args:list[str]):
    print("--------------- Installing FlatBuffers ---------------")
    num_threads = getNumberOfThreads(args)
    install_pref = getCmakeInstallPrefix(args)

    os.chdir("extern/flatbuffers")
    os.system("cmake -B build {0}".format(install_pref))
    os.system("cmake --build build --parallel {0} --target install".format(num_threads))


def build(args:list[str], top_level_dir:str):
    print("--------------- Building FUSE ---------------")
    num_threads = getNumberOfThreads(args)

    os.chdir(top_level_dir)
    os.system("cmake -B build")
    os.system("cmake --build build --parallel {0}".format(num_threads))

def help():
    print("--setup       -s         \tOnly install FlatBuffers.")
    print("--setup-build -sb        \tInstall FlatBuffers, then start building FUSE with CMake.")
    print("--install <prefix_path>  \tInstall FlatBuffers to the provided prefix path, default will be the homefolder.")
    print("-j n                     \tBuild FUSE with n threads, default is number of cores.\n")
    print("Passing neither --setup nor --setup-build will only start the building process by invoking CMake.")
    print("You can also invoke CMake yourself after having the dependencies setup by running e.g.: \
                \n\tcmake -B <build_dir> \
                \n\tcmake --build <build_dir>")

def main():
    arguments = sys.argv
    top_level_dir = os.getcwd()

    if any(help_sym in arguments for help_sym in ["--help", "-h"]):
        # print help instructions
        help()
        return

    if any(setup_sym in arguments for setup_sym in ["--setup", "--setup-build", "-s", "-sb"]):
        setup(arguments)
        # only perform setup
        if any(setup_sym in arguments for setup_sym in ["--setup", "-s"]):
            return

    # perform build step in any other case
    build(arguments, top_level_dir)

if __name__ == "__main__":
    main()
