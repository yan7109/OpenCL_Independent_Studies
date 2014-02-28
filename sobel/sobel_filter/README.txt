Sobel Filter OpenCL Design Example README file
----------------------------------------------
----------------------------------------------

This readme file for the Sobel Filter OpenCL Design Example contains information
about the design example package. For more examples, please visit the page:
http://www.altera.com/support/examples/opencl/opencl.html

This file contains the following information:

- Disclaimer
- Release History
- Software and Board Requirements
- Package Contents and Directory Structure
- Example Description
- Usage Instructions
- Contacting Altera(R)

Disclaimer
==========

Copyright (C) 2013 Altera Corporation, San Jose, California, USA. All rights reserved. 
Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to 
whom the Software is furnished to do so, subject to the following conditions: 
The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software. 
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
OTHER DEALINGS IN THE SOFTWARE. 
 
This agreement shall be governed in all respects by the laws of the State of California and 
by the laws of the United States of America. 


License for freeglut
--------------------

 The GLUT-compatible part of the freeglut library include file

 Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 Written by Pawel W. Olszta, <olszta@sourceforge.net>
 Creation date: Thu Dec 2 1999

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


Release History
===============

Version 1.1 - On Linux, fix possible compilation issues in AOCL_Utils
              by including <unistd.h>.
Version 1.0 - First release of example


Software and Board Requirements
===============================

This design example is for use with the following versions of the 
Altera Complete Design Suite and Quartus(R) II software and
Altera SDK for OpenCL software:
    - 13.1 or later

For host program compilation, the requirements are:
    - Linux: GNU Make
    - Windows: Microsoft Visual Studio 2010
    - OpenGL libraries

The host program requires a display to run.

The example comes with precompiled device binaries (AOCXs) for the following boards:
    - Bittware S5-PCIe-HQ D5
    - Bittware S5-PCIe-HQ D8
    - Nallatech PCIe385N A7
    - Nallatech PCIe385N D5

The supported operating systems for this release:
    - All operating systems supported by the Altera SDK for OpenCL


Package Contents and Directory Structure
========================================

/sobelfilter
  /device
     Contains OpenCL kernel source files (.cl)
  /host
    /inc
      Contains host header (.h) files
    /src
      Contains host source (.cpp) files
  /bin
    Contains OpenCL binaries (.aocx) and sample image data


Example Description
===================

This example is a simple Sobel filter. The single work-item OpenCL kernel detects
edges in an input RGB (8 bits per components) image and outputs a monochrome
image as the result. The kernel demonstrates how to use a sliding-window line
buffer to efficiently compute the convolution of a pixel's luma value. The result
of the convolution is compared against an input threshold value that can be
controlled by the user. The resolution of the filter is fixed through defines so
that the line buffer can be implemented efficiently in hardware.


Usage Instructions
==================

Linux:
  1. make
  2. ./bin/sobel_filter

Windows:
  1. Build the project in Visual Studio 2010.
  2. Run (Ctrl+F5).

When the program starts, it will display the unfiltered input image. The controls
for the program as listed below:

  <spacebar>:
    Toggle the filter on or off.

  <q>/<Q>/<esc>/<enter>:
    Quit the program.

  <=>:
    Restore the filter threshold to the default value.

  <+>:
    Increase the filter threshold value. 

  <->:
    Decrease the filter threshold value.

AOCX selection
--------------

The host program will use an AOCX file in the bin directory. To select
the AOCX file, it will examine the device name to extract the board name
(which was passed as the --board argument to aoc) and check for a
sobel_131_<board>.aocx file.
If it exists, then that AOCX file is used, otherwise the program will try
to load sobel.aocx.


Generating Kernel
=================

The precompiled AOCXs were compiled using the following command line:

  aoc sobel.cl --board <board>

To target a different OpenCL board change the --board argument to match the
appropriate board. If you are unsure of the board name use the following
command line to list it:

  aoc --list-boards

Place the resulting sobel.aocx file in the bin directory. 
If the board already has a AOCX file (see AOCX selection section above),
be sure to either replace or relocate that AOCX file.
 

Contacting Altera
=================

Although we have made every effort to ensure that this design example works
correctly, there might be problems that we have not encountered. If you have
a question or problem that is not answered by the information provided in 
this readme file or the example's documentation, please contact Altera(R) 
support.

http://www.altera.com/mysupport/

