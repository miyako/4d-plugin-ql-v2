# Ownership regressions

`harness.m` includes the real `4DPlugin-QL.cpp` (compiled as Objective-C without ARC, like
the Xcode target) and links it with the unchanged bundled `4DPluginAPI.c`. Only the 4D
engine (`Call4D`, in `engine.cpp`) and QuickLook's private `QLPreview` (replaced through
`+[QLPreview alloc]`) are synthetic. The fake engine tracks every string, picture, object
and handle it hands out and reports double disposal, leaks and use of dead values as named
failures.

```sh
tests/ownership/run.sh . fixed                 # every test must pass
mkdir /tmp/base && git archive a118cce547a9 | tar -x -C /tmp/base
tests/ownership/run.sh /tmp/base baseline      # defect_* fail, control_* pass
```

Each test runs in its own process, built with ASan/UBSan. A sanitizer report or
`HARNESS ERROR` is never an expected outcome. `QL Create thumbnail` is not exercised (it
needs the real QuickLook generators), and nothing is run inside a real 4D runtime.
