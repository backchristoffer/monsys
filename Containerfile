FROM fedora:rawhide

WORKDIR /app
COPY . /app

RUN dnf -y install gcc clang llvm bpftool \
  glibc-devel kernel-headers make \
  libbpf-devel elfutils-libelf-devel

RUN gcc -std=c11 -O2 -Wall -Wextra -pedantic -o netmonitor netmonitor.c

ENTRYPOINT ["./netmonitor"]


