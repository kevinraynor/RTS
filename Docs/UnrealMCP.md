# Unreal MCP notes

Working notes on the `unreal-mcp` MCP server (project-scoped in `.mcp.json`, HTTP transport
at `http://127.0.0.1:8000/mcp`, started manually from the Unreal Editor side before each
session). Keep this updated as new quirks/gaps are discovered — the point is to not
re-pay the discovery cost (large tool-schema dumps, trial and error) every session.

## Connecting

- The server must already be running in the Editor (`localhost:8000`) before Claude Code
  starts. Adding/removing it via `claude mcp add/remove` mid-session does NOT hot-load
  tools into the running session — MCP servers are wired up at session start only.
  If the server wasn't up yet, or was just added/approved, the session needs a full
  restart (exit and relaunch) before its tools appear.
- Project config lives in `.mcp.json` at repo root (`unreal-mcp` entry). This requires
  a one-time interactive approval prompt on session startup (shows as "⏸ Pending approval"
  in `claude mcp list` until then).
- Don't also add a duplicate user-scoped entry via `claude mcp add` — check
  `claude mcp list` first if unsure whether it's already covered by `.mcp.json`.

## Tool shape

Three meta-tools only (no per-feature tools are pre-loaded):
- `list_toolsets` — names + one-line descriptions of every toolset.
- `describe_toolset(toolset_name)` — full schemas for every tool in one toolset.
  **These dumps are often 50-70KB+ and get saved to a file instead of returned inline.**
  Don't try to `Read` the saved file directly (lines are too long / exceeds token limits).
  Instead:
  - For plain-JSON dumps: `python -c "import json; ..."` to load and pretty-print just the
    tool(s) you need (see recipe below). A plain `python` invocation works in this repo's
    shell even though `python3` doesn't.
  - For dumps that come back as a content-block array (`[{"type": "text", "text": "..."}]`),
    join the `text` fields first, then `json.loads` that.
  - Quick tool-name inventory of a toolset without loading full schemas: `grep`/`Bash grep -oE`
    for `"name":"<toolset>\.[A-Za-z_]+"` (or `grep -oE` on the raw file if the JSON tool's
    `-o` content mode acts up on a single huge line).
- `call_tool(tool_name, toolset_name, arguments)` — actual invocation. `tool_name` is the
  short name (no toolset prefix); `toolset_name` is required for toolset tools, omitted
  for top-level ones.

Python recipe for pulling specific tool schemas out of a saved describe_toolset dump:
```python
import json
with open(r"<saved path>", encoding="utf-8") as f:
    data = json.load(f)   # or: "".join(b["text"] for b in json.loads(raw) if b.get("type")=="text")
wanted = {"create", "compile_blueprint"}
for t in data["tools"]:
    if t["name"].rsplit(".", 1)[-1] in wanted:
        print(json.dumps(t, indent=1))
```

## Object/class references

Everywhere a tool wants a UObject/UClass, pass `{"refPath": "<content path or /Script/... path>"}`.
Examples: `/Game/Game/Maps/Development`, `/Script/RTS.RTSPlayerController`, `/Script/EnhancedInput.InputAction`.
Tool results that return object pointers come back the same shape — pass them straight
back into the next call.

## Toolsets relevant to this project (of the ~50 registered)

- `editor_toolset.toolsets.blueprint.BlueprintTools` — `create(folder_path, asset_name,
  asset_type)` makes a Blueprint parented to `asset_type` directly (no separate
  `set_parent` needed for the common case). `get_default_object(blueprint)` → CDO ref,
  then use `ObjectTools.set_properties`/`get_properties` on that CDO to set
  `EditDefaultsOnly` properties (e.g. `DefaultMappingContext`, `SelectAction` on
  `ARTSPlayerController`). `compile_blueprint(blueprint)` after edits.
- `editor_toolset.toolsets.object.ObjectTools` — `list_properties` → `get_properties` →
  `set_properties` is the required workflow for setting anything (property names are
  not guessable, especially on widgets/slots — always `list_properties` first).
  `search_subclasses(base_class, class_name)` for class discovery.
- `editor_toolset.toolsets.material.MaterialTools` — `create_material` then build the
  graph by hand: `add_expression(material, expression_class, x, y)`,
  `connect_expressions`/`connect_to_output`, `recompile(material)` once done.
  `list_expression_classes(material, search)` to find node class refs (e.g. search
  "Fresnel", "Multiply", "VectorParameter").
- `editor_toolset.toolsets.material_instance.MaterialInstanceTools` — not yet explored.
- `editor_toolset.toolsets.scene.SceneTools` — `add_to_scene_from_class` /
  `add_to_scene_from_asset` to place actors; `load_level`/`get_current_level`;
  `find_actors` for querying the level.
- `editor_toolset.toolsets.asset.AssetTools` — `create_folder`, `find_assets`
  (`folder_path: ""` searches the whole project incl. plugin content), `exists`,
  `move`/`duplicate`/`delete`. No generic "create arbitrary asset" tool — see gap below.
- `editor_toolset.toolsets.primitive.PrimitiveTools` — `add_cube`/`add_sphere`/
  `add_cylinder`/`add_cone` add a StaticMeshComponent (engine basic-shape mesh) to an
  existing actor. Good for placeholder/greybox environment art.
- `UMGToolSet.UMGToolSet` — `CreateWidgetBlueprint(folderPath, assetName, parentClass)`,
  `AddWidget(widgetBlueprint, widgetClass, widgetDisplayName, parentWidget, childIndex)`,
  `RenameWidget`, `CompileWidgetBlueprint`. Every widget/slot property still goes through
  `ObjectTools.list_properties` → `get_properties`/`set_properties` (widget property
  names can't be guessed, e.g. for `CanvasPanelSlot` position/size).
- `editor_toolset.toolsets.programmatic.ProgrammaticToolset` — **not** raw Unreal Python.
  It's a sandboxed script that can only call other already-registered MCP tools via
  `execute_tool(tool_name, json_input)`, with stdlib-only imports (`json`, `math`,
  `datetime`, `copy`, `re`, `time`). Useful for batching several tool calls into one
  round-trip; useless for anything outside the exposed toolset surface.

## Property name discovery (important, saves a lot of guesswork)

`list_properties`/`get_properties`/`set_properties` all use **camelCase** JSON keys that
mirror the underlying UPROPERTY names (`MaterialDomain` → `materialDomain`,
`DefaultGameMode` → `defaultGameMode`, `OverrideMaterials` → `overrideMaterials`, etc.) —
this is consistent enough to guess from the C++ name, but verify for anything unfamiliar.
`list_properties` results are also often 50-100KB and get saved to a file as an **escaped
JSON string** (`\"propertyName\"` with literal backslashes, because it's a JSON string
containing serialized JSON). Grepping for `"propertyName"` on that file will match
nothing — search for the bare word instead: `grep -ioE '[a-zA-Z]*keyword[a-zA-Z]*' file`.

Confirmed property names worth remembering directly (skips a `list_properties` round-trip):
- `UMaterial`: `materialDomain` (`MD_UI`, `MD_Surface`, ...), `blendMode`
  (`BLEND_Translucent`, `BLEND_Opaque`, ...), `shadingModel` (`MSM_Unlit`, ...).
- `UStaticMeshComponent` (incl. ones PrimitiveTools creates): `overrideMaterials` — array
  of material refs, set as `{"overrideMaterials": [{"refPath": "..."}]}`.
- `UBorder` (UMG): background brush is `background` (a `SlateBrush` struct) — set via
  `{"background": {"drawAs": "Image", "resourceObject": {"refPath": "<material>"}, "imageSize": {"x":32,"y":32}, "tintColor": {"specifiedColor": {"r":1,"g":1,"b":1,"a":1}, "colorUseRule": "UseColor_Specified"}}}`.
  `drawAs: "Image"` is what makes a Material resourceObject actually render (vs `Box`/`Border`
  which are for 9-slice texture brushes).
- `AWorldSettings`: GameMode override is `defaultGameMode` (not obviously named — the
  Editor UI calls it "GameMode Override").
- Blueprint CDO properties from a C++ `UPROPERTY` (e.g. `SelectAction`, `Cost`) show up
  camelCased on the CDO the same way (`selectAction`, `cost`).
- Material expression property names (from `MaterialExpression*` classes, via
  `add_expression` + inspect): `MaterialExpressionConstant3Vector`/`Constant2Vector` →
  `constant` (3-vector: `{r,g,b}`) / `r`,`g` (2-vector, no wrapper). `VectorParameter` →
  `parameterName` + `defaultValue: {r,g,b,a}`. `ScalarParameter` → `parameterName` +
  `defaultValue: <float>`. `ComponentMask` → `r`,`g`,`b`,`a` booleans (which channels pass
  through), single unnamed input pin (`get_expression_input_names` returns `["None"]` —
  pass `""` as the pin name to `connect_expressions`). `Abs` is also single-input/`"None"`.
  `Subtract`/`Add`/`Max`/`Multiply`/`Divide` → inputs `["A","B"]`. `LinearInterpolate`
  (Lerp) → `["A","B","Alpha"]`. `Step` → `["Y","X"]`, where `Y` is the threshold and `X`
  is the value being tested (`output = X >= Y`).
- `ProgrammaticToolset.execute_tool_script` is the efficient way to build a whole material
  graph (many `add_expression`/`connect_expressions`/`set_properties` calls) or place a
  batch of level actors in **one** round-trip instead of dozens — write small `ref()`/
  `call()` helpers at the top of the script (see the material-graph and village-placement
  examples in this session if you need a template). Note it raises on the *first* failing
  `execute_tool` call and everything before that point has already been applied to the
  editor (not transactional) — if a batched script errors partway, don't just re-run it
  blindly, check what already got created/renamed and adjust before retrying.
- `SceneTools.save_actor` only works for **external actor assets** (One-File-Per-Actor /
  World Partition levels). A normal single-file level (like `Development` here) rejects
  it with "not an external actor asset and cannot be saved individually" — save the whole
  level instead via `AssetTools.save_assets([])` (empty list = save everything dirty).

## Known gap: no C++ compile/hot-reload trigger

`EditorToolset.EditorAppToolset` (console vars, PIE control, viewport/content-browser
queries, screenshots) and every other toolset checked so far have no "recompile"/"Live
Coding"/"execute console command" tool. So after adding or changing a native `UCLASS`
in C++, the running Editor has no way to be told about it through MCP — a Blueprint
`create()` call targeting a brand-new C++ class (e.g. `/Script/RTS.RTSCamera` right after
it's first written) will fail or silently target a stale version until the user compiles
(Live Coding, or a full Editor restart) on their end. Workaround: write/edit the C++,
then explicitly ask the user to compile before continuing with any Blueprint work that
depends on the new/changed class; there's no way to detect "has it compiled yet" via MCP
either, so wait for them to confirm rather than polling.

## Known gap: Enhanced Input assets

No toolset here can create a `UInputAction` or `UInputMappingContext` asset — checked
`AssetTools` (no generic create-by-class), `DataAssetTools` (UDataAsset subclasses only,
these aren't DataAssets), and `ProgrammaticToolset` (can't reach raw `unreal.*` Python).
Workaround: ask the user to create the two assets by hand in the Content Browser
(right-click → Input → Input Action / Input Mapping Context — a few seconds), then
everything else (naming, key bindings, assigning to a Blueprint's CDO) is scriptable as
normal once the asset exists.

**Setting key bindings on a `UInputMappingContext` (UE 5.8):** don't write to the
top-level `mappings` property — in current UE that's a legacy/unused array. The real data
is `defaultKeyMappings.mappings` (an `FInputMappingContextMappingData` wrapper). Confirmed
via `get_properties` after a `set_properties` write: writing top-level `mappings` has no
effect, writing `defaultKeyMappings.mappings` is what `AddMappingContext` actually reads.
Example entry:
```json
{"defaultKeyMappings": {"mappings": [{
  "triggers": [], "modifiers": [],
  "action": {"refPath": "/Game/.../IA_Select.IA_Select"},
  "key": {"keyName": "LeftMouseButton"},
  "settingBehavior": "InheritSettingsFromAction",
  "playerMappableKeySettings": {"refPath": ""}
}]}}
```

## Editor camera / PIE smoke-testing

- `EditorAppToolset.StartPIE`/`StopPIE` + `LogsToolset.GetLogEntries` is a solid way to
  sanity-check a wiring change actually works (spawns, possession, no runtime errors)
  without asking the user to alt-tab and press play themselves. `GetLogEntries` with a
  `pattern` regex (e.g. `"RTS|Error|Warning"`) keeps the output small; grab the last ~20-30
  unfiltered entries too since a quiet filtered result can just mean "nothing new happened"
  rather than "nothing went wrong."
- `SceneTools.find_actors` with `actor_type` works against the **PIE world** while a
  session is running (paths come back under `UEDPIE_0_<MapName>` instead of the normal map
  path) — good for confirming something got spawned/possessed at runtime.
- `EditorAppToolset.CaptureViewport`: despite `captureTransform`/`annotations` looking
  optional in the schema (no top-level `required` list), this server's bridge rejects the
  call unless **both are fully populated** — pass a complete `ToolsetTransform` and a
  complete `ViewportAnnotationConfig` (set `gridSpacing`/`maxLabelDistance`/`maxLabels` to
  `0` and `classFilter` to `{"refPath": ""}` to effectively disable annotations). This
  captures the **editor level viewport camera**, not whatever a possessed pawn's camera is
  showing in a running PIE session — it's for "does the scene look right" checks, not for
  seeing through a gameplay camera. The result is a big base64 PNG buried in
  `returnValue.image.data`; decode it to a file with Python
  (`base64.b64decode(...)` → write bytes) rather than trying to read the raw tool output,
  then view the file directly.

## Project content layout (as of this writing)

Note: `BP_RTSPlayerController`/`BP_RTSGameMode`/the Input assets were originally created
under `/Game/Game/Player/` and `/Game/Game/GameModes/` but have since been **manually
reorganized by the user** into `/Game/Game/Core/` (and `/Game/Game/Core/Input/`) — asset
moves fix up all referencing properties automatically (verified: `PlayerControllerClass`,
`defaultGameMode`, etc. all still resolved correctly after the move), so this is safe to
do at any time; just make sure you're reading paths fresh (`find_assets`) rather than
trusting paths named earlier in a long session. `BP_RTSCamera` is still under
`/Game/Game/Player/` — wasn't moved along with the rest, may be worth relocating to
`Core/` too for consistency but nothing depends on its specific path.

- `/Game/Game/Maps/Development` — the dev/playground map. GameMode set to `BP_RTSGameMode`.
  Contains a greybox village (6 `House_N` + `VillageTower` + `MarketStall` + 4 `Wall_*`,
  all plain `Actor`s with `PrimitiveTools`-added shapes) centered on the origin, plus one
  `BP_Unit_Blue`/`BP_Unit_Red`/`BP_Building_Blue`/`BP_Building_Red` instance (each with a
  team-colored placeholder cube) off to the west (~x=-1900 to -2300) as an "our base" area.
- `/Game/Game/Core/BP_RTSPlayerController` — parented to `ARTSPlayerController`.
  `SelectionBoxWidgetClass` → `WBP_SelectionBox`, `DefaultMappingContext` → `IMC_Default`,
  `SelectAction` → `IA_Select`, `CameraClass` → `BP_RTSCamera`. All four confirmed present
  after compile via `get_properties`, and confirmed working end-to-end via a PIE smoke
  test (camera pawn spawns and gets possessed, no errors in the log).
- `/Game/Game/Core/Input/IA_Select`, `/Game/Game/Core/Input/IMC_Default` — `IMC_Default`
  maps Left Mouse Button to `IA_Select` via `defaultKeyMappings.mappings` (see the
  Enhanced Input section above for why not the top-level `mappings` field).
- `/Game/Game/Core/BP_RTSGameMode` — parented to `ARTSGameMode`, `PlayerControllerClass`
  set to `BP_RTSPlayerController`, `defaultPawnClass` explicitly cleared to none (camera
  possession is handled manually by `ARTSPlayerController::CameraClass`, not GameMode's
  auto-possess flow — don't re-set `defaultPawnClass` without removing that manual spawn).
- `ARTSCamera` (`Source/RTS/Public/Player/RTSCamera.h`) — native Pawn (spring arm + camera,
  fixed top-down pitch, no movement/input of its own yet). `/Game/Game/Player/BP_RTSCamera`
  is the BP subclass instantiated by the controller; confirmed spawning correctly in PIE.
- `/Game/Game/UI/WBP_SelectionBox` — parented to `URTSSelectionBoxWidget`. Tree is
  `RootCanvas` (CanvasPanel) → `SelectionBox` (Border, matches the C++ `BindWidget` name),
  background brush uses `M_SelectionBoxOverlay`.
- `/Game/Game/Entities/Units/BP_Unit_Blue`, `BP_Unit_Red` — parented to `ARTSUnit`.
- `/Game/Game/Entities/Buildings/BP_Building_Blue`, `BP_Building_Red` — parented to
  `ARTSBuilding`.
- `/Game/Game/MaterialLibrary/` — `M_SelectionBoxOverlay` (UI, translucent, fill+border
  shader keyed off UV distance to edge — params `FillColor`/`BorderColor`/`FillOpacity`/
  `BorderOpacity`/`BorderThickness`), `M_Team_Blue`/`M_Team_Red` (flat color, opaque, for
  team-colored placeholders), `M_Env_Wood`/`M_Env_Roof`/`M_Env_Stone` (flat color, opaque,
  village greybox).
- `/Game/Game/Environment/` — still empty; the village greybox actors above live directly
  in the map, not as separate assets here.
- `/Game/Game/Characters/` — (present, not yet used by RTS code).
