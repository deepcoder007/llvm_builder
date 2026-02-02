
FROM ubuntu:noble

RUN apt update
# RUN apt -y install htop git ninja-build cmake g++ gcc libgtest-dev llvm-18 llvm-18-dev libllvm18 libz-dev libzstd-dev
RUN apt -y install htop git ninja-build cmake g++ gcc libgtest-dev libz-dev libzstd-dev python3-pip
RUN apt -y install llvm
RUN pip install cython --break-system-packages
# RUN git clone --branch llvmorg-21-init https://github.com/llvm/llvm-project.git
# RUN cd llvm-project && cmake -S llvm -B build -GNinja -DCMAKE_BUILD_TYPE=Release && cd build && ninja && ninja install

