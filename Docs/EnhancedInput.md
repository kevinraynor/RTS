# Enhanced Input in this project

This is a from-scratch explanation of Unreal's Enhanced Input system, written to answer a
concrete design question for this project: `CommandAction` (right-click) is currently
bound to one thing (issue a unit order), but it will eventually need to mean different
things depending on context — a normal right-click order, a click-and-hold camera pan, and
a right-click inside a shop/build UI. Enhanced Input's whole job is to make that kind of
context-sensitivity declarative instead of a pile of `if` statements in `Command()`. This
doc explains the building blocks, then works through that exact problem.

Assumes UE 5.8's Enhanced Input plugin (the only input system available by default in new
5.x projects — this is not an optional upgrade over "classic" input, it's what `Project
Settings > Input` now configures).

## 1. The three building blocks

Enhanced Input separates three things that the old input system bundled together:

1. **`UInputAction`** — an abstract, named *thing that can happen* ("Select", "Command",
   "MoveCamera"). It does **not** know which key/button triggers it. It only knows its
   **value type**: `Digital (bool)` (on/off, e.g. a click), `Axis1D (float)` (e.g. mouse
   wheel), `Axis2D (Vector2D)` (e.g. WASD movement), or `Axis3D (Vector)`.
2. **`UInputMappingContext` (IMC)** — a set of *mappings*, each one wiring a physical key
   (`LeftMouseButton`, `W`, `Gamepad_FaceButton_Bottom`, ...) to one `UInputAction`. A
   context is a swappable bundle of "what the controls mean right now." This project has
   `IMC_Default` mapping `LeftMouseButton → IA_Select`.
3. **`UEnhancedInputLocalPlayerSubsystem`** — per-player runtime state that holds a
   *stack* of currently-active mapping contexts, each with a priority. This is what
   `ARTSPlayerController::BeginPlay()` already talks to:

   ```cpp
   if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
   {
       Subsystem->AddMappingContext(DefaultMappingContext, 0);
   }
   ```

   `0` here is the **priority**. You can add more contexts later, at any time, with their
   own priorities, and remove them later with `RemoveMappingContext`. This add/remove call
   is not special to `BeginPlay` — it's the mechanism you're expected to call whenever the
   game's "input mode" changes (opening a shop, entering build-placement, pausing). More on
   this in section 5.

Why it's split this way: the same `UInputAction` ("Select") can be bound to different keys
per platform/rebind (`IMC_KeyboardMouse` vs `IMC_Gamepad`) without touching gameplay code,
and gameplay code (`SetupInputComponent`, `Command()`, etc.) only ever talks to the
abstract action, never to `EKeys::LeftMouseButton` directly.

## 2. What actually happens when you bind an action

```cpp
EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Triggered, this, &ARTSPlayerController::UpdateDragSelection);
```

Each frame, for each active mapping, Enhanced Input:

1. Reads the raw hardware value for the mapped key.
2. Runs it through that mapping's **Modifiers** (see section 4) to transform the raw value.
3. Feeds the result into the action's **Triggers** (see section 3), which are a small state
   machine that decides *when in the press/release lifecycle* the action is considered
   active.
4. Fires the delegate(s) bound to whichever `ETriggerEvent` matches the trigger state's
   transition this frame.

You never bind directly to "the input value" — you bind to a *lifecycle event* of the
trigger state machine:

| `ETriggerEvent` | Fires when... | Typical use |
|---|---|---|
| `Started` | The trigger state machine goes from idle to active for the first tick. | One-shot "the interaction began" — e.g. `StartDragSelection`. |
| `Ongoing` | Between `Started` and the trigger condition being fully satisfied (e.g. mid-way through a `Hold`'s charge-up). | Show a charge-up UI/VFX; rare for simple bool actions. |
| `Triggered` | The trigger's condition is actually satisfied, and fires **every tick** it remains satisfied for continuous triggers like `Down`. | Continuous per-frame effect — e.g. `UpdateDragSelection` while the mouse stays down. |
| `Canceled` | The trigger state machine aborts *before* the condition was met (e.g. released during a `Hold` before the threshold). | Cancel a charge-up/preview. |
| `Completed` | The action stops being active *after* having triggered — typically "key released." | One-shot "the interaction ended" — e.g. `EndDragSelection`, and this project's `Command()`. |

This is exactly why the existing code binds `SelectAction` to three different events:

```cpp
EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started,    this, &ARTSPlayerController::StartDragSelection);
EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Triggered,  this, &ARTSPlayerController::UpdateDragSelection);
EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed,  this, &ARTSPlayerController::EndDragSelection);
EnhancedInputComponent->BindAction(CommandAction, ETriggerEvent::Completed, this, &ARTSPlayerController::Command);
```

`SelectAction` needs the full lifecycle (start the box, update it every frame while held,
finalize it on release). `CommandAction` only needs `Completed` — "the click finished" is
the entire semantic content of "issue an order," so there's nothing useful to do on
`Started`/`Triggered` for it today. That'll change once `CommandAction` needs to
distinguish a quick click from a hold (section 6).

If an `UInputAction` has **no Triggers assigned** (the common case for a simple click, and
almost certainly how `IA_Select`/`IA_CommandAction` are set up today), it uses an implicit
default trigger appropriate to its value type — for a `Digital (bool)` action that's
equivalent to **Down**: `Started` the frame it's pressed, `Triggered` every frame it's held,
`Completed` the frame it's released. That implicit default is exactly what makes the
binding table above work with no Trigger assets configured at all.

## 3. Triggers — *when* does the action count as active?

A Trigger is an asset-configurable rule (added to the `Triggers` array on either the
`UInputAction` itself, or per-mapping in the `IMC`, or both — they combine) that decides
when raw input becomes a `Triggered` event. This is the actual mechanism for solving
"one physical key, different meanings depending on how you press it."

| Trigger | Behavior | RTS example |
|---|---|---|
| **Down** (implicit default) | `Triggered` every tick the input is actuated. | Holding a modifier key; continuous drag-select update. |
| **Pressed** | `Triggered` once, only on the tick input crosses the actuation threshold. | A UI-style single "click registered" event. |
| **Released** | `Triggered` once, only on the tick input drops back below threshold. | Rarely used directly — `Completed` from a `Down` trigger usually covers this. |
| **Hold** | `Triggered` only after being held continuously for `HoldTimeThreshold` seconds. Has a "trigger once" vs "fire repeatedly while held" option. | Hold RMB to start panning the camera. Hold a key to charge a special ability. |
| **Hold And Release** | Like `Hold`, but only fires (once) on *release*, and only if the hold threshold was met. | "Charge and release" abilities. |
| **Tap** | `Triggered` only if the input is pressed **and released again within** `TapReleaseTimeThreshold`. If held past that window, it does **not** trigger (goes `Canceled` instead). | A quick right-click = issue a command. |
| **Pulse** | `Triggered` repeatedly at a fixed interval while held (like a metronome), independent of frame rate. | Repeat-fire while holding a button; auto-repeat a hotkey. |
| **Chorded Action** | Only triggers while *another* specified `UInputAction` is also currently triggered. | Shift+Click = queue command instead of replacing the current order. |

**Key fact for solving the CommandAction problem:** `Tap` and `Hold` are mutually
exclusive by construction for the same physical press — a press either resolves within the
tap window (fires `Tap`, never reaches `Hold`'s threshold) or it doesn't (fires `Hold` once
past the threshold, `Tap` is `Canceled` because it wasn't released in time). That means you
can map **the same key** (Right Mouse Button) to **two different `UInputAction`s** in the
*same* mapping context — one with a `Tap` trigger, one with a `Hold` trigger — and exactly
one of them will ever fire per press, with no ambiguity and no code needed to disambiguate.
See section 6.

## 4. Modifiers — transforming the raw value before triggers see it

Modifiers run before triggers, on the mapping (or on the action, applied to every mapping
of it). Common ones:

| Modifier | Effect | Example |
|---|---|---|
| **Negate** | Flips the sign of one or more axes. | Invert a camera-pitch axis without touching C++. (Note: this project's current edge-scroll camera movement is driven by manual `Tick()` math, not an Enhanced Input axis — see `RTSCamera.cpp`'s `-Direction.Y` — so this modifier doesn't apply there today, but *would* be the natural place to put a Y-invert if camera panning is ever driven by an `Axis2D` Input Action, e.g. the RMB-hold pan proposed below, or WASD camera movement.) |
| **Swizzle Input Axis Values** | Reorders/remaps axis components (e.g. turn a 1D scroll into the Y of a 2D vector). | Mouse wheel feeding a zoom axis. |
| **Dead Zone** | Ignores small values near zero (analog stick drift). | Gamepad camera pan/look. |
| **Scalar** | Multiplies the value by a constant/curve. | Scale a raw mouse-delta axis down to a sane camera-pan speed. |
| **Negate + Swizzle combos** | Common for converting mouse-delta (X right, Y down) into a world-space pan direction. | RMB-hold camera pan. |

Modifiers only make sense for `Axis1D`/`Axis2D`/`Axis3D` actions — a `Digital (bool)`
action like `IA_Select` or `IA_Command` has nothing for them to transform.

## 5. Mapping Contexts — swapping *what the controls mean*

An `IMC` is not just "a keybinding preset for rebinding UI." It's the primary tool for
**contextual** input: the same key can and should mean different things in different game
states, and the way you express that in Enhanced Input is by changing *which contexts are
currently active*, not by writing `if (bIsShopOpen)` inside a single input handler.

- `AddMappingContext(Context, Priority, Options)` pushes a context onto the per-player
  stack. `RemoveMappingContext(Context)` pops it back off. You can call these at any time,
  from anywhere you have a `ULocalPlayer` — a widget's `OnShopOpened`, a build-mode
  controller function, a pause menu, etc. — not just once in `BeginPlay`.
- **Priority** matters when two *active* contexts both map the *same physical key*. Per
  Epic's own documentation, higher-priority contexts take precedence over lower-priority
  ones for a conflicting key. Treat this as "the higher-priority context wins," but don't
  lean on precise conflict-resolution internals for anything that matters — see the next
  paragraph for why.
- **The robust pattern is to avoid relying on priority-based conflict resolution at all.**
  Instead: give a modal state (shop open, placing a building, a cutscene) its *own*
  `IMC`, `Add` it when the state begins, `Remove` it when the state ends, and only map the
  keys that state actually needs differently. If the same key needs a *different* action
  while in that mode, either don't map it at all in the base context while the modal
  context is active (i.e. actually remove/re-add `IMC_Default` too, not just stack), or
  give the modal context a clearly higher priority and treat that as the intended override.
  Ambiguity here is a design smell, not something to paper over with priority numbers.

This is exactly the tool for the "buying upgrades" part of the question: don't try to make
`CommandAction` smart enough to know "am I in a shop." Instead, add an `IMC_Shop` context
(with its own `IA_BuyUpgradeSlot1..N` actions, or number-key hotkeys) when the shop UI
opens, and remove it when the shop closes. `CommandAction`/`IMC_Default` don't need to know
the shop exists.

## 6. Worked example: redesigning `CommandAction` for this project

The problem stated: right-click currently does one thing (`Command()` → issue an order),
but needs to eventually also mean "hold and drag to pan the camera" and "buy an upgrade"
depending on context. Three separate mechanisms handle the three separate cases — don't
solve all three the same way:

### 6a. Quick click vs hold-drag, same key, same context → split by Trigger

Create two actions instead of overloading one:

- `IA_Command` (`Digital`) — mapped to `RightMouseButton` in `IMC_Default`, with a **Tap**
  trigger (e.g. `TapReleaseTimeThreshold = 0.25`). Bind its `Triggered` (or `Completed`,
  either works for a `Tap` since it fires once) to `Command()`, unchanged from today.
- `IA_PanCamera` (`Digital`, or `Axis2D` if you want mouse-delta magnitude/direction rather
  than a plain bool) — mapped to the *same* `RightMouseButton` in `IMC_Default`, with a
  **Hold** trigger (e.g. `HoldTimeThreshold = 0.25`, matching the Tap's window so there's
  no dead gap). Bind `Started`/`Triggered`/`Completed` the same way `SelectAction`'s drag
  box works today: `Started` → begin panning, `Triggered` → update pan from mouse delta
  each frame, `Completed` → stop panning.

Because `Tap` only fires for a press-and-release inside the window and `Hold` only fires
for a press held past it, a single right-click press resolves as *exactly one* of these —
no manual "was this a click or a drag" bookkeeping in C++, and no risk of both firing.

### 6b. Different meaning in a different game mode → split by Mapping Context

For "buy an upgrade": add `IMC_Shop` with its own actions (e.g. `IA_BuyUpgradeSlot1`
mapped to `1`, `IA_CloseShop` mapped to `Escape` or `RightMouseButton`). Push it when the
shop widget opens, pop it when it closes:

```cpp
// e.g. in the shop widget or a controller function it calls
if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
    {
        Subsystem->AddMappingContext(ShopMappingContext, 10); // higher priority than IMC_Default's 0
    }
}
```

and the mirrored `RemoveMappingContext(ShopMappingContext)` on close. If you don't want RMB
to *also* issue a move order while the shop is open, either don't map `RightMouseButton` to
anything conflicting in `IMC_Shop` (and rely on priority to mask `IMC_Default`'s mapping),
or explicitly `RemoveMappingContext(DefaultMappingContext)` while the shop is open and
re-add it on close. The latter is more explicit and easier to reason about — recommended
unless you specifically want some `IMC_Default` actions (e.g. camera pan) to keep working
underneath the shop UI.

Note: if "buying an upgrade" is purely a button click in a `UUserWidget` (a `Buy` button
with an `OnClicked` delegate), you don't need Enhanced Input at all for that — Enhanced
Input is for input that happens *in the game world/viewport*, not for widget click events,
which UMG already handles natively. Reach for an `IMC` here only if you want a *keyboard
shortcut* for buying (e.g. press `1` to buy the item in slot 1 while the shop is open).

### 6c. What Enhanced Input does *not* decide: move vs attack-move vs gather

`IA_Command`'s `Triggered`/`Completed` event only ever means "the command input happened
right now, at this screen position." Deciding *what kind* of order that produces — move to
empty ground, attack-move onto an enemy unit, gather onto a resource node — is gameplay
logic, not an Input concept, and belongs exactly where the existing TODO already is:

```cpp
void ARTSPlayerController::Command()
{
    FVector2D CommandPosition;
    GetMousePosition(CommandPosition.X, CommandPosition.Y);

    // TODO: project screen pos to world, use trace to find object.
}
```

The trace result (hit an enemy unit / hit a resource / hit walkable ground / hit nothing)
is what should branch into move/attack-move/gather — not additional `UInputAction`s or
`UInputTrigger`s. Keep "was a command issued" (Enhanced Input's job) and "what does this
particular command mean" (your game logic's job) separate; trying to encode the latter into
more input actions/triggers is a common over-engineering trap.

## 7. Recommended action/context inventory for this project

Not all of this exists yet — `IA_Command`/`IMC_Shop`/etc. are proposals from section 6, not
current assets. Existing assets are marked accordingly.

| Asset | Type | Status | Notes |
|---|---|---|---|
| `IMC_Default` | Mapping Context, priority 0 | **Exists** | Always active during normal gameplay. |
| `IA_Select` | Digital | **Exists** | `LeftMouseButton`, implicit `Down` trigger, full `Started`/`Triggered`/`Completed` lifecycle bound for drag-select. |
| `IA_Command` *(rename from `CommandAction`?)* | Digital | Proposed split | `RightMouseButton` with a `Tap` trigger → `Command()`. |
| `IA_PanCamera` | Digital or Axis2D | Proposed | `RightMouseButton` with a `Hold` trigger → begin/update/end camera pan. |
| `IMC_Shop` | Mapping Context, priority 10 | Proposed | Pushed on shop-open, popped on shop-close. |
| `IA_BuyUpgradeSlot1..N` | Digital | Proposed | Number-key hotkeys, only meaningful while `IMC_Shop` is active. |
| `IMC_BuildPlacement` | Mapping Context, priority 10 | Proposed (future) | For a "placing a building ghost" mode — `LeftMouseButton` → confirm placement, `RightMouseButton`/`Escape` → cancel. Same pattern as the shop: push on enter build-mode, pop on exit/confirm/cancel. |

## 8. Checklist for adding a new context-sensitive input

1. Is this really a *new meaning*, or the same meaning with a different game-state
   precondition? If the latter, you may just need an early-out in the existing handler, not
   new Input assets at all.
2. If it's genuinely a new meaning on a key that already means something else **in the same
   context**: can `Tap`/`Hold`/`Pulse`/`Chorded Action` triggers disambiguate it by *how*
   the key is pressed? If yes, add a second `UInputAction` with the differentiating
   trigger, mapped to the same key, in the same `IMC` (section 6a).
3. If it depends on *game/UI mode* rather than press style: give that mode its own `IMC`,
   `AddMappingContext` on entering the mode, `RemoveMappingContext` on leaving it (section
   6b). Decide explicitly whether the base context should keep working underneath it.
4. Resist adding more Triggers to make Enhanced Input decide *game* semantics (which enemy,
   which item, which order type). That belongs in the bound handler function via a trace or
   game-state check (section 6c).
