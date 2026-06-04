/*
 * touchupd.c — Touch Up v3 daemon prototype (2026-06-04)
 *
 * Seizes the touchscreen HID device (exclusive open — macOS/WindowServer
 * stops receiving its events), parses absolute X/Y + tip-switch reports,
 * maps them to the bound display, and posts synthetic mouse events there.
 *
 * Validated on: Prechen 12.3" (Techwin HID, VID 0x222a PID 0x335),
 * single-pointer report: GenericDesktop X/Y (12-bit) + Button 1 as tip.
 *
 * Usage:  sudo touchupd [-v] [--swap-xy] [--invert-x] [--invert-y]
 *                       [--vid 0x222a] [--pid 0x335] [--display WxH]
 *
 * Requires: Input Monitoring (seize) + Accessibility (event posting)
 * for the responsible process (Terminal during testing; the launchd
 * daemon binary in production).
 */

#include <IOKit/hid/IOHIDManager.h>
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static int  gVid = 0x222a, gPid = 0x335;
static bool gSwapXY = false, gInvX = false, gInvY = false, gVerbose = false;
static int  gTargetW = 1280, gTargetH = 480;   /* logical size of bound display */

static CGRect gTarget;
static bool   gHaveTarget = false;

static double gNX = -1, gNY = -1;   /* normalized 0..1 touch position */
static bool   gTouching = false;

static bool    gRestoreCursor = true;  /* snap cursor back to pre-touch position on lift */
static CGPoint gSavedPos;

static void findTargetDisplay(void) {
    CGDirectDisplayID ids[16]; uint32_t n = 0;
    CGGetActiveDisplayList(16, ids, &n);
    for (uint32_t i = 0; i < n; i++) {
        CGRect b = CGDisplayBounds(ids[i]);
        if ((int)b.size.width == gTargetW && (int)b.size.height == gTargetH) {
            gTarget = b; gHaveTarget = true;
            fprintf(stderr, "[touchupd] bound display id=%u  %dx%d at (%.0f,%.0f)\n",
                    ids[i], gTargetW, gTargetH, b.origin.x, b.origin.y);
            return;
        }
    }
    fprintf(stderr, "[touchupd] ERROR: no %dx%d display found. Connected displays:\n",
            gTargetW, gTargetH);
    for (uint32_t i = 0; i < n; i++) {
        CGRect b = CGDisplayBounds(ids[i]);
        fprintf(stderr, "  id=%u  %.0fx%.0f at (%.0f,%.0f)\n",
                ids[i], b.size.width, b.size.height, b.origin.x, b.origin.y);
    }
}

static CGPoint mapPoint(void) {
    double x = gNX, y = gNY;
    if (gSwapXY) { double t = x; x = y; y = t; }
    if (gInvX)   x = 1.0 - x;
    if (gInvY)   y = 1.0 - y;
    return CGPointMake(gTarget.origin.x + x * gTarget.size.width,
                       gTarget.origin.y + y * gTarget.size.height);
}

static void post(CGEventType type, CGPoint pt) {
    CGEventRef e = CGEventCreateMouseEvent(NULL, type, pt, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, e);
    CFRelease(e);
}

static void inputCallback(void *ctx, IOReturn res, void *sender, IOHIDValueRef val) {
    IOHIDElementRef el = IOHIDValueGetElement(val);
    uint32_t page  = IOHIDElementGetUsagePage(el);
    uint32_t usage = IOHIDElementGetUsage(el);
    CFIndex  v     = IOHIDValueGetIntegerValue(val);
    CFIndex  lmin  = IOHIDElementGetLogicalMin(el);
    CFIndex  lmax  = IOHIDElementGetLogicalMax(el);

    if (page == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_X && lmax > lmin) {
        gNX = (double)(v - lmin) / (double)(lmax - lmin);
        if (gTouching && gNY >= 0) post(kCGEventLeftMouseDragged, mapPoint());

    } else if (page == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_Y && lmax > lmin) {
        gNY = (double)(v - lmin) / (double)(lmax - lmin);
        if (gTouching && gNX >= 0) post(kCGEventLeftMouseDragged, mapPoint());

    } else if (page == kHIDPage_Button && usage == 1) {
        if (!gHaveTarget || gNX < 0 || gNY < 0) return;
        CGPoint pt = mapPoint();
        if (v && !gTouching) {
            gTouching = true;
            if (gRestoreCursor) {                    /* remember where the real cursor is */
                CGEventRef cur = CGEventCreate(NULL);
                gSavedPos = CGEventGetLocation(cur);
                CFRelease(cur);
            }
            post(kCGEventMouseMoved, pt);
            post(kCGEventLeftMouseDown, pt);
            if (gVerbose) fprintf(stderr, "[touchupd] down (%.0f, %.0f)\n", pt.x, pt.y);
        } else if (!v && gTouching) {
            gTouching = false;
            post(kCGEventLeftMouseUp, pt);
            if (gRestoreCursor)                      /* snap it back — no event generated */
                CGWarpMouseCursorPosition(gSavedPos);
            if (gVerbose) fprintf(stderr, "[touchupd] up   (%.0f, %.0f)\n", pt.x, pt.y);
        }
    }
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--swap-xy"))  gSwapXY = true;
        else if (!strcmp(argv[i], "--invert-x")) gInvX = true;
        else if (!strcmp(argv[i], "--invert-y")) gInvY = true;
        else if (!strcmp(argv[i], "-v"))         gVerbose = true;
        else if (!strcmp(argv[i], "--no-restore-cursor")) gRestoreCursor = false;
        else if (!strcmp(argv[i], "--vid") && i+1 < argc) gVid = (int)strtol(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "--pid") && i+1 < argc) gPid = (int)strtol(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "--display") && i+1 < argc)
            sscanf(argv[++i], "%dx%d", &gTargetW, &gTargetH);
    }

    findTargetDisplay();
    if (!gHaveTarget) return 1;

    IOHIDManagerRef mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    CFMutableDictionaryRef match = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFNumberRef v = CFNumberCreate(NULL, kCFNumberIntType, &gVid);
    CFNumberRef p = CFNumberCreate(NULL, kCFNumberIntType, &gPid);
    CFDictionarySetValue(match, CFSTR(kIOHIDVendorIDKey), v);
    CFDictionarySetValue(match, CFSTR(kIOHIDProductIDKey), p);
    IOHIDManagerSetDeviceMatching(mgr, match);
    IOHIDManagerRegisterInputValueCallback(mgr, inputCallback, NULL);

    IOReturn r = IOHIDManagerOpen(mgr, kIOHIDOptionsTypeSeizeDevice);
    fprintf(stderr, "[touchupd] open(seize) -> 0x%08x %s\n", r,
            r == kIOReturnSuccess ? "SUCCESS" : "FAIL");
    if (r != kIOReturnSuccess) return 1;

    fprintf(stderr, "[touchupd] running — touches are now exclusive to the bound display. Ctrl+C to stop.\n");
    IOHIDManagerScheduleWithRunLoop(mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    CFRunLoopRun();

    IOHIDManagerClose(mgr, kIOHIDOptionsTypeSeizeDevice);
    return 0;
}
