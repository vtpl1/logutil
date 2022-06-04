ARG VARIANT="18.04"
FROM ubuntu:${VARIANT}

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends ca-certificates software-properties-common \
    git curl wget ninja-build build-essential gdb python3-pip python3-setuptools \
    pkg-config zip unzip tar iputils-ping ccache

RUN wget -O cmake.sh https://github.com/Kitware/CMake/releases/download/v3.22.4/cmake-3.22.4-linux-x86_64.sh \
    && sh cmake.sh --prefix=/usr/local/ --exclude-subdir && rm -rf cmake.sh

RUN apt-get update && apt-get -y install gpg-agent

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add -

ARG LLVM_VERSION=13
RUN add-apt-repository "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-$LLVM_VERSION main"

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install clang-$LLVM_VERSION lldb-$LLVM_VERSION lld-$LLVM_VERSION clangd-$LLVM_VERSION \
    clang-tidy-$LLVM_VERSION clang-format-$LLVM_VERSION clang-tools-$LLVM_VERSION \
    llvm-$LLVM_VERSION-dev lld-$LLVM_VERSION lldb-$LLVM_VERSION llvm-$LLVM_VERSION-tools \
    libomp-$LLVM_VERSION-dev libc++-$LLVM_VERSION-dev libc++abi-$LLVM_VERSION-dev \
    libclang-common-$LLVM_VERSION-dev libclang-$LLVM_VERSION-dev \
    libclang-cpp$LLVM_VERSION-dev libunwind-$LLVM_VERSION-dev

ARG priority=100

RUN update-alternatives \
        --install /usr/bin/llvm-config       llvm-config      /usr/bin/llvm-config-${LLVM_VERSION} ${priority} \
        --slave   /usr/bin/llvm-ar           llvm-ar          /usr/bin/llvm-ar-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-as           llvm-as          /usr/bin/llvm-as-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-bcanalyzer   llvm-bcanalyzer  /usr/bin/llvm-bcanalyzer-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-cov          llvm-cov         /usr/bin/llvm-cov-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-diff         llvm-diff        /usr/bin/llvm-diff-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-dis          llvm-dis         /usr/bin/llvm-dis-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-dwarfdump    llvm-dwarfdump   /usr/bin/llvm-dwarfdump-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-extract      llvm-extract     /usr/bin/llvm-extract-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-link         llvm-link        /usr/bin/llvm-link-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-mc           llvm-mc          /usr/bin/llvm-mc-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-nm           llvm-nm          /usr/bin/llvm-nm-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-objdump      llvm-objdump     /usr/bin/llvm-objdump-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-ranlib       llvm-ranlib      /usr/bin/llvm-ranlib-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-readobj      llvm-readobj     /usr/bin/llvm-readobj-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-rtdyld       llvm-rtdyld      /usr/bin/llvm-rtdyld-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-size         llvm-size        /usr/bin/llvm-size-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-stress       llvm-stress      /usr/bin/llvm-stress-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-symbolizer   llvm-symbolizer  /usr/bin/llvm-symbolizer-${LLVM_VERSION} \
        --slave   /usr/bin/llvm-tblgen       llvm-tblgen      /usr/bin/llvm-tblgen-${LLVM_VERSION}

RUN update-alternatives \
        --install /usr/bin/clang                 clang                 /usr/bin/clang-${LLVM_VERSION} ${priority} \
        --slave   /usr/bin/clang++               clang++               /usr/bin/clang++-${LLVM_VERSION}  \
        --slave   /usr/bin/asan_symbolize        asan_symbolize        /usr/bin/asan_symbolize-${LLVM_VERSION} \
        --slave   /usr/bin/c-index-test          c-index-test          /usr/bin/c-index-test-${LLVM_VERSION} \
        --slave   /usr/bin/clang-check           clang-check           /usr/bin/clang-check-${LLVM_VERSION} \
        --slave   /usr/bin/clang-cl              clang-cl              /usr/bin/clang-cl-${LLVM_VERSION} \
        --slave   /usr/bin/clang-cpp             clang-cpp             /usr/bin/clang-cpp-${LLVM_VERSION} \
        --slave   /usr/bin/clang-format          clang-format          /usr/bin/clang-format-${LLVM_VERSION} \
        --slave   /usr/bin/clang-format-diff     clang-format-diff     /usr/bin/clang-format-diff-${LLVM_VERSION} \
        --slave   /usr/bin/clang-include-fixer   clang-include-fixer   /usr/bin/clang-include-fixer-${LLVM_VERSION} \
        --slave   /usr/bin/clang-offload-bundler clang-offload-bundler /usr/bin/clang-offload-bundler-${LLVM_VERSION} \
        --slave   /usr/bin/clang-query           clang-query           /usr/bin/clang-query-${LLVM_VERSION} \
        --slave   /usr/bin/clang-rename          clang-rename          /usr/bin/clang-rename-${LLVM_VERSION} \
        --slave   /usr/bin/clang-reorder-fields  clang-reorder-fields  /usr/bin/clang-reorder-fields-${LLVM_VERSION} \
        --slave   /usr/bin/clang-tidy            clang-tidy            /usr/bin/clang-tidy-${LLVM_VERSION} \
        --slave   /usr/bin/lldb                  lldb                  /usr/bin/lldb-${LLVM_VERSION} \
        --slave   /usr/bin/lldb-server           lldb-server           /usr/bin/lldb-server-${LLVM_VERSION}

ENV SHELL /bin/bash

RUN pip3 install -U pip
RUN pip3 install bump2version

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

RUN groupmod --gid $USER_GID $USERNAME \
    && usermod --uid $USER_UID --gid $USER_GID $USERNAME \
    && chown -R $USER_UID:$USER_GID /home/$USERNAME
