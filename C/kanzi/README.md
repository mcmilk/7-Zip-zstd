# Kanzi vendor source

This directory contains the Kanzi C++ sources vendored for the 7-Zip `KANZI`
codec.

Source line:

```text
BICHENG/kanzi-cpp:master
3420e938874b5f3a4058e76ce6926968c10402e1
```

This source line is based on flanglet/kanzi-cpp master and includes the fixes
used by this 7-Zip integration, including the 64-job output stream block fix.

The 7-Zip codec integration uses these sources through `CPP/7zip/Compress/Kanzi*`.
Standalone Kanzi tools, standalone project badges, external benchmark pages, and
Kanzi project build instructions are not part of this vendored 7-Zip tree.

License:

```text
Apache License 2.0
C/kanzi/LICENSE
```
