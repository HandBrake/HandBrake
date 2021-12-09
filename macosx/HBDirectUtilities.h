/* HBDirectUtilities.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#ifndef HBDirectUtilities_h
#define HBDirectUtilities_h

// Direct method and property calls increases performance and reduces binary size.
#if defined(__IPHONE_14_0) || defined(__MAC_10_16) || defined(__MAC_11_0) || defined(__TVOS_14_0) || defined(__WATCHOS_7_0)
#define HB_OBJC_DIRECT_MEMBERS __attribute__((objc_direct_members))
#define HB_OBJC_DIRECT __attribute__((objc_direct))
#define HB_DIRECT ,direct
#else
#define HB_OBJC_DIRECT_MEMBERS
#define HB_OBJC_DIRECT
#define HB_DIRECT
#endif


#endif /* HBDirectUtilities_h */
