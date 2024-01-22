import os
import sys
import multiprocessing
import platform


def getNumberOfThreads(args: list[str]):
    val = multiprocessing.cpu_count()
    try:
        i = args.index("-j")
        v = int(args[i + 1])
        if v > 1:
            val = v
    except ValueError:
        pass  # just let val stay at cpu count
    return val


def getCmakeInstallPrefix(args: list[str]):
    try:
        i = args.index("--install")
        v = args[i + 1]
        return "-DCMAKE_INSTALL_PREFIX={0}".format(v)
    except ValueError:
        return ""


def setup(args: list[str]):
    print("--------------- Installing FlatBuffers ---------------")
    num_threads = getNumberOfThreads(args)
    install_pref = getCmakeInstallPrefix(args)
    # updating flatbuffers both in MOTION and FUSE at the same time to avoid version mismatch
    os.chdir("extern/MOTION/extern/flatbuffers")
    os.system(
        'git fetch --tags && latestTag=$(git describe --tags "$(git rev-list --tags --max-count=1)") && git checkout $latestTag'
    )
    os.chdir("../../../flatbuffers")
    os.system(
        'git fetch --tags && latestTag=$(git describe --tags "$(git rev-list --tags --max-count=1)") && git checkout $latestTag'
    )
    os.system("cmake -B build {0}".format(install_pref))
    os.system("cmake --build build --parallel {0} --target install".format(num_threads))


def build(args: list[str], top_level_dir: str):
    print("--------------- Building FUSE ---------------")
    num_threads = getNumberOfThreads(args)

    os.chdir(top_level_dir)
    os.system("cmake -B build")
    os.system("cmake --build build --parallel {0}".format(num_threads))


def dependencies():
    print("--------------- Installing Dependencies ---------------")
    os.system("apt-get update")
    os.system(
        "apt-get install build-essential software-properties-common git cmake libssl-dev libgoogle-perftools-dev libboost-all-dev libz-dev curl libwww-perl"
    )
    # HyCC stuff: needs libwww-perl
    os.system("apt-get clean && apt-get install -y locales")
    os.system("locale-gen en_US.UTF-8 && apt-get install flex bison default-jdk")


def help():
    print("--setup       -s         \tOnly install FlatBuffers.")
    print(
        "--setup-build -sb        \tInstall FlatBuffers, then start building FUSE with CMake."
    )
    print("--dependencies -d        \tInstall all dependencies")
    print(
        "--install <prefix_path>  \tInstall FlatBuffers to the provided prefix path, default will be the homefolder."
    )
    print(
        "-j n                     \tBuild FUSE with n threads, default is number of cores.\n"
    )
    print(
        "Passing neither --setup nor --setup-build will only start the building process by invoking CMake."
    )
    print(
        "You can also invoke CMake yourself after having the dependencies setup by running e.g.: \
                \n\tcmake -B <build_dir> \
                \n\tcmake --build <build_dir>"
    )


def main():
    arguments = sys.argv
    top_level_dir = os.getcwd()

    if any(help_sym in arguments for help_sym in ["--help", "-h"]):
        # print help instructions
        help()
        return

    if any(dep_sym in arguments for dep_sym in ["--dependencies", "-d"]):
        # print help instructions
        dependencies()
        return

    if any(
        setup_sym in arguments
        for setup_sym in ["--setup", "--setup-build", "-s", "-sb"]
    ):
        setup(arguments)
        # only perform setup
        if any(setup_sym in arguments for setup_sym in ["--setup", "-s"]):
            return

    # perform build step in any other case
    build(arguments, top_level_dir)


if __name__ == "__main__":
    main()
