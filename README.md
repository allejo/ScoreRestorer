# Plugin Name

[![GitHub release](https://img.shields.io/github/release/USERNAME/REPO.svg)](https://github.com/USERNAME/REPO/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.0+-blue.svg)
[![License](https://img.shields.io/github/license/USERNAME/REPO.svg)](LICENSE.md)

A brief description about what the plugin does should go here

## Requirements

- List any requirements
- this plug-in will require

This plug-in follows [my standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

You should specify any command line arguments that are needed or lack thereof

```
-loadplugin pluginName...
```

### Configuration File

If the plugin requires a custom configuration file, describe it here and all of its special values

### Custom BZDB Variables

These custom BZDB variables can be configured with `-set` in configuration files and may be changed at any time in-game by using the `/set` command.

```
-set <name> <value>
```

| Name | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `_myBZBD` | int | 60 | A description of what this value does |

### Custom Slash Commands

| Command | Permission | Description |
| ------- | ---------- | ----------- |
| `/command <param>` | vote | A description of what this command does, the required parameters, and permission required |

### Custom Flags

| Abbreviation | Name | Type | Description |
| ------------ | ---- | ---- | ----------- |
| FA | Flag Name | Good | A description of what the flag does |

### Custom Map Objects

This plug-in introduces the `OBJECT` map object which supports the traditional `position`, `size`, and `rotation` attributes for rectangular objects and `position`, `height`, and `radius` for cylindrical objects.

```text
object
  position 0 0 0
  size 5 5 5
  rotation 0
  <custom parameters>
end
```

## License

[LICENSE](LICENSE.md)
