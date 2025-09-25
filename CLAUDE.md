# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**The Soul** is a WebAssembly-based Balatro seed analyzer that simulates the game's RNG system to predict shop items, packs, vouchers, bosses, and tags for any given seed. The application consists of a C++ backend compiled to WebAssembly using Emscripten and a JavaScript/HTML frontend.

## Development Commands

### Building the Project

To compile the C++ code to WebAssembly:
```bash
em++ -O3 --closure 1 -lembind -o immolate.js include/immolate.cpp -s EXPORT_NAME="'Immolate'"
```

**Note**: This requires Emscripten to be installed and configured. The command is in `build.bat`.

### Running the Application

The application is a static website. Serve the files using any web server:
```bash
python -m http.server 8000
# or
npx http-server
```

Then navigate to `http://localhost:8000`

### Testing

There are no automated tests in this project. Testing is done manually by:
1. Loading the web interface
2. Entering a seed and parameters
3. Running analysis
4. Verifying results against expected game behavior

## Architecture

### Backend (C++)
- **include/instance.hpp** - Core game instance and RNG system
- **include/functions.hpp** - Game logic (shop generation, boss selection, etc.)
- **include/items.hpp** - Item definitions and data structures
- **include/immolate.hpp** - Emscripten bindings for WebAssembly
- **include/immolate.cpp** - Implementation file
- **include/util.hpp** - Utility functions

### Frontend (JavaScript/HTML)
- **index.html** - Main web interface with input controls
- **UI.js** - Frontend rendering logic with canvas-based card visualization
- **immolate.js** - Generated WebAssembly module (built from C++)

### Key Data Flow
1. User inputs seed and parameters in HTML form
2. JavaScript calls WebAssembly functions via Emscripten bindings
3. C++ simulates game RNG and returns results
4. UI.js renders results with visual card representations

## Important Implementation Notes

### RNG System
- The game uses a pseudo-random number generator seeded with string values
- Seeds are uppercase, 1-8 characters, with '0' replaced by 'O'
- RNG state is tracked per "node" (e.g., "shop", "pack", "boss")

### Shop Generation
- Shop items are generated sequentially using `nextShopItem()`
- Item probabilities change based on stake, ante, and unlocked items
- Vouchers affect shop generation (e.g., "Tarot Merchant" increases tarot chances)

### Visual Rendering
- Cards are rendered using canvas with sprite sheets in the images/ folder
- Supports editions (Foil, Holographic, Polychrome) and stickers (Eternal, Perishable, Rental)
- Search functionality highlights matching items across the analysis

### Memory Management
- WebAssembly objects need explicit cleanup with `.delete()` calls
- Vector returns from C++ functions must be cleaned up after use

## Adding New Features

When implementing new functionality:
1. Add C++ logic in the appropriate header file (usually functions.hpp)
2. Export new functions in immolate.hpp using Emscripten bindings
3. Rebuild the WebAssembly module
4. Call new functions from JavaScript and update UI as needed

## Common Modifications

### Adding New Items/Jokers
- Update item arrays in UI.js with sprite positions
- Ensure item names match exactly between C++ and JavaScript

### Modifying Shop Logic
- Edit probability calculations in functions.hpp
- Remember to account for stakes, vouchers, and unlock status

### Changing UI Layout
- Modify HTML structure in index.html
- Update styling in the embedded CSS
- Adjust rendering logic in UI.js