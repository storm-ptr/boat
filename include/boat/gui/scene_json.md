# Scene JSON

`scene_json.hpp` provides `read_scene_json(text)` and `write_scene_json(scene)`.
Both throw on invalid data; writing omits default values.
The C++ members `view` and `db` are flattened into their containing JSON objects.

| Object | Field | Default / requirement |
| --- | --- | --- |
| Scene | `sources` | Empty ordered array |
| Scene | `center` | `[longitude, latitude]` in degrees; `[0, 0]` |
| Scene | `resolution` | Metres per pixel; `9783.94` |
| Source | `address` | Required, nonempty |
| Source | `source_name` | Empty string |
| Source | `children` | Empty ordered array of layers and sources |
| Layer | `table_name`, `column_name` | Required, nonempty |
| Layer | `schema_name` | Empty string |
| Layer | `raster`, `hidden` | `false` |
| Layer | `color`, `width` | `#000000`, `1` pixel |
| Layer | `fill` | No fill |

Coordinates and resolution must be finite; resolution must be positive.
Colors are `#RRGGBB` or `#AARRGGBB` (alpha first); width `0` keeps the cosmetic
pen. Width must be finite and nonnegative.

A child with `address` is a source; each layer uses its immediate parent's
address. Sources may nest up to 32 levels.

Known fields do not accept `null`. Unknown fields are ignored and lost on
subsequent writing.

Sources are not opened. Addresses are preserved verbatim; relative paths are
not resolved and source names are not generated.

```json
{
  "center": [12.4964, 41.9028],
  "resolution": 20,
  "sources": [{
    "address": "rome.gpkg",
    "source_name": "Rome",
    "children": [
      {"table_name": "roads", "column_name": "geom", "color": "#FF8800", "width": 2},
      {"address": "imagery.tif"},
      {"table_name": "areas", "column_name": "geom", "fill": "#804080FF", "hidden": true}
    ]
  }]
}
```
