# Importer boundary

No commercial Close Combat assets are stored in this repository.

Future import support should follow this pipeline:

```
user-selected installation/archive
        |
        v
format detector
        |
        +--> title/version adapter
        |
        v
validated intermediate representation
        |
        +--> terrain/map layers
        +--> sprite references
        +--> sound references
        +--> unit/weapon tables
        +--> scenario/campaign data
        |
        v
local app storage
```

Each adapter should be independently testable with hashes and structural fixtures that do not contain copyrighted game data.

Unknown or malformed files must fail closed with a useful diagnostic rather than guessing field layouts.
