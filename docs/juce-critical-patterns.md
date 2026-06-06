# JUCE Critical Patterns — Required Reading

## RT-safety (processBlock is sacred)
1. No heap allocation in processBlock — pre-allocate in prepareToPlay.
2. No locks, no mutexes — use lock-free FIFO / atomics for thread comms.
3. No logging, no file I/O, no JUCE String construction on the audio thread.
4. Denormals: ScopedNoDenormals at the top of processBlock.
5. Oversampling buffers allocated in prepareToPlay sized for the 32x max,
   then index down to 16x for realtime.

## GUI / painting (kill the jank)
6. Never allocate in paint(). Cache Paths, Images, Colours as members.
7. Cache the faceplate background as an Image; don't re-decode SVG per paint.
8. Filmstrip knob: load the 120-frame strip ONCE; blit the frame, never redraw.
9. Use setBufferedToImage() for static layers; repaint only dirty regions.
10. VBlankAttachment / friz for animation timing — not Timer at 60Hz.
11. melatonin_inspector in debug builds to catch overdraw and layout drift.

## Layout
12. place(x,y,w,h) scales from kRefW x kRefH against the plate-truth JSON.
13. resized() reads anchors from JSON only — zero literal coordinates.

## State
14. Magic-header XML blob is versioned and decoupled from the param list.
15. New params append with safe defaults; old sessions load without breakage.
16. setStateInformation must tolerate missing keys (forward/backward compat).

## Host correctness
17. beginChangeGesture/endChangeGesture wrap every automatable param move.
18. Report latency (setLatencySamples) for oversampling + convolution PDC.
19. Bypass is a real param honoring PDC, not a UI-only toggle.
