# Architecture

## Goal

A single 2D engine hosts multiple Close Combat-style rulesets and data adapters without copying proprietary executable code or shipping commercial assets.

## Layers

1. **Native simulation**
   - fixed 30 Hz simulation tick;
   - deterministic update order and fixed-seed RNG;
   - squads containing individual soldiers;
   - per-soldier health, weapon, ammunition, reload and fatigue;
   - squad morale and suppression;
   - movement and A* routing;
   - hard LOS blockers plus soft cover terrain;
   - automatic engagement and explicit area/target FIRE orders.
2. **Android presentation**
   - Canvas renderer for the bootstrap milestone;
   - touch selection/orders;
   - MOVE / FAST / SNEAK / FIRE / STOP command bar;
   - living-soldier pips and squad ammunition readout;
   - pan and pinch zoom;
   - no combat state stored in the View.
3. **Rulesets**
   - future title-specific values for weapon behavior, morale, movement, force structure, campaign rules and victory conditions.
4. **Importers**
   - future readers for supported original data formats;
   - imported assets stay user-supplied and outside the repository.

## Orders

The public manuals for the classic engine family consistently expose Move, Move Fast, Sneak, Fire, Smoke, Defend and Ambush. The bootstrap currently implements Move, Move Fast, Sneak and Fire plus a STOP convenience action for touch.

The numerical movement, weapon, fatigue and cover values in this bootstrap are interaction-test values. They are not asserted to match any particular Close Combat release. Those values move into title-specific rulesets as compatibility work advances.

## Soldier model

A bootstrap infantry squad currently contains five soldiers: one LMG, one SMG and three riflemen. Combat resolves ammunition and casualties per soldier rather than subtracting an abstract squad hit-point pool. Squad health in the Android snapshot is derived from living soldier health.

## Cover

Green terrain patches are soft cover zones. They reduce movement speed and incoming hit probability without blocking LOS. Brown building rectangles remain hard movement/LOS blockers. This is a temporary terrain representation until original map terrain/LOS data importers are implemented.

## Determinism

The Android UI sends orders to the native engine; it does not generate combat outcomes. The next determinism milestone is a timestamped simulation command queue suitable for replay and multiplayer lockstep.
