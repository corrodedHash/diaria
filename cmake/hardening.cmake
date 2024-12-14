add_library(diaria_hardening INTERFACE)
target_compile_definitions(diaria_hardening INTERFACE
  -D_GLIBCXX_ASSERTIONS=1)

target_compile_options(diaria_hardening INTERFACE
  -fstack-protector-strong
  -fcf-protection=full -fstack-clash-protection -Wall -Wextra -Wpedantic
  -Wconversion -Wsign-conversion -Wcast-qual -Wformat=2 -Wundef -Werror=float-equal
  -Wshadow -Wcast-align -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough
  -Wextra-semi -Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast)

target_link_options(diaria_hardening INTERFACE
  -Wl,--allow-shlib-undefined,--as-needed,-z,noexecstack,-z,relro,-z,now,-z,nodlopen)
