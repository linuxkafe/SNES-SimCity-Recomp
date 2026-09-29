/* The 16:9 presentation is arithmetic, so it is gated as arithmetic.
 *
 * Everything here is presentation-only: the pixel aspect the host uses when
 * fitting a 256x224 field to the drawable, and the window width that follows
 * from it. The PPU and the guest are not involved, which is the point - a
 * widescreen that changed emulation would be a bug this file cannot see.
 *
 * What must hold:
 *   - 16:9 is 14/9 as a pixel aspect, the only ratio that turns 256x224 into
 *     a 16:9 frame (256 * 14/9 = 398.2, and 398.2 / 224 is 16:9);
 *   - it FILLS a 16:9 drawable instead of sitting in it with pillarboxes,
 *     which is the entire reason the mode exists;
 *   - the three pre-existing aspects keep their old geometry, so adding the
 *     fourth changed nothing that was already shipping.
 */
#include "display_aspect.h"

#include <stdio.h>
#include <stdlib.h>

static int g_failures;

static void check(int condition, const char *what) {
  if (condition) {
    printf("ok   %s\n", what);
  } else {
    printf("FAIL %s\n", what);
    g_failures++;
  }
}

static void check_eq(int got, int want, const char *what) {
  if (got == want) {
    printf("ok   %s (%d)\n", what, got);
  } else {
    printf("FAIL %s: got %d, want %d\n", what, got, want);
    g_failures++;
  }
}

static void check_aspect(SnesDisplayAspect aspect, int num, int den,
                         const char *what) {
  int got_num = 0, got_den = 0;
  SnesDisplayAspect_GetPixelAspect(aspect, &got_num, &got_den);
  if (got_num == num && got_den == den) {
    printf("ok   %s (%d/%d)\n", what, got_num, got_den);
  } else {
    printf("FAIL %s: got %d/%d, want %d/%d\n", what, got_num, got_den, num, den);
    g_failures++;
  }
}

int main(void) {
  /* --- the pixel aspects, old and new ---------------------------------- */
  check_aspect(kSnesDisplayAspect_Crt4x3, 7, 6, "CRT 4:3 pixel aspect");
  check_aspect(kSnesDisplayAspect_SquarePixels8x7, 1, 1, "8:7 pixel aspect");
  check_aspect(kSnesDisplayAspect_SquareFrame1x1, 7, 8, "1:1 frame pixel aspect");
  check_aspect(kSnesDisplayAspect_Wide16x9, 14, 9, "16:9 pixel aspect");

  /* Clamping must not let the new value fall through to the old default. */
  check(SnesDisplayAspect_Clamp((int)kSnesDisplayAspect_Wide16x9) ==
            kSnesDisplayAspect_Wide16x9,
        "16:9 survives Clamp");
  check(SnesDisplayAspect_Clamp(kSnesDisplayAspect_Count) ==
            kSnesDisplayAspect_Crt4x3,
        "Clamp falls back to 4:3 past the end");

  /* --- the presented frame size ---------------------------------------- */
  int w = 0, h = 0;
  SnesDisplayAspect_ComputePresentationSize(256, 224, kSnesDisplayAspect_Wide16x9,
                                            &w, &h);
  check_eq(w, 398, "16:9 presents a 398-wide frame");
  check_eq(h, 224, "16:9 keeps 224 lines");

  SnesDisplayAspect_ComputePresentationSize(256, 224, kSnesDisplayAspect_Crt4x3,
                                            &w, &h);
  check_eq(w, 299, "4:3 still presents 299 wide");

  /* --- filling a 16:9 drawable is the whole point ---------------------- */
  SnesDisplayViewport vp;
  SnesDisplayAspect_ComputeViewport(256, 224, 1920, 1080,
                                    kSnesDisplayAspect_Wide16x9, false, false,
                                    &vp);
  check_eq(vp.x, 0, "16:9 has no left pillarbox");
  check_eq(vp.width, 1920, "16:9 fills the drawable width");
  check_eq(vp.height, 1080, "16:9 fills the drawable height");
  check_eq(vp.y, 0, "16:9 has no top letterbox");

  /* 4:3 in the same drawable must still letterbox, or the toggle would have
   * silently changed the mode the game has always run in. */
  SnesDisplayAspect_ComputeViewport(256, 224, 1920, 1080,
                                    kSnesDisplayAspect_Crt4x3, false, false,
                                    &vp);
  check_eq(vp.x, 240, "4:3 still pillarboxes by 240 per side");
  check_eq(vp.width, 1440, "4:3 still presents 1440 wide");

  /* IgnoreAspectRatio outranks every aspect, 16:9 included. */
  SnesDisplayAspect_ComputeViewport(256, 224, 1920, 1080,
                                    kSnesDisplayAspect_Wide16x9, true, false,
                                    &vp);
  check_eq(vp.width, 1920, "IgnoreAspectRatio still fills");
  SnesDisplayAspect_ComputeViewport(256, 224, 800, 600,
                                    kSnesDisplayAspect_Wide16x9, true, false,
                                    &vp);
  check_eq(vp.width, 800, "IgnoreAspectRatio wins over 16:9");

  /* --- the window follows the aspect ----------------------------------- */
  check_eq(SnesDisplayAspect_ComputeWindowWidth(256, 224, 240,
                                               kSnesDisplayAspect_Wide16x9),
           427, "16:9 window is 427 wide on 240 lines");
  check_eq(SnesDisplayAspect_ComputeWindowWidth(256, 224, 240,
                                               kSnesDisplayAspect_Crt4x3),
           320, "4:3 window is still 320 wide on 240 lines");

  /* The window this port actually opens for SimCity: 224 lines at scale 3.
   * 1194/672 is 1.7768, which is 16:9 to within a pixel of rounding. */
  int ww = SnesDisplayAspect_ComputeWindowWidth(256, 224, 224,
                                                kSnesDisplayAspect_Wide16x9);
  int scaled_w = ww * 3, scaled_h = 224 * 3;
  check_eq(scaled_w, 1194, "16:9 window at SimCity's 224 lines, scale 3");
  /* 16:9 to within the rounding of a single pixel at this width: a window
   * edge lands on an integer, so exact 16:9 is not always reachable. */
  check(abs(scaled_w * 9 - scaled_h * 16) <= 16,
        "16:9 window is 16:9 to within one pixel");

  if (g_failures) {
    printf("\n%d check(s) failed\n", g_failures);
    return 1;
  }
  printf("\nall display-aspect checks passed\n");
  return 0;
}
