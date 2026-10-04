# gloop-starter-project

A Claude Code skill for starting and working on C++ projects that use
[Gloop](https://github.com/lukeyeh/gloop) (a fork of
[abseil/gloop](https://github.com/abseil/gloop) that builds under Nix), built
with **Bazel inside a Nix dev shell**. It follows the layout of
[nix-bazel-cpp](https://github.com/lukeyeh/nix-bazel-cpp-skill), adapted for
what Gloop requires.

## What a scaffolded project has

- A Nix flake dev shell from the Gloop fork: clang with libc++ (Gloop refuses
  libstdc++), Bazel 9, buildifier, clangd/clang-format/clang-tidy, and
  profilers.
- Bazel with Bzlmod. Gloop pinned by commit; Abseil and GoogleTest from the
  Bazel Central Registry, so each binary holds the one copy Gloop also uses;
  other libraries from nixpkgs through
  [rules_nixpkgs](https://github.com/tweag/rules_nixpkgs), rebuilt against
  libc++ where they are C++.
- A working example: an LRU cache built on Gloop's `gtl::intrusive_list`.
- Tests and microbenchmarks in the same `_test.cc`, run by one binary.
- `compile_commands.json` generation for clangd, and clang-tidy and
  clang-format configs.
- `bazel run --config=flamegraph //some:target` for a flamegraph.

Unlike nix-bazel-cpp there is no Nix package: code that needs Gloop can only
be compiled by Bazel.

## Installing the skill

```
git clone https://github.com/lukeyeh/gloop-starter-project-skill ~/.claude/skills/gloop-starter-project
```

Then ask Claude Code for a new project that uses Gloop.

## What is in here

| Path | Contents |
| --- | --- |
| `SKILL.md` | The instructions: why Gloop needs its own setup, scaffolding steps, common tasks, decisions and why |
| `template/` | The project skeleton that gets copied |
| `references/pitfalls.md` | Failures this setup has produced, by error message, with cause and fix |

## Using the template without Claude

```
cp -r template myproject && cd myproject
mv gitignore .gitignore
git init && git add -A        # flakes only see tracked files
nix develop
bazel test //...
bazel run //:lru -- --capacity=2 a b a c b
```

The Gloop fork is rebased onto upstream daily, so update the commit in
`MODULE.bazel` to the newest `nix-YYYYMMDD` tag's commit first; `SKILL.md` step 4
says how.

## Status

The template was scaffolded into a fresh repository and checked: tests, the
binary, the benchmarks, compile commands and clang-tidy, on x86_64-linux with
Bazel 9.1.1, clang 21.1.8, rules_nixpkgs_core 0.14.0 and nixos-unstable as of
October 2026. The flamegraph config is carried over from nix-bazel-cpp and
was not re-run here.

Linux only, on x86-64 and ARM64, because that is all Gloop supports. Gloop
itself is not intended by its authors for use outside Google and makes
breaking changes freely; see its README.
