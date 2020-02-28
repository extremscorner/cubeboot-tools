cubeboot-tools-0.3 for the Nintendo GameCube
12 Mar 2006, Albert "isobel" Herranz

This is the third proof-of-concept release of the cubeboot-tools.

1. QUICK INTRODUCTION

   The cubeboot-tools package allows you to generate a Generic Boot Image with
   an embedded apploader capable of booting DOLs from a
   "El Torito Specification" compatible iso9660 disc.

   This Generic Boot Image can be passed to mkisofs to build a homebrew
   GameCube bootable disc.
   A disc generated this way can be directly booted by an IPL replacement, just
   like normal games are booted.

   Starting with the second release of the cubeboot-tools, discs can also be
   launched from the original IPL if the drive is first patched by any means
   to accept normal media.
   The original IPL has some restrictions though on the layout of DOLs that
   can be used. If you plan to boot discs from the original IPL first make
   sure that your booting DOL has the proper layout.

   The third release enhances udolrel, the dol relocation tool, adding
   support for disabling the xenogc on .dol startup, and fixing a padding
   bug.

   For more information, please, refer to the following article:

   http://www.gc-linux.org/wiki/Building_a_Bootable_Disc

   Have fun!


2. CREDITS

   Special thanks go to:

   - Wallbraker and Stonebone for testing the xenogc disabling code.


3. DISCLAIMER

   THIS PROGRAM IS NOT AFFILIATED WITH NINTENDO.

   IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
   SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
   OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
   BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
   AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
   ON AN "AS IS" BASIS, AND THE AUTHOR HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


