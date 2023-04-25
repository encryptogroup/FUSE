#
# FUSE Dockerfile
#
# https://github.com/norakh/FUSE
#
# authors: Github @MoritzHuppert moritz.huppert@outlook.de
#	   Github @norakh
#
# usage: docker build -t norakh/fuse -f Dockerfile .
#	 docker container run --name fuse -v . /home/user/FUSE -d [docker_image]

# Pull base image.
# old ubuntu version as it contains glibc code
# that is compatible with catch 1 used in HyCC
FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

# Install.
RUN \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y locales git make wget curl && \
  apt-get install -y libwww-perl && \
  apt-get install -y flex bison default-jdk && \
  apt-get install -y build-essential software-properties-common && \
  add-apt-repository ppa:ubuntu-toolchain-r/test && \
  apt-get install -y gcc-11 g++-11 cmake python3 && \
  apt-get install -y libssl-dev libgoogle-perftools-dev libboost-all-dev libz-dev && \
  rm -rf /var/lib/apt/lists/*

# Uncomment the en_US.UTF-8 line in /etc/locale.gen
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen
# locale-gen generates locales for all uncommented locales in /etc/locale.gen
RUN locale-gen
ENV LANG=en_US.UTF-8

# install gcc
RUN \
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 90 --slave /usr/bin/g++ g++ /usr/bin/g++-11

# temp settings: remove later when repository is public
# git clone fuse into /home/user/FUSE
WORKDIR '/home/user'
RUN \
  git clone --recurse-submodules -j8 --config core.autocrlf=input https://github.com/encryptogroup/FUSE.git

# build
WORKDIR '/home/user/FUSE'
RUN \
  python3 setup.py --setup-build

# Define default command.
CMD ["bash"]
