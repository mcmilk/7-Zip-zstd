7-Zip 2.30 Beta 24 Sources
--------------------------

7-Zip is a file archiver for Windows 95/98/ME/NT/2000/XP. 

7-Zip Copyright (C) 1999-2002 Igor Pavlov.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


How to compile
--------------
To compile sources you need Visual C++ 6.0.
For compiling some files you also need 
new Platform SDK from Microsoft' Site.




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

  Alien             Must contains third party sources
    Compress
      BZip2         BZip2 compression sources from
                    http://sources.redhat.com/bzip2/index.html


  Archive           Common archive modules
    Common
    Cab             
    cpio
    GZip            
    Rar
    RPM            
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
      SFXCon        7zCon.sfx: Console 7z SFX module
      SFXWin        7z.sfx: Windows 7z SFX module
      SFXSetup      7zS.sfx: Windows 7z SFX module for Installers
    Common          Common modules
    Console         7z.exe Console version
    Explorer        Explorer plugin
    Resource        Resources
    Far             FAR plugin  
    Format          Archive format modules
      7z
      BZip2
      Cab
      cpio
      Common
      GZip
      Rar
      RPM            
      Tar
      Zip
      arj

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
      arj
      LZMA
      MatchFinder   Match Finder for LZ modules
        BinTree     Match Finder based on Binary Tree
        Patricia    Match Finder based on Patricia algoritm
        HashChain   Match Finder based on Hash Chains
    PPM             Prediction by partial match
      PPMd          Dmitry Shkarin's PPMdH with small changes.

  Crypto            Crypto modules
  ------
    Cipher          Cipher
      Common        Interfaces
      Rar20         Cipher for Rar
      Zip           Cipher for Zip

  FileManager       File Manager
  ------
    Resource        Resources

---
End of document
  




