# Pitfalls

Failures this setup has actually produced, by symptom. Each was hit while
building the fork and the starter project on Bazel 9.1, clang 21 and
nixos-unstable in October 2026.

Contents:
- The compiler and libc++
- Gloop's flags and API
- Pinning Gloop
- Libraries from nixpkgs
- Nix and the repository
- Tooling
- Tests

## The compiler and libc++

**`error: "Gloop does not support libstdc++."` or `"Gloop requires libc++ 21
or later."` (from `gloop/enforce_gloop_support.h`) when building**
The compiler is using libstdc++. Either Bazel was run outside `nix develop`
and picked up the host compiler (nothing warns about this), or the dev shell
is a plain `mkShell` / `llvmPackages_N.stdenv` one rather than
`gloop.lib.mkDevShell`. After fixing the shell, Bazel reconfigures its
toolchain by itself on the next build.

**The same two errors, but only from clang-tidy or clangd**
The clang tools are wrapped separately from the compiler and default to
libstdc++ headers. The fork's shell uses
`llvm.clang-tools.override { enableLibcxx = true; }`. If the project's
`flake.lock` holds an old revision of the fork from before that fix, run
`nix flake update gloop`.

**`ld.gold: error: cannot find -lc++`, followed by pages of undefined
`std::__1::...` references**
Bazel links through `clang`, not `clang++`, so Nix's compiler wrapper does
not add libc++'s directory. `mkDevShell` handles it by exporting
`BAZEL_LINKOPTS=-L<libcxx>/lib:-Wl,-rpath,<libcxx>/lib` and
`BAZEL_LINKLIBS=-lc++:-lm`. Seeing this means the shell is not from
`mkDevShell`, or those variables were overridden.

**`clang: error: argument unused during compilation: '-stdlib=libc++'
[-Werror,-Wunused-command-line-argument]` in a dependency such as tcmalloc**
Something added `-stdlib=libc++` to the compile flags (`BAZEL_CXXOPTS`,
`--cxxopt`). The wrapper already selects libc++'s headers, so the flag is
unused, and dependencies built with `-Werror` reject it. Remove it; no
compile flag is needed.

**Bazel builds with the wrong compiler, or behaves differently elsewhere**
Check the compiler path in `compile_commands.json`; it should be
`/nix/store/...-clang-wrapper-<version>/bin/clang`.

## Gloop's flags and API

**`error: "Gloop does not support exceptions (use -fno-exceptions)"` or
`static assertion failed ... Gloop does not support signed chars`**
`.bazelrc` lost one of
`build --copt=-fno-exceptions --host_copt=-fno-exceptions` and
`build --copt=-funsigned-char --host_copt=-funsigned-char`.

**`no member named 'MathUtil' in namespace 'gloop'; did you mean simply
'MathUtil'?`**
Gloop's code is not in a `gloop::` namespace. It keeps the namespaces it had
inside Google: `gtl::intrusive_list`, `util::StatusToString`,
`net_base::IPAddress`, and the global namespace for `MathUtil`, `atoi32` and
others. Read the header.

**Pages of `-Wdeprecated-declarations` warnings from `external/gloop+/...`**
Expected and harmless: Gloop is largely code Google considers deprecated.
They come from compiling Gloop's own sources. To silence them, add
`build --per_file_copt=external/.*@-Wno-deprecated-declarations` to
`.bazelrc`.

**A type with an `intrusive_link` cannot be copied, moved or put in a
`flat_hash_map` / `std::vector`**
A linked element must not change address. Own such elements in a container
that never moves them (`absl::node_hash_map`, `std::unique_ptr`, a
`std::deque` that only grows), as `cache/lru_cache.h` does.

## Pinning Gloop

**`git_override` fails to fetch the commit ("could not fetch", "not our
ref", "unadvertised object")**
The fork's `nix` branch is rebased onto upstream daily, so an old branch-head
commit can disappear. Each successful sync is tagged `nix-YYYYMMDD`, and
tagged commits stay. Re-pin to a tag's commit:
`git ls-remote https://github.com/lukeyeh/gloop.git 'refs/tags/nix-*'`.

**After updating Gloop, Bazel resolves a different Abseil than
`MODULE.bazel` asks for, or warns about it**
Gloop raised its Abseil version. Bazel takes the highest version requested
anywhere, so the build still uses one Abseil, but make the project's
`bazel_dep` match Gloop's `MODULE.bazel` so the file says what is built.

**Code that compiled yesterday fails after a Gloop update**
Upstream deliberately makes breaking changes and offers no compatibility
guarantee. Fix the call sites, or stay on the previous commit.

**The dev shell and the Gloop code are from different fork commits**
That is fine. `flake.lock` pins the fork's flake (the shell) and
`MODULE.bazel` pins the fork's code; they are independent as long as each
works. `nix flake update gloop` moves the shell.

## Libraries from nixpkgs

**Undefined references to `std::__cxx11::basic_string...` (or to
`std::__1::...` from inside a nixpkgs library) at link time**
A C++ library from nixpkgs was built against libstdc++; its `std::string` is
a different type from the project's. Rebuild it for libc++ in
`nix/deps.nix`:
`pkgs.foo.override { stdenv = pkgs.llvmPackages_21.libcxxStdenv; }`, and do
the same for any C++ library it links (as `gbenchmark` does for `gtest`).
C libraries need nothing.

**Two copies of Abseil, GoogleTest or protobuf: duplicate symbols, flags
registered twice, or crashes at start-up**
One came from nixpkgs through `nix_pkg` and one from Gloop's Bazel modules.
Anything in Gloop's `MODULE.bazel` must come from the Bazel registry, never
from `nix/deps.nix`.

**`Unable to find package for @@[unknown repo 'rules_cc' requested from
@@rules_nixpkgs_core++nix_pkg+<lib>]//cc:cc_library.bzl`**
The `build_file_content` of a `nix_pkg` block is evaluated inside a
repository rules_nixpkgs creates, where the short name `@rules_cc` is not
known. Load by canonical name:
`load("@@rules_cc+//cc:cc_library.bzl", "cc_library")`.

**A library has headers but no libraries, or the reverse**
nixpkgs splits libraries into outputs (`out`, `dev`). `nix/bazel.nix` merges
those two; a library with other output names needs them added there.

**`error while loading shared libraries: libfoo.so.2: cannot open shared
object file`**
The `cc_library` listed `lib/libfoo.so`. List the file with the name stored
inside the library (`readelf -d lib/libfoo.so | grep SONAME`), e.g.
`["lib/liburing.so.2"]`.

**`symbol lookup error ... undefined symbol: _ZN9benchmark...` when running
a test**
Bazel built the project's own libraries as shared objects, which cannot
absorb a static library from Nix. Keep `build --dynamic_mode=off`.

**`nix-build` not found during a Bazel fetch**
rules_nixpkgs runs the `nix` on `PATH`. In CI, install Nix before invoking
Bazel.

**The first build after `nix flake update` takes minutes before compiling
anything**
Nix is rebuilding the libc++ variants of the nixpkgs libraries, which are
not in the binary cache.

## Nix and the repository

**`error: Path 'flake.nix' ... is not tracked by Git`**
Flakes only see tracked files. With git, `git add` them. With jj, use a
colocated repo and run `jj status`, which snapshots the working copy.

**`nix build` or `nix flake check` has nothing to build**
Intended: this setup has no Nix package, because the sources cannot be
compiled without Bazel once they use Gloop.

**`perf.data` or `flamegraph.svg` shows up as a tracked file**
jj tracks new files automatically. The template's `.gitignore` covers them;
if one slipped in, `jj file untrack <path>` after adding it to `.gitignore`.

## Tooling

**`bazel run :compile_commands` dies with `AssertionError` at
`assert source_file is not None`**
Keep both `build --features=-parse_headers` and
`build --host_features=-parse_headers`.

**The first `bazel test //...` compiles protobuf although the project does
not use it**
`//...` includes the `:compile_commands` target, whose tool needs protobuf.
It is built once. `bazel test //cache/...` avoids it.

**clang-tidy reports hundreds of warnings in Abseil's or Gloop's headers**
Keep `ExcludeHeaderFilterRegex: '(^|/)external/|^/nix/store/'` in
`.clang-tidy`.

**clangd cannot find system headers**
Start it with `--query-driver=/**/*`.

## Tests

**A test that shells out fails with `readelf: command not found` (or any
other tool from the Nix shell)**
Bazel runs tests with a minimal `PATH`. Add `test --test_env=PATH` to
`.bazelrc` if the project's tests need the shell's tools.

**Benchmark numbers are far lower than before, for no reason**
`bazel-bin` follows the most recent build, so after `bazel test` it points at
unoptimised binaries. Always `bazel run -c opt`.

**Some of Gloop's own tests fail when run from the fork**
Known and unrelated to using Gloop as a library; the fork's `NIX.md` lists
them. They assume Google's CI image (bash as `/bin/sh`, a different linker).
