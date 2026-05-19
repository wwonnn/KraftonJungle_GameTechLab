# Animation Sequence Viewer / AnimNotify Manual Verification Checklist

## Environment

- Branch:
- Commit:
- Tester:
- Date:
- Target Asset (`.sequence`):
- Linked `.animinstance` Asset:

## 1. Build / Launch

- [ ] Manual build succeeds.
- [ ] The editor launches normally.
- [ ] No new `AnimNotify`-related assert, crash, or severe error log appears on startup.

## 2. Sequence Viewer Baseline

- [ ] Opening a `.sequence` file opens the Animation Sequence Viewer correctly.
- [ ] Reopening the same `.sequence` reuses the existing tab and only focuses it.
- [ ] The preview viewport renders correctly.
- [ ] Preview camera input still behaves as before.
- [ ] Play / Pause / Stop / Loop / PlayRate still behave as before.
- [ ] Timeline scrub / zoom / pan still behave as before.

## 3. Notify Selection / Editing

- [ ] Clicking a notify marker in the track selects it.
- [ ] The selected notify is visibly highlighted.
- [ ] Dragging a marker left/right updates its time.
- [ ] Editing `Name` applies correctly.
- [ ] Editing `Type` applies correctly.
- [ ] The `Notify Class` dropdown opens and displays correctly.
- [ ] Editing `Payload` applies correctly.
- [ ] Editing `Time` applies correctly.
- [ ] Editing `Duration` applies correctly.
- [ ] Editing `Color` applies correctly.

## 4. Notify Add / Delete / Save

- [ ] A new notify can be added.
- [ ] A notify can be deleted.
- [ ] Saving and reopening preserves the notify.
- [ ] Saving and reopening preserves `Type`.
- [ ] Saving and reopening preserves `Notify Class`.
- [ ] Saving and reopening preserves `Payload`.
- [ ] Saving and reopening preserves `Time / Duration / Color`.

## 5. Recent Fired Notifies Baseline

- [ ] A standard notify at `1.000s` appears in `Recent Fired Notifies` during playback.
- [ ] Recent fired items remain as a short history instead of disappearing after one frame.
- [ ] The display distinguishes `Notify`, `Begin`, and `End` phases.

## 6. Zero-Time Boundary

- [ ] A standard notify at `0.000s` appears in recent fired notifies when playback starts from the beginning.
- [ ] With loop enabled, the `0.000s` notify fires again after wrapping.
- [ ] A `Notify State` starting at `0.000s` shows `Begin @ 0.000s`.

## 7. Notify State Behavior

- [ ] `Type = Notify State`, `Time = 1.000s`, `Duration = 0.100s` can be configured.
- [ ] Playback shows `Begin @ 1.000s`.
- [ ] Playback shows `End @ 1.100s`.
- [ ] `Duration = 0` does not cause broken immediate `Begin/End` behavior.
- [ ] `Tick` behavior can be confirmed through logs or Lua hooks when needed.

## 8. Notify Class Dropdown

- [ ] `Type = Notify` only shows `UAnimNotify`-derived classes.
- [ ] `Type = Notify State` only shows `UAnimNotifyState`-derived classes.
- [ ] `UAnimNotifyLog` appears in the list.
- [ ] `UAnimNotifyStateLog` appears in the list.
- [ ] A saved but unavailable class shows the `(missing)` label correctly.

## 9. Sample Native Notify

- [ ] `Type = Notify`, `Notify Class = UAnimNotifyLog` can be selected.
- [ ] Playback produces `UAnimNotifyLog` output in logs.
- [ ] The `Payload` string is visible in the log output.

## 10. Sample Native Notify State

- [ ] `Type = Notify State`, `Notify Class = UAnimNotifyStateLog` can be selected.
- [ ] Playback produces `Begin` log output.
- [ ] Playback produces `Tick` log output.
- [ ] Playback produces `End` log output.
- [ ] The `Payload` string is visible in the log output.

## 11. Lua Notify Hooks

- [ ] `AnimNotify(event)` is called.
- [ ] `AnimNotify_<Name>(event)` is called.
- [ ] `AnimNotifyBegin(event)` is called.
- [ ] `AnimNotifyTick(event, DeltaTime)` is called.
- [ ] `AnimNotifyEnd(event)` is called.

## 12. State Machine Integration Regression

- [ ] A `.animinstance` asset still opens correctly.
- [ ] The State Machine editor can still open linked `.sequence` assets.
- [ ] No obvious regression appears in the existing state machine runtime path.

## 13. Final Regression Pass

- [ ] Preview pose updates correctly.
- [ ] Notify add / move / delete / save feel stable overall.
- [ ] `Recent Fired Notifies` matches actual playback behavior.
- [ ] No unrelated editor behavior regressed.

## Result Summary

- Overall Result: `PASS / FAIL`
- Failed Items:
- Notes:
- Repro Steps for Failures:
