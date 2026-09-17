# Architecture

## Goal

A single 2D engine hosts multiple Close Combat-style rulesets and data adapters without copying proprietary executable code or shipping commercial assets.

## Layers

1. **Native simulation**
   - fixed world coordinates;
   - deterministic update order;
   - units, morale, suppression, health;
   - movement and A* routing;
   - line-of-sight and combat.
2. **Android presentation**
   - Canvas renderer for the bootstrap milestone;
   - touch selection/orders;
   - pan and pinch zoom;
   - no gameplay state stored in the View.
3. **Rulesets**
   - future title-specific values for weapon behavior, morale, movement, force structure, campaign rules and victory conditions.
4. **Importers**
   - future readers for supported original data formats;
   - imported assets stay user-supplied and outside the repository.

## Determinism

The prototype uses a fixed-seed linear congruential generator inside the simulation. The Android UI sends commands; it does not generate combat outcomes.

The next deterministic step is to move command input onto a timestamped simulation command queue and run the engine at a fixed tick rate.
