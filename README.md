# curve25519
High performance implementation of elliptic curve 25519
=======================================================

Copyright Mehdi Sotoodeh.  All rights reserved.
<mehdisotoodeh@gmail.com>

This code and accompanying files are put in public domain by the author.
You can freely use, copy, modify and distribute this software as long
as you comply with the license terms. See license file for details.

This library delivers high performance and high security while having a small
footprint with minimum resource requirements.
This library supports DH key exchange using curve25519 as well as sign/verify
operations based on twisted Edwards curve 25519.

Current version of the library supports random blinding when dealing with
private keys. Blinding adds extra layer of security against side channel 
attacks.


Performance:
------------
The new version of this library sets NEW SPEED RECORDS. This is achieved 
without taking advantage of special CPU instructions or multi-CPU parallel 
processing.
The library implements a new technique (I call it FOLDING) that effectively 
reduces the number of EC point operations by a factor of 2, 4 or even more. 
The trade off is the pre computation and cost of cached memory.

Google's implementation (http://code.google.com/p/curve25519-donna/) is used
here for performance comparison only. This library outperforms Google's code 
by a factor of 3.6 to 11 depending on the platform and selected language.

For best performance, use the 64-bit assembly version on AMD/Intel CPU 
architectures. The portable C code can be used for 32-bit OS's and other CPU 
types.

Note that the assembly implementation is approximately 3 times faster than C 
implementation on 64-bit platforms (C != PortableAssembly).
On 32-bit platforms, the biggest hit is due to usage of standard C library for
64-bit arithmetic operations. Numbers below indicate that GCC and glibc does a 
much better job than MSVC.


Timing for ed25519 sign/verify (short messages):
```
    windows7-64: VS2010 + MS Assembler, Intel(R) Core(TM) i7-2670QM CPU
        KeyGen: 76129 cycles = 22.391 usec @3.4GHz
          Sign: 79817 cycles = 23.476 usec @3.4GHz
        KeyGen: 76825 cycles = 22.596 usec @3.4GHz  (Blinded)
          Sign: 80419 cycles = 23.653 usec @3.4GHz  (Blinded)
        Verify: 114091 cycles = 33.556 usec @3.4GHz (Init)
                110715 cycles = 32.563 usec @3.4GHz (Check)

    windows7:  VS2010, Portable-C, 64-bit, Intel(R) Core(TM) i7-2670QM CPU
        KeyGen: 217389 cycles = 63.938 usec @3.4GHz
          Sign: 221131 cycles = 65.039 usec @3.4GHz
        KeyGen: 218203 cycles = 64.177 usec @3.4GHz  (Blinded)
          Sign: 222907 cycles = 65.561 usec @3.4GHz  (Blinded)
        Verify: 341411 cycles = 100.415 usec @3.4GHz (Init)
                308173 cycles = 90.639 usec @3.4GHz (Check)
            
    windows7:  VS2010, Portable-C, 32-bit, Intel(R) Core(TM) i7-2670QM CPU
        KeyGen: 919738 cycles = 270.511 usec @3.4GHz
          Sign: 932694 cycles = 274.322 usec @3.4GHz
        KeyGen: 924762 cycles = 271.989 usec @3.4GHz  (Blinded)
          Sign: 939076 cycles = 276.199 usec @3.4GHz  (Blinded)
        Verify: 1429798 cycles = 420.529 usec @3.4GHz (Init)
                1307928 cycles = 384.685 usec @3.4GHz (Check)

    x86_64-w64-mingw32: gcc 4.9.2 + NASM 2.11.08, Intel(R) Core(TM) i7-2670QM CPU
        KeyGen: 76686 cycles = 22.555 usec @3.4GHz
          Sign: 80644 cycles = 23.719 usec @3.4GHz
        KeyGen: 77400 cycles = 22.765 usec @3.4GHz  (Blinded)
          Sign: 81310 cycles = 23.915 usec @3.4GHz  (Blinded)
        Verify: 115300 cycles = 33.912 usec @3.4GHz (Init)
                111300 cycles = 32.735 usec @3.4GHz (Check)    
```

Timing for DH point multiplication:
```
    windows7-64: VS2010 + MS Assembler, Intel(R) Core(TM) i7-2670QM CPU
        Donna: 778914 cycles = 229.092 usec @3.4GHz -- ratio: 10.491
        Mehdi: 74248 cycles = 21.838 usec @3.4GHz -- delta: 90.47%      ** MSASM **

    Mingw-x86_64: gcc 9.9.2, nasm 2.11.08
        Donna: 852662 cycles = 250.783 usec @3.4GHz -- ratio: 11.431
        Mehdi: 74594 cycles = 21.939 usec @3.4GHz -- delta: 91.25%

    ubuntu-12.04.3-x86_64: nasm 2.09.10
        Donna: 867671 cycles = 255.197 usec @3.4GHz -- ratio: 5.464
        Mehdi: 158787 cycles = 46.702 usec @3.4GHz -- delta: 81.70%     ** NASM **

    windows7:  VS2010, Portable-C, 64-bit
        Donna: 780008 cycles = 229.414 usec @3.4GHz -- ratio: 3.692
        Mehdi: 211272 cycles = 62.139 usec @3.4GHz -- delta: 72.91%

    windows7:  VS2010, Portable-C, 32-bit, Intel(R) Core(TM) i7-2670QM CPU
        Donna: 7460048 cycles = 2194.132 usec @3.4GHz -- ratio: 8.208
        Mehdi: 908870 cycles = 267.315 usec @3.4GHz -- delta: 87.82%

    cygwin-32: gcc 4.5.3, Portable-C, 32-bit, Intel(R) Core(TM) i7-2670QM CPU
        Donna: 2550612 cycles = 750.180 usec @3.4GHz -- ratio: 3.899
        Mehdi: 654232 cycles = 192.421 usec @3.4GHz -- delta: 74.35%
```

Building:
---------
The design uses a configurable switch that defines the byte order of the
target CPU. In default mode it uses Little-endian byte order. You need to
change this configuration for Big-endian targets by setting ECP_BIG_ENDIAN
switch (see Makefile).

Second configurable switch controls usage of TSC (Time Stamp Counter). It is
only used as a high resolution timer for performance measurements. You need 
to turn ECP_NO_TSC switch on if your target does not support it.

For building the library using the assembly sources, two assemblers are currently
supported: Microsoft Assembler (Windows) and NASM (Windows/Linux). 
NASM can be downloaded from: http://www.nasm.us/pub/nasm/releasebuilds/2.11.08/

- For Windows platforms, open windows/EC25519.sln using Visual Studio 2010
  and build Asm64Test project for x64 configuration.
  You also have the option of using Mingw and nasm on windows platforms,
- For Linux platforms, Debian and Ubuntu have been tested so far. For X86 
  assembly support you need to install nasm first and then run: 'make asm' from 
  project root. Output files will be created in asm64/build/test64.

A custom tool creates a random blinder on every new build. This blinder is static
and will be part of the library. This blinder is only used for blinding the point 
multiplication when creating blinding context via ed25519_Blinding_Init() API.
