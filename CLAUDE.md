# RTS project notes

Unreal Engine 5.8 RTS project (C++ module `RTS` + content in `/Game/Game/...`).

## Unreal MCP

This project has an `unreal-mcp` MCP server (see `.mcp.json`) for driving the Unreal
Editor directly (Blueprints, materials, level content, widgets, etc.). Before doing any
Editor/asset work through it, read `Docs/UnrealMCP.md` first — it has the toolset map,
the object-reference format, the large-output-handling recipe, and known gaps (e.g. no
tool can create Enhanced Input assets) so they don't need to be rediscovered each time.
Keep that doc updated when a new lesson/gap is found.
