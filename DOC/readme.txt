7-Zip 4.08 beta Sources
-----------------------

7-Zip is a file archiver for Windows 95/98/ME/NT/2000/2003/XP. 

7-Zip Copyright (C) 1999-2004 Igor Pavlov.


License Info
------------

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


License notes
-------------

You can support development of 7-Zip by registering and 
paying $20.

7-Zip is free software distributed under the GNU LGPL.
If you need license with other conditions, write to
http://www.7-zip.org/support.html

---
Also this package contains files from LZMA SDK
you can download LZMA SDK from this page:
http://www.7-zip.org/sdk.html
read about license for LZMA SDk in file
DOC/lzma.txt


How to compile
--------------
To compile sources you need Visual C++ 6.0.
For compiling some files you also need 
new Platform SDK from Microsoft' Site:
http://www.microsoft.com/msdownload/platformsdk/sdkupdate/psdk-full.htm
or
http://www.microsoft.com/msdownload/platformsdk/sdkupdate/


Also for compiling BZip2 code you must download BZip source to folder
7zip/Compress/BZip2/Original
You can find BZip2 sources from that page:
http://sources.redhat.com/bzip2/index.html



Compiling under Unix/Linux
--------------------------
If sizeof(wchar_t) == 4 in your compiler,
you must use only 2 low bytes of wchar_t.


Notes:
------
7-Zip consists of COM modules (DLL files).
But 7-Zip doesn't use standard COM interfaces for creating objects.
Look at
7zip\UI\Client7z folder for example of using DLL files of 7-Zip. 
Some DLL files can use other DLL files from 7-Zip.
If you don't like it, you must use standalone version of DLL.
To compile standalone version of DLL you must include all used parts
to project and define some defs. 
For example, 7zip\Bundles\Format7z is a standalone version  of 7z.dll 
that works with 7z format. So you can use such DLL in your project 
without additional DLL files.


Description of 7-Zip sources package
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DOC                Documentation
---
  7zFormat.txt   - 7z format description
  copying.txt    - GNU LGPL license
  history.txt    - Sources history
  Methods.txt    - Compression method IDs
  readme.txt     - Readme file
  lzma.txt       - LZMA SDK description


Common            Common modules
Windows           Win32 wrappers

7zip
-------
  Common          Common modules for 7-zip

  Archive         7-Zip Archive Format Plugins 
  --------
    Common
    7z
    Arj
    BZip2
    Cab
    Cpio
    GZip
    Rar
    Rpm            
    Split
    Tar
    Zip

  Bundle          Modules that are bundles of other modules
  ------
    Alone         7za.exe: Standalone version of 7z
    SFXCon        7zCon.sfx: Console 7z SFX module
    SFXWin        7z.sfx: Windows 7z SFX module
    SFXSetup      7zS.sfx: Windows 7z SFX module for Installers
    Format7z      7za.dll: Standalone version of 7z.dll

  UI
  --
    Agent         Intermediary modules for FAR plugin and Explorer plugin
    Console       7z.exe Console version
    Explorer      Explorer plugin
    Resource      Resources
    Far           FAR plugin  
    Client7z      Test application for 7za.dll 

  Compress
  --------
    BZip2        BZip2 compressor
      Original   Download BZip2 compression sources from
                    http://sources.redhat.com/bzip2/index.html   
                 to that folder.
    Branch       Branch converter
    ByteSwap     Byte Swap converter
    Copy         Copy coder
    Deflate       
    Implode
    Arj
    LZMA
    PPMd          Dmitry Shkarin's PPMdH with small changes.
    LZ            Lempel - Ziv
      MT          Multi Thread Match finder
      BinTree     Match Finder based on Binary Tree
      Patricia    Match Finder based on Patricia algoritm
      HashChain   Match Finder based on Hash Chains

  Crypto          Crypto modules
  ------
    7zAES         Cipher for 7z
    AES           AES Cipher
    Rar20         Cipher for Rar 2.0
    RarAES        Cipher for Rar 3.0
    Zip           Cipher for Zip

  FileManager       File Manager


---
Igor Pavlov
http://www.7-zip.org


---
End of document

