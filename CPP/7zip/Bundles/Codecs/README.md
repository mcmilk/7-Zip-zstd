
This archive contains two precompiled DLL's for 7-Zip v17.00 or higher.

## Installation

1. download the codec archiv from https://mcmilk.de/projects/7-Zip-zstd/
2. create a new directory named "Codecs"
3. put in there the files of the Codecs-ARCH.7z archiv to your Installation
   - normally, the x32 should go to: "C:\Program Files (x86)\7-Zip\Codecs"
   - x64: version should go in here: "C:\Program Files\7-Zip\Codecs"

## Usage

- when compressing binaries (*.exe, *.dll), you have to explicitly disable
  the bcj2 filter via `-m0=bcj`
- so the usage should look like this:

```
7z a archiv.7z -m0=bcj -m1=zstd -mx1   ...Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx..  ...
7z a archiv.7z -m0=bcj -m1=zstd -mx21  ...2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx22  ...Ultra Mode, with BCJ preprocessor on executables
```

## License and redistribution

- the same as the original 7-Zip, which means GNU LGPL
