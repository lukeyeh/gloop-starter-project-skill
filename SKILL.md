---
name: gloop-starter-project
description: Scaffold and maintain a C++ project that uses Gloop (Google's abseil/gloop library, via the lukeyeh/gloop fork) built with Bazel inside a Nix flake dev shell, in the nix-bazel-cpp layout with rules_nixpkgs. Use this whenever the user wants to start a project that uses Gloop or any gloop/ header (gtl::intrusive_list, util/math, util/status, net/base, ...), asks for a "gloop starter" or "gloop project", wants to add Gloop to a Nix + Bazel C++ project, wants to update the Gloop pin in such a project, or hits build errors mentioning Gloop, libc++ vs libstdc++, or enforce_gloop_support.h, even if they do not name this skill. Prefer it over nix-bazel-cpp when Gloop is involved, because Gloop changes where the compiler and several libraries come from.
---

# Gloop starter project

Sets up a C++ project that calls into [Gloop](https://github.com/lukeyeh/gloop)
(a fork of abseil/gloop that builds under Nix), following the
[nix-bazel-cpp](https://github.com/lukeyeh/nix-bazel-cpp-skill) layout:

- `nix develop` provides the compiler, Bazel and the tools.
- Bazel builds, tests and benchmarks the code, and fetches Gloop.
- Other third-party libraries come from nixpkgs through rules_nixpkgs.

`template/` next to this file is a working project: an LRU cache built on
Gloop's `gtl::intrusive_list`, with tests, benchmarks and a small binary. It
was built and run end to end (tests, binary, benchmarks, compile commands,
clang-tidy) before being saved here. If it fails, suspect stale pins first
(step 4).

## Why Gloop needs its own setup

Gloop enforces its requirements with `#error`, and three of them shape the
project. Knowing them explains every difference from plain nix-bazel-cpp, and
most build failures.

1. **Clang >= 21 with libc++, never libstdc++.** nixpkgs' normal clang stdenv
   uses libstdc++, so the dev shell comes from the fork's flake instead
   (`gloop.lib.mkDevShell`), which uses the libc++ stdenv and tells Bazel's
   auto-detected toolchain to link libc++.
2. **Gloop exists only as a Bazel module that builds its own Abseil.** It
   cannot come from nixpkgs, and a second Abseil from nixpkgs in the same
   binary would clash with Gloop's. So Gloop, Abseil and GoogleTest come from
   Bazel modules; everything else still comes from nixpkgs.
3. **`-fno-exceptions` and `-funsigned-char`** must be on for every file that
   includes a Gloop header.

Two consequences to tell the user about if they come up:

- A C++ library from nixpkgs is built against libstdc++ and must be rebuilt
  for libc++ in `nix/deps.nix` (the template does this for Google Benchmark).
  C libraries work as they are.
- There is no `nix/package.nix` and no `nix build`. nix-bazel-cpp's package
  compiles the sources without Bazel, which cannot work for code that needs
  Gloop. Deploy the binary Bazel builds.

## Scaffolding a project

1. **Create the repository** with whatever version control the user uses.
   Nix flakes only see files the VCS tracks, so the repo has to exist before
   `nix develop` will work. With jj, use a colocated repo
   (`jj git init --colocate <dir>`) and run `jj status` after adding files;
   with git, `git add` them.

2. **Copy the template**: everything in `template/` into the project root,
   including dotfiles, and rename `gitignore` to `.gitignore`. Bazel
   generates `MODULE.bazel.lock` itself.

3. **Rename** the placeholders:
   - `module(name = "gloop_starter")` in `MODULE.bazel`
   - `description` in `flake.nix`
   - the title and the `lru` examples in `README.md`
   - the `lru` binary (`BUILD`, `main.cc`) and the `cache/` example library:
     replace them with the user's code if they said what they are building or
     which part of Gloop they want; otherwise leave them as a working example.
     When replacing, also update the `//cache:lru_cache_test` mentions in
     `testing/main.cc`.

4. **Refresh the pins.** The fork is rebased onto upstream daily, so the
   template's Gloop commit goes stale quickly.
   - **Gloop commit** for the `git_override` in `MODULE.bazel`. List what the
     fork offers:
     ```
     git ls-remote https://github.com/lukeyeh/gloop.git 'refs/tags/nix-*' refs/heads/nix
     ```
     Use the commit of the newest `nix-YYYYMMDD` tag. Each successful sync is
     tagged, and a tagged commit stays fetchable after later rebases; the bare
     branch head may not.
   - **Abseil and GoogleTest versions** in `MODULE.bazel` should match Gloop's
     at that commit, so one version is resolved:
     ```
     curl -s https://raw.githubusercontent.com/lukeyeh/gloop/<commit>/MODULE.bazel | grep -E '"(abseil-cpp|googletest)"'
     ```
   - `nix flake update` (this also moves the `gloop` flake input, which
     supplies the dev shell), then set `.bazelversion` to what
     `bazel --version` prints inside `nix develop`.
   - The `llvmPackages_<N>` in `nix/deps.nix` should be the one in the fork's
     `flake.nix` (`llvmFor`), so nixpkgs libraries are built with the shell's
     libc++.
   - For `rules_cc`, `rules_nixpkgs_core` and `wolfd_bazel_compile_commands`,
     take the newest version from
     `https://raw.githubusercontent.com/bazelbuild/bazel-central-registry/main/modules/<name>/metadata.json`.

5. **Verify** inside `nix develop`. The first build takes several minutes:
   Bazel builds Abseil (and protobuf, for the compile-commands tool) and Nix
   builds Google Benchmark.
   ```
   bazel test //...
   bazel run //:lru -- --capacity=2 a b a c b       # hits=1 misses=4
   bazel run -c opt //cache:lru_cache_test -- --benchmark_filter=all
   bazel run :compile_commands                      # writes compile_commands.json
   clang-tidy main.cc */*.cc
   ```
   Then confirm the compiler pin took effect: the compiler path in
   `compile_commands.json` should be
   `/nix/store/...-clang-wrapper-<version>/bin/clang`.

6. **Update the README** for what the project now is, keeping the sections on
   what Gloop changes and where libraries come from. Commit, but do not push
   or create a remote repository unless asked.

## How it fits together

```
flake.lock ─┬─► gloop flake ─► lib.mkDevShell ─► clang + libc++, Bazel, clang tools   (the shell)
            └─► nixpkgs ─► nix/deps.nix ─► nix/bazel.nix ─► rules_nixpkgs ─► @gbenchmark, ...

MODULE.bazel ─┬─► git_override ─► @gloop//gloop/...
              └─► Bazel registry ─► @abseil-cpp//absl/..., @googletest//:gtest
```

| File | Role |
| --- | --- |
| `flake.nix` | The dev shell, built by the Gloop fork's `lib.mkDevShell`; extra tools go in its `packages` |
| `MODULE.bazel` | Gloop by commit, Abseil and GoogleTest from the registry, and one `nix_pkg.file` block per nixpkgs library |
| `nix/deps.nix` | The list of libraries taken from nixpkgs, rebuilt for libc++ where they are C++ |
| `nix/bazel.nix` | Presents `deps.nix` to Bazel, merging each library's Nix outputs into one directory |
| `.bazelrc` | C++20, Gloop's two required flags, static linking of the project's own libraries, profiling configs |
| `testing/main.cc` | The `main` every test links: runs tests, or benchmarks when given a `--benchmark...` flag |
| `cache/` | The example: `LruCache` on `gtl::intrusive_list`, and `Replay` |

## Common tasks

**Use another part of Gloop.** Find the header under `gloop/` in the fork;
the `BUILD` file next to it names the target. Add it to `deps` as
`"@gloop//gloop/util/math:mathutil"` and include it by full path,
`#include "gloop/util/math/mathutil.h"`. Gloop's code is not in a `gloop::`
namespace: it keeps its original ones (`gtl::`, `util::`, `net_base::`) or
the global namespace (`MathUtil`, `atoi32`), so read the header. Much of
Gloop is marked deprecated in favour of Abseil equivalents; prefer Abseil
where both exist.

**Add a library from nixpkgs** (say liburing):
1. Add it to `nix/deps.nix`: `liburing = pkgs.liburing;`. For a C++ library,
   override its stdenv to `libcxxStdenv` as `gbenchmark` does, including any
   C++ dependency it links.
2. Add a `nix_pkg.file(...)` block in `MODULE.bazel`, copied from the existing
   one, and its name to the `use_repo` line. List shared libraries by their
   versioned name, e.g. `srcs = ["lib/liburing.so.2"]`
   (`ls $(nix build --no-link --print-out-paths nixpkgs#liburing.out)/lib`).
3. Depend on it as `"@liburing"`.

Do not add Abseil, GoogleTest or protobuf this way. Take anything Gloop
already depends on (see its `MODULE.bazel`) from the Bazel registry with a
`bazel_dep`, at Gloop's version.

**Add a directory of code**: give it a `BUILD` file with a `cc_library` and a
`cc_test` per source file, as in `cache/`. Use Bazel `visibility` to keep
dependencies pointing one way.

**Add a test with benchmarks**: one `foo_test.cc` holds both. Link
`//testing:main`, `@googletest//:gtest` and `@gbenchmark`.
`testing/main_test.cc` is the smallest example.

**Update Gloop**: redo the Gloop parts of step 4 (commit, Abseil and
GoogleTest versions, `nix flake update gloop`), then step 5. Upstream makes
breaking changes freely, so expect to fix call sites.

**Profile something**: `bazel run --config=flamegraph //pkg:target` writes
`/tmp/flamegraph.svg`. `perf` needs `kernel.perf_event_paranoid` to be 2 or
lower.

## When something breaks

Read `references/pitfalls.md` before debugging from scratch. It lists the
failures this setup has actually produced, by error message, and several have
non-obvious causes. The most common:

- **`Gloop does not support libstdc++` / `requires libc++ 21 or later`**:
  Bazel was run outside `nix develop`, or the shell was not built with
  `gloop.lib.mkDevShell`.
- **`could not fetch` the Gloop commit**: the pinned commit was rebased away.
  Re-pin to a `nix-YYYYMMDD` tag's commit.
- **Undefined `std::__cxx11` or `std::__1` symbols at link time**: a C++
  library from nixpkgs was not rebuilt for libc++.

## Decisions already made, and why

Explain the trade-off if the user asks, but do not reopen these unprompted.

- **Gloop comes from the fork, not abseil/gloop.** The fork adds the Nix
  shell and nothing else, and follows upstream daily. Upstream says it is not
  meant for outside use and breaks compatibility freely; pinning by commit is
  what makes that tolerable.
- **Abseil and GoogleTest come from the Bazel registry here**, unlike
  nix-bazel-cpp, to keep one copy per binary.
- **Gloop's own BUILD files are not rewritten to use nixpkgs libraries.**
  That would touch nearly every upstream file and break the daily sync.
- **No Nix package.** See above; revisit only if the user needs to deploy
  with Nix, and then the options are running Bazel inside the Nix build with
  vendored dependencies, or packaging the needed part of Gloop as a Nix
  library.
- **Bazel stays.** If Bazel causes friction, solve it within Bazel.
