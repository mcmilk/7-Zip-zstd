# Kanzi vendor source

This directory contains the Kanzi C++ sources vendored for the 7-Zip `KANZI`
codec.

Source line:

```text
flanglet/kanzi-cpp:2.6.0
1330414143be82209e0a0860d2b9f8ab567c94b5
```

This source line is the upstream flanglet/kanzi-cpp 2.6.0 release.

The 7-Zip bridge uses these sources through `CPP/7zip/Compress/Kanzi*`.
`src/concurrent.hpp` retains the `_MSVC_LANG` standard-detection adjustment
required by the MSVC build.

License:

```text
Apache License 2.0
C/kanzi/LICENSE
```
