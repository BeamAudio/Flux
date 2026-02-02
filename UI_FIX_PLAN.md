# UI Fix Plan: Beam Audio Flux

**Status:** PENDING
**Goal:** Fix critical UI bugs and functional deficiencies identified in the codebase audit.

## 1. FlexBox Layout Engine Overhaul
**Problem:** `FlexBox` lacks `flex-wrap` and proper padding support, causing items to overflow or shrink illegibly instead of wrapping to a new line.
**Fix Steps:**
1.  **Add Wrapping**: Implement `flexWrap` property in `FlexBox`.
    *   Update `performLayout` to detect when `currentPos + itemSize > mainSize`.
    *   Move to next "line" (cross-axis increment) when wrapping occurs.
2.  **Add Padding**: Add `padding` (Left, Right, Top, Bottom) to the `FlexBox` class itself (not just items).
    *   Ensure `performLayout` respects container padding before placing items.
3.  **Refactor `LayoutItem`**: Ensure `margin` is correctly calculated during wrapping.

## 2. GenericNodeEditor Layout Correction
**Problem:** `GenericNodeEditor` stacks labels and sliders vertically in a single column, ignoring the intended "Row" layout (Label | Slider). This wastes space and looks unprofessional.
**Fix Steps:**
1.  **Implement Row Layout**:
    *   In `buildUI`, for each parameter, create a `FlexBox` container (or a lightweight helper struct) representing a Row.
    *   Add Label (Fixed Width) and Slider (Flexible Width) to this Row.
    *   Add the Row to the main Column layout.
2.  **Visual Polish**: Align labels to the right (or center-vertical) to match the slider.

## 3. QuadBatcher Clipping Fix
**Problem:** `glScissor` requires bottom-left coordinates, but the UI system uses top-left. Any mismatch in `screenHeight` or coordinate transform causes incorrect clipping, especially for text.
**Fix Steps:**
1.  **Verify Coordinate Transform**: Check `QuadBatcher::pushClip` and `setScissor`.
    *   Ensure `glScissor(x, screenHeight - y - h, w, h)` is used correctly.
    *   Verify `screenHeight` is passed correctly from `BeamHost::render`.
2.  **Nested Clipping**: Ensure `pushClip` intersects the *new* rect with the *current* scissor rect (if any), rather than replacing it blindly.

## 4. Interaction & Event Handling
**Problem:** Dragging logic is heavy (triggers full `resized()` on every mouse move). Z-ordering works but hit-testing might be inefficient.
**Fix Steps:**
1.  **Optimize Dragging**:
    *   Defer `resized()` calls or check if bounds *actually* changed significantly before re-layout.
    *   Consider `setBounds` optimization: only call `resized()` if W/H changed. If only X/Y changed, just move.
2.  **Focus Management**: Ensure clicking a generic editor field gives it focus (e.g., for text entry in the future, though mainly for sliders now).

## 5. Sidebar/TopBar State Wiring
**Problem:** Buttons toggle visual state but might not persist or reflect the *actual* engine state if changed externally.
**Fix Steps:**
1.  **Bi-directional Binding**: Ensure TopBar listeners (Play/Pause) also *subscribe* to Engine transport changes (e.g., if the engine stops due to end-of-track).
2.  **Mode Persistence**: Ensure `DAWMode` switching correctly hides/shows components without layout glitches (requires robust FlexBox).

## Execution Order
1.  `FlexBox` Upgrade (Core dependency).
2.  `GenericNodeEditor` Fix (Visual usability).
3.  `QuadBatcher` Clipping (Rendering correctness).
4.  Interaction Optimization.
