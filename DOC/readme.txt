7-Zip 2.30 Beta 12 Sources
--------------------------

7-Zip is a file archiver for Windows 95/98/ME/NT/2000/XP. 

7-Zip is free software distributed under the GNU LGPL. 

If you will disclose any bugs in programs, sources, 
documentation, please, send some report to the author.

You may freely send your comments and suggestions to 
Igor Pavlov, the author of the 7-Zip:


How to compile
--------------
To compile sources you need Visual C++ 6.0.
For compiling some files you also need 
new Platform SDK from Microsoft' Site.

Also you must add SDK folder to include paths.




WWW:
     http://www.7-zip.org
E-mail:
     support@7-zip.org



Description of 7-Zip sources package
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DOC                Documentation
---
  7zFormat.txt   - 7z format description
  copying.txt    - GNU LGPL license
  history.txt    - Sources history
  Methods.txt    - Compression method IDs
  readme.txt     - Readme file

  
SDK                 Common files
---

  Archive           Common archive modules
    Cab             
    GZip            
    Rar             
    Tar             
    Zip             
  Common            Common modules
  Compression       Huffman and arithmetic modules 
  Far               FAR interface wrappers
  Interface         Common interface mudules and some implementations
  Stream            Byte / Bit / Sliding Window Streams
  Util              Utils
  Windows           Win32 wrappers
    Control         Win32 GUI controls wrappers


Project
-------
  Archiver          7-Zip Archiver
  --------
    Agent           Intermediary modules for FAR plugin and Explorer plugin
    Bundle          Modules that are bundles of other modules
      Alone         7za.exe: Standalone version of 7z
      SFXCon        7z.sfx: Console 7z SFX module
    Common          Common modules
    Console         7z.exe Console version
    Explorer        Explorer plugin
    Far             FAR plugin  
    Format          Archive format modules
      7z
      BZip2
      Cab
      Common
      GZip
      Rar
      Tar
      Zip
    Config          7-Zip Configuration program

  Compress
  --------

    Interface       Interfacec for compress modules
    
    BWT             Burrows Wheeler Transform 
      BZip2         
    
    Convert         Convert modules
      Branch        Branch converter
      ByteSwap      Byte Swap converter
      Copy          Copy converter
    
    LZ              Lempel - Ziv
      Deflate       
      Implode
      LZMA
      MatchFinder   Match Finder for LZ modules
        BinTree     Match Finder based on Binary Tree
        Patricia    Match Finder based on Patricia algoritm
    PPM             Prediction by partial match
      PPMd          Dmitry Shkarin's PPMdH with small changes.

  Crypto            Crypto modules
  ------
    Cipher          Cipher
      Common        Interfaces
      Rar20         Cipher for Rar
      Zip           Cipher for Zip

---
End of document
  




